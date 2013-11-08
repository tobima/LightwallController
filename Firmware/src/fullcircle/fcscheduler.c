
#include "fcscheduler.h"
#include <stdio.h>
#include <string.h>
#include "chprintf.h"
#include "ch.h"

#include "ini/ini.h"
#include <string.h>
#include <stdlib.h>

#include "hwal.h"	/* Needed for memcpy */
#include "fcstatic.h"

#define FCSCHED_CONFIGURATION_FILE	"fc/conf/wall"
#define FCSCHED_FILE_ROOT			"fc/static\0"	/**< Folder on the sdcard to check */

#define	FILENAME_LENGTH	512	/**< Including the absolut path to the file */

#define INPUT_MAILBOX_SIZE		4

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

#define FCSHED_PRINT( ... )	if (gDebugShell) { chprintf(gDebugShell, __VA_ARGS__); }

/******************************************************************************
 * LOCAL VARIABLES for this module
 ******************************************************************************/

static BaseSequentialStream * gDebugShell = NULL;

/* Mailbox, checked by the fullcircle scheduler thread */
static uint32_t buffer4mailbox2[INPUT_MAILBOX_SIZE];
static MAILBOX_DECL(mailboxIn, buffer4mailbox2, INPUT_MAILBOX_SIZE);

/******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/

static void fcsched_handleInputMailbox(void)
{
	msg_t msg1, msg2, status;
	int newMessages;
	
	/* Use nonblocking function to count incoming messages */
	newMessages = chMBGetUsedCountI(&mailboxIn);
	
	if (newMessages >= 2)
	{
		/* First retrieve the given pointer */
		status = chMBFetch(&mailboxIn, &msg1, TIME_INFINITE);
		if (status == RDY_OK)
		{
			status = chMBFetch(&mailboxIn, &msg2, TIME_INFINITE);
			if (status == RDY_OK)
			{
				chSysLock();
				switch ((uint32_t) msg1) {
					case 1:
						gDebugShell = (BaseSequentialStream *) msg2;
						break;
					default:
						break;
				}
				
				chSysUnlock();
			}
		}
	}
}

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
	int row = strtol(section, NULL, 10);
	int	col;
	uint32_t dmxval;
	
	if (MATCH("global", "width")) {
		pconfig->width = strtol(value, NULL, 10);
	} else if (MATCH("global", "height")) {
		pconfig->height = strtol(value, NULL, 10);
	} else if (MATCH("global", "fps")) {
		pconfig->fps = strtol(value, NULL, 10);
	} if ((row >= 0) && (row < pconfig->height) ) {
		/* when the function was called the first time, take some memory */
		if (pconfig->pLookupTable == NULL)
		{
			col = sizeof(uint32_t) * pconfig->width * pconfig->height;
			pconfig->pLookupTable = chHeapAlloc(0, col /* reused for memory length */ );
		}
		col = strtol(name, NULL, 10);
		dmxval = (uint32_t) strtol(value, NULL, 10);
		pconfig->pLookupTable[row * pconfig->width + col] = dmxval;		
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
	int res, resOpen;
	char path[FILENAME_LENGTH];	
	char *filename = NULL;
	uint32_t filenameLength = 0;
	char* root = FCSCHED_FILE_ROOT;
	
	chRegSetThreadName("fcscheduler");
	(void)p;
			
	/* Prepare Mailbox to communicate with the others */
	chMBInit(&mailboxIn, (msg_t *)buffer4mailbox2, INPUT_MAILBOX_SIZE);
	path[0] = 0;
	
	resOpen = fcstatic_open_sdcard();
	
	/* initialize the folder to search in */	 
	hwal_memcpy(path, root, strlen(FCSCHED_FILE_ROOT));
		
	do {
		fcsched_handleInputMailbox();
		if (resOpen)
		{
			/*FIXME check dynamic fullcircle for a new client */
			
			/* pick the next file */
			res = fcstatic_getnext_file(path, FILENAME_LENGTH, &filenameLength, filename);
									
			if (res)
			{
				FCSHED_PRINT("%s ...\r\n", path);
				/* Play the file */
				
				/*extract filename from path for the next cycle */
				fcstatic_remove_filename(path, &filename, filenameLength);
			}
			else
			{
				/*Reset all and start with rootfolder again */
				hwal_memcpy(path, root, strlen(FCSCHED_FILE_ROOT));
				chHeapFree( filename );
				filename = NULL;
			}

		}
		else
		{
			FCSHED_PRINT("Reopen SDcard\r\n");
			resOpen = fcstatic_open_sdcard();
		}
		
		/*Debug code: */
		chThdSleep(MS2ST(5000 /* convert milliseconds to system ticks */));
	} while ( TRUE );	
	
	FCSHED_PRINT("Scheduler stopped!\r\n");
	
	return RDY_OK;
}

void fcscheduler_cmdline(BaseSequentialStream *chp, int argc, char *argv[])
{
	if(argc < 1)
	{
		chprintf(chp, "Usage {debug, debugOn, debugOff}\r\n");
		return;
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
		else if (strcmp(argv[0], "debugOn") == 0)
		{
			/* Activate the debugging */
			chprintf(chp, "Activate the logging for Fullcircle Scheduler\r\n");
			chSysLock();
			chMBPostI(&mailboxIn, (uint32_t) 1);
			chMBPostI(&mailboxIn, (uint32_t) chp);
			chSysUnlock();
		}
		else if (strcmp(argv[0], "debugOff") == 0)
		{
			/* Activate the debugging */
			chprintf(chp, "Deactivate the logging for Fullcircle Scheduler\r\n");
			chSysLock();
			chMBPostI(&mailboxIn, (uint32_t) 1);
			chMBPostI(&mailboxIn, (uint32_t) 0);
			chSysUnlock();
		}
	}
	
}
