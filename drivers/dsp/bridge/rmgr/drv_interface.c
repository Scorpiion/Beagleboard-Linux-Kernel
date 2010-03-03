/*
 * drv_interface.c
 *
 * DSP-BIOS Bridge driver support functions for TI OMAP processors.
 *
 * DSP/BIOS Bridge driver interface.
 *
 * Copyright (C) 2005-2006 Texas Instruments, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/*  ----------------------------------- Host OS */

#include <dspbridge/host_os.h>
#include <linux/platform_device.h>
#include <linux/pm.h>

#ifdef MODULE
#include <linux/module.h>
#endif

#include <linux/device.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/cdev.h>

/*  ----------------------------------- DSP/BIOS Bridge */
#include <dspbridge/std.h>
#include <dspbridge/dbdefs.h>
#include <dspbridge/errbase.h>

/*  ----------------------------------- Trace & Debug */
#include <dspbridge/dbc.h>

/*  ----------------------------------- OS Adaptation Layer */
#include <dspbridge/services.h>
#include <dspbridge/sync.h>
#include <dspbridge/reg.h>

/*  ----------------------------------- Platform Manager */
#include <dspbridge/wcdioctl.h>
#include <dspbridge/_dcd.h>
#include <dspbridge/dspdrv.h>

/*  ----------------------------------- Resource Manager */
#include <dspbridge/pwr.h>

/*  ----------------------------------- This */
#include <drv_interface.h>

#include <dspbridge/cfg.h>
#include <dspbridge/resourcecleanup.h>
#include <dspbridge/chnl.h>
#include <dspbridge/proc.h>
#include <dspbridge/dev.h>
#include <dspbridge/drvdefs.h>
#include <dspbridge/drv.h>

#ifdef CONFIG_BRIDGE_DVFS
#include <mach-omap2/omap3-opp.h>
#endif

#define BRIDGE_NAME "C6410"
/*  ----------------------------------- Globals */
#define DRIVER_NAME  "DspBridge"
#define DSPBRIDGE_VERSION	"0.1"
s32 dsp_debug;

struct platform_device *omap_dspbridge_dev;
struct device *bridge;

/* This is a test variable used by Bridge to test different sleep states */
s32 dsp_test_sleepstate;

static struct cdev bridge_cdev;

static struct class *bridge_class;

static u32 driverContext;
static s32 driver_major;
static char *base_img;
char *iva_img;
static s32 shm_size = 0x500000;	/* 5 MB */
static u32 phys_mempool_base;
static u32 phys_mempool_size;
static int tc_wordswapon;	/* Default value is always false */

#ifdef CONFIG_PM
struct omap34xx_bridge_suspend_data {
	int suspended;
	wait_queue_head_t suspend_wq;
};

static struct omap34xx_bridge_suspend_data bridge_suspend_data;

static int omap34xxbridge_suspend_lockout(
		struct omap34xx_bridge_suspend_data *s, struct file *f)
{
	if ((s)->suspended) {
		if ((f)->f_flags & O_NONBLOCK)
			return DSP_EDPMSUSPEND;
		wait_event_interruptible((s)->suspend_wq, (s)->suspended == 0);
	}
	return 0;
}
#endif

module_param(dsp_debug, int, 0);
MODULE_PARM_DESC(dsp_debug, "Wait after loading DSP image. default = false");

module_param(dsp_test_sleepstate, int, 0);
MODULE_PARM_DESC(dsp_test_sleepstate, "DSP Sleep state = 0");

module_param(base_img, charp, 0);
MODULE_PARM_DESC(base_img, "DSP base image, default = NULL");

module_param(shm_size, int, 0);
MODULE_PARM_DESC(shm_size, "SHM size, default = 4 MB, minimum = 64 KB");

module_param(phys_mempool_base, uint, 0);
MODULE_PARM_DESC(phys_mempool_base,
		"Physical memory pool base passed to driver");

module_param(phys_mempool_size, uint, 0);
MODULE_PARM_DESC(phys_mempool_size,
		"Physical memory pool size passed to driver");
module_param(tc_wordswapon, int, 0);
MODULE_PARM_DESC(tc_wordswapon, "TC Word Swap Option. default = 0");

MODULE_AUTHOR("Texas Instruments");
MODULE_LICENSE("GPL");
MODULE_VERSION(DSPBRIDGE_VERSION);

static char *driver_name = DRIVER_NAME;

static const struct file_operations bridge_fops = {
	.open		= bridge_open,
	.release	= bridge_release,
	.unlocked_ioctl	= bridge_ioctl,
	.mmap		= bridge_mmap,
};

#ifdef CONFIG_PM
static u32 timeOut = 1000;
#ifdef CONFIG_BRIDGE_DVFS
static struct clk *clk_handle;
#endif

#endif

struct dspbridge_platform_data *omap_dspbridge_pdata;

#ifdef CONFIG_BRIDGE_DVFS
static int dspbridge_scale_notification(struct notifier_block *op,
		unsigned long val, void *ptr)
{
	struct dspbridge_platform_data *pdata =
					omap_dspbridge_dev->dev.platform_data;
	if (CPUFREQ_POSTCHANGE == val && pdata->dsp_get_opp)
		PWR_PM_PostScale(PRCM_VDD1, pdata->dsp_get_opp());
	return 0;
}

static struct notifier_block iva_clk_notifier = {
	.notifier_call = dspbridge_scale_notification,
	NULL,
};
#endif

static int __devinit omap34xx_bridge_probe(struct platform_device *pdev)
{
	int status;
	u32 initStatus;
	u32 temp;
	dev_t   dev = 0 ;
	int     result;
	struct dspbridge_platform_data *pdata = pdev->dev.platform_data;

	omap_dspbridge_dev = pdev;

	/* Global bridge device */
	bridge = &omap_dspbridge_dev->dev;

	/* use 2.6 device model */
	result = alloc_chrdev_region(&dev, 0, 1, driver_name);
	if (result < 0) {
		pr_err("%s: Can't get major %d\n", __func__, driver_major);
		goto err1;
	}

	driver_major = MAJOR(dev);

	cdev_init(&bridge_cdev, &bridge_fops);
	bridge_cdev.owner = THIS_MODULE;

	status = cdev_add(&bridge_cdev, dev, 1);
	if (status) {
		pr_err("%s: Failed to add bridge device\n", __func__);
		goto err2;
	}

	/* udev support */
	bridge_class = class_create(THIS_MODULE, "ti_bridge");

	if (IS_ERR(bridge_class))
		pr_err("%s: Error creating bridge class\n", __func__);

	device_create(bridge_class, NULL, MKDEV(driver_major, 0),
			NULL, "DspBridge");

#ifdef CONFIG_PM
	/* Initialize the wait queue */
	if (!status) {
		bridge_suspend_data.suspended = 0;
		init_waitqueue_head(&bridge_suspend_data.suspend_wq);
	}
#endif

	SERVICES_Init();

	/*  Autostart flag.  This should be set to true if the DSP image should
	 *  be loaded and run during bridge module initialization  */

	if (base_img) {
		temp = true;
		REG_SetValue(AUTOSTART, (u8 *)&temp, sizeof(temp));
		REG_SetValue(DEFEXEC, (u8 *)base_img, strlen(base_img) + 1);
	} else {
		temp = false;
		REG_SetValue(AUTOSTART, (u8 *)&temp, sizeof(temp));
		REG_SetValue(DEFEXEC, (u8 *) "\0", (u32)2);
	}

	if (shm_size >= 0x10000) {	/* 64 KB */
		initStatus = REG_SetValue(SHMSIZE, (u8 *)&shm_size,
					  sizeof(shm_size));
	} else {
		initStatus = DSP_EINVALIDARG;
		status = -1;
		pr_err("%s: SHM size must be at least 64 KB\n", __func__);
	}
	dev_dbg(bridge, "%s: requested shm_size = 0x%x\n", __func__, shm_size);

	if (pdata->phys_mempool_base && pdata->phys_mempool_size) {
		phys_mempool_base = pdata->phys_mempool_base;
		phys_mempool_size = pdata->phys_mempool_size;
	}

	dev_dbg(bridge, "%s: phys_mempool_base = 0x%x \n", __func__,
							phys_mempool_base);

	dev_dbg(bridge, "%s: phys_mempool_size = 0x%x\n", __func__,
							phys_mempool_base);
	if ((phys_mempool_base > 0x0) && (phys_mempool_size > 0x0))
		MEM_ExtPhysPoolInit(phys_mempool_base, phys_mempool_size);
	if (tc_wordswapon) {
		dev_dbg(bridge, "%s: TC Word Swap is enabled\n", __func__);
		REG_SetValue(TCWORDSWAP, (u8 *)&tc_wordswapon,
			    sizeof(tc_wordswapon));
	} else {
		dev_dbg(bridge, "%s: TC Word Swap is disabled\n", __func__);
		REG_SetValue(TCWORDSWAP, (u8 *)&tc_wordswapon,
			    sizeof(tc_wordswapon));
	}
	if (DSP_SUCCEEDED(initStatus)) {
#ifdef CONFIG_BRIDGE_DVFS
		clk_handle = clk_get(NULL, "iva2_ck");
		if (!clk_handle)
			pr_err("%s: clk_get failed to get iva2_ck\n", __func__);

		if (cpufreq_register_notifier(&iva_clk_notifier,
						CPUFREQ_TRANSITION_NOTIFIER))
			pr_err("%s: cpufreq_register_notifier failed for "
							"iva2_ck\n", __func__);
#endif
		driverContext = DSP_Init(&initStatus);
		if (DSP_FAILED(initStatus)) {
			status = -1;
			pr_err("DSP Bridge driver initialization failed\n");
		} else {
			pr_info("DSP Bridge driver loaded\n");
		}
	}

	DBC_Assert(status == 0);
	DBC_Assert(DSP_SUCCEEDED(initStatus));

	return 0;

err2:
	unregister_chrdev_region(dev, 1);
err1:
	return result;
}

static int __devexit omap34xx_bridge_remove(struct platform_device *pdev)
{
	dev_t devno;
	bool ret;
	DSP_STATUS dsp_status = DSP_SOK;
	HANDLE hDrvObject = NULL;

	dsp_status = CFG_GetObject((u32 *)&hDrvObject, REG_DRV_OBJECT);
	if (DSP_FAILED(dsp_status))
		goto func_cont;

#ifdef CONFIG_BRIDGE_DVFS
	if (cpufreq_unregister_notifier(&iva_clk_notifier,
						CPUFREQ_TRANSITION_NOTIFIER))
		pr_err("%s: clk_notifier_unregister failed for iva2_ck\n",
								__func__);
#endif /* #ifdef CONFIG_BRIDGE_DVFS */

	if (driverContext) {
		/* Put the DSP in reset state */
		ret = DSP_Deinit(driverContext);
		driverContext = 0;
		DBC_Assert(ret == true);
	}

#ifdef CONFIG_BRIDGE_DVFS
	clk_put(clk_handle);
	clk_handle = NULL;
#endif /* #ifdef CONFIG_BRIDGE_DVFS */

func_cont:
	MEM_ExtPhysPoolRelease();

	SERVICES_Exit();

	devno = MKDEV(driver_major, 0);
	cdev_del(&bridge_cdev);
	unregister_chrdev_region(devno, 1);
	if (bridge_class) {
		/* remove the device from sysfs */
		device_destroy(bridge_class, MKDEV(driver_major, 0));
		class_destroy(bridge_class);

	}
	return 0;
}


#ifdef CONFIG_PM
static int bridge_suspend(struct platform_device *pdev, pm_message_t state)
{
	u32 status;
	u32 command = PWR_EMERGENCYDEEPSLEEP;

	status = PWR_SleepDSP(command, timeOut);
	if (DSP_FAILED(status))
		return -1;

	bridge_suspend_data.suspended = 1;
	return 0;
}

static int bridge_resume(struct platform_device *pdev)
{
	u32 status;

	status = PWR_WakeDSP(timeOut);
	if (DSP_FAILED(status))
		return -1;

	bridge_suspend_data.suspended = 0;
	wake_up(&bridge_suspend_data.suspend_wq);
	return 0;
}
#else
#define bridge_suspend NULL
#define bridge_resume NULL
#endif

static struct platform_driver bridge_driver = {
	.driver = {
		.name = BRIDGE_NAME,
	},
	.probe	 = omap34xx_bridge_probe,
	.remove	 = __devexit_p(omap34xx_bridge_remove),
	.suspend = bridge_suspend,
	.resume	 = bridge_resume,
};

static int __init bridge_init(void)
{
	return platform_driver_register(&bridge_driver);
}

static void __exit bridge_exit(void)
{
	platform_driver_unregister(&bridge_driver);
}

/*
 * This function is called when an application opens handle to the
 * bridge driver.
 */
static int bridge_open(struct inode *ip, struct file *filp)
{
	int status = 0;
	struct PROCESS_CONTEXT *pr_ctxt = NULL;

	/*
	 * Allocate a new process context and insert it into global
	 * process context list.
	 */
	pr_ctxt = MEM_Calloc(sizeof(struct PROCESS_CONTEXT), MEM_PAGED);
	if (pr_ctxt) {
		pr_ctxt->resState = PROC_RES_ALLOCATED;
		spin_lock_init(&pr_ctxt->dmm_map_lock);
		INIT_LIST_HEAD(&pr_ctxt->dmm_map_list);
		spin_lock_init(&pr_ctxt->dmm_rsv_lock);
		INIT_LIST_HEAD(&pr_ctxt->dmm_rsv_list);
		mutex_init(&pr_ctxt->node_mutex);
		mutex_init(&pr_ctxt->strm_mutex);
	} else {
		status = -ENOMEM;
	}

	filp->private_data = pr_ctxt;

	return status;
}

/*
 * This function is called when an application closes handle to the bridge
 * driver.
 */
static int bridge_release(struct inode *ip, struct file *filp)
{
	int status = 0;
	struct PROCESS_CONTEXT *pr_ctxt;

	if (!filp->private_data) {
		status = -EIO;
		goto err;
	}

	pr_ctxt = filp->private_data;
	flush_signals(current);
	DRV_RemoveAllResources(pr_ctxt);
	PROC_Detach(pr_ctxt);
	kfree(pr_ctxt);

	filp->private_data = NULL;

err:
	return status;
}

/* This function provides IO interface to the bridge driver. */
static long bridge_ioctl(struct file *filp, unsigned int code,
		unsigned long args)
{
	int status;
	u32 retval = DSP_SOK;
	union Trapped_Args pBufIn;

	DBC_Require(filp != NULL);
#ifdef CONFIG_PM
	status = omap34xxbridge_suspend_lockout(&bridge_suspend_data, filp);
	if (status != 0)
		return status;
#endif

	if (!filp->private_data) {
		status = -EIO;
		goto err;
	}

	status = copy_from_user(&pBufIn, (union Trapped_Args *)args,
				sizeof(union Trapped_Args));

	if (!status) {
		status = WCD_CallDevIOCtl(code, &pBufIn, &retval,
				filp->private_data);

		if (DSP_SUCCEEDED(status)) {
			status = retval;
		} else {
			dev_dbg(bridge, "%s: IOCTL Failed, code: 0x%x "
				"status 0x%x\n", __func__, code, status);
			status = -1;
		}

	}

err:
	return status;
}

/* This function maps kernel space memory to user space memory. */
static int bridge_mmap(struct file *filp, struct vm_area_struct *vma)
{
#if GT_TRACE
	u32 offset = vma->vm_pgoff << PAGE_SHIFT;
#endif
	u32 status;

	DBC_Assert(vma->vm_start < vma->vm_end);

	vma->vm_flags |= VM_RESERVED | VM_IO;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	dev_dbg(bridge, "%s: vm filp %p offset %x start %lx end %lx page_prot "
				"%lx flags %lx\n", __func__, filp, offset,
				vma->vm_start, vma->vm_end, vma->vm_page_prot,
				vma->vm_flags);

	status = remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
				vma->vm_end - vma->vm_start, vma->vm_page_prot);
	if (status != 0)
		status = -EAGAIN;

	return status;
}

/* To remove all process resources before removing the process from the
 * process context list*/
DSP_STATUS DRV_RemoveAllResources(HANDLE hPCtxt)
{
	DSP_STATUS status = DSP_SOK;
	struct PROCESS_CONTEXT *pCtxt = (struct PROCESS_CONTEXT *)hPCtxt;
	DRV_RemoveAllSTRMResElements(pCtxt);
	DRV_RemoveAllNodeResElements(pCtxt);
	DRV_RemoveAllDMMResElements(pCtxt);
	pCtxt->resState = PROC_RES_FREED;
	return status;
}

/* Bridge driver initialization and de-initialization functions */
module_init(bridge_init);
module_exit(bridge_exit);

