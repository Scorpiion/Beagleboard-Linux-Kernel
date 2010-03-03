/*
 * services.c
 *
 * DSP-BIOS Bridge driver support functions for TI OMAP processors.
 *
 * Provide SERVICES loading.
 *
 * Copyright (C) 2005-2006 Texas Instruments, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <dspbridge/host_os.h>

/*  ----------------------------------- DSP/BIOS Bridge */
#include <dspbridge/std.h>
#include <dspbridge/dbdefs.h>

/*  ----------------------------------- Trace & Debug */
#include <dspbridge/dbc.h>

/*  ----------------------------------- OS Adaptation Layer */
#include <dspbridge/cfg.h>
#include <dspbridge/mem.h>
#include <dspbridge/ntfy.h>
#include <dspbridge/reg.h>
#include <dspbridge/sync.h>
#include <dspbridge/clk.h>

/*  ----------------------------------- This */
#include <dspbridge/services.h>

/*
 *  ======== SERVICES_Exit ========
 *  Purpose:
 *      Discontinue usage of module; free resources when reference count
 *      reaches 0.
 */
void SERVICES_Exit(void)
{
	/* Uninitialize all SERVICES modules here */
	NTFY_Exit();
	SYNC_Exit();
	CLK_Exit();
	REG_Exit();
	CFG_Exit();
	MEM_Exit();
}

/*
 *  ======== SERVICES_Init ========
 *  Purpose:
 *      Initializes SERVICES modules.
 */
bool SERVICES_Init(void)
{
	bool fInit = true;
	bool fCFG, fMEM;
	bool fREG, fSYNC, fCLK, fNTFY;

	/* Perform required initialization of SERVICES modules. */
	fMEM = MEM_Init();
	fREG = REG_Init();
	fCFG = CFG_Init();
	fSYNC = SYNC_Init();
	fCLK  = CLK_Init();
	fNTFY = NTFY_Init();

	fInit = fCFG && fMEM && fREG && fSYNC && fCLK;

	if (!fInit) {
		if (fNTFY)
			NTFY_Exit();

		if (fSYNC)
			SYNC_Exit();

		if (fCLK)
			CLK_Exit();

		if (fREG)
			REG_Exit();

		if (fCFG)
			CFG_Exit();

		if (fMEM)
			MEM_Exit();

	}

	return fInit;
}

