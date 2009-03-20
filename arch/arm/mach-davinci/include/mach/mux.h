/*
 * Table of the DAVINCI register configurations for the PINMUX combinations
 *
 * Author: Vladimir Barinov, MontaVista Software, Inc. <source@mvista.com>
 *
 * Based on linux/include/asm-arm/arch-omap/mux.h:
 * Copyright (C) 2003 - 2005 Nokia Corporation
 *
 * Written by Tony Lindgren
 *
 * 2007 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 *
 * Copyright (C) 2008 Texas Instruments.
 */

#ifndef __INC_MACH_MUX_H
#define __INC_MACH_MUX_H

#include <mach/dm644x.h>

/* System module registers */
#define PINMUX0			0x00
#define PINMUX1			0x04
/* dm355 only */
#define PINMUX2			0x08
#define PINMUX3			0x0c
#define PINMUX4			0x10
#define INTMUX			0x18
#define EVTMUX			0x1c

struct mux_config {
	char *name;
	const char *mux_reg_name;
	const unsigned int mux_reg;
	const unsigned char reg_index;
	const unsigned char mask_offset;
	const unsigned char mask;
	const unsigned char mode;
	bool debug;
};

enum davinci_dm644x_index {
	/* ATA and HDDIR functions */
	DM644X_HDIREN,
	DM644X_ATAEN,
	DM644X_ATAEN_DISABLE,

	/* HPI functions */
	DM644X_HPIEN_DISABLE,

	/* AEAW functions */
	DM644X_AEAW,

	/* Memory Stick */
	DM644X_MSTK,

	/* I2C */
	DM644X_I2C,

	/* ASP function */
	DM644X_MCBSP,

	/* UART1 */
	DM644X_UART1,

	/* UART2 */
	DM644X_UART2,

	/* PWM0 */
	DM644X_PWM0,

	/* PWM1 */
	DM644X_PWM1,

	/* PWM2 */
	DM644X_PWM2,

	/* VLYNQ function */
	DM644X_VLYNQEN,
	DM644X_VLSCREN,
	DM644X_VLYNQWD,

	/* EMAC and MDIO function */
	DM644X_EMACEN,

	/* GPIO3V[0:16] pins */
	DM644X_GPIO3V,

	/* GPIO pins */
	DM644X_GPIO0,
	DM644X_GPIO3,
	DM644X_GPIO43_44,
	DM644X_GPIO46_47,

	/* VPBE */
	DM644X_RGB666,

	/* LCD */
	DM644X_LOEEN,
	DM644X_LFLDEN,
};

enum davinci_dm646x_index {
	/* ATA function */
	DM646X_ATAEN,

	/* AUDIO Clock */
	DM646X_AUDCK1,
	DM646X_AUDCK0,
};

enum davinci_dm355_index {
	/* MMC/SD 0 */
	DM355_MMCSD0,

	/* MMC/SD 1 */
	DM355_SD1_CLK,
	DM355_SD1_CMD,
	DM355_SD1_DATA3,
	DM355_SD1_DATA2,
	DM355_SD1_DATA1,
	DM355_SD1_DATA0,

	/* I2C */
	DM355_I2C_SDA,
	DM355_I2C_SCL,

	/* ASP0 function */
	DM355_MCBSP0_BDX,
	DM355_MCBSP0_X,
	DM355_MCBSP0_BFSX,
	DM355_MCBSP0_BDR,
	DM355_MCBSP0_R,
	DM355_MCBSP0_BFSR,

	/* SPI0 */
	DM355_SPI0_SDI,
	DM355_SPI0_SDENA0,
	DM355_SPI0_SDENA1,

	/* IRQ muxing */
	DM355_INT_EDMA_CC,
	DM355_INT_EDMA_TC0_ERR,
	DM355_INT_EDMA_TC1_ERR,

	/* EDMA event muxing */
	DM355_EVT8_ASP1_TX,
	DM355_EVT9_ASP1_RX,
	DM355_EVT26_MMC0_RX,
};

enum davinci_da830_index {
	/* UART0 function */
	DA830_NUART0_CTS,
	DA830_NUART0_RTS,
	DA830_UART0_RXD,
	DA830_UART0_TXD,

	/* UART1 function */
	DA830_UART1_RXD,
	DA830_UART1_TXD,

	/* UART2 function */
	DA830_UART2_RXD,
	DA830_UART2_TXD,

	/* I2C function */
	DA830_I2C1_SCL,
	DA830_I2C1_SDA,
	DA830_I2C0_SDA,
	DA830_I2C0_SCL,

	/* EMAC function */
	DA830_RMII_TXD_0,
	DA830_RMII_TXD_1,
	DA830_RMII_TXEN,
	DA830_RMII_CRS_DV,
	DA830_RMII_RXD_0,
	DA830_RMII_RXD_1,
	DA830_RMII_RXER,
	DA830_MDIO_CLK,
	DA830_MDIO_D,

	/* MMC/SD function */
	DA830_MMCSD_DAT_0,
	DA830_MMCSD_DAT_1,
	DA830_MMCSD_DAT_2,
	DA830_MMCSD_DAT_3,
	DA830_MMCSD_DAT_4,
	DA830_MMCSD_DAT_5,
	DA830_MMCSD_DAT_6,
	DA830_MMCSD_DAT_7,
	DA830_MMCSD_CLK,
	DA830_MMCSD_CMD,

	/* EMIFA function */
	DA830_EMA_D_0,
	DA830_EMA_D_1,
	DA830_EMA_D_2,
	DA830_EMA_D_3,
	DA830_EMA_D_4,
	DA830_EMA_D_5,
	DA830_EMA_D_6,
	DA830_EMA_D_7,
	DA830_EMA_D_8,
	DA830_EMA_D_9,
	DA830_EMA_D_10,
	DA830_EMA_D_11,
	DA830_EMA_D_12,
	DA830_EMA_D_13,
	DA830_EMA_D_14,
	DA830_EMA_D_15,
	DA830_EMA_A_0,
	DA830_EMA_A_1,
	DA830_EMA_A_2,
	DA830_EMA_A_3,
	DA830_EMA_A_4,
	DA830_EMA_A_5,
	DA830_EMA_A_6,
	DA830_EMA_A_7,
	DA830_EMA_A_8,
	DA830_EMA_A_9,
	DA830_EMA_A_10,
	DA830_EMA_A_11,
	DA830_EMA_A_12,
	DA830_EMA_BA_1,
	DA830_EMA_BA_0,
	DA830_EMA_CLK,
	DA830_EMA_SDCKE,
	DA830_NEMA_CAS,
	DA830_NEMA_CS_4,
	DA830_NEMA_RAS,
	DA830_NEMA_WE,
	DA830_NEMA_CS_0,
	DA830_NEMA_CS_2,
	DA830_NEMA_CS_3,
	DA830_NEMA_OE,
	DA830_NEMA_WE_DQM_1,
	DA830_NEMA_WE_DQM_0,
	DA830_NEMA_CS_5,
	DA830_EMA_WAIT_0,

	/* EMIFB function */
	DA830_EMB_SDCKE,
	DA830_EMB_CLK_GLUE,
	DA830_EMB_CLK,
	DA830_NEMB_CS_0,
	DA830_NEMB_CAS,
	DA830_NEMB_RAS,
	DA830_NEMB_WE,
	DA830_EMB_BA_1,
	DA830_EMB_BA_0,
	DA830_EMB_A_0,
	DA830_EMB_A_1,
	DA830_EMB_A_2,
	DA830_EMB_A_3,
	DA830_EMB_A_4,
	DA830_EMB_A_5,
	DA830_EMB_A_6,
	DA830_EMB_A_7,
	DA830_EMB_A_8,
	DA830_EMB_A_9,
	DA830_EMB_A_10,
	DA830_EMB_A_11,
	DA830_EMB_A_12,
	DA830_EMB_D_31,
	DA830_EMB_D_30,
	DA830_EMB_D_29,
	DA830_EMB_D_28,
	DA830_EMB_D_27,
	DA830_EMB_D_26,
	DA830_EMB_D_25,
	DA830_EMB_D_24,
	DA830_EMB_D_23,
	DA830_EMB_D_22,
	DA830_EMB_D_21,
	DA830_EMB_D_20,
	DA830_EMB_D_19,
	DA830_EMB_D_18,
	DA830_EMB_D_17,
	DA830_EMB_D_16,
	DA830_NEMB_WE_DQM_3,
	DA830_NEMB_WE_DQM_2,
	DA830_EMB_D_0,
	DA830_EMB_D_1,
	DA830_EMB_D_2,
	DA830_EMB_D_3,
	DA830_EMB_D_4,
	DA830_EMB_D_5,
	DA830_EMB_D_6,
	DA830_EMB_D_7,
	DA830_EMB_D_8,
	DA830_EMB_D_9,
	DA830_EMB_D_10,
	DA830_EMB_D_11,
	DA830_EMB_D_12,
	DA830_EMB_D_13,
	DA830_EMB_D_14,
	DA830_EMB_D_15,
	DA830_NEMB_WE_DQM_1,
	DA830_NEMB_WE_DQM_0,

	/* SPI0 function */
	DA830_SPI0_SOMI_0,
	DA830_SPI0_SIMO_0,
	DA830_SPI0_CLK,
	DA830_NSPI0_ENA,
	DA830_NSPI0_SCS_0,

	/* SPI1 function */
	DA830_SPI1_SOMI_0,
	DA830_SPI1_SIMO_0,
	DA830_SPI1_CLK,
	DA830_NSPI1_ENA,
	DA830_NSPI1_SCS_0,
	DA830_GPIO3_10,

	/* LCD function */
	DA830_LCD_D_8,
	DA830_LCD_D_9,
	DA830_LCD_D_7,
	DA830_LCD_D_10,
	DA830_LCD_D_11,
	DA830_LCD_D_12,
	DA830_LCD_D_13,
	DA830_LCD_D_14,
	DA830_LCD_D_15,
	DA830_LCD_D_6,
	DA830_LCD_D_3,
	DA830_LCD_D_2,
	DA830_LCD_D_1,
	DA830_LCD_D_0,
	DA830_LCD_PCLK,
	DA830_LCD_HSYNC,
	DA830_LCD_VSYNC,
	DA830_NLCD_AC_ENB_CS,
	DA830_LCD_MCLK,
	DA830_LCD_D_5,
	DA830_LCD_D_4,
};

#ifdef CONFIG_DAVINCI_MUX
/* setup pin muxing */
extern void davinci_mux_init(void);
extern int davinci_mux_register(const struct mux_config *pins,
				unsigned long size, unsigned long *in_use);
extern int davinci_cfg_reg(unsigned long reg_cfg);
#else
/* boot loader does it all (no warnings from CONFIG_DAVINCI_MUX_WARNINGS) */
static inline void davinci_mux_init(void) {}
static inline int davinci_mux_register(const struct mux_config *pins,
				       unsigned long size) { return 0; }
static inline int davinci_cfg_reg(unsigned long reg_cfg) { return 0; }
#endif

#endif /* __INC_MACH_MUX_H */
