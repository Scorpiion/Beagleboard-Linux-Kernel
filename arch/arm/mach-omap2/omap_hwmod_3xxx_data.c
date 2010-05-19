/*
 * omap_hwmod_3xxx_data.c - hardware modules present on the OMAP3xxx chips
 *
 * Copyright (C) 2009-2010 Nokia Corporation
 * Paul Walmsley
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * The data in this file should be completely autogeneratable from
 * the TI hardware database or other technical documentation.
 *
 * XXX these should be marked initdata for multi-OMAP kernels
 */
#include <plat/omap_hwmod.h>
#include <mach/irqs.h>
#include <plat/cpu.h>
#include <plat/dma.h>
#include <plat/gpio.h>
#include <plat/l4_3xxx.h>
#include <plat/i2c.h>
#include <plat/serial.h>

#include "omap_hwmod_common_data.h"

#include "prm-regbits-34xx.h"

/*
 * OMAP3xxx hardware module integration data
 *
 * ALl of the data in this section should be autogeneratable from the
 * TI hardware database or other technical documentation.  Data that
 * is driver-specific or driver-kernel integration-specific belongs
 * elsewhere.
 */

static struct omap_hwmod omap3xxx_mpu_hwmod;
static struct omap_hwmod omap3xxx_l3_hwmod;
static struct omap_hwmod omap3xxx_l4_core_hwmod;
static struct omap_hwmod omap3xxx_l4_per_hwmod;

/* L3 -> L4_CORE interface */
static struct omap_hwmod_ocp_if omap3xxx_l3__l4_core = {
	.master	= &omap3xxx_l3_hwmod,
	.slave	= &omap3xxx_l4_core_hwmod,
	.user	= OCP_USER_MPU | OCP_USER_SDMA,
};

/* L3 -> L4_PER interface */
static struct omap_hwmod_ocp_if omap3xxx_l3__l4_per = {
	.master = &omap3xxx_l3_hwmod,
	.slave	= &omap3xxx_l4_per_hwmod,
	.user	= OCP_USER_MPU | OCP_USER_SDMA,
};

/* MPU -> L3 interface */
static struct omap_hwmod_ocp_if omap3xxx_mpu__l3 = {
	.master = &omap3xxx_mpu_hwmod,
	.slave	= &omap3xxx_l3_hwmod,
	.user	= OCP_USER_MPU,
};

/* Slave interfaces on the L3 interconnect */
static struct omap_hwmod_ocp_if *omap3xxx_l3_slaves[] = {
	&omap3xxx_mpu__l3,
};

/* Master interfaces on the L3 interconnect */
static struct omap_hwmod_ocp_if *omap3xxx_l3_masters[] = {
	&omap3xxx_l3__l4_core,
	&omap3xxx_l3__l4_per,
};

/* L3 */
static struct omap_hwmod omap3xxx_l3_hwmod = {
	.name		= "l3_hwmod",
	.class		= &l3_hwmod_class,
	.masters	= omap3xxx_l3_masters,
	.masters_cnt	= ARRAY_SIZE(omap3xxx_l3_masters),
	.slaves		= omap3xxx_l3_slaves,
	.slaves_cnt	= ARRAY_SIZE(omap3xxx_l3_slaves),
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP3430)
};

static struct omap_hwmod omap3xxx_l4_wkup_hwmod;
static struct omap_hwmod omap3xxx_gpio1_hwmod;
static struct omap_hwmod omap3xxx_gpio2_hwmod;
static struct omap_hwmod omap3xxx_gpio3_hwmod;
static struct omap_hwmod omap3xxx_gpio4_hwmod;
static struct omap_hwmod omap3xxx_gpio5_hwmod;
static struct omap_hwmod omap3xxx_gpio6_hwmod;
static struct omap_hwmod omap3xxx_i2c1_hwmod;
static struct omap_hwmod omap3xxx_i2c2_hwmod;
static struct omap_hwmod omap3xxx_i2c3_hwmod;
static struct omap_hwmod omap3xxx_uart1_hwmod;
static struct omap_hwmod omap3xxx_uart2_hwmod;
static struct omap_hwmod omap3xxx_uart3_hwmod;

/* L4_CORE -> L4_WKUP interface */
static struct omap_hwmod_ocp_if omap3xxx_l4_core__l4_wkup = {
	.master	= &omap3xxx_l4_core_hwmod,
	.slave	= &omap3xxx_l4_wkup_hwmod,
	.user	= OCP_USER_MPU | OCP_USER_SDMA,
};

/* L4 WKUP -> GPIO1 interface */
static struct omap_hwmod_addr_space omap3xxx_gpio1_addr_space[] = {
	{
		.pa_start	= OMAP34XX_GPIO1_BASE,
		.pa_end		= OMAP34XX_GPIO1_BASE + SZ_4K - 1,
		.flags		= ADDR_TYPE_RT
	},
};

static struct omap_hwmod_ocp_if omap3xxx_l4_wkup__gpio1 = {
	.master		= &omap3xxx_l4_wkup_hwmod,
	.slave		= &omap3xxx_gpio1_hwmod,
	.clk		= "gpio1_ick",
	.addr		= omap3xxx_gpio1_addr_space,
	.addr_cnt	= ARRAY_SIZE(omap3xxx_gpio1_addr_space),
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

/* L4 PER -> GPIO2 interface */
static struct omap_hwmod_addr_space omap3xxx_gpio2_addr_space[] = {
	{
		.pa_start	= OMAP34XX_GPIO2_BASE,
		.pa_end		= OMAP34XX_GPIO2_BASE + SZ_4K - 1,
		.flags		= ADDR_TYPE_RT
	},
};

static struct omap_hwmod_ocp_if omap3xxx_l4_per__gpio2 = {
	.master		= &omap3xxx_l4_per_hwmod,
	.slave		= &omap3xxx_gpio2_hwmod,
	.clk		= "gpio2_ick",
	.addr		= omap3xxx_gpio2_addr_space,
	.addr_cnt	= ARRAY_SIZE(omap3xxx_gpio2_addr_space),
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

/* L4 PER -> GPIO3 interface */
static struct omap_hwmod_addr_space omap3xxx_gpio3_addr_space[] = {
	{
		.pa_start	= OMAP34XX_GPIO3_BASE,
		.pa_end		= OMAP34XX_GPIO3_BASE + SZ_4K - 1,
		.flags		= ADDR_TYPE_RT
	},
};

static struct omap_hwmod_ocp_if omap3xxx_l4_per__gpio3 = {
	.master		= &omap3xxx_l4_per_hwmod,
	.slave		= &omap3xxx_gpio3_hwmod,
	.clk		= "gpio3_ick",
	.addr		= omap3xxx_gpio3_addr_space,
	.addr_cnt	= ARRAY_SIZE(omap3xxx_gpio3_addr_space),
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

/* L4 PER -> GPIO4 interface */
static struct omap_hwmod_addr_space omap3xxx_gpio4_addr_space[] = {
	{
		.pa_start	= OMAP34XX_GPIO4_BASE,
		.pa_end		= OMAP34XX_GPIO4_BASE + SZ_4K - 1,
		.flags		= ADDR_TYPE_RT
	},
};

static struct omap_hwmod_ocp_if omap3xxx_l4_per__gpio4 = {
	.master		= &omap3xxx_l4_per_hwmod,
	.slave		= &omap3xxx_gpio4_hwmod,
	.clk		= "gpio4_ick",
	.addr		= omap3xxx_gpio4_addr_space,
	.addr_cnt	= ARRAY_SIZE(omap3xxx_gpio4_addr_space),
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

/* L4 PER -> GPIO5 interface */
static struct omap_hwmod_addr_space omap3xxx_gpio5_addr_space[] = {
	{
		.pa_start	= OMAP34XX_GPIO5_BASE,
		.pa_end		= OMAP34XX_GPIO5_BASE + SZ_4K - 1,
		.flags		= ADDR_TYPE_RT
	},
};

static struct omap_hwmod_ocp_if omap3xxx_l4_per__gpio5 = {
	.master		= &omap3xxx_l4_per_hwmod,
	.slave		= &omap3xxx_gpio5_hwmod,
	.clk		= "gpio5_ick",
	.addr		= omap3xxx_gpio5_addr_space,
	.addr_cnt	= ARRAY_SIZE(omap3xxx_gpio5_addr_space),
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

/* L4 PER -> GPIO6 interface */
static struct omap_hwmod_addr_space omap3xxx_gpio6_addr_space[] = {
	{
		.pa_start	= OMAP34XX_GPIO6_BASE,
		.pa_end		= OMAP34XX_GPIO6_BASE + SZ_4K - 1,
		.flags		= ADDR_TYPE_RT
	},
};

static struct omap_hwmod_ocp_if omap3xxx_l4_per__gpio6 = {
	.master		= &omap3xxx_l4_per_hwmod,
	.slave		= &omap3xxx_gpio6_hwmod,
	.clk		= "gpio6_ick",
	.addr		= omap3xxx_gpio6_addr_space,
	.addr_cnt	= ARRAY_SIZE(omap3xxx_gpio6_addr_space),
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

#define OMAP3_I2C1_BASE			(L4_34XX_BASE + 0x70000)
#define OMAP3_I2C2_BASE			(L4_34XX_BASE + 0x72000)
#define OMAP3_I2C3_BASE			(L4_34XX_BASE + 0x60000)

/* I2C IP block address space length (in bytes) */
#define OMAP2_I2C_AS_LEN		128

/* L4 CORE -> I2C1 interface */
static struct omap_hwmod_addr_space omap3xxx_i2c1_addr_space[] = {
	{
		.pa_start	= OMAP3_I2C1_BASE,
		.pa_end		= OMAP3_I2C1_BASE + OMAP2_I2C_AS_LEN - 1,
		.flags		= ADDR_TYPE_RT,
	},
};

static struct omap_hwmod_ocp_if omap3_l4_core__i2c1 = {
	.master		= &omap3xxx_l4_core_hwmod,
	.slave		= &omap3xxx_i2c1_hwmod,
	.clk		= "i2c1_ick",
	.addr		= omap3xxx_i2c1_addr_space,
	.addr_cnt	= ARRAY_SIZE(omap3xxx_i2c1_addr_space),
	.fw = {
		.omap2 = {
			.l4_fw_region  = OMAP3_L4_CORE_FW_I2C1_REGION,
			.l4_prot_group = 7,
		}
	},
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
	.flags		= OMAP_FIREWALL_L4
};

/* L4 CORE -> I2C2 interface */
static struct omap_hwmod_addr_space omap3xxx_i2c2_addr_space[] = {
	{
		.pa_start	= OMAP3_I2C2_BASE,
		.pa_end		= OMAP3_I2C2_BASE + OMAP2_I2C_AS_LEN - 1,
		.flags		= ADDR_TYPE_RT,
	},
};

static struct omap_hwmod_ocp_if omap3_l4_core__i2c2 = {
	.master		= &omap3xxx_l4_core_hwmod,
	.slave		= &omap3xxx_i2c2_hwmod,
	.clk		= "i2c2_ick",
	.addr		= omap3xxx_i2c2_addr_space,
	.addr_cnt	= ARRAY_SIZE(omap3xxx_i2c2_addr_space),
	.fw = {
		.omap2 = {
			.l4_fw_region  = OMAP3_L4_CORE_FW_I2C2_REGION,
			.l4_prot_group = 7,
		}
	},
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
	.flags		= OMAP_FIREWALL_L4
};

/* L4 CORE -> I2C3 interface */
static struct omap_hwmod_addr_space omap3xxx_i2c3_addr_space[] = {
	{
		.pa_start	= OMAP3_I2C3_BASE,
		.pa_end		= OMAP3_I2C3_BASE + OMAP2_I2C_AS_LEN - 1,
		.flags		= ADDR_TYPE_RT,
	},
};

static struct omap_hwmod_ocp_if omap3_l4_core__i2c3 = {
	.master		= &omap3xxx_l4_core_hwmod,
	.slave		= &omap3xxx_i2c3_hwmod,
	.clk		= "i2c3_ick",
	.addr		= omap3xxx_i2c3_addr_space,
	.addr_cnt	= ARRAY_SIZE(omap3xxx_i2c3_addr_space),
	.fw = {
		.omap2 = {
			.l4_fw_region  = OMAP3_L4_CORE_FW_I2C3_REGION,
			.l4_prot_group = 7,
		}
	},
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
	.flags		= OMAP_FIREWALL_L4
};

/* L4 CORE -> UART1 interface */
static struct omap_hwmod_addr_space omap3xxx_uart1_addr_space[] = {
	{
		.pa_start	= OMAP3_UART1_BASE,
		.pa_end		= OMAP3_UART1_BASE + SZ_8K - 1,
		.flags		= ADDR_MAP_ON_INIT | ADDR_TYPE_RT,
	},
};

static struct omap_hwmod_ocp_if omap3_l4_core__uart1 = {
	.master		= &omap3xxx_l4_core_hwmod,
	.slave		= &omap3xxx_uart1_hwmod,
	.clk		= "uart1_ick",
	.addr		= omap3xxx_uart1_addr_space,
	.addr_cnt	= ARRAY_SIZE(omap3xxx_uart1_addr_space),
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

/* L4 CORE -> UART2 interface */
static struct omap_hwmod_addr_space omap3xxx_uart2_addr_space[] = {
	{
		.pa_start	= OMAP3_UART2_BASE,
		.pa_end		= OMAP3_UART2_BASE + SZ_1K - 1,
		.flags		= ADDR_MAP_ON_INIT |ADDR_TYPE_RT,
	},
};

static struct omap_hwmod_ocp_if omap3_l4_core__uart2 = {
	.master		= &omap3xxx_l4_core_hwmod,
	.slave		= &omap3xxx_uart2_hwmod,
	.clk		= "uart2_ick",
	.addr		= omap3xxx_uart2_addr_space,
	.addr_cnt	= ARRAY_SIZE(omap3xxx_uart2_addr_space),
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

/* L4 PER -> UART3 interface */
static struct omap_hwmod_addr_space omap3xxx_uart3_addr_space[] = {
	{
		.pa_start	= OMAP3_UART3_BASE,
		.pa_end		= OMAP3_UART3_BASE + SZ_1K - 1,
		.flags		= ADDR_MAP_ON_INIT | ADDR_TYPE_RT,
	},
};

static struct omap_hwmod_ocp_if omap3_l4_per__uart3 = {
	.master		= &omap3xxx_l4_per_hwmod,
	.slave		= &omap3xxx_uart3_hwmod,
	.clk		= "uart3_ick",
	.addr		= omap3xxx_uart3_addr_space,
	.addr_cnt	= ARRAY_SIZE(omap3xxx_uart3_addr_space),
	.user		= OCP_USER_MPU | OCP_USER_SDMA,
};

/* Slave interfaces on the L4_CORE interconnect */
static struct omap_hwmod_ocp_if *omap3xxx_l4_core_slaves[] = {
	&omap3xxx_l3__l4_core,
};

/* Master interfaces on the L4_CORE interconnect */
static struct omap_hwmod_ocp_if *omap3xxx_l4_core_masters[] = {
	&omap3xxx_l4_core__l4_wkup,
	&omap3_l4_core__i2c1,
	&omap3_l4_core__i2c2,
	&omap3_l4_core__i2c3,
	&omap3_l4_core__uart1,
	&omap3_l4_core__uart2,
};

/* L4 CORE */
static struct omap_hwmod omap3xxx_l4_core_hwmod = {
	.name		= "l4_core_hwmod",
	.class		= &l4_hwmod_class,
	.masters	= omap3xxx_l4_core_masters,
	.masters_cnt	= ARRAY_SIZE(omap3xxx_l4_core_masters),
	.slaves		= omap3xxx_l4_core_slaves,
	.slaves_cnt	= ARRAY_SIZE(omap3xxx_l4_core_slaves),
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP3430)
};

/* Slave interfaces on the L4_PER interconnect */
static struct omap_hwmod_ocp_if *omap3xxx_l4_per_slaves[] = {
	&omap3xxx_l3__l4_per,
	&omap3xxx_l4_per__gpio2,
	&omap3xxx_l4_per__gpio3,
	&omap3xxx_l4_per__gpio4,
	&omap3xxx_l4_per__gpio5,
	&omap3xxx_l4_per__gpio6,
	&omap3_l4_per__uart3,
};

/* Master interfaces on the L4_PER interconnect */
static struct omap_hwmod_ocp_if *omap3xxx_l4_per_masters[] = {
	&omap3_l4_per__uart3,
};

/* L4 PER */
static struct omap_hwmod omap3xxx_l4_per_hwmod = {
	.name		= "l4_per_hwmod",
	.class		= &l4_hwmod_class,
	.masters	= omap3xxx_l4_per_masters,
	.masters_cnt	= ARRAY_SIZE(omap3xxx_l4_per_masters),
	.slaves		= omap3xxx_l4_per_slaves,
	.slaves_cnt	= ARRAY_SIZE(omap3xxx_l4_per_slaves),
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP3430)
};

/* Slave interfaces on the L4_WKUP interconnect */
static struct omap_hwmod_ocp_if *omap3xxx_l4_wkup_slaves[] = {
	&omap3xxx_l4_core__l4_wkup,
	&omap3xxx_l4_wkup__gpio1,
};

/* Master interfaces on the L4_WKUP interconnect */
static struct omap_hwmod_ocp_if *omap3xxx_l4_wkup_masters[] = {
};

/* L4 WKUP */
static struct omap_hwmod omap3xxx_l4_wkup_hwmod = {
	.name		= "l4_wkup_hwmod",
	.class		= &l4_hwmod_class,
	.masters	= omap3xxx_l4_wkup_masters,
	.masters_cnt	= ARRAY_SIZE(omap3xxx_l4_wkup_masters),
	.slaves		= omap3xxx_l4_wkup_slaves,
	.slaves_cnt	= ARRAY_SIZE(omap3xxx_l4_wkup_slaves),
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP3430)
};

/* Master interfaces on the MPU device */
static struct omap_hwmod_ocp_if *omap3xxx_mpu_masters[] = {
	&omap3xxx_mpu__l3,
};

/* MPU */
static struct omap_hwmod omap3xxx_mpu_hwmod = {
	.name		= "mpu",
	.class		= &mpu_hwmod_class,
	.main_clk	= "arm_fck",
	.masters	= omap3xxx_mpu_masters,
	.masters_cnt	= ARRAY_SIZE(omap3xxx_mpu_masters),
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP3430),
};

/* UART common */

static struct omap_hwmod_class_sysconfig uart_sysc = {
	.rev_offs	= 0x50,
	.sysc_offs	= 0x54,
	.syss_offs	= 0x58,
	.sysc_flags	= (SYSC_HAS_SIDLEMODE |
			   SYSC_HAS_ENAWAKEUP | SYSC_HAS_SOFTRESET |
			   SYSC_HAS_AUTOIDLE),
	.idlemodes	= (SIDLE_FORCE | SIDLE_NO | SIDLE_SMART),
	.sysc_fields    = &omap_hwmod_sysc_type1,
};

static struct omap_hwmod_class uart_class = {
	.name = "uart",
	.sysc = &uart_sysc,
};

/* UART1 */

static struct omap_hwmod_irq_info uart1_mpu_irqs[] = {
	{ .irq = INT_24XX_UART1_IRQ, },
};

static struct omap_hwmod_dma_info uart1_sdma_chs[] = {
	{ .name = "rx",	.dma_ch = OMAP24XX_DMA_UART1_RX, },
	{ .name = "tx",	.dma_ch = OMAP24XX_DMA_UART1_TX, },
};

static struct omap_hwmod_ocp_if *omap3xxx_uart1_slaves[] = {
	&omap3_l4_core__uart1,
};

static struct omap_hwmod omap3xxx_uart1_hwmod = {
	.name		= "uart1",
	.mpu_irqs	= uart1_mpu_irqs,
	.mpu_irqs_cnt	= ARRAY_SIZE(uart1_mpu_irqs),
	.sdma_chs	= uart1_sdma_chs,
	.sdma_chs_cnt	= ARRAY_SIZE(uart1_sdma_chs),
	.main_clk	= "uart1_fck",
	.prcm		= {
		.omap2 = {
			.prcm_reg_id = 1,
			.module_bit = OMAP3430_EN_UART1_SHIFT,
		},
	},
	.slaves		= omap3xxx_uart1_slaves,
	.slaves_cnt	= ARRAY_SIZE(omap3xxx_uart1_slaves),
	.class		= &uart_class,
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP3430),
};

/* UART2 */

static struct omap_hwmod_irq_info uart2_mpu_irqs[] = {
	{ .irq = INT_24XX_UART2_IRQ, },
};

static struct omap_hwmod_dma_info uart2_sdma_chs[] = {
	{ .name = "rx",	.dma_ch = OMAP24XX_DMA_UART2_RX, },
	{ .name = "tx",	.dma_ch = OMAP24XX_DMA_UART2_TX, },
};

static struct omap_hwmod_ocp_if *omap3xxx_uart2_slaves[] = {
	&omap3_l4_core__uart2,
};

static struct omap_hwmod omap3xxx_uart2_hwmod = {
	.name		= "uart2",
	.mpu_irqs	= uart2_mpu_irqs,
	.mpu_irqs_cnt	= ARRAY_SIZE(uart2_mpu_irqs),
	.sdma_chs	= uart2_sdma_chs,
	.sdma_chs_cnt	= ARRAY_SIZE(uart2_sdma_chs),
	.main_clk	= "uart2_fck",
	.prcm		= {
		.omap2 = {
			.prcm_reg_id = 1,
			.module_bit = OMAP3430_EN_UART2_SHIFT,
		},
	},
	.slaves		= omap3xxx_uart2_slaves,
	.slaves_cnt	= ARRAY_SIZE(omap3xxx_uart2_slaves),
	.class		= &uart_class,
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP3430),
};

/* UART3 */

static struct omap_hwmod_irq_info uart3_mpu_irqs[] = {
	{ .irq = INT_24XX_UART3_IRQ, },
};

static struct omap_hwmod_dma_info uart3_sdma_chs[] = {
	{ .name = "rx",	.dma_ch = OMAP24XX_DMA_UART3_RX, },
	{ .name = "tx",	.dma_ch = OMAP24XX_DMA_UART3_TX, },
};

static struct omap_hwmod_ocp_if *omap3xxx_uart3_slaves[] = {
	&omap3_l4_per__uart3,
};

static struct omap_hwmod omap3xxx_uart3_hwmod = {
	.name		= "uart3",
	.mpu_irqs	= uart3_mpu_irqs,
	.mpu_irqs_cnt	= ARRAY_SIZE(uart3_mpu_irqs),
	.sdma_chs	= uart3_sdma_chs,
	.sdma_chs_cnt	= ARRAY_SIZE(uart3_sdma_chs),
	.main_clk	= "uart3_fck",
	.prcm		= {
		.omap2 = {
			.prcm_reg_id = 1,
			.module_bit = OMAP3430_EN_UART3_SHIFT,
		},
	},
	.slaves		= omap3xxx_uart3_slaves,
	.slaves_cnt	= ARRAY_SIZE(omap3xxx_uart3_slaves),
	.class		= &uart_class,
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP3430),
};

/* GPIO common */
static struct omap_gpio_dev_attr gpio_dev_attr = {
	.gpio_bank_count = 6,
	.gpio_bank_bits = 32,
	.dbck_flag = true,
	.fck_flag = false,
};

static struct omap_hwmod_class_sysconfig gpio_sysc = {
	.rev_offs	= 0x0000,
	.sysc_offs	= 0x0010,
	.syss_offs	= 0x0014,
	.sysc_flags	= (SYSC_HAS_SIDLEMODE |
			   SYSC_HAS_ENAWAKEUP | SYSC_HAS_SOFTRESET |
			   SYSC_HAS_AUTOIDLE),
	.idlemodes	= (SIDLE_FORCE | SIDLE_NO | SIDLE_SMART),
	.sysc_fields    = &omap_hwmod_sysc_type1,
};

static struct omap_hwmod_class gpio_class = {
	.name = "gpio",
	.sysc = &gpio_sysc,
};

/* GPIO1 */

static struct omap_hwmod_irq_info gpio1_mpu_irqs[] = {
	{ .name = "gpio_mpu_irq", .irq = INT_34XX_GPIO_BANK1 },
};

static struct omap_hwmod_opt_clk gpio1_opt_clks[] = {
	{ .role = "gpio1_dbclk", .clk = "gpio1_dbck", },
};

static struct omap_hwmod_ocp_if *omap3xxx_gpio1_slaves[] = {
	&omap3xxx_l4_wkup__gpio1,
};

static struct omap_hwmod omap3xxx_gpio1_hwmod = {
	.name		= "gpio1_hwmod",
	.mpu_irqs	= gpio1_mpu_irqs,
	.mpu_irqs_cnt	= ARRAY_SIZE(gpio1_mpu_irqs),
	.main_clk	= NULL,
	.opt_clks	= gpio1_opt_clks,
	.opt_clks_cnt	= ARRAY_SIZE(gpio1_opt_clks),
	.prcm		= {
		.omap2 = {
			.prcm_reg_id = 1,
			.module_bit = OMAP3430_EN_GPIO1_SHIFT,
			.module_offs = WKUP_MOD,
			.idlest_reg_id = 1,
			.idlest_idle_bit = 3,
		},
	},
	.slaves		= omap3xxx_gpio1_slaves,
	.slaves_cnt	= ARRAY_SIZE(omap3xxx_gpio1_slaves),
	.class		= &gpio_class,
	.dev_attr	= &gpio_dev_attr,
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP3430),
};

/* GPIO2 */

static struct omap_hwmod_irq_info gpio2_mpu_irqs[] = {
	{ .name = "gpio_mpu_irq", .irq = INT_34XX_GPIO_BANK2 },
};

static struct omap_hwmod_opt_clk gpio2_opt_clks[] = {
	{ .role = "gpio2_dbclk", .clk = "gpio2_dbck", },
};

static struct omap_hwmod_ocp_if *omap3xxx_gpio2_slaves[] = {
	&omap3xxx_l4_per__gpio2,
};

static struct omap_hwmod omap3xxx_gpio2_hwmod = {
	.name		= "gpio2_hwmod",
	.mpu_irqs	= gpio2_mpu_irqs,
	.mpu_irqs_cnt	= ARRAY_SIZE(gpio2_mpu_irqs),
	.main_clk	= NULL,
	.opt_clks	= gpio2_opt_clks,
	.opt_clks_cnt	= ARRAY_SIZE(gpio2_opt_clks),
	.prcm		= {
		.omap2 = {
			.prcm_reg_id = 1,
			.module_bit = OMAP3430_EN_GPIO2_SHIFT,
			.module_offs = OMAP3430_PER_MOD,
			.idlest_reg_id = 1,
			.idlest_idle_bit = 13,
		},
	},
	.slaves		= omap3xxx_gpio2_slaves,
	.slaves_cnt	= ARRAY_SIZE(omap3xxx_gpio2_slaves),
	.class		= &gpio_class,
	.dev_attr	= &gpio_dev_attr,
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP3430),
};

/* GPIO3 */

static struct omap_hwmod_irq_info gpio3_mpu_irqs[] = {
	{ .name = "gpio_mpu_irq", .irq = INT_34XX_GPIO_BANK3 },
};

static struct omap_hwmod_opt_clk gpio3_opt_clks[] = {
	{ .role = "gpio3_dbclk", .clk = "gpio3_dbck", },
};

static struct omap_hwmod_ocp_if *omap3xxx_gpio3_slaves[] = {
	&omap3xxx_l4_per__gpio3,
};

static struct omap_hwmod omap3xxx_gpio3_hwmod = {
	.name		= "gpio3_hwmod",
	.mpu_irqs	= gpio3_mpu_irqs,
	.mpu_irqs_cnt	= ARRAY_SIZE(gpio3_mpu_irqs),
	.main_clk	= NULL,
	.opt_clks	= gpio3_opt_clks,
	.opt_clks_cnt	= ARRAY_SIZE(gpio3_opt_clks),
	.prcm		= {
		.omap2 = {
			.prcm_reg_id = 1,
			.module_bit = OMAP3430_EN_GPIO3_SHIFT,
			.module_offs = OMAP3430_PER_MOD,
			.idlest_reg_id = 1,
			.idlest_idle_bit = 14,
		},
	},
	.slaves		= omap3xxx_gpio3_slaves,
	.slaves_cnt	= ARRAY_SIZE(omap3xxx_gpio3_slaves),
	.class		= &gpio_class,
	.dev_attr	= &gpio_dev_attr,
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP3430),
};

/* GPIO4 */

static struct omap_hwmod_irq_info gpio4_mpu_irqs[] = {
	{ .name = "gpio_mpu_irq", .irq = INT_34XX_GPIO_BANK4 },
};

static struct omap_hwmod_opt_clk gpio4_opt_clks[] = {
	{ .role = "gpio4_dbclk", .clk = "gpio4_dbck", },
};

static struct omap_hwmod_ocp_if *omap3xxx_gpio4_slaves[] = {
	&omap3xxx_l4_per__gpio4,
};

static struct omap_hwmod omap3xxx_gpio4_hwmod = {
	.name		= "gpio4_hwmod",
	.mpu_irqs	= gpio4_mpu_irqs,
	.mpu_irqs_cnt	= ARRAY_SIZE(gpio4_mpu_irqs),
	.main_clk	= NULL,
	.opt_clks	= gpio4_opt_clks,
	.opt_clks_cnt	= ARRAY_SIZE(gpio4_opt_clks),
	.prcm		= {
		.omap2 = {
			.prcm_reg_id = 1,
			.module_bit = OMAP3430_EN_GPIO4_SHIFT,
			.module_offs = OMAP3430_PER_MOD,
			.idlest_reg_id = 1,
			.idlest_idle_bit = 15,
		},
	},
	.slaves		= omap3xxx_gpio4_slaves,
	.slaves_cnt	= ARRAY_SIZE(omap3xxx_gpio4_slaves),
	.class		= &gpio_class,
	.dev_attr	= &gpio_dev_attr,
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP3430),
};


/* GPIO5 */

static struct omap_hwmod_irq_info gpio5_mpu_irqs[] = {
	{ .name = "gpio_mpu_irq", .irq = INT_34XX_GPIO_BANK5 },
};

static struct omap_hwmod_opt_clk gpio5_opt_clks[] = {
	{ .role = "gpio5_dbclk", .clk = "gpio5_dbck", },
};

static struct omap_hwmod_ocp_if *omap3xxx_gpio5_slaves[] = {
	&omap3xxx_l4_per__gpio5,
};

static struct omap_hwmod omap3xxx_gpio5_hwmod = {
	.name		= "gpio5_hwmod",
	.mpu_irqs	= gpio5_mpu_irqs,
	.mpu_irqs_cnt	= ARRAY_SIZE(gpio5_mpu_irqs),
	.main_clk	= NULL,
	.opt_clks	= gpio5_opt_clks,
	.opt_clks_cnt	= ARRAY_SIZE(gpio5_opt_clks),
	.prcm		= {
		.omap2 = {
			.prcm_reg_id = 1,
			.module_bit = OMAP3430_EN_GPIO5_SHIFT,
			.module_offs = OMAP3430_PER_MOD,
			.idlest_reg_id = 1,
			.idlest_idle_bit = 16,
		},
	},
	.slaves		= omap3xxx_gpio5_slaves,
	.slaves_cnt	= ARRAY_SIZE(omap3xxx_gpio5_slaves),
	.class		= &gpio_class,
	.dev_attr	= &gpio_dev_attr,
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP3430),
};

/* GPIO6 */

static struct omap_hwmod_irq_info gpio6_mpu_irqs[] = {
	{ .name = "gpio_mpu_irq", .irq = INT_34XX_GPIO_BANK6 },
};

static struct omap_hwmod_opt_clk gpio6_opt_clks[] = {
	{ .role = "gpio6_dbclk", .clk = "gpio6_dbck", },
};

static struct omap_hwmod_ocp_if *omap3xxx_gpio6_slaves[] = {
	&omap3xxx_l4_per__gpio6,
};

static struct omap_hwmod omap3xxx_gpio6_hwmod = {
	.name		= "gpio6_hwmod",
	.mpu_irqs	= gpio6_mpu_irqs,
	.mpu_irqs_cnt	= ARRAY_SIZE(gpio6_mpu_irqs),
	.main_clk	= NULL,
	.opt_clks	= gpio6_opt_clks,
	.opt_clks_cnt	= ARRAY_SIZE(gpio6_opt_clks),
	.prcm		= {
		.omap2 = {
			.prcm_reg_id = 1,
			.module_bit = OMAP3430_EN_GPIO6_SHIFT,
			.module_offs = OMAP3430_PER_MOD,
			.idlest_reg_id = 1,
			.idlest_idle_bit = 17,
		},
	},
	.slaves		= omap3xxx_gpio6_slaves,
	.slaves_cnt	= ARRAY_SIZE(omap3xxx_gpio6_slaves),
	.class		= &gpio_class,
	.dev_attr	= &gpio_dev_attr,
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP3430),
};

/* I2C common */
static struct omap_hwmod_class_sysconfig i2c_sysc = {
	.rev_offs	= 0x00,
	.sysc_offs	= 0x20,
	.syss_offs	= 0x10,
	.sysc_flags	= (SYSC_HAS_CLOCKACTIVITY | SYSC_HAS_SIDLEMODE |
			   SYSC_HAS_ENAWAKEUP | SYSC_HAS_SOFTRESET |
			   SYSC_HAS_AUTOIDLE),
	.idlemodes	= (SIDLE_FORCE | SIDLE_NO | SIDLE_SMART),
	.sysc_fields    = &omap_hwmod_sysc_type1,
};

static struct omap_hwmod_class i2c_class = {
	.name = "i2c",
	.sysc = &i2c_sysc,
};

/* I2C1 */

static struct omap_i2c_dev_attr i2c1_dev_attr = {
	.fifo_depth	= 8, /* bytes */
};

static struct omap_hwmod_irq_info i2c1_mpu_irqs[] = {
	{ .irq = INT_24XX_I2C1_IRQ, },
};

static struct omap_hwmod_dma_info i2c1_sdma_chs[] = {
	{ .name = "tx", .dma_ch = OMAP24XX_DMA_I2C1_TX },
	{ .name = "rx", .dma_ch = OMAP24XX_DMA_I2C1_RX },
};

static struct omap_hwmod_ocp_if *omap3xxx_i2c1_slaves[] = {
	&omap3_l4_core__i2c1,
};

static struct omap_hwmod omap3xxx_i2c1_hwmod = {
	.name		= "i2c1",
	.mpu_irqs	= i2c1_mpu_irqs,
	.mpu_irqs_cnt	= ARRAY_SIZE(i2c1_mpu_irqs),
	.sdma_chs	= i2c1_sdma_chs,
	.sdma_chs_cnt	= ARRAY_SIZE(i2c1_sdma_chs),
	.main_clk	= "i2c1_fck",
	.prcm		= {
		.omap2 = {
			.prcm_reg_id = 1,
			.module_bit = OMAP3430_GRPSEL_I2C1_SHIFT,
			.module_offs = CORE_MOD,
			.idlest_reg_id = 1,
			.idlest_idle_bit = 15,
		},
	},
	.slaves		= omap3xxx_i2c1_slaves,
	.slaves_cnt	= ARRAY_SIZE(omap3xxx_i2c1_slaves),
	.class		= &i2c_class,
	.dev_attr	= &i2c1_dev_attr,
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP3430),
};

/* I2C2 */

static struct omap_i2c_dev_attr i2c2_dev_attr = {
	.fifo_depth	= 8, /* bytes */
};

static struct omap_hwmod_irq_info i2c2_mpu_irqs[] = {
	{ .irq = INT_24XX_I2C2_IRQ, },
};

static struct omap_hwmod_dma_info i2c2_sdma_chs[] = {
	{ .name = "tx", .dma_ch = OMAP24XX_DMA_I2C2_TX },
	{ .name = "rx", .dma_ch = OMAP24XX_DMA_I2C2_RX },
};

static struct omap_hwmod_ocp_if *omap3xxx_i2c2_slaves[] = {
	&omap3_l4_core__i2c2,
};

static struct omap_hwmod omap3xxx_i2c2_hwmod = {
	.name		= "i2c2",
	.mpu_irqs	= i2c2_mpu_irqs,
	.mpu_irqs_cnt	= ARRAY_SIZE(i2c2_mpu_irqs),
	.sdma_chs	= i2c2_sdma_chs,
	.sdma_chs_cnt	= ARRAY_SIZE(i2c2_sdma_chs),
	.main_clk	= "i2c2_fck",
	.prcm		= {
		.omap2 = {
			.prcm_reg_id = 1,
			.module_bit = OMAP3430_GRPSEL_I2C2_SHIFT,
			.module_offs = CORE_MOD,
			.idlest_reg_id = 1,
			.idlest_idle_bit = 16,
		},
	},
	.slaves		= omap3xxx_i2c2_slaves,
	.slaves_cnt	= ARRAY_SIZE(omap3xxx_i2c2_slaves),
	.class		= &i2c_class,
	.dev_attr	= &i2c2_dev_attr,
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP3430),
};

/* I2C3 */

static struct omap_i2c_dev_attr i2c3_dev_attr = {
	.fifo_depth	= 64, /* bytes */
};

static struct omap_hwmod_irq_info i2c3_mpu_irqs[] = {
	{ .irq = INT_34XX_I2C3_IRQ, },
};

static struct omap_hwmod_dma_info i2c3_sdma_chs[] = {
	{ .name = "tx", .dma_ch = OMAP34XX_DMA_I2C3_TX },
	{ .name = "rx", .dma_ch = OMAP34XX_DMA_I2C3_RX },
};

static struct omap_hwmod_ocp_if *omap3xxx_i2c3_slaves[] = {
	&omap3_l4_core__i2c3,
};

static struct omap_hwmod omap3xxx_i2c3_hwmod = {
	.name		= "i2c3",
	.mpu_irqs	= i2c3_mpu_irqs,
	.mpu_irqs_cnt	= ARRAY_SIZE(i2c3_mpu_irqs),
	.sdma_chs	= i2c3_sdma_chs,
	.sdma_chs_cnt	= ARRAY_SIZE(i2c3_sdma_chs),
	.main_clk	= "i2c3_fck",
	.prcm		= {
		.omap2 = {
			.prcm_reg_id = 1,
			.module_bit = OMAP3430_GRPSEL_I2C3_SHIFT,
			.module_offs = CORE_MOD,
			.idlest_reg_id = 1,
			.idlest_idle_bit = 17,
		},
	},
	.slaves		= omap3xxx_i2c3_slaves,
	.slaves_cnt	= ARRAY_SIZE(omap3xxx_i2c3_slaves),
	.class		= &i2c_class,
	.dev_attr	= &i2c3_dev_attr,
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP3430),
};

static __initdata struct omap_hwmod *omap3xxx_hwmods[] = {
	&omap3xxx_l3_hwmod,
	&omap3xxx_l4_core_hwmod,
	&omap3xxx_l4_per_hwmod,
	&omap3xxx_l4_wkup_hwmod,
	&omap3xxx_mpu_hwmod,
	&omap3xxx_gpio1_hwmod,
	&omap3xxx_gpio2_hwmod,
	&omap3xxx_gpio3_hwmod,
	&omap3xxx_gpio4_hwmod,
	&omap3xxx_gpio5_hwmod,
	&omap3xxx_gpio6_hwmod,
	&omap3xxx_i2c1_hwmod,
	&omap3xxx_i2c2_hwmod,
	&omap3xxx_i2c3_hwmod,
	&omap3xxx_uart1_hwmod,
	&omap3xxx_uart2_hwmod,
	&omap3xxx_uart3_hwmod,
	NULL,
};

int __init omap3xxx_hwmod_init(void)
{
	return omap_hwmod_init(omap3xxx_hwmods);
}
