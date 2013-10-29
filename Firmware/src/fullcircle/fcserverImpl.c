#include "fcserverImpl.h"
#include <stdio.h>
#include <string.h>
#include "chprintf.h"
#include "ch.h"

#include <unistd.h>
#include "fcserver.h"

#define MB_SIZE		1024

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
	chMBPost(&mb1, 'I', TIME_INFINITE);
	
	fcserver_setactive(&server, 1 /* TRUE */);
	
	do {
		ret = fcserver_process(&server);
		
		chThdSleep(MS2ST(FCSERVER_IMPL_SLEEPTIME /* convert milliseconds to system ticks */));
	} while ( ret == FCSERVER_RET_OK);
	
	/* clean everything */
	fcserver_close(&server);
	chMBPost(&mb1, 'E', TIME_INFINITE);	
	
	return RDY_OK;
}

FRESULT fcsserverImpl_cmdline(BaseSequentialStream *chp, int argc, char *argv[])
{
	FRESULT res = FR_OK;
	msg_t msg1, status;
	int i, newMessages;
	
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
			newMessages = chMBGetUsedCountI(&mb1);
			chprintf(chp, "%d Messages found\r\n", newMessages );
			for (i=0; i < newMessages; i++) {
				status = chMBFetch(&mb1, &msg1, TIME_INFINITE);
				if (status != RDY_OK)
				{
					chprintf(chp, "Failed accessing message queue: %d\r\n", status );
				}
				else
				{
					chSysLock();
					chprintf(chp, "%c", (char) msg1 );
					chSysUnlock();
				}
			}
			chprintf(chp, "\r\n" );
		}
		
	
	}
	
	return res;
}
