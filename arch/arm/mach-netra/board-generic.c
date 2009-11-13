/*
 * linux/arch/arm/mach-omap2/board-generic.c
 *
 * Copyright (C) 2005 Nokia Corporation
 * Author: Paul Mundt <paul.mundt@nokia.com>
 *
 * Modified from mach-omap/omap1/board-generic.c
 *
 * Code for generic OMAP2 board. Should work on many OMAP2 systems where
 * the bootloader passes the board-specific data to the kernel.
 * Do not put any board specific code to this file; create a new machine
 * type if you need custom low-level initializations.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>

#include <mach/hardware.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include <mach/irqs.h>
#include <mach/mux.h>
#include <mach/usb.h>
#include <mach/board.h>
#include <mach/common.h>

#include "clock.h"
#include "clockdomains.h"
#include "powerdomains.h"

static void __init omap_generic_init_irq(void)
{
	/* !@0 */
#if defined(CONFIG_ARCH_NETRA)
	omap2_set_globals_343x();
	/* Avoid these for HAPS54 */
#ifdef NO_HAPS54
	pwrdm_init(powerdomains_omap);
	clkdm_init(clockdomains_omap, clkdm_pwrdm_autodeps);
	omap2_clk_init();
#endif
#else
	omap2_init_common_hw(NULL);
#endif
	printk("*** Calling omap_init_irq...\n");
	omap_init_irq();
}

static struct omap_uart_config generic_uart_config __initdata = {
	.enabled_uarts = ((1 << 0) | (1 << 1) | (1 << 2)),
};

static struct omap_board_config_kernel generic_config[] __initdata = {
	{ OMAP_TAG_UART,	&generic_uart_config },
};

static void __init omap_generic_init(void)
{
	omap_board_config = generic_config;
	omap_board_config_size = ARRAY_SIZE(generic_config);
	omap_serial_init();
	omap_register_i2c_bus(1, 100, NULL, 0);
	omap_register_i2c_bus(2, 100, NULL, 0);
}

static void __init omap_generic_map_io(void)
{
	/* !@0 */
#if 0
	omap2_set_globals_242x(); /* should be 242x, 243x, or 343x */
#endif
	omap2_map_common_io();
}

extern struct sys_timer timer16;

MACHINE_START(NETRA, "Netra")
	/* Maintainer: Paul Mundt <paul.mundt@nokia.com> */
#if (L4_34XX_BASE==0x48000000)
#warning **************fgdfgd
#endif
        .phys_io        = 0x08600000,
//	.phys_io	= L4_34XX_BASE,
	.io_pg_offst	= ((0xd8600000) >> 18) & 0xfffc,
	.boot_params	= 0x80000100,
	.map_io		= omap_generic_map_io,
	.init_irq	= omap_generic_init_irq,
	.init_machine	= omap_generic_init,
	.timer		= &timer16,
MACHINE_END
