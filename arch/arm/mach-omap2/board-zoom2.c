/*
 * Copyright (C) 2009 Texas Instruments Inc.
 * Mikkel Christensen <mlc@ti.com>
 *
 * Modified from mach-omap2/board-ldp.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/input/matrix_keypad.h>
#include <linux/gpio.h>
#include <linux/i2c/twl4030.h>
#include <linux/regulator/machine.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include <plat/common.h>
#include <plat/usb.h>
#include <plat/mux.h>

#include "mmc-twl4030.h"
#include "sdram-micron-mt46h32m32lf-6.h"
#include "omap3-opp.h"

static void __init omap_zoom_map_io(void)
{
	omap2_set_globals_343x();
	omap2_map_common_io();
}

static struct omap_board_config_kernel zoom_config[] __initdata = {
};

static void __init omap_zoom_init_irq(void)
{

	omap_board_config = zoom_config;
	omap_board_config_size = ARRAY_SIZE(zoom_config);

	omap2_init_common_hw(mt46h32m32lf6_sdrc_params,
				mt46h32m32lf6_sdrc_params, omap3_mpu_rate_table,
				omap3_dsp_rate_table, omap3_l3_rate_table);
	omap_init_irq();
	omap_gpio_init();
}

extern int __init omap_zoom2_debugboard_init(void);
extern void __init zoom_peripherals_init(void);

static void __init omap_zoom_init(void)
{
	zoom_peripherals_init();
	omap_zoom2_debugboard_init();
}

MACHINE_START(OMAP_ZOOM2, "OMAP Zoom2 board")
	.phys_io        = 0x48000000,
	.io_pg_offst    = ((0xfa000000) >> 18) & 0xfffc,
	.boot_params    = 0x80000100,
	.map_io         = omap_zoom_map_io,
	.init_irq       = omap_zoom_init_irq,
	.init_machine   = omap_zoom_init,
	.timer          = &omap_timer,
MACHINE_END
