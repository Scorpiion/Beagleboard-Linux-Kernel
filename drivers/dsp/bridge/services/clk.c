/*
 * clk.c
 *
 * DSP-BIOS Bridge driver support functions for TI OMAP processors.
 *
 * Clock and Timer services.
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

/*  ----------------------------------- DSP/BIOS Bridge */
#include <dspbridge/std.h>
#include <dspbridge/dbdefs.h>
#include <dspbridge/errbase.h>

/*  ----------------------------------- Trace & Debug */
#include <dspbridge/dbc.h>
#include <dspbridge/gt.h>

/*  ----------------------------------- OS Adaptation Layer */
#include <dspbridge/mem.h>

/*  ----------------------------------- This */
#include <dspbridge/clk.h>

/*  ----------------------------------- Defines, Data Structures, Typedefs */

typedef volatile unsigned long  REG_UWORD32;

#define OMAP_SSI_OFFSET			0x58000
#define OMAP_SSI_SIZE			0x1000
#define OMAP_SSI_SYSCONFIG_OFFSET	0x10

#define SSI_AUTOIDLE			(1 << 0)
#define SSI_SIDLE_SMARTIDLE		(2 << 3)
#define SSI_MIDLE_NOIDLE		(1 << 12)

struct SERVICES_Clk_t {
	struct clk *clk_handle;
	const char *clk_name;
	const char *dev;
};

/* The row order of the below array needs to match with the clock enumerations
 * 'SERVICES_ClkId' provided in the header file.. any changes in the
 * enumerations needs to be fixed in the array as well */
static struct SERVICES_Clk_t SERVICES_Clks[] = {
	{NULL, "iva2_ck", NULL},
	{NULL, "gpt5_fck", NULL},
	{NULL, "gpt5_ick", NULL},
	{NULL, "gpt6_fck", NULL},
	{NULL, "gpt6_ick", NULL},
	{NULL, "gpt7_fck", NULL},
	{NULL, "gpt7_ick", NULL},
	{NULL, "gpt8_fck", NULL},
	{NULL, "gpt8_ick", NULL},
	{NULL, "wdt3_fck", NULL},
	{NULL, "wdt3_ick", NULL},
	{NULL, "fck", "omap-mcbsp.1"},
	{NULL, "ick", "omap-mcbsp.1"},
	{NULL, "fck", "omap-mcbsp.2"},
	{NULL, "ick", "omap-mcbsp.2"},
	{NULL, "fck", "omap-mcbsp.3"},
	{NULL, "ick", "omap-mcbsp.3"},
	{NULL, "fck", "omap-mcbsp.4"},
	{NULL, "ick", "omap-mcbsp.4"},
	{NULL, "fck", "omap-mcbsp.5"},
	{NULL, "ick", "omap-mcbsp.5"},
	{NULL, "ssi_ssr_sst_fck", NULL},
	{NULL, "ssi_ick", NULL},
	{NULL, "omap_32k_fck", NULL},
	{NULL, "sys_ck", NULL},
	{NULL, ""}
};

/* Generic TIMER object: */
struct TIMER_OBJECT {
	struct timer_list timer;
};

/*  ----------------------------------- Globals */
#if GT_TRACE
static struct GT_Mask CLK_debugMask = { NULL, NULL };	/* GT trace variable */
#endif

/*
 *  ======== CLK_Exit ========
 *  Purpose:
 *      Cleanup CLK module.
 */
void CLK_Exit(void)
{
	int i = 0;

	/* Relinquish the clock handles */
	while (i < SERVICESCLK_NOT_DEFINED) {
		if (SERVICES_Clks[i].clk_handle)
			clk_put(SERVICES_Clks[i].clk_handle);

		SERVICES_Clks[i].clk_handle = NULL;
		i++;
	}

}

/*
 *  ======== CLK_Init ========
 *  Purpose:
 *      Initialize CLK module.
 */
bool CLK_Init(void)
{
	static struct platform_device dspbridge_device;
	struct clk *clk_handle;
	int i = 0;
	GT_create(&CLK_debugMask, "CK");	/* CK for CLK */

	dspbridge_device.dev.bus = &platform_bus_type;

	/* Get the clock handles from base port and store locally */
	while (i < SERVICESCLK_NOT_DEFINED) {
		/* get the handle from BP */
		clk_handle = clk_get_sys(SERVICES_Clks[i].dev,
			     SERVICES_Clks[i].clk_name);

		if (!clk_handle) {
			GT_2trace(CLK_debugMask, GT_7CLASS,
				  "CLK_Init: failed to get Clk handle %s, "
				  "CLK dev id = %s\n",
				  SERVICES_Clks[i].clk_name,
				  SERVICES_Clks[i].dev);
			/* should we fail here?? */
		}
		SERVICES_Clks[i].clk_handle = clk_handle;
		i++;
	}

	return true;
}

/*
 *  ======== CLK_Enable ========
 *  Purpose:
 *      Enable Clock .
 *
*/
DSP_STATUS CLK_Enable(IN enum SERVICES_ClkId clk_id)
{
	DSP_STATUS status = DSP_SOK;
	struct clk *pClk;

	DBC_Require(clk_id < SERVICESCLK_NOT_DEFINED);

	pClk = SERVICES_Clks[clk_id].clk_handle;
	if (pClk) {
		if (clk_enable(pClk)) {
			pr_err("CLK_Enable: failed to Enable CLK %s, "
					"CLK dev id = %s\n",
					SERVICES_Clks[clk_id].clk_name,
					SERVICES_Clks[clk_id].dev);
			status = DSP_EFAIL;
		}
	} else {
		pr_err("CLK_Enable: failed to get CLK %s, CLK dev id = %s\n",
					SERVICES_Clks[clk_id].clk_name,
					SERVICES_Clks[clk_id].dev);
		status = DSP_EFAIL;
	}
	/* The SSI module need to configured not to have the Forced idle for
	 * master interface. If it is set to forced idle, the SSI module is
	 * transitioning to standby thereby causing the client in the DSP hang
	 * waiting for the SSI module to be active after enabling the clocks
	 */
	if (clk_id == SERVICESCLK_ssi_fck)
		SSI_Clk_Prepare(true);

	return status;
}
/*
 *  ======== CLK_Set_32KHz ========
 *  Purpose:
 *      To Set parent of a clock to 32KHz.
 */

DSP_STATUS CLK_Set_32KHz(IN enum SERVICES_ClkId clk_id)
{
	DSP_STATUS status = DSP_SOK;
	struct clk *pClk;
	struct clk *pClkParent;
	pClkParent =  SERVICES_Clks[SERVICESCLK_sys_32k_ck].clk_handle;

	DBC_Require(clk_id < SERVICESCLK_NOT_DEFINED);

	pClk = SERVICES_Clks[clk_id].clk_handle;
	if (pClk) {
		if (!(clk_set_parent(pClk, pClkParent) == 0x0)) {
			GT_2trace(CLK_debugMask, GT_7CLASS, "CLK_Set_32KHz: "
				"Failed to set to 32KHz %s, CLK dev id = %s\n",
				SERVICES_Clks[clk_id].clk_name,
				SERVICES_Clks[clk_id].dev);
			status = DSP_EFAIL;
		}
	}
	return status;
}

/*
 *  ======== CLK_Disable ========
 *  Purpose:
 *      Disable the clock.
 *
*/
DSP_STATUS CLK_Disable(IN enum SERVICES_ClkId clk_id)
{
	DSP_STATUS status = DSP_SOK;
	struct clk *pClk;
	s32 clkUseCnt;

	DBC_Require(clk_id < SERVICESCLK_NOT_DEFINED);

	pClk = SERVICES_Clks[clk_id].clk_handle;

	clkUseCnt = CLK_Get_UseCnt(clk_id);
	if (clkUseCnt == -1) {
		pr_err("CLK_Disable: failed to get CLK Use count for CLK %s,"
				"CLK dev id = %s\n",
				SERVICES_Clks[clk_id].clk_name,
				SERVICES_Clks[clk_id].dev);
	} else if (clkUseCnt == 0) {
		return status;
	}
	if (clk_id == SERVICESCLK_ssi_ick)
		SSI_Clk_Prepare(false);

		if (pClk) {
			clk_disable(pClk);
		} else {
			pr_err("CLK_Disable: failed to get CLK %s,"
					"CLK dev id = %s\n",
					SERVICES_Clks[clk_id].clk_name,
					SERVICES_Clks[clk_id].dev);
			status = DSP_EFAIL;
		}
	return status;
}

/*
 *  ======== CLK_GetRate ========
 *  Purpose:
 *      GetClock Speed.
 *
 */

DSP_STATUS CLK_GetRate(IN enum SERVICES_ClkId clk_id, u32 *speedKhz)
{
	DSP_STATUS status = DSP_SOK;
	struct clk *pClk;
	u32 clkSpeedHz;

	DBC_Require(clk_id < SERVICESCLK_NOT_DEFINED);
	*speedKhz = 0x0;

	pClk = SERVICES_Clks[clk_id].clk_handle;
	if (pClk) {
		clkSpeedHz = clk_get_rate(pClk);
		*speedKhz = clkSpeedHz / 1000;
		GT_2trace(CLK_debugMask, GT_6CLASS,
			  "CLK_GetRate: clkSpeedHz = %d , "
			 "speedinKhz=%d\n", clkSpeedHz, *speedKhz);
	} else {
		GT_2trace(CLK_debugMask, GT_7CLASS,
			 "CLK_GetRate: failed to get CLK %s, "
			 "CLK dev Id = %s\n", SERVICES_Clks[clk_id].clk_name,
			 SERVICES_Clks[clk_id].dev);
		status = DSP_EFAIL;
	}
	return status;
}

s32 CLK_Get_UseCnt(IN enum SERVICES_ClkId clk_id)
{
	DSP_STATUS status = DSP_SOK;
	struct clk *pClk;
	s32 useCount = -1;
	DBC_Require(clk_id < SERVICESCLK_NOT_DEFINED);

	pClk = SERVICES_Clks[clk_id].clk_handle;

	if (pClk) {
		/* FIXME: usecount shouldn't be used */
		useCount = pClk->usecount;
	} else {
		GT_2trace(CLK_debugMask, GT_7CLASS,
			 "CLK_GetRate: failed to get CLK %s, "
			 "CLK dev Id = %s\n", SERVICES_Clks[clk_id].clk_name,
			 SERVICES_Clks[clk_id].dev);
		status = DSP_EFAIL;
	}
	return useCount;
}

void SSI_Clk_Prepare(bool FLAG)
{
	void __iomem *ssi_base;
	unsigned int value;

	ssi_base = ioremap(L4_34XX_BASE + OMAP_SSI_OFFSET, OMAP_SSI_SIZE);
	if (!ssi_base) {
		pr_err("%s: error, SSI not configured\n", __func__);
		return;
	}

	if (FLAG) {
		/* Set Autoidle, SIDLEMode to smart idle, and MIDLEmode to
		 * no idle
		 */
		value = SSI_AUTOIDLE | SSI_SIDLE_SMARTIDLE | SSI_MIDLE_NOIDLE;
	} else {
		/* Set Autoidle, SIDLEMode to forced idle, and MIDLEmode to
		 * forced idle
		 */
		value = SSI_AUTOIDLE;
	}

	__raw_writel(value, ssi_base + OMAP_SSI_SYSCONFIG_OFFSET);
	iounmap(ssi_base);
}
