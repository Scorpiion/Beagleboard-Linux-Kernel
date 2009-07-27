/*
 * ducatienabler.h
 *
 * Syslink driver support for OMAP Processors.
 *
 * Copyright (C) 2009-2010 Texas Instruments, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef _DDUCATIMMU_ENABLER_H_
#define _DDUCATIMMU_ENABLER_H_

#include <linux/types.h>
#include <syslink/hw_defs.h>
#include <syslink/hw_mmu.h>


#define PAGE_SIZE_4KB				0x1000
#define PAGE_SIZE_64KB				0x10000
#define PAGE_SIZE_1MB				0x100000
#define PAGE_SIZE_16MB				0x1000000

/* Define the Peripheral PAs and their Ducati VAs. */
#define L4_PERIPHERAL_MBOX			0x4A0F4000
#define DSPVA_PERIPHERAL_MBOX			0xAA0F4000

#define L4_PERIPHERAL_I2C1			0x48070000
#define DSPVA_PERIPHERAL_I2C1			0xA8070000
#define L4_PERIPHERAL_I2C2			0x48072000
#define DSPVA_PERIPHERAL_I2C2			0xA8072000
#define L4_PERIPHERAL_I2C3			0x48060000
#define DSPVA_PERIPHERAL_I2C3			0xA8060000

#define L4_PERIPHERAL_DMA			0x4A056000
#define DSPVA_PERIPHERAL_DMA			0xAA056000

#define L4_PERIPHERAL_GPIO1			0x4A310000
#define DSPVA_PERIPHERAL_GPIO1		0xAA310000
#define L4_PERIPHERAL_GPIO2			0x48055000
#define DSPVA_PERIPHERAL_GPIO2		0xA8055000
#define L4_PERIPHERAL_GPIO3			0x48057000
#define DSPVA_PERIPHERAL_GPIO3		0xA8057000

#define L4_PERIPHERAL_GPTIMER3			0x48034000
#define DSPVA_PERIPHERAL_GPTIMER3		0xA8034000
#define L4_PERIPHERAL_GPTIMER4			0x48036000
#define DSPVA_PERIPHERAL_GPTIMER4		0xA8036000
#define L4_PERIPHERAL_GPTIMER9			0x48040000
#define DSPVA_PERIPHERAL_GPTIMER9		0xA8040000
#define L4_PERIPHERAL_GPTIMER11		0x48088000
#define DSPVA_PERIPHERAL_GPTIMER11		0xA8088000


#define L3_TILER_VIEW0_ADDR			0x60000000
#define DUCATIVA_TILER_VIEW0_ADDR		0x60000000
#define DUCATIVA_TILER_VIEW0_LEN		0x20000000




/* Define the various Ducati Memory Regions. */
/* The first 4K page of BOOTVECS is programmed as a TLB entry. The remaining */
/* three pages are not used and are mapped to minimize number of PTEs */
#define DUCATI_BOOTVECS_ADDR			0x1000
#define DUCATI_BOOTVECS_LEN			0x3000

#define DUCATI_EXTMEM_SYSM3_ADDR		0x4000
#define DUCATI_EXTMEM_SYSM3_LEN		0x1FC000

#define DUCATI_EXTMEM_APPM3_ADDR		0x10000000
#define DUCATI_EXTMEM_APPM3_LEN		0x200000

#define DUCATI_PRIVATE_SYSM3_DATA_ADDR	0x84000000
#define DUCATI_PRIVATE_SYSM3_DATA_LEN	0x200000

#define DUCATI_PRIVATE_APPM3_DATA_ADDR	0x8A000000
#define DUCATI_PRIVATE_APPM3_DATA_LEN	0x200000

#define DUCATI_SHARED_M3_DATA_ADDR		0x90000000
#define DUCATI_SHARED_M3_DATA_LEN		0x100000

#define DUCATI_SHARED_IPC_ADDR		0x98000000
#define DUCATI_SHARED_IPC_LEN			0x100000

#define DUCATI_SW_DMM_ADDR			0x80000000
#define DUCATI_SW_DMM_LEN			0x4000000


/* Types of mapping attributes */

/* MPU address is virtual and needs to be translated to physical addr */
#define DSP_MAPVIRTUALADDR			0x00000000
#define DSP_MAPPHYSICALADDR			0x00000001

/* Mapped data is big endian */
#define DSP_MAPBIGENDIAN			0x00000002
#define DSP_MAPLITTLEENDIAN			0x00000000

/* Element size is based on DSP r/w access size */
#define DSP_MAPMIXEDELEMSIZE			0x00000004

/*
 * Element size for MMU mapping (8, 16, 32, or 64 bit)
 * Ignored if DSP_MAPMIXEDELEMSIZE enabled
 */
#define DSP_MAPELEMSIZE8			0x00000008
#define DSP_MAPELEMSIZE16			0x00000010
#define DSP_MAPELEMSIZE32			0x00000020
#define DSP_MAPELEMSIZE64			0x00000040

#define DSP_MAPVMALLOCADDR			0x00000080

struct mmu_entry {
	u32 ul_phy_addr ;
	u32 ul_virt_addr ;
	u32 ul_size ;
};

struct memory_entry {
	u32 ul_virt_addr;
	u32 ul_size;
};

static const struct mmu_entry l4_map[] = {
	/*  Mailbox 4KB*/
	{L4_PERIPHERAL_MBOX, DSPVA_PERIPHERAL_MBOX, HW_PAGE_SIZE_4KB},
};

static const struct  memory_entry l3_memory_regions[] = {
	/*  Remaining BootVecs regions */
	{DUCATI_BOOTVECS_ADDR, DUCATI_BOOTVECS_LEN},
	/*  EXTMEM_CORE0: 0x4000 to 0xFFFFF */
	{DUCATI_EXTMEM_SYSM3_ADDR, DUCATI_EXTMEM_SYSM3_LEN},
	/*  EXTMEM_CORE1: 0x10000000 to 0x100FFFFF */
	{DUCATI_EXTMEM_APPM3_ADDR, DUCATI_EXTMEM_APPM3_LEN},
	/*  PRIVATE_SYSM3_DATA*/
	{DUCATI_PRIVATE_SYSM3_DATA_ADDR, DUCATI_PRIVATE_SYSM3_DATA_LEN},
	/*  PRIVATE_APPM3_DATA*/
	{DUCATI_PRIVATE_APPM3_DATA_ADDR, DUCATI_PRIVATE_APPM3_DATA_LEN},
	/*  SHARED_M3_DATA*/
	{DUCATI_SHARED_M3_DATA_ADDR, DUCATI_SHARED_M3_DATA_LEN},
	/*  IPC*/
	{DUCATI_SHARED_IPC_ADDR, DUCATI_SHARED_IPC_LEN},
};

void dbg_print_ptes(bool ashow_inv_entries, bool ashow_repeat_entries);
int ducati_setup(void);
void ducati_destroy(void);
u32 get_ducati_virt_mem();

#endif /* _DDUCATIMMU_ENABLER_H_*/
