/*
 * drv_ducatidrv.c
 *
 * Syslink driver support functions for TI OMAP processors.
 *
 * Copyright (C) 2008-2009 Texas Instruments, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */


/*  ----------------------------------- OS Specific Headers         */
#include <linux/autoconf.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/io.h>

#include <linux/io.h>
#include <asm/pgtable.h>
#include <syslink/notifydefs.h>
#include <syslink/notify_ducatidriver.h>
#include <syslink/notify_ducatidriver_defs.h>
#include <syslink/GlobalTypes.h>


/* Major number of driver */
static signed long int major = 234 ;


/* driver function to open the notify mailbox driver object. */
static int drvducati_open(struct inode *inode, struct file *filp);
static int drvducati_release(struct inode *inode, struct file *filp);
static int drvducati_ioctl(struct inode *inode,
			   struct file *filp,
			   unsigned int cmd,
			   unsigned long args);


/* notify mailbox driver initialization function. */
static int __init drvducati_initialize_module(void) ;

/* notify mailbox driver cleanup function. */
static void  __exit drvducati_finalize_module(void) ;


/* Function to invoke the APIs through ioctl. */
static const struct file_operations driver_ops = {
	.open = drvducati_open,
	.release = drvducati_release,
	.ioctl = drvducati_ioctl,

};

/* Initialization function */
static int __init drvducati_initialize_module(void)
{
	int result = 0;

	result = register_chrdev(major, "notifyducatidrv", &driver_ops);
	if (result < 0)
		pr_err("Notify ducati driver initialization file\n");

	return result ;
}

/* Finalization function */
static void __exit drvducati_finalize_module(void)
{
	unregister_chrdev(major, "notifyducatidrv");
}
/* driver open*/
static int drvducati_open(struct inode *inode, struct file *filp)
{
	return 0 ;
}

/*drivr close*/
static int drvducati_release(struct inode *inode, struct file *filp)
{
	return 0;
}



/*
 *  brief  linux driver function to invoke the APIs through ioctl.
 *
 */
static int drvducati_ioctl(struct inode *inode,
			   struct file *filp,
			   unsigned int cmd,
			   unsigned long args)
{
	int  status = 0;
	int retval = 0;
	struct notify_ducatidrv_cmdargs *cmd_args =
		(struct notify_ducatidrv_cmdargs *) args;
	struct notify_ducatidrv_cmdargs common_args;

	switch (cmd) {
	case CMD_NOTIFYDRIVERSHM_GETCONFIG: {
		struct notify_ducatidrv_cmdargs_getconfig *src_args =
			(struct notify_ducatidrv_cmdargs_getconfig *) args;
		struct notify_ducatidrv_config cfg;
		notify_ducatidrv_getconfig(&cfg);

		retval = copy_to_user((void *)(src_args->cfg),
				      (const void *) &cfg,
				      sizeof(struct notify_ducatidrv_config));

		if (WARN_ON(retval != 0))
			goto func_end;

	}
	break;

	case CMD_NOTIFYDRIVERSHM_SETUP: {
		struct notify_ducatidrv_cmdargs_setup *src_args =
			(struct notify_ducatidrv_cmdargs_setup *) args;
		struct notify_ducatidrv_config cfg;
		retval = copy_from_user((void *) &cfg,
					(const void *)(src_args->cfg),
					sizeof(struct notify_ducatidrv_config));
		if (WARN_ON(retval != 0))
			goto func_end;

		status = notify_ducatidrv_setup(&cfg);
		if (status < 0)
			printk(KERN_ERR "FAIL: notify_ducatidrv_setup\n");
	}
	break;

	case CMD_NOTIFYDRIVERSHM_DESTROY: {
		status = notify_ducatidrv_destroy();

		if (status < 0)
			printk(KERN_ERR "FAIL: notify_ducatidrv_destroy\n");
	}
	break;

	case CMD_NOTIFYDRIVERSHM_PARAMS_INIT: {
		struct notify_ducatidrv_cmdargs_paramsinit src_args;
		struct notify_ducatidrv_params params;
		retval = copy_from_user((void *) &src_args,
				(const void *)(args),
				sizeof(
				struct notify_ducatidrv_cmdargs_paramsinit));
		if (WARN_ON(retval != 0))
			goto func_end;
		notify_ducatidrv_params_init(src_args.handle, &params);

		retval = copy_to_user((void *)(src_args.params),
			      (const void *) &params,
			      sizeof(struct notify_ducatidrv_params));
		if (WARN_ON(retval != 0))
			goto func_end;
	}
	break;

	case CMD_NOTIFYDRIVERSHM_CREATE: {
		struct notify_ducatidrv_cmdargs_create src_args;
		retval = copy_from_user((void *) &src_args,
				(const void *)(args),
				sizeof(struct notify_ducatidrv_cmdargs_create));
		if (WARN_ON(retval != 0))
			goto func_end;
		src_args.handle = notify_ducatidrv_create(
					  src_args.driverName,
					  &(src_args.params));
		if (src_args.handle == NULL) {
			status = -EFAULT;
			printk(KERN_ERR "drvducati_ioctl:status 0x%x,"
			       "NotifyDriverShm_create failed",
			       status);
		}
		retval = copy_to_user((void *)(args),
			      (const void *) &src_args,
			      sizeof(struct notify_ducatidrv_cmdargs_create));
		if (WARN_ON(retval != 0))
			goto func_end;
	}
	break;

	case CMD_NOTIFYDRIVERSHM_DELETE: {
		struct notify_ducatidrv_cmdargs_delete src_args;
		retval = copy_from_user((void *) &src_args,
				(const void *)(args),
				sizeof(struct notify_ducatidrv_cmdargs_delete));
		if (WARN_ON(retval != 0))
			goto func_end;

		status = notify_ducatidrv_delete(&(src_args.handle));
		if (status < 0)
			printk(KERN_ERR "drvducati_ioctl:"
			       " notify_ducatidrv_delete failed"
			       " status = %d\n", status);
	}
	break;

	case CMD_NOTIFYDRIVERSHM_OPEN: {
		struct notify_ducatidrv_cmdargs_open src_args;

		retval = copy_from_user((void *) &src_args,
				(const void *)(args),
				sizeof(struct notify_ducatidrv_cmdargs_open));
		if (WARN_ON(retval != 0))
			goto func_end;

		status = notify_ducatidrv_open(src_args.driverName,
					       &(src_args.handle));
		if (status < 0)
			printk(KERN_ERR "drvducati_ioctl:"
			       " notify_ducatidrv_open failed"
			       " status = %d\n", status);
		retval = copy_to_user((void *)(args),
			      (const void *) &src_args,
			      sizeof(struct notify_ducatidrv_cmdargs_open));
		if (WARN_ON(retval != 0))
			goto func_end;
	}
	break;

	case CMD_NOTIFYDRIVERSHM_CLOSE: {
		struct notify_ducatidrv_cmdargs_close src_args;

		retval = copy_from_user((void *) &src_args,
				(const void *)(args),
				sizeof(struct notify_ducatidrv_cmdargs_close));
		if (WARN_ON(retval != 0))
			goto func_end;

		status = notify_ducatidrv_close(&(src_args.handle));
		if (status < 0)
			printk(KERN_ERR "drvducati_ioctl:"
			       " notify_ducatidrv_close"
			       " failed status = %d\n", status);
	}
	break;

	default: {
		status = -EINVAL;
		printk(KERN_ERR "drivducati_ioctl:Unsupported"
		       " ioctl command specified");
	}
	break;
	}

func_end:
	/* Set the status and copy the common args to user-side. */
	common_args.api_status = status;
	status = copy_to_user((void *) cmd_args,
		      (const void *) &common_args,
		      sizeof(struct notify_ducatidrv_cmdargs));

	if (status < 0)
		retval = -EFAULT;

	return retval;
}




MODULE_LICENSE("GPL");
module_init(drvducati_initialize_module);
module_exit(drvducati_finalize_module);
