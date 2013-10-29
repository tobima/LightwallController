#include "fcserverImpl.h"
#include <stdio.h>
#include <string.h>
#include "chprintf.h"
#include "ch.h"

#include <unistd.h>
#include "fcserver.h"

#define MB_SIZE		5

static MAILBOX_DECL(mb1, wa_fc_server, MB_SIZE);

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

	/* Put something in the mailbox */
	chMBPostl(&mb1, "Server was successfull initialized");

	
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
	msg_t msg1;
	
	if(argc < 1)
	{
		chprintf(chp, "Usage {status}\r\n");
		res = FR_INT_ERR;
		return res;
	}
	else if(argc >= 1)
        {
                if (strcmp(argv[0], "status") == 0)
                {
			chprintf(chp, "%d Messages found\r\n", chMBGetFreeCountI(&mb1));
		
		}
	}
	
	return res;
}
