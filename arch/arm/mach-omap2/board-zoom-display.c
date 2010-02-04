/*
 * Copyright (C) 2009 Texas Instruments Inc.
 *
 * Modified from mach-omap2/board-zoom-peripherals.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/i2c/twl.h>
#include <linux/spi/spi.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/consumer.h>
#include <plat/common.h>
#include <plat/control.h>
#include <plat/mcspi.h>
#include <plat/display.h>

#define LCD_PANEL_ENABLE_GPIO		(7 + OMAP_MAX_GPIO_LINES)
#define LCD_PANEL_RESET_GPIO_PROD	96
#define LCD_PANEL_RESET_GPIO_PILOT	55
#define LCD_PANEL_QVGA_GPIO		56
#define TV_PANEL_ENABLE_GPIO		95
#define SIL9022_RESET_GPIO		97

struct zoom_dss_board_info {
	int gpio_flag;
};

static void zoom_lcd_tv_panel_init(void)
{
	int ret;
	unsigned char lcd_panel_reset_gpio;

	if (omap_rev() > OMAP3430_REV_ES3_0) {
		/* Production Zoom2 board:
		 * GPIO-96 is the LCD_RESET_GPIO
		 */
		lcd_panel_reset_gpio = LCD_PANEL_RESET_GPIO_PROD;
	} else {
		/* Pilot Zoom2 board:
		 * GPIO-55 is the LCD_RESET_GPIO
		 */
		lcd_panel_reset_gpio = LCD_PANEL_RESET_GPIO_PILOT;
	}

	ret = gpio_request(lcd_panel_reset_gpio, "lcd reset");
	if (ret) {
		pr_err("Failed to get LCD reset GPIO.\n");
		return;
	}
	gpio_direction_output(lcd_panel_reset_gpio, 1);

	ret = gpio_request(LCD_PANEL_QVGA_GPIO, "lcd qvga");
	if (ret) {
		pr_err("Failed to get LCD_PANEL_QVGA_GPIO.\n");
		goto err0;
	}
	gpio_direction_output(LCD_PANEL_QVGA_GPIO, 1);

	ret = gpio_request(TV_PANEL_ENABLE_GPIO, "tv panel");
	if (ret) {
		pr_err("Failed to get TV_PANEL_ENABLE_GPIO.\n");
		goto err1;
	}
	gpio_direction_output(TV_PANEL_ENABLE_GPIO, 0);

	ret = gpio_request(SIL9022_RESET_GPIO, "hdmi reset");
	if (ret) {
		pr_err(KERN_ERR "Failed to get SIL9022_RESET_GPIO.\n");
		goto err2;
	}
	gpio_direction_output(SIL9022_RESET_GPIO, 0);
 
	return;
err2:
	gpio_free(TV_PANEL_ENABLE_GPIO);
err1:
	gpio_free(LCD_PANEL_QVGA_GPIO);
err0:
	gpio_free(lcd_panel_reset_gpio);

}

static int zoom_panel_power_enable(int enable)
{
	int ret;
	struct regulator *vpll2_reg;

	vpll2_reg = regulator_get(NULL, "vpll2");
	if (IS_ERR(vpll2_reg)) {
		pr_err("Unable to get vpll2 regulator\n");
		return PTR_ERR(vpll2_reg);
	}

	if (enable)
		ret = regulator_enable(vpll2_reg);
	else
		ret = regulator_disable(vpll2_reg);

	return ret;
}

static int zoom_panel_enable_lcd(struct omap_dss_device *dssdev)
{
	int ret;
	struct zoom_dss_board_info *pdata;

	ret = zoom_panel_power_enable(1);
	if (ret < 0)
		return ret;
	pdata = dssdev->dev.platform_data;
	if (pdata->gpio_flag == 0) {
		ret = gpio_request(LCD_PANEL_ENABLE_GPIO, "lcd enable");
		if (ret) {
			pr_err("Failed to get LCD_PANEL_ENABLE_GPIO.\n");
			return ret;
		}
		gpio_direction_output(LCD_PANEL_ENABLE_GPIO, 1);
		pdata->gpio_flag = 1;
	} else {
		gpio_set_value(LCD_PANEL_ENABLE_GPIO, 1);
	}

	return 0;
}

static void zoom_panel_disable_lcd(struct omap_dss_device *dssdev)
{
	zoom_panel_power_enable(0);
	gpio_set_value(LCD_PANEL_ENABLE_GPIO, 0);
}

static int zoom_panel_enable_tv(struct omap_dss_device *dssdev)
{
	int ret;
	struct regulator *vdac_reg;

	vdac_reg = regulator_get(NULL, "vdda_dac");
	if (IS_ERR(vdac_reg)) {
		pr_err("Unable to get vdac regulator\n");
		return PTR_ERR(vdac_reg);
	}
	ret = regulator_enable(vdac_reg);
	if (ret < 0)
		return ret;
	gpio_set_value(TV_PANEL_ENABLE_GPIO, 0);

	return 0;
}

static void zoom_panel_disable_tv(struct omap_dss_device *dssdev)
{
	struct regulator *vdac_reg;

	vdac_reg = regulator_get(NULL, "vdda_dac");
	if (IS_ERR(vdac_reg)) {
		pr_err("Unable to get vpll2 regulator\n");
		return;
	}
	regulator_disable(vdac_reg);
	gpio_set_value(TV_PANEL_ENABLE_GPIO, 1);
}

static void zoom_hdmi_reset_enable(int level)
{
	gpio_set_value(SIL9022_RESET_GPIO, level);

}

static int zoom_panel_enable_hdmi(struct omap_dss_device *dssdev)
{
	zoom_panel_power_enable(1);
	zoom_hdmi_reset_enable(1);

	return 0;
}

static void zoom_panel_disable_hdmi(struct omap_dss_device *dssdev)
{
	zoom_hdmi_reset_enable(0);
	zoom_panel_power_enable(0);
}

static struct omap_dss_device zoom_hdmi_device = {
	.name = "hdmi",
	.driver_name = "hdmi_panel",
	.type = OMAP_DISPLAY_TYPE_HDMI,
	.phy.dpi.data_lines = 24,
	.platform_enable = zoom_panel_enable_hdmi,
	.platform_disable = zoom_panel_disable_hdmi,
};

static struct zoom_dss_board_info zoom_dss_lcd_data = {
	.gpio_flag = 0,
};

static struct omap_dss_device zoom_lcd_device = {
	.name = "lcd",
	.driver_name = "NEC_8048_panel",
	.type = OMAP_DISPLAY_TYPE_DPI,
	.phy.dpi.data_lines = 24,
	.platform_enable = zoom_panel_enable_lcd,
	.platform_disable = zoom_panel_disable_lcd,
	.dev = {
		.platform_data = &zoom_dss_lcd_data,
	},
};

static struct omap_dss_device zoom_tv_device = {
	.name                   = "tv",
	.driver_name            = "venc",
	.type                   = OMAP_DISPLAY_TYPE_VENC,
	.phy.venc.type          = -1,
	.platform_enable        = zoom_panel_enable_tv,
	.platform_disable       = zoom_panel_disable_tv,
};

static struct omap_dss_device *zoom_dss_devices[] = {
	&zoom_lcd_device,
	&zoom_tv_device,
	&zoom_hdmi_device,
};

static struct omap_dss_board_info zoom_dss_data = {
	.num_devices = ARRAY_SIZE(zoom_dss_devices),
	.devices = zoom_dss_devices,
	.default_device = &zoom_lcd_device,
};

static struct platform_device zoom_dss_device = {
	.name          = "omapdss",
	.id            = -1,
	.dev            = {
		.platform_data = &zoom_dss_data,
	},
};

static struct omap2_mcspi_device_config dss_lcd_mcspi_config = {
	.turbo_mode             = 0,
	.single_channel         = 1,  /* 0: slave, 1: master */
};

static struct spi_board_info nec_8048_spi_board_info[] __initdata = {
	[0] = {
		.modalias               = "nec_8048_spi",
		.bus_num                = 1,
		.chip_select            = 2,
		.max_speed_hz           = 375000,
		.controller_data        = &dss_lcd_mcspi_config,
	},
};

static struct platform_device *zoom_display_devices[] __initdata = {
	&zoom_dss_device,
};

void __init zoom_display_init(enum omap_dss_venc_type venc_type)
{
	zoom_tv_device.phy.venc.type = venc_type;
	platform_add_devices(zoom_display_devices,
				ARRAY_SIZE(zoom_display_devices));
	spi_register_board_info(nec_8048_spi_board_info,
				ARRAY_SIZE(nec_8048_spi_board_info));
	zoom_lcd_tv_panel_init();
	zoom_hdmi_reset_enable(1);
}

