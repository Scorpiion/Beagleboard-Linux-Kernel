/*
 * Defines for zoom boards
 */
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <plat/display.h>

struct flash_partitions {
	struct mtd_partition *parts;
	int nr_parts;
};

#define ZOOM_NAND_CS	0

extern void __init zoom_flash_init(struct flash_partitions [], int);
extern int __init zoom_debugboard_init(void);
extern void __init zoom_peripherals_init(void *);
extern void __init zoom_display_init(enum omap_dss_venc_type venc_type);

#ifdef CONFIG_TWL5030_GLITCH_FIX
extern void twl5030_glitchfix_changes(void);
#endif

