#include "fcserverImpl.h"
#include <stdio.h>
#include <string.h>
#include "chprintf.h"
#include "ch.h"

#include <unistd.h>
#include "fcserver.h"
#include "customHwal.h"	/* Needed to activate debugging in server implementation */

#define MAILBOX_SIZE		5
#define MAILBOX2_SIZE		5

/* Mailbox, filled by the fc_server thread */
static uint32_t buffer4mailbox[MAILBOX_SIZE];
static MAILBOX_DECL(mailboxOut, buffer4mailbox, MAILBOX_SIZE);

/* Mailbox, checked by the fc_server thread */
static uint32_t buffer4mailbox2[MAILBOX2_SIZE];
static MAILBOX_DECL(mailboxIn, buffer4mailbox2, MAILBOX2_SIZE);

/******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/

void handleInputMailbox(void)
{
	msg_t msg1, msg2, status;
	
	/* First retrieve the given pointer */
	status = chMBFetch(&mailboxIn, &msg1, TIME_INFINITE);
	if (status == RDY_OK)
	{
		status = chMBFetch(&mailboxIn, &msg2, TIME_INFINITE);
		if (status == RDY_OK)
		{
			chSysLock();
			if ((uint32_t) msg1 == 1)
			{
				chprintf((BaseSequentialStream *) msg2, "Debugging works\r\n");
				hwal_init((BaseSequentialStream *) msg2); /* No Debug output for the sequence library */
			}
			chSysUnlock();
		}
	}
}

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
	chMBPostI(&mailboxOut, (uint32_t) action);
	chMBPostI(&mailboxOut, (uint32_t) clientsocket);
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
	chMBInit(&mailboxOut, (msg_t *)buffer4mailbox, MAILBOX_SIZE);
	chMBInit(&mailboxIn, (msg_t *)buffer4mailbox2, MAILBOX2_SIZE);
	
	ret = fcserver_init(&server, &onNewImage, &onClientChange, 
						10 /* width of wall */, 12 /* height of wall */);
	
	if (ret != FCSERVER_RET_OK)
	{
		/* printf("Server initialization failed with returncode %d\n", ret); */
		return FR_INT_ERR;
	}
		
	fcserver_setactive(&server, 1 /* TRUE */);
	
	do {
		handleInputMailbox();
		
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
	msg_t msg1, msg2, status;
	int i, newMessages;
	
	if(argc < 1)
	{
		chprintf(chp, "Usage {status, debugOn}\r\n");
		res = FR_INT_ERR;
		return res;
	}
	else if(argc >= 1)
    {
		if (strcmp(argv[0], "status") == 0)
		{
			newMessages = chMBGetUsedCountI(&mailboxOut);
			
			chprintf(chp, "%d Messages found\r\n", newMessages );
			for (i=0; i < newMessages; i += 2) {
				status = chMBFetch(&mailboxOut, &msg1, TIME_INFINITE);
				
				if (status != RDY_OK)
				{
					chprintf(chp, "Failed accessing message queue: %d\r\n", status );
				}
				else
				{
					status = chMBFetch(&mailboxOut, &msg2, TIME_INFINITE);
					if (status == RDY_OK)
					{
						chSysLock();
						chprintf(chp, "%d = %d\r\n", (uint32_t) msg1, (uint32_t) msg2);
						chSysUnlock();
					}
					else
					{
						chprintf(chp, "Could only extract key (%d)\r\n", (uint32_t) msg1);
					}

				}
			}
		}
		else if (strcmp(argv[0], "debugOn") == 0)
		{
			/* Activate the debugging */
			chprintf(chp, "Activate the debugging for fullcircle server\r\n");
			chSysLock();
			chMBPostI(&mailboxIn, (uint32_t) 1);
			chMBPostI(&mailboxIn, (uint32_t) chp);
			chSysUnlock();
		}
	}
	
	return res;
}
