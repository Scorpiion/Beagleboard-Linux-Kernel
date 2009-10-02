#ifndef __OMAP3_OPP_H_
#define __OMAP3_OPP_H_

#include <mach/omap-pm.h>

/* MPU speeds */
#define S1000M  1000000000
#define S800M   800000000
#define S720M   720000000
#define S600M   600000000
#define S550M   550000000
#define S500M   500000000
#define S300M   300000000
#define S250M   250000000
#define S150M   150000000
#define S125M   125000000

/* DSP speeds */
#define S520M   520000000
#define S430M   430000000
#define S400M   400000000
#define S360M   360000000
#define S260M   260000000
#define S180M   180000000
#define S130M   130000000
#define S90M    90000000

/* L3 speeds */
#define S50M    50000000
#define S83M    83000000
#define S100M   100000000
#define S166M   166000000
#define S200M   200000000

#ifndef CONFIG_ARCH_OMAP3630
static struct omap_opp omap3_mpu_rate_table[] = {
	{0, 0, 0},
	/*OPP1*/
	{S125M, VDD1_OPP1, 0x1E},
	/*OPP2*/
	{S250M, VDD1_OPP2, 0x26},
	/*OPP3*/
	{S500M, VDD1_OPP3, 0x30},
	/*OPP4*/
	{S550M, VDD1_OPP4, 0x36},
	/*OPP5*/
	{S600M, VDD1_OPP5, 0x3C},
};

static struct omap_opp omap3_l3_rate_table[] = {
	{0, 0, 0},
	/*OPP1*/
	{0, VDD2_OPP1, 0x1E},
	/*OPP2*/
	{S83M, VDD2_OPP2, 0x24},
	/*OPP3*/
	{S166M, VDD2_OPP3, 0x2C},
};

static struct omap_opp omap3_dsp_rate_table[] = {
	{0, 0, 0},
	/*OPP1*/
	{S90M, VDD1_OPP1, 0x1E},
	/*OPP2*/
	{S180M, VDD1_OPP2, 0x26},
	/*OPP3*/
	{S360M, VDD1_OPP3, 0x30},
	/*OPP4*/
	{S400M, VDD1_OPP4, 0x36},
	/*OPP5*/
	{S430M, VDD1_OPP5, 0x3C},
};
#else
static struct omap_opp omap3_mpu_rate_table[] = {
	{0, 0, 0},
	/*OPP1 (OPP25)*/
	{S150M, VDD1_OPP1, 0x14},
	/*OPP2 (OPP50)*/   
	{S300M, VDD1_OPP2, 0x1C},
	/*OPP3 (OPP100)*/  
	{S600M, VDD1_OPP3, 0x28},
	/*OPP4 (OPP120)*/  
	{S720M, VDD1_OPP4, 0x30},
	/*OPP5 (OPP-TM)*/  
	{S800M, VDD1_OPP5, 0x34},
	/*OPP6 (OPP-SB)*/  
	{S1000M, VDD1_OPP6,0x34},

};

static struct omap_opp omap3_l3_rate_table[] = {
	{0, 0, 0},
	/*OPP1*/
	{0, VDD2_OPP1, 0x1E},
	/*OPP2 (OPP25)*/
	{S50M, VDD2_OPP2,  0x14},
	/*OPP3 (OPP50)*/   
	{S100M, VDD2_OPP3, 0x1C},
	/*OPP3 (OPP100)*/  
	{S200M, VDD2_OPP4, 0x28},
};

static struct omap_opp omap3_dsp_rate_table[] = {
	{0, 0, 0},
	/*OPP1 (OPP25)*/
	{S130M, VDD1_OPP1, 0x14},
	/*OPP2 (OPP50)*/   
	{S260M, VDD1_OPP2, 0x1C},
	/*OPP3 (OPP100)*/  
	{S520M, VDD1_OPP3, 0x28},
	/*OPP4 (OPP120)*/  
	{S600M, VDD1_OPP4, 0x30},
	/*OPP5 (OPP-TM)*/  
	{S720M, VDD1_OPP5, 0x34},
	/*OPP6 (OPP-SB)*/  
	{S720M, VDD1_OPP6, 0x34},
};
#endif

#endif
