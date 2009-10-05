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
#include <linux/i2c/twl4030.h>
#include <linux/regulator/machine.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/synaptics_i2c_rmi.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include <mach/board-zoom.h>
#include <mach/common.h>
#include <mach/usb.h>
#include <mach/keypad.h>
#include <mach/mux.h>
#include <mach/gpio.h>

#include <mach/mcspi.h>
#include <linux/spi/spi.h>
#include <mach/display.h>

#include "mmc-twl4030.h"
#include "omap3-opp.h"
#include "sdram-micron-mt46h32m32lf-6.h"

#define ZOOM2_QUART_PHYS        0x10000000
#define ZOOM2_QUART_VIRT        0xFB000000
#define ZOOM2_QUART_SIZE        SZ_1M

#include <media/v4l2-int-device.h>

#if defined(CONFIG_VIDEO_IMX046) || defined(CONFIG_VIDEO_IMX046_MODULE)
#include <media/imx046.h>
extern struct imx046_platform_data zoom2_imx046_platform_data;
#endif

extern void zoom2_cam_init(void);

#ifdef CONFIG_PM
#include <../drivers/media/video/omap/omap_voutdef.h>
#endif

#ifdef CONFIG_VIDEO_LV8093
#include <media/lv8093.h>
extern struct imx046_platform_data zoom2_lv8093_platform_data;
#endif

#define OMAP_SYNAPTICS_GPIO		163


#define LCD_PANEL_BACKLIGHT_GPIO        (15 + OMAP_MAX_GPIO_LINES)
#define LCD_PANEL_ENABLE_GPIO           (7 + OMAP_MAX_GPIO_LINES)

#define LCD_PANEL_RESET_GPIO            55
#define LCD_PANEL_QVGA_GPIO             56

#define TV_PANEL_ENABLE_GPIO            95


#define ENABLE_VAUX2_DEDICATED          0x09
#define ENABLE_VAUX2_DEV_GRP            0x20
#define ENABLE_VAUX3_DEDICATED          0x03
#define ENABLE_VAUX3_DEV_GRP            0x20

#define ENABLE_VPLL2_DEDICATED          0x05
#define ENABLE_VPLL2_DEV_GRP            0xE0
#define TWL4030_VPLL2_DEV_GRP           0x33
#define TWL4030_VPLL2_DEDICATED         0x36

/*#define SIL9022_RESET_GPIO              97*/


static void zoom2_lcd_tv_panel_init(void)
{
	unsigned char lcd_panel_reset_gpio;

	omap_cfg_reg(AF21_34XX_GPIO8);
	omap_cfg_reg(B23_34XX_GPIO167);
	omap_cfg_reg(AB1_34XX_McSPI1_CS2);
	omap_cfg_reg(A24_34XX_GPIO94);

	if (omap_rev() > OMAP3430_REV_ES3_0) {
		/* Production Zoom2 Board:
		 * GPIO-96 is the LCD_RESET_GPIO
		 */
		omap_cfg_reg(C25_34XX_GPIO96);
		lcd_panel_reset_gpio = 96;
	} else {
		/* Pilot Zoom2 board
		 * GPIO-55 is the LCD_RESET_GPIO
		 */
		omap_cfg_reg(T8_34XX_GPIO55);
		lcd_panel_reset_gpio = 55;
	}

	gpio_request(lcd_panel_reset_gpio, "lcd reset");
	gpio_request(LCD_PANEL_QVGA_GPIO, "lcd qvga");
	gpio_request(LCD_PANEL_ENABLE_GPIO, "lcd panel");
	gpio_request(LCD_PANEL_BACKLIGHT_GPIO, "lcd backlight");

	gpio_request(TV_PANEL_ENABLE_GPIO, "tv panel");

	gpio_direction_output(LCD_PANEL_QVGA_GPIO, 0);
	gpio_direction_output(lcd_panel_reset_gpio, 0);
	gpio_direction_output(LCD_PANEL_ENABLE_GPIO, 0);
	gpio_direction_output(LCD_PANEL_BACKLIGHT_GPIO, 0);
	gpio_direction_output(TV_PANEL_ENABLE_GPIO, 0);

	gpio_direction_output(LCD_PANEL_QVGA_GPIO, 1);
	gpio_direction_output(lcd_panel_reset_gpio, 1);
}

static int zoom2_panel_power_enable(int enable)
{
	twl4030_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
				(enable) ? ENABLE_VPLL2_DEDICATED : 0,
				TWL4030_VPLL2_DEDICATED);
	twl4030_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,
				(enable) ? ENABLE_VPLL2_DEV_GRP : 0,
				TWL4030_VPLL2_DEV_GRP);
	return 0;
}

static int zoom2_panel_enable_lcd(struct omap_dss_device *dssdev)
{
	zoom2_panel_power_enable(1);

	gpio_request(LCD_PANEL_ENABLE_GPIO, "lcd panel");
	gpio_direction_output(LCD_PANEL_ENABLE_GPIO, 1);
	gpio_request(LCD_PANEL_BACKLIGHT_GPIO, "lcd backlight");
	/* conflict with headset select gpio */
	/* gpio_direction_output(LCD_PANEL_BACKLIGHT_GPIO, 1); */

	return 0;
}

static void zoom2_panel_disable_lcd(struct omap_dss_device *dssdev)
{
	zoom2_panel_power_enable(0);

	gpio_request(LCD_PANEL_ENABLE_GPIO, "lcd panel");
	gpio_direction_output(LCD_PANEL_ENABLE_GPIO, 0);
	gpio_request(LCD_PANEL_BACKLIGHT_GPIO, "lcd backlight");
	gpio_direction_output(LCD_PANEL_BACKLIGHT_GPIO, 0);

}

static struct omap_dss_device zoom2_lcd_device = {
	.name = "lcd",
	.driver_name = "NEC_panel",
	.type = OMAP_DISPLAY_TYPE_DPI,
	.phy.dpi.data_lines = 24,
	.platform_enable = zoom2_panel_enable_lcd,
	.platform_disable = zoom2_panel_disable_lcd,
 };

static struct omap_dss_device *zoom2_dss_devices[] = {
	&zoom2_lcd_device,
/*        &zoom2_tv_device,
#ifdef CONFIG_SIL9022
	&zoom2_hdmi_device,
#endif
*/
};



static struct omap_dss_board_info zoom2_dss_data = {
	.num_devices = ARRAY_SIZE(zoom2_dss_devices),
	.devices = zoom2_dss_devices,
	.default_device = &zoom2_lcd_device,
};

static struct platform_device zoom2_dss_device = {
	.name          = "omapdss",
	.id            = -1,
	.dev            = {
		.platform_data = &zoom2_dss_data,
	},
};

static struct regulator_consumer_supply zoom2_vdda_dac_supply = {
	.supply         = "vdda_dac",
	.dev            = &zoom2_dss_device.dev,
};


#ifdef CONFIG_FB_OMAP2
static struct resource zoom2_vout_resource[3 - CONFIG_FB_OMAP2_NUM_FBS] = {
};
#else
static struct resource zoom2_vout_resource[2] = {
};
#endif


static struct omap2_mcspi_device_config zoom2_lcd_mcspi_config = {
	.turbo_mode             = 0,
	.single_channel         = 1,  /* 0: slave, 1: master */
};

static struct spi_board_info zoom2_spi_board_info[] __initdata = {
	[0] = {
		.modalias               = "zoom2_disp_spi",
		.bus_num                = 1,
		.chip_select            = 2,
		.max_speed_hz           = 375000,
		.controller_data        = &zoom2_lcd_mcspi_config,
	},
};


/* Zoom2 has Qwerty keyboard*/
static int zoom2_twl4030_keymap[] = {
	KEY(0, 0, KEY_E),
	KEY(1, 0, KEY_R),
	KEY(2, 0, KEY_T),
	KEY(3, 0, KEY_HOME),
	KEY(6, 0, KEY_I),
	KEY(7, 0, KEY_LEFTSHIFT),
	KEY(0, 1, KEY_D),
	KEY(1, 1, KEY_F),
	KEY(2, 1, KEY_G),
	KEY(3, 1, KEY_SEND),
	KEY(6, 1, KEY_K),
	KEY(7, 1, KEY_ENTER),
	KEY(0, 2, KEY_X),
	KEY(1, 2, KEY_C),
	KEY(2, 2, KEY_V),
	KEY(3, 2, KEY_END),
	KEY(6, 2, KEY_DOT),
	KEY(7, 2, KEY_CAPSLOCK),
	KEY(0, 3, KEY_Z),
	KEY(1, 3, KEY_KPPLUS),
	KEY(2, 3, KEY_B),
	KEY(3, 3, KEY_F1),
	KEY(6, 3, KEY_O),
	KEY(7, 3, KEY_SPACE),
	KEY(0, 4, KEY_W),
	KEY(1, 4, KEY_Y),
	KEY(2, 4, KEY_U),
	KEY(3, 4, KEY_F2),
	KEY(4, 4, KEY_VOLUMEUP),
	KEY(6, 4, KEY_L),
	KEY(7, 4, KEY_LEFT),
	KEY(0, 5, KEY_S),
	KEY(1, 5, KEY_H),
	KEY(2, 5, KEY_J),
	KEY(3, 5, KEY_F3),
	KEY(5, 5, KEY_VOLUMEDOWN),
	KEY(6, 5, KEY_M),
	KEY(4, 5, KEY_ENTER),
	KEY(7, 5, KEY_RIGHT),
	KEY(0, 6, KEY_Q),
	KEY(1, 6, KEY_A),
	KEY(2, 6, KEY_N),
	KEY(3, 6, KEY_BACKSPACE),
	KEY(6, 6, KEY_P),
	KEY(7, 6, KEY_UP),
	KEY(6, 7, KEY_SELECT),
	KEY(7, 7, KEY_DOWN),
	KEY(0, 7, KEY_PROG1),	/*MACRO 1 <User defined> */
	KEY(1, 7, KEY_PROG2),	/*MACRO 2 <User defined> */
	KEY(2, 7, KEY_PROG3),	/*MACRO 3 <User defined> */
	KEY(3, 7, KEY_PROG4),	/*MACRO 4 <User defined> */
	0
};

static struct twl4030_keypad_data zoom2_kp_twl4030_data = {
	.rows		= 8,
	.cols		= 8,
	.keymap		= zoom2_twl4030_keymap,
	.keymapsize	= ARRAY_SIZE(zoom2_twl4030_keymap),
	.rep		= 1,
};

static struct omap_board_config_kernel zoom2_config[] __initdata = {
};

static struct platform_device zoom2_cam_device = {
	.name		= "zoom2_cam",
	.id		= -1,
};

static struct regulator_consumer_supply zoom2_vaux2_supplies[] = {
	{
		.supply		= "vaux2_1",
		.dev		= &zoom2_cam_device.dev,
	},
};

static struct regulator_consumer_supply zoom2_vaux4_supplies[] = {
	{
		.supply		= "vaux4_1",
		.dev		= &zoom2_cam_device.dev,
	},
};

static struct regulator_consumer_supply zoom2_vmmc1_supply = {
	.supply		= "vmmc",
};

static struct regulator_consumer_supply zoom2_vsim_supply = {
	.supply		= "vmmc_aux",
};

static struct regulator_consumer_supply zoom2_vmmc2_supply = {
	.supply		= "vmmc",
};

/* VAUX2 for camera module */
static struct regulator_init_data zoom2_vaux2 = {
	.constraints = {
		.min_uV			= 2800000,
		.max_uV			= 2800000,
		.apply_uV		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(zoom2_vaux2_supplies),
	.consumer_supplies	= zoom2_vaux2_supplies,
};

/* VAUX4 for OMAP VDD_CSI2 (camera) */
static struct regulator_init_data zoom2_vaux4 = {
	.constraints = {
		.min_uV			= 1800000,
		.max_uV			= 1800000,
		.apply_uV		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(zoom2_vaux4_supplies),
	.consumer_supplies	= zoom2_vaux4_supplies,
};

/* VMMC1 for OMAP VDD_MMC1 (i/o) and MMC1 card */
static struct regulator_init_data zoom2_vmmc1 = {
	.constraints = {
		.min_uV			= 1850000,
		.max_uV			= 3150000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &zoom2_vmmc1_supply,
};

/* VMMC2 for MMC2 card */
static struct regulator_init_data zoom2_vmmc2 = {
	.constraints = {
		.min_uV			= 1850000,
		.max_uV			= 1850000,
		.apply_uV		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &zoom2_vmmc2_supply,
};

/* VSIM for OMAP VDD_MMC1A (i/o for DAT4..DAT7) */
static struct regulator_init_data zoom2_vsim = {
	.constraints = {
		.min_uV			= 1800000,
		.max_uV			= 3000000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &zoom2_vsim_supply,
};

static struct regulator_init_data zoom2_vdac = {
	.constraints = {
		.min_uV                 = 1800000,
		.max_uV                 = 1800000,
		.valid_modes_mask       = REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask         = REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &zoom2_vdda_dac_supply,
};


static struct twl4030_hsmmc_info mmc[] __initdata = {
	{
		.mmc		= 1,
		.wires		= 4,
		.gpio_wp	= -EINVAL,
		.power_saving	= true,
	},
	{
		.mmc		= 2,
		.wires		= 4,
		.gpio_wp	= -EINVAL,
		.power_saving	= true,
	},
	{}      /* Terminator */
};

static int zoom2_twl_gpio_setup(struct device *dev,
		unsigned gpio, unsigned ngpio)
{
	/* gpio + 0 is "mmc0_cd" (input/IRQ),
	 * gpio + 1 is "mmc1_cd" (input/IRQ)
	 */
	mmc[0].gpio_cd = gpio + 0;
	mmc[1].gpio_cd = gpio + 1;
	twl4030_mmc_init(mmc);

	/* link regulators to MMC adapters ... we "know" the
	 * regulators will be set up only *after* we return.
	*/
	zoom2_vmmc1_supply.dev = mmc[0].dev;
	zoom2_vsim_supply.dev = mmc[0].dev;
	zoom2_vmmc2_supply.dev = mmc[1].dev;

	return 0;
}


static int zoom2_batt_table[] = {
/* 0 C*/
30800, 29500, 28300, 27100,
26000, 24900, 23900, 22900, 22000, 21100, 20300, 19400, 18700, 17900,
17200, 16500, 15900, 15300, 14700, 14100, 13600, 13100, 12600, 12100,
11600, 11200, 10800, 10400, 10000, 9630,  9280,  8950,  8620,  8310,
8020,  7730,  7460,  7200,  6950,  6710,  6470,  6250,  6040,  5830,
5640,  5450,  5260,  5090,  4920,  4760,  4600,  4450,  4310,  4170,
4040,  3910,  3790,  3670,  3550
};

static struct twl4030_bci_platform_data zoom2_bci_data = {
	.battery_tmp_tbl	= zoom2_batt_table,
	.tblsize		= ARRAY_SIZE(zoom2_batt_table),
};

static struct twl4030_usb_data zoom2_usb_data = {
	.usb_mode	= T2_USB_MODE_ULPI,
};

static void __init omap_zoom2_init_irq(void)
{
	omap_board_config = zoom2_config;
	omap_board_config_size = ARRAY_SIZE(zoom2_config);
	omap2_init_common_hw(mt46h32m32lf6_sdrc_params, NULL,
		omap3_mpu_rate_table, omap3_dsp_rate_table,
					omap3_l3_rate_table);
	omap_init_irq();
	omap_gpio_init();
}

static struct twl4030_gpio_platform_data zoom2_gpio_data = {
	.gpio_base	= OMAP_MAX_GPIO_LINES,
	.irq_base	= TWL4030_GPIO_IRQ_BASE,
	.irq_end	= TWL4030_GPIO_IRQ_END,
	.setup		= zoom2_twl_gpio_setup,
};

static struct twl4030_madc_platform_data zoom2_madc_data = {
	.irq_line	= 1,
};

static struct twl4030_ins __initdata sleep_on_seq[] = {
	/* Broadcast message to put res to sleep */
	{MSG_BROADCAST(DEV_GRP_NULL, RES_GRP_ALL, RES_TYPE_R1, RES_TYPE2_R0,
							RES_STATE_SLEEP), 2},
};

static struct twl4030_script sleep_on_script __initdata = {
	.script	= sleep_on_seq,
	.size	= ARRAY_SIZE(sleep_on_seq),
	.flags	= TRITON_SLEEP_SCRIPT,
};

static struct twl4030_ins wakeup_seq[] __initdata = {
	/* Broadcast message to put res to active */
	{MSG_BROADCAST(DEV_GRP_NULL, RES_GRP_ALL, RES_TYPE_R1, RES_TYPE2_R0,
			RES_STATE_ACTIVE), 2},
};

static struct twl4030_script wakeup_script __initdata = {
	.script = wakeup_seq,
	.size   = ARRAY_SIZE(wakeup_seq),
	.flags  = TRITON_WAKEUP_SCRIPT,
};

static struct twl4030_ins wrst_seq[] __initdata = {
/*
 * Reset twl4030.
 * Reset VDD1 regulator.
 * Reset VDD2 regulator.
 * Reset VPLL1 regulator.
 * Enable sysclk output.
 * Reenable twl4030.
 */
	{MSG_SINGULAR(DEV_GRP_NULL, 0x1b, RES_STATE_OFF), 2},
	{MSG_SINGULAR(DEV_GRP_P1, 0xf, RES_STATE_WRST), 15},
	{MSG_SINGULAR(DEV_GRP_P1, 0x10, RES_STATE_WRST), 15},
	{MSG_SINGULAR(DEV_GRP_P1, 0x7, RES_STATE_WRST), 0x60},
	{MSG_SINGULAR(DEV_GRP_P1, 0x19, RES_STATE_ACTIVE), 2},
	{MSG_SINGULAR(DEV_GRP_NULL, 0x1b, RES_STATE_ACTIVE), 2},
};

static struct twl4030_script wrst_script __initdata = {
	.script = wrst_seq,
	.size   = ARRAY_SIZE(wrst_seq),
	.flags  = TRITON_WRST_SCRIPT,
};

static struct twl4030_script *twl4030_scripts[] __initdata = {
	&sleep_on_script,
	&wakeup_script,
	&wrst_script,
};

static struct twl4030_resconfig twl4030_rconfig[] = {
	{ .resource = RES_HFCLKOUT, .devgroup = DEV_GRP_P3, .type = 1,
		.type2 = -1, .remap_sleep = -1 },
	{ .resource = RES_VDD1, .devgroup = DEV_GRP_P1, .type = 1,
		.type2 = -1, .remap_sleep = RES_STATE_OFF },
	{ .resource = RES_VDD2, .devgroup = DEV_GRP_P1, .type = 1,
		.type2 = -1, .remap_sleep = RES_STATE_OFF },
	{ .resource = RES_VPLL1, .devgroup = DEV_GRP_P1, .type = 1,
		.type2 = -1, .remap_sleep = RES_STATE_OFF },
	{ .resource = RES_VINTANA2, .devgroup = DEV_GRP_ALL, .type = 1,
		.type2 = -1, .remap_sleep = -1 },
	{ .resource = RES_VIO, .devgroup = DEV_GRP_ALL, .type = 1,
		.type2 = -1, .remap_sleep = -1 },
	{ .resource = RES_REGEN, .devgroup = DEV_GRP_ALL, .type = 1,
		.type2 = -1, .remap_sleep = -1 },
	{ .resource = RES_NRES_PWRON, .devgroup = DEV_GRP_ALL, .type = 1,
		.type2 = -1, .remap_sleep = -1 },
	{ .resource = RES_CLKEN, .devgroup = DEV_GRP_ALL, .type = 1,
		.type2 = -1, .remap_sleep = -1 },
	{ .resource = RES_SYSEN, .devgroup = DEV_GRP_ALL, .type = 1,
		.type2 = -1, .remap_sleep = -1 },
	{ .resource = RES_32KCLKOUT, .devgroup = DEV_GRP_ALL, .type = 1,
		.type2 = -1, .remap_sleep = -1 },
	{ .resource = RES_RESET, .devgroup = DEV_GRP_ALL, .type = 1,
		.type2 = -1, .remap_sleep = -1 },
	{ .resource = RES_Main_Ref, .devgroup = DEV_GRP_ALL, .type = 1,
		.type2 = -1, .remap_sleep = -1 },
	{ 0, 0 },
};

static struct twl4030_power_data zoom2_t2scripts_data __initdata = {
	.scripts	= twl4030_scripts,
	.size		= ARRAY_SIZE(twl4030_scripts),
	.resource_config = twl4030_rconfig,
};

static struct twl4030_platform_data zoom2_twldata = {
	.irq_base	= TWL4030_IRQ_BASE,
	.irq_end	= TWL4030_IRQ_END,

	/* platform_data for children goes here */
	.bci		= &zoom2_bci_data,
	.madc		= &zoom2_madc_data,
	.usb		= &zoom2_usb_data,
	.gpio		= &zoom2_gpio_data,
	.keypad		= &zoom2_kp_twl4030_data,
	.power		= &zoom2_t2scripts_data,
	.vaux2		= &zoom2_vaux2,
	.vaux4		= &zoom2_vaux4,
	.vmmc1          = &zoom2_vmmc1,
	.vmmc2          = &zoom2_vmmc2,
	.vsim           = &zoom2_vsim,
	.vdac           = &zoom2_vdac,

};

static struct i2c_board_info __initdata zoom2_i2c_boardinfo[] = {
	{
		I2C_BOARD_INFO("twl4030", 0x48),
		.flags		= I2C_CLIENT_WAKE,
		.irq		= INT_34XX_SYS_NIRQ,
		.platform_data	= &zoom2_twldata,
	},
};

static void synaptics_dev_init(void)
{
	/* Set the ts_gpio pin mux */
	omap_cfg_reg(H18_34XX_GPIO163);

	if (gpio_request(OMAP_SYNAPTICS_GPIO, "touch") < 0) {
		printk(KERN_ERR "can't get synaptics pen down GPIO\n");
		return;
	}
	gpio_direction_input(OMAP_SYNAPTICS_GPIO);
	omap_set_gpio_debounce(OMAP_SYNAPTICS_GPIO, 1);
	omap_set_gpio_debounce_time(OMAP_SYNAPTICS_GPIO, 0xa);
}

static int synaptics_power(int power_state)
{
	/* TODO: synaptics is powered by vbatt */
	return 0;
}

static struct synaptics_i2c_rmi_platform_data synaptics_platform_data[] = {
	{
		.version	= 0x0,
		.power		= &synaptics_power,
		.flags		= SYNAPTICS_SWAP_XY,
		.irqflags	= IRQF_TRIGGER_LOW,
	}
};

static struct i2c_board_info __initdata zoom2_i2c_boardinfo2[] = {
	{
		I2C_BOARD_INFO(SYNAPTICS_I2C_RMI_NAME,  0x20),
		.platform_data = &synaptics_platform_data,
		.irq = OMAP_GPIO_IRQ(OMAP_SYNAPTICS_GPIO),
	},
#if defined(CONFIG_VIDEO_IMX046) || defined(CONFIG_VIDEO_IMX046_MODULE)
	{
		I2C_BOARD_INFO("imx046", IMX046_I2C_ADDR),
		.platform_data = &zoom2_imx046_platform_data,
	},
#endif
#ifdef CONFIG_VIDEO_LV8093
	{
		I2C_BOARD_INFO(LV8093_NAME,  LV8093_AF_I2C_ADDR),
		.platform_data = &zoom2_lv8093_platform_data,
	},
#endif
};

static int __init omap_i2c_init(void)
{
	omap_register_i2c_bus(1, 2600, zoom2_i2c_boardinfo,
			ARRAY_SIZE(zoom2_i2c_boardinfo));
	omap_register_i2c_bus(2, 100, zoom2_i2c_boardinfo2,
			ARRAY_SIZE(zoom2_i2c_boardinfo2));
	omap_register_i2c_bus(3, 400, NULL, 0);
	return 0;
}

extern int __init omap_zoom2_debugboard_init(void);


#ifdef CONFIG_PM
struct vout_platform_data zoom2_vout_data = {
	.set_min_bus_tput = omap_pm_set_min_bus_tput,
	.set_max_mpu_wakeup_lat =  omap_pm_set_max_mpu_wakeup_lat,
	.set_cpu_freq = omap_pm_cpu_set_freq,
};
#endif


static struct platform_device zoom2_vout_device = {
	.name           = "omap_vout",
	.num_resources  = ARRAY_SIZE(zoom2_vout_resource),
	.resource       = &zoom2_vout_resource[0],
	.id             = -1,

#ifdef CONFIG_PM
	.dev            = {
		.platform_data = &zoom2_vout_data,
	}
#else

	.dev            = {
		.platform_data = NULL,
	}
#endif
};


static struct platform_device *zoom2_devices[] __initdata = {
	&zoom2_cam_device,
	&zoom2_dss_device,
	/*&zoom2_vout_device,*/
};

static void enable_board_wakeup_source(void)
{
	omap_cfg_reg(AF26_34XX_SYS_NIRQ);
}

static void __init omap_zoom2_init(void)
{
	omap_i2c_init();
	platform_add_devices(zoom2_devices, ARRAY_SIZE(zoom2_devices));
	synaptics_dev_init();
	spi_register_board_info(zoom2_spi_board_info,
				ARRAY_SIZE(zoom2_spi_board_info));
	omap_serial_init();
	omap_zoom2_debugboard_init();
	usb_musb_init();
	zoom2_cam_init();
	zoom2_lcd_tv_panel_init();
	enable_board_wakeup_source();
	zoom_flash_init();
}

static struct map_desc zoom2_io_desc[] __initdata = {
	{
		.virtual	= ZOOM2_QUART_VIRT,
		.pfn		= __phys_to_pfn(ZOOM2_QUART_PHYS),
		.length		= ZOOM2_QUART_SIZE,
		.type		= MT_DEVICE
	},
};

static void __init omap_zoom2_map_io(void)
{
	omap2_set_globals_343x();
	iotable_init(zoom2_io_desc, ARRAY_SIZE(zoom2_io_desc));
	omap2_map_common_io();
}

MACHINE_START(OMAP_ZOOM2, "OMAP Zoom2 board")
	/* phys_io is only used for DEBUG_LL early printing.  The Zoom2's
	 * console is on an external quad UART sitting at address 0x10000000
	 */
	.phys_io	= 0x10000000,
	.io_pg_offst	= ((0xfb000000) >> 18) & 0xfffc,
	.boot_params	= 0x80000100,
	.map_io		= omap_zoom2_map_io,
	.init_irq	= omap_zoom2_init_irq,
	.init_machine	= omap_zoom2_init,
	.timer		= &omap_timer,
MACHINE_END
