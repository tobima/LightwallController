#include "fcserverImpl.h"
#include <stdio.h>
#include <string.h>
#include "chprintf.h"
#include "ch.h"

#include <unistd.h>
#include "fcserver.h"

#define MAILBOX_SIZE		5

/* Mailbox, filled by the fc_server thread */
static MAILBOX_DECL(mb1, wa_fc_server, MAILBOX_SIZE);

/******************************************************************************
 * IMPLEMENTATION FOR THE NECESSARY CALLBACKS
 ******************************************************************************/

void onNewImage(uint8_t* rgb24Buffer, int width, int height)
{
	/* printf("%d x %d\n", width, height); */
}

void onClientChange(uint8_t totalAmount, fclientstatus_t action, int clientsocket)
{
	//printf("Callback client %d did %X\t[%d clients]\n", clientsocket, action, totalAmount);
	chSysLock();
	chMBPostI(&mb1, (char) action);
	chMBPostI(&mb1, (char) 1);
	chSysUnlock();
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
	
	/* Prepare Mailbox to communicate with the others */
	chMBInit(&mb1, (msg_t *)wa_fc_server, MAILBOX_SIZE);
	
	ret = fcserver_init(&server, &onNewImage, &onClientChange, 
						10 /* width of wall */, 12 /* height of wall */);
	
	if (ret != FCSERVER_RET_OK)
	{
		/* printf("Server initialization failed with returncode %d\n", ret); */
		return FR_INT_ERR;
	}
	
	chMBPost(&mb1, 'I', TIME_INFINITE);
	
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
	msg_t msg1, status;
	char val = '?';
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
					chprintf(chp, "%c", (char) msg1);
					chSysUnlock();
				}
			}
			chprintf(chp, "\r\n");
		}
		
	
	}
	
	return res;
}
