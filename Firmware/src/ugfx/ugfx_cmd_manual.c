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
#include <string.h> /* Necessary for "memset" */

static int activated = 0;

void ugfx_cmd_manualtesting_init()
{

	/* clear the DMX buffer */
	memset(dmx_fb, 0, DMX_BUFFER_MAX);

	activated = TRUE;
}

void ugfx_cmd_manualtesting_stop()
{
	activated = FALSE;

}

