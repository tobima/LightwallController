/** @file wall_simu.c
 * @brief Visualization of Lightwall on the display
 * @author Ollo
 *
 * @date 13.06.2014 - Created
 * @defgroup UGFX
 *
 */

#include "wall_simu.h"
#include "fcscheduler.h"
#include "fcwall.h"
#include "dmx/dmx.h"
#include "ugfx_cmd.h"

static wallconf_t	wallcfg;
static uint8_t		gWallSimuRunning = TRUE;

WORKING_AREA(wa_fc_wallsimu, UGFX_WALL_SIMU_THREAD_STACK_SIZE);

static msg_t fc_wallsimu(void *p)
{
	int row, col, offset;

	chRegSetThreadName("dmx2ugfx");
	(void) p;

	/* Load wall configuration */
    offset = readConfigurationFile(&wallcfg);

    if (offset != 0)
    {
    	PRINT("%s: Could not parse WALL configuration file \r\n", __FILE__);
    	gWallSimuRunning = FALSE;
    	return RDY_OK;
    }

    fcwall_init(wallcfg.width, wallcfg.height);
    PRINT("%s wall %dx%d\r\n", __FILE__, wallcfg.width, wallcfg.height);
    while (gWallSimuRunning)
    {
		for (row = 0; row < wallcfg.height; row++)
		{
		  for (col = 0; col < wallcfg.width; col++)
			{
			  offset = (row * wallcfg.width + col);
			  setBox(col, row,
								dmx_buffer.buffer[wallcfg.pLookupTable[offset] + 0],
								dmx_buffer.buffer[wallcfg.pLookupTable[offset] + 1],
								dmx_buffer.buffer[wallcfg.pLookupTable[offset] + 2]);
			}
		}

		chThdSleep(MS2ST(1000 / wallcfg.fps));
    }

    if (wallcfg.pLookupTable)
    {
    	chHeapFree(wallcfg.pLookupTable);
    }
	return RDY_OK;
}

void ugfx_wall_simu_startThread(void)
{
	PRINT("%s starting...\r\n", __FILE__);
	chSysLock();
	gWallSimuRunning = TRUE;
	chSysUnlock();
	chThdCreateStatic(wa_fc_wallsimu, sizeof(wa_fc_wallsimu), NORMALPRIO + 1,
			fc_wallsimu, NULL);
}

void ugfx_wall_simu_stopThread(void)
{
	chSysLock();
	gWallSimuRunning = FALSE;
	chSysUnlock();
}

int ugfx_wall_simu_isRunning(void)
{
	return gWallSimuRunning;
}
