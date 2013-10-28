#include "fcserverImpl.h"
#include <stdio.h>
#include <string.h>
#include "chprintf.h"
#include "ch.h"

#include <unistd.h>
#include "fcserver.h"

/******************************************************************************
 * IMPLEMENTATION FOR THE NECESSARY CALLBACKS
 ******************************************************************************/

void onNewImage(uint8_t* rgb24Buffer, int width, int height)
{
	/* printf("%d x %d\n", width, height); */
}

/******************************************************************************
 * GLOBAL FUNCTIONS
 ******************************************************************************/

/**
 * Stack area for the http thread.
 */
WORKING_AREA(wa_fc_server, FCSERVERIMPL_THREAD_STACK_SIZE);

/**
 * HTTP server thread.
 */
msg_t fc_server(void *p)
{		
	fcserver_ret_t	ret;
	fcserver_t		server;
	chRegSetThreadName("dynfc-server");
	(void)p;
		
	ret = fcserver_init(&server, &onNewImage, 10, 12);
	if (ret != FCSERVER_RET_OK)
	{
		/* printf("Server initialization failed with returncode %d\n", ret); */
		return FR_INT_ERR;
	}
	
	fcserver_setactive(&server, 1 /* TRUE */);
	
	do {
		ret = fcserver_process(&server);
		
		chThdSleep(MS2ST(FCSERVER_IMPL_SLEEPTIME /* convert milliseconds to system ticks */));
	} while ( ret == FCSERVER_RET_OK);
	
	/* clean everything */
	fcserver_close(&server);
		
	return RDY_OK;
}

FRESULT fcsserverImpl_cmdline(BaseSequentialStream *chp, int argc, char *argv[])
{
	FRESULT res = FR_OK;
	
	if(argc < 1)
	{
		chprintf(chp, "Usage UNDEFINED\r\n");
		res = FR_INT_ERR;
		return res;
	}
	
	return res;
}
