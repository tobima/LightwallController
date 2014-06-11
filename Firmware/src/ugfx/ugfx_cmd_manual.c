/*
 * @file ugfx_cmd_manual.c
 *
 *  Created on: June 8, 2014
 *  Author: Ollo
 */

#include "ugfx_cmd.h"

#include "fcscheduler.h"
#include "fcwall.h"
#include "dmx/dmx.h"

static wallconf_t* pWallconfig;
static int activated = 0;

void ugfx_cmd_manualtesting_init()
{
	int row, col, offset, retStatus;

	pWallconfig = chHeapAlloc(0, sizeof(wallconf_t));

	/* Load wall configuration */
	retStatus = readConfigurationFile(pWallconfig);

	if (retStatus == 0)
	{
		/* clear the DMX buffer */
		for (row = 0; row < pWallconfig->height; row++)
		{
			for (col = 0; col < pWallconfig->width; col++)
			{
				offset = (row * pWallconfig->width + col);
				dmx_buffer.buffer[pWallconfig->pLookupTable[offset] + 0] = 0;
				dmx_buffer.buffer[pWallconfig->pLookupTable[offset] + 1] = 0;
				dmx_buffer.buffer[pWallconfig->pLookupTable[offset] + 2] = 0;
			}
		}
		PRINT("%s DMX buffer cleaned, wall resolution is %dx%d\r\n", __FILE__, pWallconfig->width, pWallconfig->height);
	}
	activated = TRUE;
}

void ugfx_cmd_manualtesting_stop()
{
	activated = FALSE;
	chHeapFree(pWallconfig->pLookupTable);
	chHeapFree(pWallconfig);
}

void ugfx_cmd_manualtesting_process()
{
	int row, col, offset;

	/* Nothing will be done if deactivated */
	if (!activated)
		return;

	/* Display the current DMX buffer on the screen */
	for (row = 0; row < pWallconfig->height; row++)
	{
		for (col = 0; col < pWallconfig->width; col++)
		{
			offset = (row * pWallconfig->width + col);
			setBox(col, row,
					dmx_buffer.buffer[pWallconfig->pLookupTable[offset] + 0],
					dmx_buffer.buffer[pWallconfig->pLookupTable[offset] + 1],
					dmx_buffer.buffer[pWallconfig->pLookupTable[offset] + 2]);
		}
	}
}
