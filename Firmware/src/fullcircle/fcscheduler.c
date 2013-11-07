
#include "fcscheduler.h"
#include <stdio.h>
#include <string.h>
#include "chprintf.h"
#include "ch.h"

#include "ini/ini.h"
#include <string.h>
#include <stdlib.h>


#define FCSCHED_CONFIGURATION_FILE "fc/conf/wall"

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

/******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/

/** @fn static int wall_handler(void* config, const char* section, const char* name, const char* value)
 * @brief Extract the configuration for the wall
 *
 * After using, the memory of this structure must be freed!
 *
 * @param[in]	config	structure, all found values are stored
 * @param[in]	section	section, actual found
 * @param[in]	name	key
 * @param[in]	value	value
 *
 * @return < 0 on errors
 */
static int wall_handler(void* config, const char* section, const char* name, 
					   const char* value)
{
	wallconf_t* pconfig = (wallconf_t*) config;
	int row = strtol(value, NULL, 10);
	int height;
	
	if (MATCH("global", "width")) {
		pconfig->width = strtol(value, NULL, 10);
	} else if (MATCH("global", "height")) {
		pconfig->height = strtol(value, NULL, 10);
	} if ((row >= 0) && (row < pconfig->height) ) {
		/* when the function was called the first time, take some memory */
		if (pconfig->pLookupTable == NULL)
		{
			pconfig->pLookupTable = chHeapAlloc(0, 
												sizeof(uint32_t) * 
												pconfig->width * 
												pconfig->height);
		}
		height = strtol(name, NULL, 10);
		pconfig->pLookupTable[row * pconfig->width + height] = (uint32_t) strtol(value, NULL, 10);
		
	} else {
		return 0;  /* unknown section/name, error */
	}
	return 1;
}

/******************************************************************************
 * GLOBAL FUNCTIONS
 ******************************************************************************/

/**
 * Stack area for the scheduler thread.
 */
WORKING_AREA(wa_fc_scheduler, FCSCHEDULER_THREAD_STACK_SIZE);

/**
 * HTTP server thread.
 */
msg_t fc_scheduler(void *p)
{
	chRegSetThreadName("fcscheduler");
	(void)p;
		
	return RDY_OK;
}

FRESULT fcscheduler_cmdline(BaseSequentialStream *chp, int argc, char *argv[])
{
	FRESULT res = FR_OK;
	
	if(argc < 1)
	{
		chprintf(chp, "Usage {debug}\r\n");
		res = FR_INT_ERR;
		return res;
	}
	else if(argc >= 1)
    {
		if (strcmp(argv[0], "debug") == 0)
		{
			wallconf_t wallcfg;
			wallcfg.width			= 0;
			wallcfg.height			= 0;
			wallcfg.pLookupTable	= 0;
			int ret = ini_parse(FCSCHED_CONFIGURATION_FILE, wall_handler, &wallcfg);
			chprintf(chp, "Extracted %dx%d\t[Returned %d]\r\n", wallcfg.width, wallcfg.height, ret);
			
			if ( wallcfg.width > 0 && wallcfg.height > 0)
			{
				int row, col;
				for (row=0; row < wallcfg.height; row++) {
					for (col=0; col < wallcfg.width; col++) {
						chprintf(chp, "%03d ", wallcfg.pLookupTable[row * wallcfg.width + col]);
					}
					chprintf(chp, "\r\n");
				}
			}
			
			/* clean */
			if (wallcfg.pLookupTable)
			{
				chHeapFree(wallcfg.pLookupTable);
			}
		}
	}
	
	return res;
}
