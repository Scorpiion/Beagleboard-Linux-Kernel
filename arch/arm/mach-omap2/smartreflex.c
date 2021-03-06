/*
 * linux/arch/arm/mach-omap3/smartreflex.c
 *
 * OMAP34XX SmartReflex Voltage Control
 *
 * Copyright (C) 2008 Nokia Corporation
 * Kalle Jokiniemi
 *
 * Copyright (C) 2007 Texas Instruments, Inc.
 * Lesly A M <x0080970@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/i2c/twl.h>
#include <linux/io.h>

#include <plat/omap34xx.h>
#include <plat/control.h>
#include <plat/clock.h>
#include <plat/omap-pm.h>

#include "prm.h"
#include "omap3-opp.h"
#include "smartreflex.h"
#include "prm-regbits-34xx.h"

#define MAX_TRIES 100

#define SWCALC_OPP6_DELTA_NNT	379
#define SWCALC_OPP6_DELTA_PNT	227

#define ABB_MAX_SETTLING_TIME	30

#define ABB_FAST_OPP			1
#define ABB_NOMINAL_OPP			2
#define ABB_SLOW_OPP			3

/*
 * VDD1 and VDD2 OPPs derived from the bootarg 'mpurate'
 */
extern unsigned int vdd1_opp;
extern unsigned int vdd2_opp;
extern bool vdd_scale_down;

extern int __init omap2_clk_set_freq(void);


struct omap_sr {
	int		srid;
	int		is_sr_reset;
	int		is_autocomp_active;
	struct clk	*clk;
	struct clk	*vdd_opp_clk;
	u32		clk_length;
	u32		req_opp_no;
	u32		opp1_nvalue, opp2_nvalue, opp3_nvalue, opp4_nvalue;
	u32		opp5_nvalue, opp6_nvalue;
	u32		senp_mod, senn_mod;
	void __iomem	*srbase_addr;
	void __iomem	*vpbase_addr;
};

#define SR_REGADDR(offs)	(sr->srbase_addr + offset)

static inline void sr_write_reg(struct omap_sr *sr, unsigned offset, u32 value)
{
	__raw_writel(value, SR_REGADDR(offset));
}

static inline void sr_modify_reg(struct omap_sr *sr, unsigned offset, u32 mask,
					u32 value)
{
	u32 reg_val;

	reg_val = __raw_readl(SR_REGADDR(offset));
	reg_val &= ~mask;
	reg_val |= value;

	__raw_writel(reg_val, SR_REGADDR(offset));
}

static inline u32 sr_read_reg(struct omap_sr *sr, unsigned offset)
{
	return __raw_readl(SR_REGADDR(offset));
}

static int sr_clk_enable(struct omap_sr *sr)
{
	if (clk_enable(sr->clk) != 0) {
		pr_err("Could not enable %s\n", sr->clk->name);
		return -1;
	}

	if (cpu_is_omap3630())
		sr_modify_reg(sr, ERRCONFIG_36XX, SR_IDLEMODE_MASK,
				SR_SMART_IDLE);
	else
		/* set fclk- active , iclk- idle */
		sr_modify_reg(sr, ERRCONFIG, SR_CLKACTIVITY_MASK,
			SR_CLKACTIVITY_IOFF_FON);

	return 0;
}

static void sr_clk_disable(struct omap_sr *sr)
{
	if (cpu_is_omap3630())
		sr_modify_reg(sr, ERRCONFIG_36XX, SR_IDLEMODE_MASK,
				SR_FORCE_IDLE);
	else
		/* set fclk, iclk- idle */
		sr_modify_reg(sr, ERRCONFIG, SR_CLKACTIVITY_MASK,
				SR_CLKACTIVITY_IOFF_FOFF);

	clk_disable(sr->clk);
	sr->is_sr_reset = 1;
}

static struct omap_sr sr1 = {
	.srid			= SR1,
	.is_sr_reset		= 1,
	.is_autocomp_active	= 0,
	.clk_length		= 0,
	.srbase_addr		= OMAP2_L4_IO_ADDRESS(OMAP34XX_SR1_BASE),
};

static struct omap_sr sr2 = {
	.srid			= SR2,
	.is_sr_reset		= 1,
	.is_autocomp_active	= 0,
	.clk_length		= 0,
	.srbase_addr		= OMAP2_L4_IO_ADDRESS(OMAP34XX_SR2_BASE),
};

static void cal_reciprocal(u32 sensor, u32 *sengain, u32 *rnsen)
{
	u32 gn, rn, mul;

	for (gn = 0; gn < GAIN_MAXLIMIT; gn++) {
		mul = 1 << (gn + 8);
		rn = mul / sensor;
		if (rn < R_MAXLIMIT) {
			*sengain = gn;
			*rnsen = rn;
		}
	}
}

static u32 cal_test_nvalue(u32 sennval, u32 senpval)
{
	u32 senpgain, senngain;
	u32 rnsenp, rnsenn;

	/* Calculating the gain and reciprocal of the SenN and SenP values */
	cal_reciprocal(senpval, &senpgain, &rnsenp);
	cal_reciprocal(sennval, &senngain, &rnsenn);

	return (senpgain << NVALUERECIPROCAL_SENPGAIN_SHIFT) |
		(senngain << NVALUERECIPROCAL_SENNGAIN_SHIFT) |
		(rnsenp << NVALUERECIPROCAL_RNSENP_SHIFT) |
		(rnsenn << NVALUERECIPROCAL_RNSENN_SHIFT);
}

/* determine the current OPP from the frequency
 * we need to give this function last element of OPP rate table
 * and the frequency
 */
static u16 get_opp(struct omap_opp *opp_freq_table,
					unsigned long freq)
{
	struct omap_opp *prcm_config;

	prcm_config = opp_freq_table;

	if (prcm_config->rate <= freq)
		return prcm_config->opp_id; /* Return the Highest OPP */
	for (; prcm_config->rate; prcm_config--)
		if (prcm_config->rate < freq)
			return (prcm_config+1)->opp_id;
		else if (prcm_config->rate == freq)
			return prcm_config->opp_id;
	/* Return the least OPP */
	return (prcm_config+1)->opp_id;
}

static u16 get_vdd1_opp(void)
{
	u16 opp;

	if (sr1.vdd_opp_clk == NULL || IS_ERR(sr1.vdd_opp_clk) ||
							mpu_opps == NULL)
		return 0;

	opp = get_opp(mpu_opps + get_max_vdd1(), sr1.vdd_opp_clk->rate);
	return opp;
}

static u16 get_vdd2_opp(void)
{
	u16 opp;

	if (sr2.vdd_opp_clk == NULL || IS_ERR(sr2.vdd_opp_clk) ||
							l3_opps == NULL)
		return 0;

	opp = get_opp(l3_opps + get_max_vdd2(), sr2.vdd_opp_clk->rate);
	return opp;
}


static void sr_set_clk_length(struct omap_sr *sr)
{
	struct clk *sys_ck;
	u32 sys_clk_speed;

	sys_ck = clk_get(NULL, "sys_ck");
	sys_clk_speed = clk_get_rate(sys_ck);
	clk_put(sys_ck);

	switch (sys_clk_speed) {
	case 12000000:
		sr->clk_length = SRCLKLENGTH_12MHZ_SYSCLK;
		break;
	case 13000000:
		sr->clk_length = SRCLKLENGTH_13MHZ_SYSCLK;
		break;
	case 19200000:
		sr->clk_length = SRCLKLENGTH_19MHZ_SYSCLK;
		break;
	case 26000000:
		sr->clk_length = SRCLKLENGTH_26MHZ_SYSCLK;
		break;
	case 38400000:
		sr->clk_length = SRCLKLENGTH_38MHZ_SYSCLK;
		break;
	default:
		pr_err("Invalid sysclk value: %d\n", sys_clk_speed);
		break;
	}
}

static void swcalc_opp6_RG(u32 rFuse, u32 gainFuse, u32 deltaNT,
				u32* rAdj, u32* gainAdj)
{
	u32 nAdj;
	u32 g, r;

	nAdj = ((1 << (gainFuse + 8))/rFuse) + deltaNT;

	for (g = 0; g < GAIN_MAXLIMIT; g++) {
		r = (1 << (g + 8)) / nAdj;
		if (r < 256) {
			*rAdj = r;
			*gainAdj = g;
		}
	}
}

static u32 swcalc_opp6_nvalue(void)
{
	u32 opp5_nvalue, opp6_nvalue;
	u32 opp5_senPgain, opp5_senNgain, opp5_senPRN, opp5_senNRN;
	u32 opp6_senPgain, opp6_senNgain, opp6_senPRN, opp6_senNRN;

	opp5_nvalue = omap_ctrl_readl(OMAP343X_CONTROL_FUSE_OPP5_VDD1);

	opp5_senPgain = (opp5_nvalue & 0x00f00000) >> 0x14;
	opp5_senNgain = (opp5_nvalue & 0x000f0000) >> 0x10;

	opp5_senPRN = (opp5_nvalue & 0x0000ff00) >> 0x8;
	opp5_senNRN = (opp5_nvalue & 0x000000ff);

	swcalc_opp6_RG(opp5_senNRN, opp5_senNgain, SWCALC_OPP6_DELTA_NNT,
				&opp6_senNRN, &opp6_senNgain);

	swcalc_opp6_RG(opp5_senPRN, opp5_senPgain, SWCALC_OPP6_DELTA_PNT,
				&opp6_senPRN, &opp6_senPgain);

	opp6_nvalue = (opp6_senPgain << 0x14) | (opp6_senNgain < 0x10) |
			(opp6_senPRN << 0x8) | opp6_senNRN;

	return opp6_nvalue;
}

static void sr_set_efuse_nvalues(struct omap_sr *sr)
{
	if (sr->srid == SR1) {
		if (cpu_is_omap3630()) {
			sr->senn_mod = sr->senp_mod = 0x1;

			sr->opp4_nvalue = omap_ctrl_readl(OMAP36XX_CONTROL_FUSE_OPP4_VDD1);
			sr->opp3_nvalue = omap_ctrl_readl(OMAP36XX_CONTROL_FUSE_OPP3_VDD1);
			sr->opp2_nvalue = omap_ctrl_readl(OMAP36XX_CONTROL_FUSE_OPP2_VDD1);
			sr->opp1_nvalue = omap_ctrl_readl(OMAP36XX_CONTROL_FUSE_OPP1_VDD1);
		} else {
			sr->senn_mod = (omap_ctrl_readl(OMAP343X_CONTROL_FUSE_SR) &
						OMAP343X_SR1_SENNENABLE_MASK) >>
						OMAP343X_SR1_SENNENABLE_SHIFT;
			sr->senp_mod = (omap_ctrl_readl(OMAP343X_CONTROL_FUSE_SR) &
						OMAP343X_SR1_SENPENABLE_MASK) >>
						OMAP343X_SR1_SENPENABLE_SHIFT;

			sr->opp6_nvalue = swcalc_opp6_nvalue();
			sr->opp5_nvalue = omap_ctrl_readl(
						OMAP343X_CONTROL_FUSE_OPP5_VDD1);
			sr->opp4_nvalue = omap_ctrl_readl(
						OMAP343X_CONTROL_FUSE_OPP4_VDD1);
			sr->opp3_nvalue = omap_ctrl_readl(
						OMAP343X_CONTROL_FUSE_OPP3_VDD1);
			sr->opp2_nvalue = omap_ctrl_readl(
						OMAP343X_CONTROL_FUSE_OPP2_VDD1);
			sr->opp1_nvalue = omap_ctrl_readl(
						OMAP343X_CONTROL_FUSE_OPP1_VDD1);
		}
	} else if (sr->srid == SR2) {
		if (cpu_is_omap3630()) {
			sr->senn_mod = sr->senp_mod = 0x1;

			sr->opp1_nvalue = omap_ctrl_readl(OMAP36XX_CONTROL_FUSE_OPP1_VDD2);
			sr->opp2_nvalue = omap_ctrl_readl(OMAP36XX_CONTROL_FUSE_OPP2_VDD2);
		} else {
			sr->senn_mod = (omap_ctrl_readl(OMAP343X_CONTROL_FUSE_SR) &
						OMAP343X_SR2_SENNENABLE_MASK) >>
						OMAP343X_SR2_SENNENABLE_SHIFT;

			sr->senp_mod = (omap_ctrl_readl(OMAP343X_CONTROL_FUSE_SR) &
						OMAP343X_SR2_SENPENABLE_MASK) >>
						OMAP343X_SR2_SENPENABLE_SHIFT;

			sr->opp3_nvalue = omap_ctrl_readl(
						OMAP343X_CONTROL_FUSE_OPP3_VDD2);
			sr->opp2_nvalue = omap_ctrl_readl(
						OMAP343X_CONTROL_FUSE_OPP2_VDD2);
			sr->opp1_nvalue = omap_ctrl_readl(
						OMAP343X_CONTROL_FUSE_OPP1_VDD2);
		}
	}
}

/* Hard coded nvalues for testing purposes, may cause device to hang! */
static void sr_set_testing_nvalues(struct omap_sr *sr)
{
	if (sr->srid == SR1) {
		if (cpu_is_omap3630()) {
			sr->senp_mod = 0x1;
			sr->senn_mod = 0x1;

			/* calculate nvalues for each opp */
			sr->opp1_nvalue = cal_test_nvalue(581, 489);
			sr->opp2_nvalue = cal_test_nvalue(1072, 910);
			sr->opp3_nvalue = cal_test_nvalue(1405, 1200);
			sr->opp4_nvalue = cal_test_nvalue(1842, 1580);
			sr->opp5_nvalue = cal_test_nvalue(1842, 1580);
        } else {
			sr->senp_mod = 0x03;	/* SenN-M5 enabled */
			sr->senn_mod = 0x03;

			/* calculate nvalues for each opp */
			sr->opp5_nvalue = cal_test_nvalue(0xacd + 0x330, 0x848 + 0x330);
			sr->opp4_nvalue = cal_test_nvalue(0x964 + 0x2a0, 0x727 + 0x2a0);
			sr->opp3_nvalue = cal_test_nvalue(0x85b + 0x200, 0x655 + 0x200);
			sr->opp2_nvalue = cal_test_nvalue(0x506 + 0x1a0, 0x3be + 0x1a0);
			sr->opp1_nvalue = cal_test_nvalue(0x373 + 0x100, 0x28c + 0x100);
		}
	} else if (sr->srid == SR2) {
		if (cpu_is_omap3630()) {
			sr->senp_mod = 0x1;
			sr->senn_mod = 0x1;

			sr->opp1_nvalue = cal_test_nvalue(556, 468);
			sr->opp2_nvalue = cal_test_nvalue(1099, 933);
		} else {
			sr->senp_mod = 0x03;
			sr->senn_mod = 0x03;

			sr->opp3_nvalue = cal_test_nvalue(0x76f + 0x200, 0x579 + 0x200);
			sr->opp2_nvalue = cal_test_nvalue(0x4f5 + 0x1c0, 0x390 + 0x1c0);
			sr->opp1_nvalue = cal_test_nvalue(0x359, 0x25d);
		}
	}

}

static void sr_set_nvalues(struct omap_sr *sr)
{
	if (SR_TESTING_NVALUES)
		sr_set_testing_nvalues(sr);
	else
		sr_set_efuse_nvalues(sr);
}

/**
 * sr_voltagescale_adaptive_body_bias - controls ABB ldo during voltage scaling
 * @target_volt: target voltage determines if ABB ldo is active or bypassed
 *
 * Adaptive Body-Bias is a technique in all OMAP silicon that uses the 45nm
 * process.  ABB can boost voltage in high OPPs for silicon with weak
 * characteristics (forward Body-Bias) as well as lower voltage in low OPPs
 * for silicon with strong characteristics (Reverse Body-Bias).
 *
 * Only Foward Body-Bias for operating at high OPPs is implemented below, per
 * recommendations from silicon team.
 * Reverse Body-Bias for saving power in active cases and sleep cases is not
 * yet implemented.
 */
static int sr_voltagescale_adaptive_body_bias(u32 target_opp_no)
{
	u32 sr2en_enabled;
	int timeout;
	int sr2_wtcnt_value;
	struct clk *sys_ck;

	sys_ck = clk_get(NULL, "sys_ck");
	if (IS_ERR(sys_ck)) {
		pr_warning("%s: Could not get the sys clk to calculate"
            "SR2_WTCNT_VALUE \n", __func__);
        return -ENOENT;
    }

	/* calculate SR2_WTCNT_VALUE settling time */
	sr2_wtcnt_value = (ABB_MAX_SETTLING_TIME *
		(clk_get_rate(sys_ck) / 1000000) / 8);

	clk_put(sys_ck);

	/* has SR2EN been enabled previously? */
	sr2en_enabled = (prm_read_mod_reg(OMAP3430_GR_MOD,
			OMAP3_PRM_LDO_ABB_CTRL_OFFSET) &
			OMAP3630_SR2EN);

	/* select fast, nominal or slow OPP for ABB ldo */
	if (target_opp_no >= VDD1_OPP4) {
		/* program for fast opp - enable FBB */
		prm_rmw_mod_reg_bits(OMAP3630_OPP_SEL_MASK,
				(ABB_FAST_OPP << OMAP3630_OPP_SEL_SHIFT),
				OMAP3430_GR_MOD,
				OMAP3_PRM_LDO_ABB_SETUP_OFFSET);

		/* enable the ABB ldo if not done already */
		if (!sr2en_enabled)
			prm_set_mod_reg_bits(OMAP3630_SR2EN,
					OMAP3430_GR_MOD,
					OMAP3_PRM_LDO_ABB_CTRL_OFFSET);
	} else if (sr2en_enabled) {
		/* program for nominal opp - bypass ABB ldo */
		prm_rmw_mod_reg_bits(OMAP3630_OPP_SEL_MASK,
				(ABB_NOMINAL_OPP << OMAP3630_OPP_SEL_SHIFT),
				OMAP3430_GR_MOD,
				OMAP3_PRM_LDO_ABB_SETUP_OFFSET);
	} else {
		/* nothing to do here yet... might enable RBB here someday */
		return 0;
	}

	/* set ACTIVE_FBB_SEL for all 45nm silicon */
	prm_set_mod_reg_bits(OMAP3630_ACTIVE_FBB_SEL,
			OMAP3430_GR_MOD,
			OMAP3_PRM_LDO_ABB_CTRL_OFFSET);

	/* program settling time of 30us for ABB ldo transition */
	prm_rmw_mod_reg_bits(OMAP3630_SR2_WTCNT_VALUE_MASK,
			(sr2_wtcnt_value << OMAP3630_SR2_WTCNT_VALUE_SHIFT),
			OMAP3430_GR_MOD,
			OMAP3_PRM_LDO_ABB_CTRL_OFFSET);

	/* clear ABB ldo interrupt status */
	prm_write_mod_reg(OMAP3630_ABB_LDO_TRANXDONE_ST,
			OCP_MOD,
			OMAP2_PRCM_IRQSTATUS_MPU_OFFSET);

	/* enable ABB LDO OPP change */
	prm_set_mod_reg_bits(OMAP3630_OPP_CHANGE,
			OMAP3430_GR_MOD,
			OMAP3_PRM_LDO_ABB_SETUP_OFFSET);

	timeout = 0;

	/* wait until OPP change completes */
	while ((timeout < ABB_MAX_SETTLING_TIME ) &&
			(!(prm_read_mod_reg(OCP_MOD,
						OMAP2_PRCM_IRQSTATUS_MPU_OFFSET) &
					OMAP3630_ABB_LDO_TRANXDONE_ST))) {
		udelay(1);
		timeout++;
	}

	if (timeout == ABB_MAX_SETTLING_TIME)
		pr_debug("ABB: TRANXDONE timed out waiting for OPP change\n");

	timeout = 0;

	/* Clear all pending TRANXDONE interrupts/status */
	while (timeout < ABB_MAX_SETTLING_TIME) {
		prm_write_mod_reg(OMAP3630_ABB_LDO_TRANXDONE_ST,
				OCP_MOD,
				OMAP2_PRCM_IRQSTATUS_MPU_OFFSET);
		if (!(prm_read_mod_reg(OCP_MOD,
						OMAP2_PRCM_IRQSTATUS_MPU_OFFSET)
					& OMAP3630_ABB_LDO_TRANXDONE_ST))
			break;

		udelay(1);
		timeout++;
	}
	if (timeout == ABB_MAX_SETTLING_TIME)
		pr_debug("ABB: TRANXDONE timed out trying to clear status\n");

	return 0;
}


static void sr_configure_vp(int srid)
{
	u32 vpconfig;
	u32 vsel;
	u32 target_opp_no;

	if (srid == SR1) {
		if (vdd1_opp == 0)
			target_opp_no = get_vdd1_opp();
		else
			target_opp_no = vdd1_opp;

		if (!target_opp_no)
			/* Assume Nominal OPP as current OPP unknown */
			target_opp_no = VDD1_OPP3;

		vsel = mpu_opps[target_opp_no].vsel;

		vpconfig = PRM_VP1_CONFIG_ERROROFFSET |
			PRM_VP1_CONFIG_ERRORGAIN |
			PRM_VP1_CONFIG_TIMEOUTEN |
			vsel << OMAP3430_INITVOLTAGE_SHIFT;

		/*
		 * Update the 'ON' voltage levels based on the VSEL.
		 * (See spruf8c.pdf sec 1.5.3.1)
		 */
		prm_rmw_mod_reg_bits(OMAP3430_VC_CMD_ON_MASK,
				(vsel << OMAP3430_VC_CMD_ON_SHIFT),
				OMAP3430_GR_MOD,
				OMAP3_PRM_VC_CMD_VAL_0_OFFSET);

		prm_write_mod_reg(vpconfig, OMAP3430_GR_MOD,
					OMAP3_PRM_VP1_CONFIG_OFFSET);
		prm_write_mod_reg(PRM_VP1_VSTEPMIN_SMPSWAITTIMEMIN |
					PRM_VP1_VSTEPMIN_VSTEPMIN,
					OMAP3430_GR_MOD,
					OMAP3_PRM_VP1_VSTEPMIN_OFFSET);

		prm_write_mod_reg(PRM_VP1_VSTEPMAX_SMPSWAITTIMEMAX |
					PRM_VP1_VSTEPMAX_VSTEPMAX,
					OMAP3430_GR_MOD,
					OMAP3_PRM_VP1_VSTEPMAX_OFFSET);

		prm_write_mod_reg(PRM_VP1_VLIMITTO_VDDMAX |
					PRM_VP1_VLIMITTO_VDDMIN |
					PRM_VP1_VLIMITTO_TIMEOUT,
					OMAP3430_GR_MOD,
					OMAP3_PRM_VP1_VLIMITTO_OFFSET);

		/* Trigger initVDD value copy to voltage processor */
		prm_set_mod_reg_bits(PRM_VP1_CONFIG_INITVDD, OMAP3430_GR_MOD,
				     OMAP3_PRM_VP1_CONFIG_OFFSET);

		/* Clear initVDD copy trigger bit */
		prm_clear_mod_reg_bits(PRM_VP1_CONFIG_INITVDD, OMAP3430_GR_MOD,
				       OMAP3_PRM_VP1_CONFIG_OFFSET);

		/* Force update of voltage */
		prm_set_mod_reg_bits(OMAP3430_FORCEUPDATE, OMAP3430_GR_MOD,
				     OMAP3_PRM_VP1_CONFIG_OFFSET);
		/* Clear force bit */
		prm_clear_mod_reg_bits(OMAP3430_FORCEUPDATE, OMAP3430_GR_MOD,
				       OMAP3_PRM_VP1_CONFIG_OFFSET);

		if(cpu_is_omap3630())
			sr_voltagescale_adaptive_body_bias(target_opp_no);
	} else if (srid == SR2) {
		if (vdd2_opp == 0)
			target_opp_no = get_vdd2_opp();
		else
			target_opp_no = vdd2_opp;

		if (!target_opp_no) {
			/* Assume Nominal OPP */
			if (cpu_is_omap3630())
				target_opp_no = VDD2_OPP2;
			else
				target_opp_no = VDD2_OPP3;
		}

		vsel = l3_opps[target_opp_no].vsel;

		/*
		 * Update the 'ON' voltage levels based on the VSEL.
		 * (See spruf8c.pdf sec 1.5.3.1)
		 */
		prm_rmw_mod_reg_bits(OMAP3430_VC_CMD_ON_MASK,
				(vsel << OMAP3430_VC_CMD_ON_SHIFT),
				OMAP3430_GR_MOD,
				OMAP3_PRM_VC_CMD_VAL_1_OFFSET);

		vpconfig = PRM_VP2_CONFIG_ERROROFFSET |
			PRM_VP2_CONFIG_ERRORGAIN |
			PRM_VP2_CONFIG_TIMEOUTEN |
			vsel << OMAP3430_INITVOLTAGE_SHIFT;

		prm_write_mod_reg(vpconfig, OMAP3430_GR_MOD,
					OMAP3_PRM_VP2_CONFIG_OFFSET);
		prm_write_mod_reg(PRM_VP2_VSTEPMIN_SMPSWAITTIMEMIN |
					PRM_VP2_VSTEPMIN_VSTEPMIN,
					OMAP3430_GR_MOD,
					OMAP3_PRM_VP2_VSTEPMIN_OFFSET);

		prm_write_mod_reg(PRM_VP2_VSTEPMAX_SMPSWAITTIMEMAX |
					PRM_VP2_VSTEPMAX_VSTEPMAX,
					OMAP3430_GR_MOD,
					OMAP3_PRM_VP2_VSTEPMAX_OFFSET);

		prm_write_mod_reg(PRM_VP2_VLIMITTO_VDDMAX |
					PRM_VP2_VLIMITTO_VDDMIN |
					PRM_VP2_VLIMITTO_TIMEOUT,
					OMAP3430_GR_MOD,
					OMAP3_PRM_VP2_VLIMITTO_OFFSET);

		/* Trigger initVDD value copy to voltage processor */
		prm_set_mod_reg_bits(PRM_VP1_CONFIG_INITVDD, OMAP3430_GR_MOD,
				     OMAP3_PRM_VP2_CONFIG_OFFSET);

		/* Clear initVDD copy trigger bit */
		prm_clear_mod_reg_bits(PRM_VP1_CONFIG_INITVDD, OMAP3430_GR_MOD,
				       OMAP3_PRM_VP2_CONFIG_OFFSET);

		/* Force update of voltage */
		prm_set_mod_reg_bits(OMAP3430_FORCEUPDATE, OMAP3430_GR_MOD,
				     OMAP3_PRM_VP2_CONFIG_OFFSET);
		/* Clear force bit */
		prm_clear_mod_reg_bits(OMAP3430_FORCEUPDATE, OMAP3430_GR_MOD,
				       OMAP3_PRM_VP2_CONFIG_OFFSET);
	}
}

static void sr_configure(struct omap_sr *sr)
{
	u32 sr_config;
	u32 senp_en , senn_en;
	u32 senp_en_shift, senn_en_shift, err_config;

	if (cpu_is_omap3630()) {
		senp_en_shift = SRCONFIG_SENPENABLE_SHIFT_36XX;
		senn_en_shift = SRCONFIG_SENNENABLE_SHIFT_36XX;
		err_config = ERRCONFIG_36XX;
	} else {
		senp_en_shift = SRCONFIG_SENPENABLE_SHIFT;
		senn_en_shift = SRCONFIG_SENNENABLE_SHIFT;
		err_config = ERRCONFIG;
	}

	if (sr->clk_length == 0)
		sr_set_clk_length(sr);

	senp_en = sr->senp_mod;
	senn_en = sr->senn_mod;
	if (sr->srid == SR1) {
		sr_config = SR1_SRCONFIG_ACCUMDATA |
			(sr->clk_length << SRCONFIG_SRCLKLENGTH_SHIFT) |
			SRCONFIG_SENENABLE | SRCONFIG_ERRGEN_EN |
			SRCONFIG_MINMAXAVG_EN |
			(senn_en << senn_en_shift) |
			(senp_en << senp_en_shift) |
			SRCONFIG_DELAYCTRL;

		sr_write_reg(sr, SRCONFIG, sr_config);
		sr_write_reg(sr, AVGWEIGHT, SR1_AVGWEIGHT_SENPAVGWEIGHT |
					SR1_AVGWEIGHT_SENNAVGWEIGHT);

		sr_modify_reg(sr, err_config, (SR_ERRWEIGHT_MASK |
			SR_ERRMAXLIMIT_MASK | SR_ERRMINLIMIT_MASK),
			(SR1_ERRWEIGHT | SR1_ERRMAXLIMIT | SR1_ERRMINLIMIT));

	} else if (sr->srid == SR2) {
		sr_config = SR2_SRCONFIG_ACCUMDATA |
			(sr->clk_length << SRCONFIG_SRCLKLENGTH_SHIFT) |
			SRCONFIG_SENENABLE | SRCONFIG_ERRGEN_EN |
			SRCONFIG_MINMAXAVG_EN |
			(senn_en << senn_en_shift) |
			(senp_en << senp_en_shift) |
			SRCONFIG_DELAYCTRL;

		sr_write_reg(sr, SRCONFIG, sr_config);
		sr_write_reg(sr, AVGWEIGHT, SR2_AVGWEIGHT_SENPAVGWEIGHT |
					SR2_AVGWEIGHT_SENNAVGWEIGHT);
		sr_modify_reg(sr, err_config, (SR_ERRWEIGHT_MASK |
			SR_ERRMAXLIMIT_MASK | SR_ERRMINLIMIT_MASK),
			(SR2_ERRWEIGHT | SR2_ERRMAXLIMIT | SR2_ERRMINLIMIT));

	}
	sr->is_sr_reset = 0;
}

static int sr_reset_voltage(int srid)
{
	u32 target_opp_no, vsel = 0;
	u32 reg_addr = 0;
	u32 loop_cnt = 0, retries_cnt = 0;
	u32 vc_bypass_value;
	u32 t2_smps_steps = 0;
	u32 t2_smps_delay = 0;
	u32 prm_vp1_voltage, prm_vp2_voltage;

	if (srid == SR1) {
		target_opp_no = get_vdd1_opp();

		if (!target_opp_no) {
			pr_info("Current OPP unknown: Cannot reset voltage\n");
			return 1;
		}
		vsel = mpu_opps[target_opp_no].vsel;
		reg_addr = R_VDD1_SR_CONTROL;
		prm_vp1_voltage = prm_read_mod_reg(OMAP3430_GR_MOD,
						OMAP3_PRM_VP1_VOLTAGE_OFFSET);
		t2_smps_steps = abs(vsel - prm_vp1_voltage);
	} else if (srid == SR2) {
		target_opp_no = get_vdd2_opp();
		if (!target_opp_no) {
			pr_info("Current OPP unknown: Cannot reset voltage\n");
			return 1;
		}
		vsel = l3_opps[target_opp_no].vsel;
		reg_addr = R_VDD2_SR_CONTROL;
		prm_vp2_voltage = prm_read_mod_reg(OMAP3430_GR_MOD,
						OMAP3_PRM_VP2_VOLTAGE_OFFSET);
		t2_smps_steps = abs(vsel - prm_vp2_voltage);
	}

	vc_bypass_value = (vsel << OMAP3430_DATA_SHIFT) |
			(reg_addr << OMAP3430_REGADDR_SHIFT) |
			(R_SRI2C_SLAVE_ADDR << OMAP3430_SLAVEADDR_SHIFT);

	prm_write_mod_reg(vc_bypass_value, OMAP3430_GR_MOD,
			OMAP3_PRM_VC_BYPASS_VAL_OFFSET);

	vc_bypass_value = prm_set_mod_reg_bits(OMAP3430_VALID, OMAP3430_GR_MOD,
					OMAP3_PRM_VC_BYPASS_VAL_OFFSET);

	while ((vc_bypass_value & OMAP3430_VALID) != 0x0) {
		loop_cnt++;
		if (retries_cnt > 10) {
			pr_info("Loop count exceeded in check SR I2C"
								"write\n");
			return 1;
		}
		if (loop_cnt > 50) {
			retries_cnt++;
			loop_cnt = 0;
			udelay(10);
		}
		vc_bypass_value = prm_read_mod_reg(OMAP3430_GR_MOD,
					OMAP3_PRM_VC_BYPASS_VAL_OFFSET);
	}

	/*
	 *  T2 SMPS slew rate (min) 4mV/uS, step size 12.5mV,
	 *  2us added as buffer.
	 */
	t2_smps_delay = ((t2_smps_steps * 125) / 40) + 2;
	udelay(t2_smps_delay);

	return 0;
}

static int sr_enable(struct omap_sr *sr, u32 target_opp_no)
{
	u32 nvalue_reciprocal, v;
	u32 inten, intst, err_config;

	if (!(mpu_opps && l3_opps)) {
		pr_notice("VSEL values not found\n");
		return false;
	}

	sr->req_opp_no = target_opp_no;

	if (sr->srid == SR1) {
		switch (target_opp_no) {
		case 6:
			nvalue_reciprocal = sr->opp6_nvalue;
			break;
		case 5:
			nvalue_reciprocal = sr->opp5_nvalue;
			break;
		case 4:
			nvalue_reciprocal = sr->opp4_nvalue;
			break;
		case 3:
			nvalue_reciprocal = sr->opp3_nvalue;
			break;
		case 2:
			nvalue_reciprocal = sr->opp2_nvalue;
			break;
		case 1:
			nvalue_reciprocal = sr->opp1_nvalue;
			break;
		default:
			nvalue_reciprocal = sr->opp3_nvalue;
			break;
		}
	} else {
		switch (target_opp_no) {
		case 3:
			nvalue_reciprocal = sr->opp3_nvalue;
			break;
		case 2:
			nvalue_reciprocal = sr->opp2_nvalue;
			break;
		case 1:
			nvalue_reciprocal = sr->opp1_nvalue;
			break;
		default:
			nvalue_reciprocal = sr->opp3_nvalue;
			break;
		}
	}

	if (nvalue_reciprocal == 0) {
		pr_notice("OPP%d doesn't support SmartReflex\n",
								target_opp_no);
		return false;
	}

	sr_write_reg(sr, NVALUERECIPROCAL, nvalue_reciprocal);

	/* Enable the interrupt */
	if (cpu_is_omap3630()) {
		inten = ERRCONFIG_VPBOUNDINTEN_36XX;
		intst = ERRCONFIG_VPBOUNDINTST_36XX;
		err_config = ERRCONFIG_36XX;
	} else {
		inten = ERRCONFIG_VPBOUNDINTEN;
		intst = ERRCONFIG_VPBOUNDINTST;
		err_config = ERRCONFIG;
	}

	sr_modify_reg(sr, err_config,
			(inten | intst),
			(inten | intst));

	if (sr->srid == SR1) {
		/* set/latch init voltage */
		v = prm_read_mod_reg(OMAP3430_GR_MOD,
				     OMAP3_PRM_VP1_CONFIG_OFFSET);
		v &= ~(OMAP3430_INITVOLTAGE_MASK | OMAP3430_INITVDD);
		v |= mpu_opps[target_opp_no].vsel <<
			OMAP3430_INITVOLTAGE_SHIFT;
		prm_write_mod_reg(v, OMAP3430_GR_MOD,
				  OMAP3_PRM_VP1_CONFIG_OFFSET);
		/* write1 to latch */
		prm_set_mod_reg_bits(OMAP3430_INITVDD, OMAP3430_GR_MOD,
				     OMAP3_PRM_VP1_CONFIG_OFFSET);
		/* write2 clear */
		prm_clear_mod_reg_bits(OMAP3430_INITVDD, OMAP3430_GR_MOD,
				       OMAP3_PRM_VP1_CONFIG_OFFSET);
		/* Enable VP1 */
		prm_set_mod_reg_bits(PRM_VP1_CONFIG_VPENABLE, OMAP3430_GR_MOD,
				     OMAP3_PRM_VP1_CONFIG_OFFSET);
	} else if (sr->srid == SR2) {
		/* set/latch init voltage */
		v = prm_read_mod_reg(OMAP3430_GR_MOD,
				     OMAP3_PRM_VP2_CONFIG_OFFSET);
		v &= ~(OMAP3430_INITVOLTAGE_MASK | OMAP3430_INITVDD);
		v |= l3_opps[target_opp_no].vsel <<
			OMAP3430_INITVOLTAGE_SHIFT;
		prm_write_mod_reg(v, OMAP3430_GR_MOD,
				  OMAP3_PRM_VP2_CONFIG_OFFSET);
		/* write1 to latch */
		prm_set_mod_reg_bits(OMAP3430_INITVDD, OMAP3430_GR_MOD,
				     OMAP3_PRM_VP2_CONFIG_OFFSET);
		/* write2 clear */
		prm_clear_mod_reg_bits(OMAP3430_INITVDD, OMAP3430_GR_MOD,
				       OMAP3_PRM_VP2_CONFIG_OFFSET);
		/* Enable VP2 */
		prm_set_mod_reg_bits(PRM_VP2_CONFIG_VPENABLE, OMAP3430_GR_MOD,
				     OMAP3_PRM_VP2_CONFIG_OFFSET);
	}

	/* SRCONFIG - enable SR */
	sr_modify_reg(sr, SRCONFIG, SRCONFIG_SRENABLE, SRCONFIG_SRENABLE);
	return true;
}

static void sr_disable(struct omap_sr *sr)
{
	u32 i = 0;

	sr->is_sr_reset = 1;

	/* SRCONFIG - disable SR */
	sr_modify_reg(sr, SRCONFIG, SRCONFIG_SRENABLE, ~SRCONFIG_SRENABLE);

	if (sr->srid == SR1) {
		/* Wait for VP idle before disabling VP */
		while ((!prm_read_mod_reg(OMAP3430_GR_MOD,
					OMAP3_PRM_VP1_STATUS_OFFSET))
					&& i++ < MAX_TRIES)
			udelay(1);

		if (i >= MAX_TRIES)
			pr_warning("VP1 not idle, still going ahead with \
							VP1 disable\n");

		/* Disable VP1 */
		prm_clear_mod_reg_bits(PRM_VP1_CONFIG_VPENABLE, OMAP3430_GR_MOD,
					OMAP3_PRM_VP1_CONFIG_OFFSET);

	} else if (sr->srid == SR2) {
		/* Wait for VP idle before disabling VP */
		while ((!prm_read_mod_reg(OMAP3430_GR_MOD,
					OMAP3_PRM_VP2_STATUS_OFFSET))
					&& i++ < MAX_TRIES)
			udelay(1);

		if (i >= MAX_TRIES)
			pr_warning("VP2 not idle, still going ahead with \
							 VP2 disable\n");

		/* Disable VP2 */
		prm_clear_mod_reg_bits(PRM_VP2_CONFIG_VPENABLE, OMAP3430_GR_MOD,
					OMAP3_PRM_VP2_CONFIG_OFFSET);
	}
}


void sr_start_vddautocomap(int srid, u32 target_opp_no)
{
	struct omap_sr *sr = NULL;

	if (srid == SR1)
		sr = &sr1;
	else if (srid == SR2)
		sr = &sr2;
	else
		return;

	if (sr->is_sr_reset == 1) {
		sr_clk_enable(sr);
		sr_configure(sr);
	}

	sr->is_autocomp_active = 1;
	if (!sr_enable(sr, target_opp_no)) {
		sr->is_autocomp_active = 0;
		if (sr->is_sr_reset == 1)
			sr_clk_disable(sr);
	}
}
EXPORT_SYMBOL(sr_start_vddautocomap);

int sr_stop_vddautocomap(int srid)
{
	struct omap_sr *sr = NULL;

	if (srid == SR1)
		sr = &sr1;
	else if (srid == SR2)
		sr = &sr2;
	else
		return -EINVAL;

	if (sr->is_autocomp_active == 1) {
		sr_disable(sr);
		sr_clk_disable(sr);
		sr->is_autocomp_active = 0;
		/* Reset the volatage for current OPP */
		sr_reset_voltage(srid);
		return true;
	} else
		return false;

}
EXPORT_SYMBOL(sr_stop_vddautocomap);

void enable_smartreflex(int srid)
{
	u32 target_opp_no = 0;
	struct omap_sr *sr = NULL;

	if (srid == SR1)
		sr = &sr1;
	else if (srid == SR2)
		sr = &sr2;
	else
		return;

	if (sr->is_autocomp_active == 1) {
		if (sr->is_sr_reset == 1) {
			/* Enable SR clks */
			sr_clk_enable(sr);

			if (srid == SR1)
				target_opp_no = get_vdd1_opp();
			else if (srid == SR2)
				target_opp_no = get_vdd2_opp();

			if (!target_opp_no) {
				pr_info("Current OPP unknown \
						 Cannot configure SR\n");
			}

			sr_configure(sr);

			if (!sr_enable(sr, target_opp_no))
				sr_clk_disable(sr);
		}
	}
}

void disable_smartreflex(int srid)
{
	u32 i = 0;

	struct omap_sr *sr = NULL;

	if (srid == SR1)
		sr = &sr1;
	else if (srid == SR2)
		sr = &sr2;
	else
		return;

	if (sr->is_autocomp_active == 1) {
		if (sr->is_sr_reset == 0) {

			sr->is_sr_reset = 1;
			/* SRCONFIG - disable SR */
			sr_modify_reg(sr, SRCONFIG, SRCONFIG_SRENABLE,
							~SRCONFIG_SRENABLE);

			/* Disable SR clk */
			sr_clk_disable(sr);
			if (sr->srid == SR1) {
				/* Wait for VP idle before disabling VP */
				while ((!prm_read_mod_reg(OMAP3430_GR_MOD,
						OMAP3_PRM_VP1_STATUS_OFFSET))
						&& i++ < MAX_TRIES)
					udelay(1);

				if (i >= MAX_TRIES)
					pr_warning("VP1 not idle, still going \
						ahead with VP1 disable\n");

				/* Disable VP1 */
				prm_clear_mod_reg_bits(PRM_VP1_CONFIG_VPENABLE,
						OMAP3430_GR_MOD,
						OMAP3_PRM_VP1_CONFIG_OFFSET);
			} else if (sr->srid == SR2) {
				/* Wait for VP idle before disabling VP */
				while ((!prm_read_mod_reg(OMAP3430_GR_MOD,
						OMAP3_PRM_VP2_STATUS_OFFSET))
						&& i++ < MAX_TRIES)
					udelay(1);

				if (i >= MAX_TRIES)
					pr_warning("VP2 not idle, still going \
						 ahead with VP2 disable\n");

				/* Disable VP2 */
				prm_clear_mod_reg_bits(PRM_VP2_CONFIG_VPENABLE,
						OMAP3430_GR_MOD,
						OMAP3_PRM_VP2_CONFIG_OFFSET);
			}
			/* Reset the volatage for current OPP */
			sr_reset_voltage(srid);
		}
	}
}

/* Voltage Scaling using SR VCBYPASS */
int sr_voltagescale_vcbypass(u32 target_opp, u32 current_opp,
					u8 target_vsel, u8 current_vsel)
{
	int sr_status = 0;
	u32 vdd, target_opp_no, current_opp_no;
	u32 vc_bypass_value;
	u32 reg_addr = 0;
	u32 loop_cnt = 0, retries_cnt = 0;
	u32 t2_smps_steps = 0;
	u32 t2_smps_delay = 0;

	vdd = get_vdd(target_opp);
	target_opp_no = get_opp_no(target_opp);
	current_opp_no = get_opp_no(current_opp);

	if (vdd == VDD1_OPP) {
		sr_status = sr_stop_vddautocomap(SR1);
		t2_smps_steps = abs(target_vsel - current_vsel);

		prm_rmw_mod_reg_bits(OMAP3430_VC_CMD_ON_MASK,
				(target_vsel << OMAP3430_VC_CMD_ON_SHIFT),
				OMAP3430_GR_MOD,
				OMAP3_PRM_VC_CMD_VAL_0_OFFSET);
		reg_addr = R_VDD1_SR_CONTROL;

	} else if (vdd == VDD2_OPP) {
		sr_status = sr_stop_vddautocomap(SR2);
		t2_smps_steps =  abs(target_vsel - current_vsel);

		prm_rmw_mod_reg_bits(OMAP3430_VC_CMD_ON_MASK,
				(target_vsel << OMAP3430_VC_CMD_ON_SHIFT),
				OMAP3430_GR_MOD,
				OMAP3_PRM_VC_CMD_VAL_1_OFFSET);
		reg_addr = R_VDD2_SR_CONTROL;
	}

	vc_bypass_value = (target_vsel << OMAP3430_DATA_SHIFT) |
			(reg_addr << OMAP3430_REGADDR_SHIFT) |
			(R_SRI2C_SLAVE_ADDR << OMAP3430_SLAVEADDR_SHIFT);

	prm_write_mod_reg(vc_bypass_value, OMAP3430_GR_MOD,
			OMAP3_PRM_VC_BYPASS_VAL_OFFSET);

	vc_bypass_value = prm_set_mod_reg_bits(OMAP3430_VALID, OMAP3430_GR_MOD,
					OMAP3_PRM_VC_BYPASS_VAL_OFFSET);

	while ((vc_bypass_value & OMAP3430_VALID) != 0x0) {
		loop_cnt++;
		if (retries_cnt > 10) {
			pr_info("Loop count exceeded in check SR I2C"
								"write\n");
			return 1;
		}
		if (loop_cnt > 50) {
			retries_cnt++;
			loop_cnt = 0;
			udelay(10);
		}
		vc_bypass_value = prm_read_mod_reg(OMAP3430_GR_MOD,
					OMAP3_PRM_VC_BYPASS_VAL_OFFSET);
	}

	/*
	 *  T2 SMPS slew rate (min) 4mV/uS, step size 12.5mV,
	 *  2us added as buffer.
	 */
	t2_smps_delay = ((t2_smps_steps * 125) / 40) + 2;
	udelay(t2_smps_delay);

	if (cpu_is_omap3630() && (vdd == VDD1_OPP))
		sr_voltagescale_adaptive_body_bias(target_opp_no);

	if (sr_status) {
		if (vdd == VDD1_OPP)
			sr_start_vddautocomap(SR1, target_opp_no);
		else if (vdd == VDD2_OPP)
			sr_start_vddautocomap(SR2, target_opp_no);
	}

	return 0;
}

/* Sysfs interface to select SR VDD1 auto compensation */
static ssize_t omap_sr_vdd1_autocomp_show(struct kobject *kobj,
					struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", sr1.is_autocomp_active);
}

static ssize_t omap_sr_vdd1_autocomp_store(struct kobject *kobj,
					struct kobj_attribute *attr,
					const char *buf, size_t n)
{
	unsigned short value;

	if (sscanf(buf, "%hu", &value) != 1 || (value > 1)) {
		pr_err("sr_vdd1_autocomp: Invalid value\n");
		return -EINVAL;
	}

	if (value == 0) {
		sr_stop_vddautocomap(SR1);
	} else {
		u32 current_vdd1opp_no = get_vdd1_opp();
		if (!current_vdd1opp_no) {
			pr_err("sr_vdd1_autocomp: Current VDD1 opp unknown\n");
			return -EINVAL;
		}
		sr_start_vddautocomap(SR1, current_vdd1opp_no);
	}
	return n;
}

static struct kobj_attribute sr_vdd1_autocomp = {
	.attr = {
	.name = __stringify(sr_vdd1_autocomp),
	.mode = 0644,
	},
	.show = omap_sr_vdd1_autocomp_show,
	.store = omap_sr_vdd1_autocomp_store,
};

/* Sysfs interface to select SR VDD2 auto compensation */
static ssize_t omap_sr_vdd2_autocomp_show(struct kobject *kobj,
					struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", sr2.is_autocomp_active);
}

static ssize_t omap_sr_vdd2_autocomp_store(struct kobject *kobj,
					struct kobj_attribute *attr,
					const char *buf, size_t n)
{
	unsigned short value;

	if (sscanf(buf, "%hu", &value) != 1 || (value > 1)) {
		pr_err("sr_vdd2_autocomp: Invalid value\n");
		return -EINVAL;
	}

	if (value == 0) {
		sr_stop_vddautocomap(SR2);
	} else {
		u32 current_vdd2opp_no = get_vdd2_opp();
		if (!current_vdd2opp_no) {
			pr_err("sr_vdd2_autocomp: Current VDD2 opp unknown\n");
			return -EINVAL;
		}
		sr_start_vddautocomap(SR2, current_vdd2opp_no);
	}
	return n;
}

static struct kobj_attribute sr_vdd2_autocomp = {
	.attr = {
	.name = __stringify(sr_vdd2_autocomp),
	.mode = 0644,
	},
	.show = omap_sr_vdd2_autocomp_show,
	.store = omap_sr_vdd2_autocomp_store,
};

static int __init omap3_sr_init(void)
{
	int ret = 0;
	u8 RdReg;

	/* Exit if OPP tables are not defined */
        if (!(mpu_opps && l3_opps)) {
                pr_err("SR: OPP rate tables not defined for platform, not enabling SmartReflex\n");
		return -ENODEV;
        }

	/* Enable SR on T2 */
	ret = twl_i2c_read_u8(TWL4030_MODULE_PM_RECEIVER, &RdReg,
			      R_DCDC_GLOBAL_CFG);

	RdReg |= DCDC_GLOBAL_CFG_ENABLE_SRFLX;
	ret |= twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER, RdReg,
				R_DCDC_GLOBAL_CFG);

	if (cpu_is_omap34xx()) {
		sr1.clk = clk_get(NULL, "sr1_fck");
		sr2.clk = clk_get(NULL, "sr2_fck");
	}
	sr1.vdd_opp_clk = clk_get(NULL, "dpll1_ck");
	sr2.vdd_opp_clk = clk_get(NULL, "l3_ick");
	sr_set_clk_length(&sr1);
	sr_set_clk_length(&sr2);

	/* For OPP scale down, scale down frequency before voltage */
	if (cpu_is_omap34xx() && vdd_scale_down)
        omap2_clk_set_freq();

	/* Call the VPConfig, VCConfig, set N Values. */
	sr_set_nvalues(&sr1);
	sr_configure_vp(SR1);

	sr_set_nvalues(&sr2);
	sr_configure_vp(SR2);

	/* For OPP scale up, scale up the frequency after voltage */
	if (cpu_is_omap34xx() && !vdd_scale_down)
		omap2_clk_set_freq();

	ret = sysfs_create_file(power_kobj, &sr_vdd1_autocomp.attr);
	if (ret)
		pr_err("sysfs_create_file failed: %d\n", ret);

	ret = sysfs_create_file(power_kobj, &sr_vdd2_autocomp.attr);
	if (ret)
		pr_err("sysfs_create_file failed: %d\n", ret);

	pr_info("SmartReflex driver initialized\n");

	return 0;
}

late_initcall(omap3_sr_init);
