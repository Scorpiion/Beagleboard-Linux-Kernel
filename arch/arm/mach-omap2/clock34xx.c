/*
 * OMAP3-specific clock framework functions
 *
 * Copyright (C) 2007-2008 Texas Instruments, Inc.
 * Copyright (C) 2007-2009 Nokia Corporation
 *
 * Written by Paul Walmsley
 * Testing and integration fixes by Jouni Högander
 *
 * Parts of this code are based on code written by
 * Richard Woodruff, Tony Lindgren, Tuukka Tikkanen, Karthik Dasu
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#undef DEBUG

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/limits.h>
#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/i2c/twl.h>

#include <plat/cpu.h>
#include <plat/clock.h>
#include <plat/sram.h>
#include <plat/sdrc.h>
#include <plat/prcm.h>
#include <plat/omap-pm.h>

#include <asm/div64.h>
#include <asm/clkdev.h>

#include <plat/sdrc.h>
#include "clock.h"
#include "clock34xx.h"
#include "sdrc.h"
#include "prm.h"
#include "prm-regbits-34xx.h"
#include "cm.h"
#include "cm-regbits-34xx.h"

#define CYCLES_PER_MHZ			1000000

#define        DPLL_M_MASK     0x7ff
#define        DPLL_N_MASK     0x7f
#define        DPLL_M2_MASK    0x1f
#define        SHIFT_DPLL_M    16
#define        SHIFT_DPLL_N    8
#define        SHIFT_DPLL_M2   27

/*
 * DPLL5_FREQ_FOR_USBHOST: USBHOST and USBTLL are the only clocks
 * that are sourced by DPLL5, and both of these require this clock
 * to be at 120 MHz for proper operation.
 */
#define DPLL5_FREQ_FOR_USBHOST		120000000

/* needed by omap3_core_dpll_m2_set_rate() */
struct clk *sdrc_ick_p, *arm_fck_p;

/**
 * omap3430es2_clk_ssi_find_idlest - return CM_IDLEST info for SSI
 * @clk: struct clk * being enabled
 * @idlest_reg: void __iomem ** to store CM_IDLEST reg address into
 * @idlest_bit: pointer to a u8 to store the CM_IDLEST bit shift into
 *
 * The OMAP3430ES2 SSI target CM_IDLEST bit is at a different shift
 * from the CM_{I,F}CLKEN bit.  Pass back the correct info via
 * @idlest_reg and @idlest_bit.  No return value.
 */
static void omap3430es2_clk_ssi_find_idlest(struct clk *clk,
					    void __iomem **idlest_reg,
					    u8 *idlest_bit)
{
	u32 r;

	r = (((__force u32)clk->enable_reg & ~0xf0) | 0x20);
	*idlest_reg = (__force void __iomem *)r;
	*idlest_bit = OMAP3430ES2_ST_SSI_IDLE_SHIFT;
}

const struct clkops clkops_omap3430es2_ssi_wait = {
	.enable		= omap2_dflt_clk_enable,
	.disable	= omap2_dflt_clk_disable,
	.find_idlest	= omap3430es2_clk_ssi_find_idlest,
	.find_companion = omap2_clk_dflt_find_companion,
};

/**
 * omap3430es2_clk_dss_usbhost_find_idlest - CM_IDLEST info for DSS, USBHOST
 * @clk: struct clk * being enabled
 * @idlest_reg: void __iomem ** to store CM_IDLEST reg address into
 * @idlest_bit: pointer to a u8 to store the CM_IDLEST bit shift into
 *
 * Some OMAP modules on OMAP3 ES2+ chips have both initiator and
 * target IDLEST bits.  For our purposes, we are concerned with the
 * target IDLEST bits, which exist at a different bit position than
 * the *CLKEN bit position for these modules (DSS and USBHOST) (The
 * default find_idlest code assumes that they are at the same
 * position.)  No return value.
 */
static void omap3430es2_clk_dss_usbhost_find_idlest(struct clk *clk,
						    void __iomem **idlest_reg,
						    u8 *idlest_bit)
{
	u32 r;

	r = (((__force u32)clk->enable_reg & ~0xf0) | 0x20);
	*idlest_reg = (__force void __iomem *)r;
	/* USBHOST_IDLE has same shift */
	*idlest_bit = OMAP3430ES2_ST_DSS_IDLE_SHIFT;
}

const struct clkops clkops_omap3430es2_dss_usbhost_wait = {
	.enable		= omap2_dflt_clk_enable,
	.disable	= omap2_dflt_clk_disable,
	.find_idlest	= omap3430es2_clk_dss_usbhost_find_idlest,
	.find_companion = omap2_clk_dflt_find_companion,
};

/**
 * omap3430es2_clk_hsotgusb_find_idlest - return CM_IDLEST info for HSOTGUSB
 * @clk: struct clk * being enabled
 * @idlest_reg: void __iomem ** to store CM_IDLEST reg address into
 * @idlest_bit: pointer to a u8 to store the CM_IDLEST bit shift into
 *
 * The OMAP3430ES2 HSOTGUSB target CM_IDLEST bit is at a different
 * shift from the CM_{I,F}CLKEN bit.  Pass back the correct info via
 * @idlest_reg and @idlest_bit.  No return value.
 */
static void omap3430es2_clk_hsotgusb_find_idlest(struct clk *clk,
						 void __iomem **idlest_reg,
						 u8 *idlest_bit)
{
	u32 r;

	r = (((__force u32)clk->enable_reg & ~0xf0) | 0x20);
	*idlest_reg = (__force void __iomem *)r;
	*idlest_bit = OMAP3430ES2_ST_HSOTGUSB_IDLE_SHIFT;
}

const struct clkops clkops_omap3430es2_hsotgusb_wait = {
	.enable		= omap2_dflt_clk_enable,
	.disable	= omap2_dflt_clk_disable,
	.find_idlest	= omap3430es2_clk_hsotgusb_find_idlest,
	.find_companion = omap2_clk_dflt_find_companion,
};

/** omap3_pwrdn_clk_enable_with_hsdiv_restore - enable clocks suffering
 *         from HSDivider problem.
 * @clk: DPLL output struct clk
 *
 * 3630 only: dpll3_m3_ck, dpll4_m2_ck, dpll4_m3_ck, dpll4_m4_ck, dpll4_m5_ck
 * & dpll4_m6_ck dividers get lost after their respective PWRDN bits are set.
 * Any write to the corresponding CM_CLKSEL register will refresh the
 * dividers.  Only x2 clocks are affected, so it is safe to trust the parent
 * clock information to refresh the CM_CLKSEL registers.
 */
int omap3_pwrdn_clk_enable_with_hsdiv_restore(struct clk *clk)
{
	u32 v;
	int ret;

	/* enable the clock */
	ret = omap2_dflt_clk_enable(clk);

	/* Restore the dividers */
	if (!ret) {
		v = __raw_readl(clk->parent->clksel_reg);
		v += (1 << clk->parent->clksel_shift);
		__raw_writel(v, clk->parent->clksel_reg);
		v -= (1 << clk->parent->clksel_shift);
		__raw_writel(v, clk->parent->clksel_reg);
	}
	return ret;
}

const struct clkops clkops_omap3_pwrdn_with_hsdiv_wait_restore = {
	.enable		= omap3_pwrdn_clk_enable_with_hsdiv_restore,
	.disable	= omap2_dflt_clk_disable,
	.find_companion	= omap2_clk_dflt_find_companion,
	.find_idlest	= omap2_clk_dflt_find_idlest,
};

const struct clkops clkops_noncore_dpll_ops = {
	.enable		= omap3_noncore_dpll_enable,
	.disable	= omap3_noncore_dpll_disable,
};

int omap3_dpll4_set_rate(struct clk *clk, unsigned long rate)
{
	/*
	 * According to the 12-5 CDP code from TI, "Limitation 2.5"
	 * on 3430ES1 prevents us from changing DPLL multipliers or dividers
	 * on DPLL4.
	 */
	if (omap_rev() == OMAP3430_REV_ES1_0) {
		printk(KERN_ERR "clock: DPLL4 cannot change rate due to "
		       "silicon 'Limitation 2.5' on 3430ES1.\n");
		return -EINVAL;
	}
	return omap3_noncore_dpll_set_rate(clk, rate);
}


/*
 * CORE DPLL (DPLL3) rate programming functions
 *
 * These call into SRAM code to do the actual CM writes, since the SDRAM
 * is clocked from DPLL3.
 */

/**
 * omap3_core_dpll_m2_set_rate - set CORE DPLL M2 divider
 * @clk: struct clk * of DPLL to set
 * @rate: rounded target rate
 *
 * Program the DPLL M2 divider with the rounded target rate.  Returns
 * -EINVAL upon error, or 0 upon success.
 */
int omap3_core_dpll_m2_set_rate(struct clk *clk, unsigned long rate)
{
	u32 new_div = 0;
	u32 unlock_dll = 0;
	u32 c;
	unsigned long validrate, sdrcrate, _mpurate;
	struct omap_sdrc_params *sdrc_cs0;
	struct omap_sdrc_params *sdrc_cs1;
	int ret;
	u32 clk_sel_regval;
	u32 core_dpll_mul_m, core_dpll_div_n, core_dpll_clkoutdiv_m2;
	u32 sys_clk_rate, sdrc_clk_stab;
	u32 nr1, nr2, nr, dr;
	unsigned int delay_sram;

	if (!clk || !rate)
		return -EINVAL;

	validrate = omap2_clksel_round_rate_div(clk, rate, &new_div);
	if (validrate != rate)
		return -EINVAL;

	sdrcrate = sdrc_ick_p->rate;
	if (rate > clk->rate)
		sdrcrate <<= ((rate / clk->rate) >> 1);
	else
		sdrcrate >>= ((clk->rate / rate) >> 1);

	ret = omap2_sdrc_get_params(sdrcrate, &sdrc_cs0, &sdrc_cs1);
	if (ret)
		return -EINVAL;

	if (sdrcrate < MIN_SDRC_DLL_LOCK_FREQ) {
		pr_debug("clock: will unlock SDRC DLL\n");
		unlock_dll = 1;
	}

	clk_sel_regval = cm_read_mod_reg(PLL_MOD, CM_CLKSEL);

	/* Get the M, N and M2 values required for getting sdrc clk stab */
	core_dpll_mul_m = (clk_sel_regval >> SHIFT_DPLL_M) & DPLL_M_MASK;
	core_dpll_div_n = (clk_sel_regval >> SHIFT_DPLL_N) & DPLL_N_MASK;
	core_dpll_clkoutdiv_m2 = (clk_sel_regval >> SHIFT_DPLL_M2) &
	DPLL_M2_MASK;
	sys_clk_rate = clk_get_rate(clk_get(NULL, "osc_sys_ck"));

	sys_clk_rate = sys_clk_rate / 1000000;

	/* wait time for L3 clk stabilization = 4*REFCLK + 8*CLKOUTX2 */
	nr1 = (4 * (core_dpll_div_n + 1) * 2 * core_dpll_clkoutdiv_m2 *
	core_dpll_mul_m);
	nr2 = 8 * (core_dpll_div_n + 1);
	nr = nr1 + nr2;

	dr = 2 * sys_clk_rate * core_dpll_mul_m * core_dpll_clkoutdiv_m2;

	sdrc_clk_stab = nr / dr;

	/* Adding 2us to sdrc clk stab */
	sdrc_clk_stab = sdrc_clk_stab + 2;

	delay_sram = delay_sram_val();

	/* Calculate the number of MPU cycles to wait for SDRC to stabilize */
	_mpurate = arm_fck_p->rate / CYCLES_PER_MHZ;

	c = ((sdrc_clk_stab * _mpurate) / (delay_sram * 2));

	pr_debug("clock: changing CORE DPLL rate from %lu to %lu\n", clk->rate,
		 validrate);
	pr_debug("clock: SDRC CS0 timing params used:"
		 " RFR %08x CTRLA %08x CTRLB %08x MR %08x\n",
		 sdrc_cs0->rfr_ctrl, sdrc_cs0->actim_ctrla,
		 sdrc_cs0->actim_ctrlb, sdrc_cs0->mr);
	if (sdrc_cs1)
		pr_debug("clock: SDRC CS1 timing params used: "
		 " RFR %08x CTRLA %08x CTRLB %08x MR %08x\n",
		 sdrc_cs1->rfr_ctrl, sdrc_cs1->actim_ctrla,
		 sdrc_cs1->actim_ctrlb, sdrc_cs1->mr);

	if (sdrc_cs1)
		omap3_configure_core_dpll(
				  new_div, unlock_dll, c, rate > clk->rate,
				  sdrc_cs0->rfr_ctrl, sdrc_cs0->actim_ctrla,
				  sdrc_cs0->actim_ctrlb, sdrc_cs0->mr,
				  sdrc_cs1->rfr_ctrl, sdrc_cs1->actim_ctrla,
				  sdrc_cs1->actim_ctrlb, sdrc_cs1->mr);
	else
		omap3_configure_core_dpll(
				  new_div, unlock_dll, c, rate > clk->rate,
				  sdrc_cs0->rfr_ctrl, sdrc_cs0->actim_ctrla,
				  sdrc_cs0->actim_ctrlb, sdrc_cs0->mr,
				  0, 0, 0, 0);

	return 0;
}

/* Common clock code */

/*
 * As it is structured now, this will prevent an OMAP2/3 multiboot
 * kernel from compiling.  This will need further attention.
 */
#ifdef CONFIG_ARCH_OMAP3

struct clk_functions omap2_clk_functions = {
	.clk_enable		= omap2_clk_enable,
	.clk_disable		= omap2_clk_disable,
	.clk_round_rate		= omap2_clk_round_rate,
	.clk_set_rate		= omap2_clk_set_rate,
	.clk_set_parent		= omap2_clk_set_parent,
	.clk_disable_unused	= omap2_clk_disable_unused,
};

/*
 * Set clocks for bypass mode for reboot to work.
 */
void omap2_clk_prepare_for_reboot(void)
{
	int err = 0;

	/* REVISIT: Not ready for 343x */
#if 0
	u32 rate;

	if (vclk == NULL || sclk == NULL)
		return;

	rate = clk_get_rate(sclk);
	clk_set_rate(vclk, rate);
#endif

	/*
	 * PRCM on OMAP3 will drive SYS_OFFMODE low during DPLL3 warm reset.
	 * This causes Gaia sleep script to execute, usually killing VDD1 and
	 * VDD2 while code is running.  WA is to disable the sleep script
	 * before warm reset.
	 */
#ifdef CONFIG_TWL4030_POWER
	err = twl4030_remove_script(TWL4030_SLEEP_SCRIPT);
	if (err)
		pr_err("twl4030: error trying to disable sleep script!\n");
#endif
}

void omap3_clk_lock_dpll5(void)
{
	struct clk *dpll5_clk;
	struct clk *dpll5_m2_clk;

	dpll5_clk = clk_get(NULL, "dpll5_ck");
	clk_set_rate(dpll5_clk, DPLL5_FREQ_FOR_USBHOST);
	clk_enable(dpll5_clk);

	/* Enable autoidle to allow it to enter low power bypass */
	omap3_dpll_allow_idle(dpll5_clk);

	/* Program dpll5_m2_clk divider for no division */
	dpll5_m2_clk = clk_get(NULL, "dpll5_m2_ck");
	clk_enable(dpll5_m2_clk);
	clk_set_rate(dpll5_m2_clk, DPLL5_FREQ_FOR_USBHOST);

	clk_disable(dpll5_m2_clk);
	clk_disable(dpll5_clk);
	return;
}

/* REVISIT: Move this init stuff out into clock.c */

/*
 * Switch the MPU rate if specified on cmdline.
 * We cannot do this early until cmdline is parsed.
 */
static int __init omap2_clk_arch_init(void)
{
	struct clk *osc_sys_ck, *dpll1_ck, *arm_fck, *core_ck;
	unsigned long osc_sys_rate;

	if (!mpurate)
		return -EINVAL;

	/* XXX test these for success */
	dpll1_ck = clk_get(NULL, "dpll1_ck");
	arm_fck = clk_get(NULL, "arm_fck");
	core_ck = clk_get(NULL, "core_ck");
	osc_sys_ck = clk_get(NULL, "osc_sys_ck");

	/* REVISIT: not yet ready for 343x */
	if (clk_set_rate(dpll1_ck, mpurate))
		printk(KERN_ERR "*** Unable to set MPU rate\n");

	recalculate_root_clocks();

	osc_sys_rate = clk_get_rate(osc_sys_ck);

	pr_info("Switched to new clocking rate (Crystal/Core/MPU): "
		"%ld.%01ld/%ld/%ld MHz\n",
		(osc_sys_rate / 1000000),
		((osc_sys_rate / 100000) % 10),
		(clk_get_rate(core_ck) / 1000000),
		(clk_get_rate(arm_fck) / 1000000));

	calibrate_delay();

	return 0;
}
arch_initcall(omap2_clk_arch_init);


#endif
