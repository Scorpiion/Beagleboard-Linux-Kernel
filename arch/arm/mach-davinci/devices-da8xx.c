/*
 * DA8XX/OMAP L1XX platform device data
 *
 * Copyright (c) 2007-2009, MontaVista Software, Inc. <source@mvista.com>
 * Derived from code that was:
 *	Copyright (C) 2006 Komal Shah <komal_shah802003@yahoo.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/serial_8250.h>
#include <linux/spi/davinci_spi_master.h>

#include <mach/cputype.h>
#include <mach/common.h>
#include <mach/time.h>
#include <mach/da8xx.h>
#include <mach/cpuidle.h>
#include <mach/cppi41.h>

#include "clock.h"

#define DA8XX_TPCC_BASE			0x01c00000
#define DA8XX_TPTC0_BASE		0x01c08000
#define DA8XX_TPTC1_BASE		0x01c08400
#define DA8XX_WDOG_BASE			0x01c21000 /* DA8XX_TIMER64P1_BASE */
#define DA8XX_I2C0_BASE			0x01c22000
#define DA8XX_RTC_BASE			0x01C23000
#define DA8XX_EMAC_CPPI_PORT_BASE	0x01e20000
#define DA8XX_EMAC_CPGMACSS_BASE	0x01e22000
#define DA8XX_EMAC_CPGMAC_BASE		0x01e23000
#define DA8XX_EMAC_MDIO_BASE		0x01e24000
#define DA8XX_GPIO_BASE			0x01e26000
#define DA8XX_I2C1_BASE			0x01e28000

#define DA8XX_EMAC_CTRL_REG_OFFSET	0x3000
#define DA8XX_EMAC_MOD_REG_OFFSET	0x2000
#define DA8XX_EMAC_RAM_OFFSET		0x0000
#define DA8XX_MDIO_REG_OFFSET		0x4000
#define DA8XX_EMAC_CTRL_RAM_SIZE	SZ_8K

void __iomem *da8xx_syscfg_base;
EXPORT_SYMBOL(da8xx_syscfg_base);
static struct plat_serial8250_port da8xx_serial_pdata[] = {
	{
		.mapbase	= DA8XX_UART0_BASE,
		.irq		= IRQ_DA8XX_UARTINT0,
		.flags		= UPF_BOOT_AUTOCONF | UPF_SKIP_TEST |
					UPF_IOREMAP,
		.iotype		= UPIO_MEM,
		.regshift	= 2,
	},
	{
		.mapbase	= DA8XX_UART1_BASE,
		.irq		= IRQ_DA8XX_UARTINT1,
		.flags		= UPF_BOOT_AUTOCONF | UPF_SKIP_TEST |
					UPF_IOREMAP,
		.iotype		= UPIO_MEM,
		.regshift	= 2,
	},
	{
		.mapbase	= DA8XX_UART2_BASE,
		.irq		= IRQ_DA8XX_UARTINT2,
		.flags		= UPF_BOOT_AUTOCONF | UPF_SKIP_TEST |
					UPF_IOREMAP,
		.iotype		= UPIO_MEM,
		.regshift	= 2,
	},
	{
		.flags	= 0,
	},
};

struct platform_device da8xx_serial_device = {
	.name	= "serial8250",
	.id	= PLAT8250_DEV_PLATFORM,
	.dev	= {
		.platform_data	= da8xx_serial_pdata,
	},
};

static const s8 da8xx_dma_chan_no_event[] = {
	20, 21,
	-1
};

static const s8 da8xx_queue_tc_mapping[][2] = {
	/* {event queue no, TC no} */
	{0, 0},
	{1, 1},
	{-1, -1}
};

static const s8 da8xx_queue_priority_mapping[][2] = {
	/* {event queue no, Priority} */
	{0, 3},
	{1, 7},
	{-1, -1}
};

static struct edma_soc_info da8xx_edma_info[] = {
	{
		.n_channel		= 32,
		.n_region		= 4,
		.n_slot			= 128,
		.n_tc			= 2,
		.n_cc			= 1,
		.noevent		= da8xx_dma_chan_no_event,
		.queue_tc_mapping	= da8xx_queue_tc_mapping,
		.queue_priority_mapping	= da8xx_queue_priority_mapping,
	},
};

static struct resource da8xx_edma_resources[] = {
	{
		.name	= "edma_cc0",
		.start	= DA8XX_TPCC_BASE,
		.end	= DA8XX_TPCC_BASE + SZ_32K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "edma_tc0",
		.start	= DA8XX_TPTC0_BASE,
		.end	= DA8XX_TPTC0_BASE + SZ_1K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "edma_tc1",
		.start	= DA8XX_TPTC1_BASE,
		.end	= DA8XX_TPTC1_BASE + SZ_1K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "edma0",
		.start	= IRQ_DA8XX_CCINT0,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name	= "edma0_err",
		.start	= IRQ_DA8XX_CCERRINT,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device da8xx_edma_device = {
	.name		= "edma",
	.id		= -1,
	.dev = {
		.platform_data	= da8xx_edma_info,
	},
	.num_resources	= ARRAY_SIZE(da8xx_edma_resources),
	.resource	= da8xx_edma_resources,
};

int __init da8xx_register_edma(void)
{
	return platform_device_register(&da8xx_edma_device);
}

static struct resource da8xx_i2c_resources0[] = {
	{
		.start	= DA8XX_I2C0_BASE,
		.end	= DA8XX_I2C0_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= IRQ_DA8XX_I2CINT0,
		.end	= IRQ_DA8XX_I2CINT0,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device da8xx_i2c_device0 = {
	.name		= "i2c_davinci",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(da8xx_i2c_resources0),
	.resource	= da8xx_i2c_resources0,
};

static struct resource da8xx_i2c_resources1[] = {
	{
		.start	= DA8XX_I2C1_BASE,
		.end	= DA8XX_I2C1_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= IRQ_DA8XX_I2CINT1,
		.end	= IRQ_DA8XX_I2CINT1,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device da8xx_i2c_device1 = {
	.name		= "i2c_davinci",
	.id		= 2,
	.num_resources	= ARRAY_SIZE(da8xx_i2c_resources1),
	.resource	= da8xx_i2c_resources1,
};

int __init da8xx_register_i2c(int instance,
		struct davinci_i2c_platform_data *pdata)
{
	struct platform_device *pdev;

	if (instance == 0)
		pdev = &da8xx_i2c_device0;
	else if (instance == 1)
		pdev = &da8xx_i2c_device1;
	else
		return -EINVAL;

	pdev->dev.platform_data = pdata;
	return platform_device_register(pdev);
}

static struct resource da8xx_watchdog_resources[] = {
	{
		.start	= DA8XX_WDOG_BASE,
		.end	= DA8XX_WDOG_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
};

struct platform_device davinci_wdt_device = {
	.name		= "watchdog",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(da8xx_watchdog_resources),
	.resource	= da8xx_watchdog_resources,
};

int __init da8xx_register_watchdog(void)
{
	return platform_device_register(&davinci_wdt_device);
}

static struct resource da8xx_emac_resources[] = {
	{
		.start	= DA8XX_EMAC_CPPI_PORT_BASE,
		.end	= DA8XX_EMAC_CPPI_PORT_BASE + 0x5000 - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= IRQ_DA8XX_C0_RX_THRESH_PULSE,
		.end	= IRQ_DA8XX_C0_RX_THRESH_PULSE,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= IRQ_DA8XX_C0_RX_PULSE,
		.end	= IRQ_DA8XX_C0_RX_PULSE,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= IRQ_DA8XX_C0_TX_PULSE,
		.end	= IRQ_DA8XX_C0_TX_PULSE,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= IRQ_DA8XX_C0_MISC_PULSE,
		.end	= IRQ_DA8XX_C0_MISC_PULSE,
		.flags	= IORESOURCE_IRQ,
	},
};

struct emac_platform_data da8xx_emac_pdata = {
	.ctrl_reg_offset	= DA8XX_EMAC_CTRL_REG_OFFSET,
	.ctrl_mod_reg_offset	= DA8XX_EMAC_MOD_REG_OFFSET,
	.ctrl_ram_offset	= DA8XX_EMAC_RAM_OFFSET,
	.mdio_reg_offset	= DA8XX_MDIO_REG_OFFSET,
	.ctrl_ram_size		= DA8XX_EMAC_CTRL_RAM_SIZE,
	.version		= EMAC_VERSION_2,
};

static struct platform_device da8xx_emac_device = {
	.name		= "davinci_emac",
	.id		= 1,
	.dev = {
		.platform_data	= &da8xx_emac_pdata,
	},
	.num_resources	= ARRAY_SIZE(da8xx_emac_resources),
	.resource	= da8xx_emac_resources,
};

int __init da8xx_register_emac(void)
{
	return platform_device_register(&da8xx_emac_device);
}

static struct resource da830_mcasp1_resources[] = {
	{
		.name	= "mcasp1",
		.start	= DAVINCI_DA830_MCASP1_REG_BASE,
		.end	= DAVINCI_DA830_MCASP1_REG_BASE + (SZ_1K * 12) - 1,
		.flags	= IORESOURCE_MEM,
	},
	/* TX event */
	{
		.start	= DAVINCI_DA830_DMA_MCASP1_AXEVT,
		.end	= DAVINCI_DA830_DMA_MCASP1_AXEVT,
		.flags	= IORESOURCE_DMA,
	},
	/* RX event */
	{
		.start	= DAVINCI_DA830_DMA_MCASP1_AREVT,
		.end	= DAVINCI_DA830_DMA_MCASP1_AREVT,
		.flags	= IORESOURCE_DMA,
	},
};

static struct platform_device da830_mcasp1_device = {
	.name		= "davinci-mcasp",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(da830_mcasp1_resources),
	.resource	= da830_mcasp1_resources,
};

static struct resource da850_mcasp_resources[] = {
	{
		.name	= "mcasp",
		.start	= DAVINCI_DA8XX_MCASP0_REG_BASE,
		.end	= DAVINCI_DA8XX_MCASP0_REG_BASE + (SZ_1K * 12) - 1,
		.flags	= IORESOURCE_MEM,
	},
	/* TX event */
	{
		.start	= DAVINCI_DA8XX_DMA_MCASP0_AXEVT,
		.end	= DAVINCI_DA8XX_DMA_MCASP0_AXEVT,
		.flags	= IORESOURCE_DMA,
	},
	/* RX event */
	{
		.start	= DAVINCI_DA8XX_DMA_MCASP0_AREVT,
		.end	= DAVINCI_DA8XX_DMA_MCASP0_AREVT,
		.flags	= IORESOURCE_DMA,
	},
};

static struct platform_device da850_mcasp_device = {
	.name		= "davinci-mcasp",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(da850_mcasp_resources),
	.resource	= da850_mcasp_resources,
};

void __init da8xx_register_mcasp(int id, struct snd_platform_data *pdata)
{
	/* DA830/OMAP-L137 has 3 instances of McASP */
	if (cpu_is_davinci_da830() && id == 1) {
		da830_mcasp1_device.dev.platform_data = pdata;
		platform_device_register(&da830_mcasp1_device);
	} else if (cpu_is_davinci_da850()) {
		da850_mcasp_device.dev.platform_data = pdata;
		platform_device_register(&da850_mcasp_device);
	}
}

static const struct display_panel disp_panel = {
	QVGA,
	16,
	16,
	COLOR_ACTIVE,
};

static struct lcd_ctrl_config lcd_cfg = {
	&disp_panel,
	.ac_bias		= 255,
	.ac_bias_intrpt		= 0,
	.dma_burst_sz		= 16,
	.bpp			= 16,
	.fdd			= 255,
	.tft_alt_mode		= 0,
	.stn_565_mode		= 0,
	.mono_8bit_mode		= 0,
	.invert_line_clock	= 1,
	.invert_frm_clock	= 1,
	.sync_edge		= 0,
	.sync_ctrl		= 1,
	.raster_order		= 0,
};

struct da8xx_lcdc_platform_data sharp_lcd035q3dg01_pdata = {
	.manu_name		= "sharp",
	.controller_data	= &lcd_cfg,
	.type			= "Sharp_LCD035Q3DG01",
};

struct da8xx_lcdc_platform_data sharp_lk043t1dg01_pdata = {
	.manu_name		= "sharp",
	.controller_data	= &lcd_cfg,
	.type			= "Sharp_LK043T1DG01",
};

#if !defined(CONFIG_FB_DA8XX) && !defined(CONFIG_FB_DA8XX_MODULE)
static struct da8xx_clcd_platform_data da8xx_evm_clcd_pdata = {
	.version = CONFIG_SPACE_1,
};
#endif

static struct resource da8xx_lcdc_resources[] = {
	[0] = { /* registers */
		.start  = DA8XX_LCD_CNTRL_BASE,
		.end    = DA8XX_LCD_CNTRL_BASE + SZ_4K - 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = { /* interrupt */
		.start  = IRQ_DA8XX_LCDINT,
		.end    = IRQ_DA8XX_LCDINT,
		.flags  = IORESOURCE_IRQ,
	},
};

static struct platform_device da8xx_lcdc_device = {
	.name		= "da8xx_lcdc",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(da8xx_lcdc_resources),
	.resource	= da8xx_lcdc_resources,
};

int __init da8xx_register_lcdc(struct da8xx_lcdc_platform_data *pdata)
{
#if !defined(CONFIG_FB_DA8XX) && !defined(CONFIG_FB_DA8XX_MODULE)
	if (cpu_is_davinci_da830())
		da8xx_evm_clcd_pdata.version = CONFIG_SPACE_0;

	da8xx_lcdc_device.dev.platform_data = &da8xx_evm_clcd_pdata;
#else
	da8xx_lcdc_device.dev.platform_data = pdata;
#endif
	return platform_device_register(&da8xx_lcdc_device);
}

static struct resource da8xx_mmcsd0_resources[] = {
	{		/* registers */
		.start	= DA8XX_MMCSD0_BASE,
		.end	= DA8XX_MMCSD0_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{		/* interrupt */
		.start	= IRQ_DA8XX_MMCSDINT0,
		.end	= IRQ_DA8XX_MMCSDINT0,
		.flags	= IORESOURCE_IRQ,
	},
	{		/* DMA RX */
		.start	= EDMA_CTLR_CHAN(0, 16),
		.end	= EDMA_CTLR_CHAN(0, 16),
		.flags	= IORESOURCE_DMA,
	},
	{		/* DMA TX */
		.start	= EDMA_CTLR_CHAN(0, 17),
		.end	= EDMA_CTLR_CHAN(0, 17),
		.flags	= IORESOURCE_DMA,
	},
};

static struct platform_device da8xx_mmcsd0_device = {
	.name		= "davinci_mmc",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(da8xx_mmcsd0_resources),
	.resource	= da8xx_mmcsd0_resources,
};

int __init da8xx_register_mmcsd0(struct davinci_mmc_config *config)
{
	da8xx_mmcsd0_device.dev.platform_data = config;
	return platform_device_register(&da8xx_mmcsd0_device);
}

static struct resource da8xx_rtc_resources[] = {
	{
		.start		= DA8XX_RTC_BASE,
		.end		= DA8XX_RTC_BASE + SZ_4K - 1,
		.flags		= IORESOURCE_MEM,
	},
	{ /* timer irq */
		.start		= IRQ_DA8XX_RTC,
		.end		= IRQ_DA8XX_RTC,
		.flags		= IORESOURCE_IRQ,
	},
	{ /* alarm irq */
		.start		= IRQ_DA8XX_RTC,
		.end		= IRQ_DA8XX_RTC,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct platform_device da8xx_rtc_device = {
	.name           = "omap_rtc",
	.id             = -1,
	.num_resources	= ARRAY_SIZE(da8xx_rtc_resources),
	.resource	= da8xx_rtc_resources,
};

int da8xx_register_rtc(void)
{
	/* Unlock the rtc's registers */
	__raw_writel(0x83e70b13, IO_ADDRESS(DA8XX_RTC_BASE + 0x6c));
	__raw_writel(0x95a4f1e0, IO_ADDRESS(DA8XX_RTC_BASE + 0x70));

	return platform_device_register(&da8xx_rtc_device);
}

static struct resource da8xx_cpuidle_resources[] = {
	{
		.start		= DA8XX_DDR2_CTL_BASE,
		.end		= DA8XX_DDR2_CTL_BASE + SZ_32K - 1,
		.flags		= IORESOURCE_MEM,
	},
};

/* DA8XX devices support DDR2 power down */
static struct davinci_cpuidle_config da8xx_cpuidle_pdata = {
	.ddr2_pdown	= 1,
};


static struct platform_device da8xx_cpuidle_device = {
	.name			= "cpuidle-davinci",
	.num_resources		= ARRAY_SIZE(da8xx_cpuidle_resources),
	.resource		= da8xx_cpuidle_resources,
	.dev = {
		.platform_data	= &da8xx_cpuidle_pdata,
	},
};

int __init da8xx_register_cpuidle(void)
{
	return platform_device_register(&da8xx_cpuidle_device);
}

static struct davinci_spi_platform_data da850_spi_pdata1 = {
	.version = DAVINCI_SPI_VERSION_2,
	.clk_name = "SPICLK"
};

static struct resource da850_spi_resources1[] = {
	[0] = {
		.start = 0x01F0E000,
		.end = 0x01F0E000 + 0xfff,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_DA8XX_SPINT1,
		.end = IRQ_DA8XX_SPINT1,
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		.start = EDMA_CTLR_CHAN(0, 18),
		.end = EDMA_CTLR_CHAN(0, 18),
		.flags = IORESOURCE_DMA | IORESOURCE_DMA_RX_CHAN,
	},
	[3] = {
		.start = EDMA_CTLR_CHAN(0, 19),
		.end = EDMA_CTLR_CHAN(0, 19),
		.flags = IORESOURCE_DMA | IORESOURCE_DMA_TX_CHAN,
	},
	[4] = {
		.start = 1,
		.end = 1,
		.flags = IORESOURCE_DMA | IORESOURCE_DMA_EVENT_Q,
	},
};

static struct platform_device da850_spi_pdev1 = {
	.name = "dm_spi",
	.id = 1,
	.resource = da850_spi_resources1,
	.num_resources = ARRAY_SIZE(da850_spi_resources1),
	.dev = {
		.platform_data = &da850_spi_pdata1,
	},
};

void __init da850_init_spi1(unsigned char* chip_sel, unsigned int num_sel,
	struct spi_board_info *info, unsigned num_dev)
{
	struct davinci_spi_platform_data *pdata =
			da850_spi_pdev1.dev.platform_data;

	spi_register_board_info(info, num_dev);

	pdata->chip_sel = chip_sel;
	pdata->num_chipselect = num_sel;
	platform_device_register(&da850_spi_pdev1);
}

static struct davinci_spi_platform_data da830_spi_pdata0 = {
	.version = DAVINCI_SPI_VERSION_2,
	.clk_name = "SPICLK"
};

static struct resource da830_spi_resources0[] = {
	[0] = {
		.start = 0x01C41000,
		.end = 0x01C41000 + 0xfff,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_DA8XX_SPINT0,
		.end = IRQ_DA8XX_SPINT0,
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		.start = EDMA_CTLR_CHAN(0, 14),
		.end = EDMA_CTLR_CHAN(0, 14),
		.flags = IORESOURCE_DMA | IORESOURCE_DMA_RX_CHAN,
	},
	[3] = {
		.start = EDMA_CTLR_CHAN(0, 15),
		.end = EDMA_CTLR_CHAN(0, 15),
		.flags = IORESOURCE_DMA | IORESOURCE_DMA_TX_CHAN,
	},
	[4] = {
		.start = 1,
		.end = 1,
		.flags = IORESOURCE_DMA | IORESOURCE_DMA_EVENT_Q,
	},
};

static struct platform_device da830_spi_pdev0 = {
	.name = "dm_spi",
	.id = 0,
	.resource = da830_spi_resources0,
	.num_resources = ARRAY_SIZE(da830_spi_resources0),
	.dev = {
		.platform_data = &da830_spi_pdata0,
	},
};

void __init da830_init_spi0(unsigned char* chip_sel, unsigned int num_sel,
	struct spi_board_info *info, unsigned num_dev)
{
	struct davinci_spi_platform_data *pdata =
			da830_spi_pdev0.dev.platform_data;

	spi_register_board_info(info, num_dev);

	pdata->chip_sel = chip_sel;
	pdata->num_chipselect = num_sel;
	platform_device_register(&da830_spi_pdev0);
}

static struct resource da850_mcbsp0_resources[] = {
	{
		.start	= 0x01D10000,
		.end	= 0x01D10FFF,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= 0x01F10000,
		.end	= 0x01F10FFF,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= IRQ_DA850_MCBSP0RINT,
		.end	= IRQ_DA850_MCBSP0RINT,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= IRQ_DA850_MCBSP0XINT,
		.end	= IRQ_DA850_MCBSP0XINT,
		.flags	= IORESOURCE_IRQ,
	},
	/* first RX, then TX */
	{
		.start	= 2,
		.end	= 2,
		.flags	= IORESOURCE_DMA,
	},
	{
		.start	= 3,
		.end	= 3,
		.flags	= IORESOURCE_DMA,
	},
};

static struct resource da850_mcbsp1_resources[] = {
	{
		.start	= 0x01D11000,
		.end	= 0x01D11FFF,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= 0x01F11000,
		.end	= 0x01F11FFF,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= IRQ_DA850_MCBSP1RINT,
		.end	= IRQ_DA850_MCBSP1RINT,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= IRQ_DA850_MCBSP1XINT,
		.end	= IRQ_DA850_MCBSP1XINT,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= 4,
		.end	= 4,
		.flags	= IORESOURCE_DMA,
	},
	{
		.start	= 5,
		.end	= 5,
		.flags	= IORESOURCE_DMA,
	},
};
static struct platform_device da850_mcbsp0_device = {
	.name		= "davinci-mcbsp",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(da850_mcbsp0_resources),
	.resource	= da850_mcbsp0_resources,
};

static struct platform_device da850_mcbsp1_device = {
	.name		= "davinci-mcbsp",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(da850_mcbsp1_resources),
	.resource	= da850_mcbsp1_resources,
};

int
__init da850_init_mcbsp(struct davinci_mcbsp_platform_data *pdata)
{
	struct platform_device *pdev;

	if (!pdata->inst) {
		pdev = &da850_mcbsp0_device;
	} else if (pdata->inst == 1) {
		pdev = &da850_mcbsp1_device;
	} else {
		printk(KERN_ERR "Cannot initialize McBSP device, Invalid id\n");
		return -EINVAL;
	}

	pdev->dev.platform_data = pdata;
	return platform_device_register(pdev);
}

#define DA8XX_USB0_BASE		0x01e00000
#ifdef CONFIG_USB_TI_CPPI41_DMA
/* DMA block configuration */
static const struct cppi41_tx_ch tx_ch_info[] = {
	[0] = {
		.port_num	= 1,
		.num_tx_queue	= 2,
		.tx_queue	= { { 0, 16 }, { 0, 17 } }
	},
	[1] = {
		.port_num	= 2,
		.num_tx_queue	= 2,
		.tx_queue	= { { 0, 18 }, { 0, 19 } }
	},
	[2] = {
		.port_num	= 3,
		.num_tx_queue	= 2,
		.tx_queue	= { { 0, 20 }, { 0, 21 } }
	},
	[3] = {
		.port_num	= 4,
		.num_tx_queue	= 2,
		.tx_queue	= { { 0, 22 }, { 0, 23 } }
	}
};

#define DA8XX_USB0_CFG_BASE IO_ADDRESS(DA8XX_USB0_BASE)
const struct cppi41_dma_block cppi41_dma_block[CPPI41_NUM_DMA_BLOCK] = {
	[0] = {
		.global_ctrl_base  = DA8XX_USB0_CFG_BASE + 0x1000,
		.ch_ctrl_stat_base = DA8XX_USB0_CFG_BASE + 0x1800,
		.sched_ctrl_base  = DA8XX_USB0_CFG_BASE + 0x2000,
		.sched_table_base = DA8XX_USB0_CFG_BASE + 0x2800,
		.num_tx_ch        = 4,
		.num_rx_ch        = 4,
		.tx_ch_info       = tx_ch_info
	}
};
EXPORT_SYMBOL(cppi41_dma_block);

/* Queues 0 to 27 are pre-assigned, others are spare */
static const u32 assigned_queues[] = { 0x0fffffff, 0 };

/* Queue manager information */
struct cppi41_queue_mgr cppi41_queue_mgr[CPPI41_NUM_QUEUE_MGR] = {
	[0] = {
		.q_mgr_rgn_base    = DA8XX_USB0_CFG_BASE + 0x4000,
		.desc_mem_rgn_base = DA8XX_USB0_CFG_BASE + 0x5000,
		.q_mgmt_rgn_base   = DA8XX_USB0_CFG_BASE + 0x6000,
		.q_stat_rgn_base   = DA8XX_USB0_CFG_BASE + 0x6800,

		.num_queue       = 64,
		.queue_types     = CPPI41_FREE_DESC_BUF_QUEUE |
					CPPI41_UNASSIGNED_QUEUE,
		.base_fdbq_num    = 0,
		.assigned       = assigned_queues
	}
};
EXPORT_SYMBOL(cppi41_queue_mgr);

const u8	cppi41_num_queue_mgr = 1;
const u8	cppi41_num_dma_block = 1;

/* Fair scheduling */
u8 dma_sched_table[] = {
	0x0, 0x80, 0x01, 0x81, 0x02, 0x82, 0x03, 0x83
};

#define USB_CPPI41_NUM_CH 4

int __init cppi41_init(void)
{
	u16 numch, order;

	/* Allocate memory for region 0 */
	cppi41_queue_mgr[0].ptr_rgn0 = dma_alloc_coherent(NULL, 0x10000,
						&cppi41_queue_mgr[0].
						phys_ptr_rgn0,
						GFP_KERNEL | GFP_DMA);
	/* Initialize Queue Manager 0, alloc for region 0 */
	cppi41_queue_mgr_init(0, cppi41_queue_mgr[0].phys_ptr_rgn0, 0x3fff);

	numch =  USB_CPPI41_NUM_CH * 2;
	order = get_count_order(numch);

	if (order < 5)
		order = 5;

	cppi41_dma_block_init(0, 0, order, dma_sched_table, numch);

	return 0;
}
EXPORT_SYMBOL(cppi41_init);
#endif
