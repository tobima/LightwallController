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

#define		STATUS_KILLED		5

static uint8_t		gWallSimuRunning = TRUE;

WORKING_AREA(wa_fc_wallsimu, UGFX_WALL_SIMU_THREAD_STACK_SIZE);

static msg_t fc_wallsimu(void *p)
{
	int row, col, offset;

	chRegSetThreadName("dmx2ugfx");
	(void) p;

	int width, height, fps = -1, dim;
	dmx_getScreenresolution(&width, &height);
	dmx_getDefaultConfiguration(&fps, &dim);

    if (width <= 0 || height <= 0 || fps <= 0)
    {
    	PRINT("%s: Could not parse WALL configuration file \r\n", __FILE__);
    	gWallSimuRunning = FALSE;
    	return RDY_OK;
    }

    PRINT("%s wall %dx%d\r\n", __FILE__, width, height);

    fcwall_init(width, height);

    while (gWallSimuRunning)
    {
		for (row = 0; row < height; row++)
		{
		  for (col = 0; col < width; col++)
			{
			  offset = (col + (row * width)) * DMX_RGB_COLOR_WIDTH;
			  fcwall_setBox(col, row, dmx_fb[offset + 0],
					  	  	   dmx_fb[offset + 1],
								dmx_fb[offset + 2]);
			}
		}

		chThdSleep(MS2ST(1000 / fps));
    }

    gWallSimuRunning = STATUS_KILLED;
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
	return (gWallSimuRunning != STATUS_KILLED);
}
