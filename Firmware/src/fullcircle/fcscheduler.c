
#include "fcscheduler.h"
#include <stdio.h>
#include <string.h>
#include "chprintf.h"
#include "ch.h"


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
			
		}
	}
	
	return res;
}
