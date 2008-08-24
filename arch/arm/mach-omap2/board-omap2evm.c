/*
 * linux/arch/arm/mach-omap2/board-omap2evm.c
 *
 * Copyright (C) 2008 Mistral Solutions Pvt Ltd
 *
 * Modified from mach-omap2/board-generic.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/input.h>
#include <linux/i2c/twl4030.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand.h>

#include <mach/hardware.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/flash.h>

#include <mach/gpio.h>
#include <mach/board.h>
#include <mach/common.h>
#include <mach/hsmmc.h>
#include <mach/keypad.h>
#include <mach/gpmc.h>
#include <mach/nand.h>

#define GPMC_OFF_CONFIG1_0 0x60

static struct mtd_partition omap2evm_nand_partitions[] = {
	{
		.name		= "X-Loader",
		.offset		= 0,
		.size		= 1 * (64 * 2048),
		.mask_flags	= MTD_WRITEABLE,	/* force read-only */
	},
	{
		.name		= "U-Boot",
		.offset		= MTDPART_OFS_APPEND,
		.size		= 3 * (64 * 2048),
		.mask_flags	= MTD_WRITEABLE,	/* force read-only */
	},
	{
		.name		= "U-Boot Environment",
		.offset		= MTDPART_OFS_APPEND,
		.size		= 1 * (64 * 2048),
	 },
	{
		.name		= "Kernel",
		.offset		= MTDPART_OFS_APPEND,
		.size		= 16 * (64 * 2048),	/* 2MB */
	},
	{
		.name		= "Ramdisk",
		.offset		= MTDPART_OFS_APPEND,
		.size		= 32 * (64 * 2048),	/* 4MB */
	},
	{
		.name		= "Filesystem",
		.offset		= MTDPART_OFS_APPEND,
		.size		= MTDPART_SIZ_FULL,
	}
};

static struct omap_nand_platform_data omap2evm_nand_data = {
	.parts		= omap2evm_nand_partitions,
	.nr_parts	= ARRAY_SIZE(omap2evm_nand_partitions),
	.dma_channel	= -1,	/* disable DMA in OMAP NAND driver */
};

static struct resource omap2evm_nand_resource = {
	.flags		= IORESOURCE_MEM,
};

static struct platform_device omap2evm_nand_device = {
	.name		= "omap2-nand",
	.id		= -1,
	.dev		= {
		.platform_data	= &omap2evm_nand_data,
	},
	.num_resources	= 1,
	.resource	= &omap2evm_nand_resource,
};

void __init omap2evm_flash_init(void)
{
	void __iomem *gpmc_base_add, *gpmc_cs_base_add;
	unsigned char cs = 0;

	gpmc_base_add = (__force void __iomem *)OMAP243X_GPMC_VIRT;
	while (cs < GPMC_CS_NUM) {
		int ret = 0;

		/* Each GPMC set for a single CS is at offset 0x30 */
		gpmc_cs_base_add = (gpmc_base_add + GPMC_OFF_CONFIG1_0 +
				    (cs * 0x30));

		/* xloader/Uboot would have programmed the NAND
		 * base address for us This is a ugly hack. The proper
		 * way of doing this is to pass the setup of u-boot up
		 * to kernel using kernel params - something on the
		 * lines of machineID. Check if Nand is
		 * configured */
		ret = __raw_readl(gpmc_cs_base_add + GPMC_CS_CONFIG1);
		if ((ret & 0xC00) == (0x800)) {
			/* Found it!! */
			printk(KERN_INFO "NAND: Found NAND on CS %d \n", cs);
			break;
		}
		cs++;
	}
	if (cs >= GPMC_CS_NUM) {
		printk(KERN_INFO "MTD: Unable to find MTD configuration in "
				 "GPMC   - not registering.\n");
		return;
	}

	omap2evm_nand_data.cs			= cs;
	omap2evm_nand_data.gpmc_cs_baseaddr	= gpmc_cs_base_add;
	omap2evm_nand_data.gpmc_baseaddr	= gpmc_base_add;

	if (platform_device_register(&omap2evm_nand_device) < 0) {
		printk(KERN_ERR "Unable to register NAND device\n");
		return;
	}
}

static struct resource omap2evm_smc911x_resources[] = {
	[0] =   {
		.start  = OMAP2EVM_ETHR_START,
		.end    = (OMAP2EVM_ETHR_START + OMAP2EVM_ETHR_SIZE - 1),
		.flags  = IORESOURCE_MEM,
	},
	[1] =   {
		.start  = OMAP_GPIO_IRQ(OMAP2EVM_ETHR_GPIO_IRQ),
		.end    = OMAP_GPIO_IRQ(OMAP2EVM_ETHR_GPIO_IRQ),
		.flags  = IORESOURCE_IRQ,
	},
};

static struct platform_device omap2evm_smc911x_device = {
	.name       = "smc911x",
	.id     = -1,
	.num_resources  = ARRAY_SIZE(omap2evm_smc911x_resources),
	.resource   = &omap2evm_smc911x_resources [0],
};

static inline void __init omap2evm_init_smc911x(void)
{
	int gpio = OMAP2EVM_ETHR_GPIO_IRQ;
	int ret;

	ret = gpio_request(gpio, "smc911x IRQ");
	if (ret < 0) {
		printk(KERN_ERR "Failed to request GPIO %d for smc911x IRQ\n",
				gpio);
		return;
	}
	gpio_direction_input(gpio);

}

static struct platform_device omap2_evm_lcd_device = {
	.name		= "omap2evm_lcd",
	.id		= -1,
};

static struct omap_lcd_config omap2_evm_lcd_config __initdata = {
	.ctrl_name	= "internal",
};

static int omap2evm_keymap[] = {
	KEY(0, 0, KEY_LEFT),
	KEY(0, 1, KEY_RIGHT),
	KEY(0, 2, KEY_A),
	KEY(0, 3, KEY_B),
	KEY(1, 0, KEY_DOWN),
	KEY(1, 1, KEY_UP),
	KEY(1, 2, KEY_E),
	KEY(1, 3, KEY_F),
	KEY(2, 0, KEY_ENTER),
	KEY(2, 1, KEY_I),
	KEY(2, 2, KEY_J),
	KEY(2, 3, KEY_K),
	KEY(3, 0, KEY_M),
	KEY(3, 1, KEY_N),
	KEY(3, 2, KEY_O),
	KEY(3, 3, KEY_P)
};

static struct omap_kp_platform_data omap2evm_kp_data = {
	.rows		= 4,
	.cols		= 4,
	.keymap 	= omap2evm_keymap,
	.keymapsize	= ARRAY_SIZE(omap2evm_keymap),
	.rep		= 1,
	.irq		= TWL4030_MODIRQ_KEYPAD,
};

static struct platform_device omap2evm_kp_device = {
	.name		= "omap_twl4030keypad",
	.id		= -1,
	.dev		= {
				.platform_data = &omap2evm_kp_data,
			},
};

static void __init omap2_evm_init_irq(void)
{
	omap2_init_common_hw(NULL);
	omap_init_irq();
	omap_gpio_init();
	omap2evm_init_smc911x();
}

static struct omap_uart_config omap2_evm_uart_config __initdata = {
	.enabled_uarts	= ((1 << 0) | (1 << 1) | (1 << 2)),
};

static struct omap_mmc_config omap2_evm_mmc_config __initdata = {
	.mmc [0] = {
		.enabled        = 1,
		.wire4          = 1,
	},
};

static struct omap_board_config_kernel omap2_evm_config[] __initdata = {
	{ OMAP_TAG_UART,	&omap2_evm_uart_config },
	{ OMAP_TAG_LCD,		&omap2_evm_lcd_config },
	{ OMAP_TAG_MMC,		&omap2_evm_mmc_config },
};

static int __init omap2_evm_i2c_init(void)
{
	/*
	 * Registering bus 2 first to avoid twl4030 misbehaving as OMAP2EVM
	 * has twl4030 on bus 2
	 */
	omap_register_i2c_bus(2, 2600, NULL, 0);
	omap_register_i2c_bus(1, 400, NULL, 0);
	return 0;
}

static struct platform_device *omap2_evm_devices[] __initdata = {
	&omap2_evm_lcd_device,
	&omap2evm_smc911x_device,
	&omap2evm_kp_device,
};

static void __init omap2_evm_init(void)
{
	platform_add_devices(omap2_evm_devices, ARRAY_SIZE(omap2_evm_devices));
	omap_board_config = omap2_evm_config;
	omap_board_config_size = ARRAY_SIZE(omap2_evm_config);
	omap_serial_init();
	hsmmc_init();
	omap2evm_flash_init();
}

static void __init omap2_evm_map_io(void)
{
	omap2_set_globals_243x();
	omap2_map_common_io();
}

arch_initcall(omap2_evm_i2c_init);

MACHINE_START(OMAP2EVM, "OMAP2EVM Board")
	/* Maintainer:  Arun KS <arunks@mistralsolutions.com> */
	.phys_io	= 0x48000000,
	.io_pg_offst	= ((0xd8000000) >> 18) & 0xfffc,
	.boot_params	= 0x80000100,
	.map_io		= omap2_evm_map_io,
	.init_irq	= omap2_evm_init_irq,
	.init_machine	= omap2_evm_init,
	.timer		= &omap_timer,
MACHINE_END
