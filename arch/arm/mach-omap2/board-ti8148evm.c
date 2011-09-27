/*
 * Code for TI8148 EVM.
 *
 * Copyright (C) 2010 Texas Instruments, Inc. - http://www.ti.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/init.h>

#include <mach/hardware.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include <plat/irqs.h>
#include <plat/board.h>
#include <plat/common.h>
#include <plat/usb.h>

static struct omap_musb_board_data musb_board_data = {
	.interface_type         = MUSB_INTERFACE_ULPI,
	.mode           = MUSB_OTG,
	.power                  = 500,
	.instances              = 1,
};

static struct omap_board_config_kernel ti8148_evm_config[] __initdata = {
};

static void __init ti8148_init_early(void)
{
	omap2_init_common_infrastructure();
}

static void __init ti8148_evm_init(void)
{
	omap_serial_init();
	omap_sdrc_init(NULL, NULL);
	omap_board_config = ti8148_evm_config;
	omap_board_config_size = ARRAY_SIZE(ti8148_evm_config);
	/* initialize usb */
	usb_musb_init(&musb_board_data);
}

static void __init ti8148_evm_map_io(void)
{
	omap2_set_globals_ti81xx();
	omapti81xx_map_common_io();
}

MACHINE_START(TI8148EVM, "ti8148evm")
	/* Maintainer: Texas Instruments */
	.boot_params	= 0x80000100,
	.map_io		= ti8148_evm_map_io,
	.init_early	= ti8148_init_early,
	.init_irq	= ti81xx_init_irq,
	.timer		= &omap3_timer,
	.init_machine	= ti8148_evm_init,
MACHINE_END
