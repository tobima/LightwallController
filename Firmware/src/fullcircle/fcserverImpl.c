#include "fcserverImpl.h"
#include <stdio.h>
#include <string.h>
#include "chprintf.h"
#include "ch.h"

/**
 * Stack area for the http thread.
 */
WORKING_AREA(wa_fc_server, FCSERVERIMPL_THREAD_STACK_SIZE);

/**
 * HTTP server thread.
 */
msg_t fc_server(void *p) {	
	chRegSetThreadName("dynfc-server");
	(void)p;
	
	
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
