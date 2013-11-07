/* @file fcscheduler.h
 * @brief Brain of Lightwall controller
 * @author Ollo
 *
 * @date 07.11.2013 - Created
 * @defgroup LightwallController
 *
 */

#include "ch.h"
#include "hal.h"
#include "ff.h"


#ifndef _FCSCHEDULER_H
#define _FCSCHEDULER_H

#ifndef FCSCHEDULER_THREAD_STACK_SIZE
#define FCSCHEDULER_THREAD_STACK_SIZE   THD_WA_SIZE(1024)
#endif

/**
 * Stack area for the scheduler thread.
 */
extern WORKING_AREA(wa_fc_scheduler, FCSCHEDULER_THREAD_STACK_SIZE);

#ifdef __cplusplus
extern "C" {
#endif
	msg_t fc_scheduler(void *p);
#ifdef __cplusplus
}
#endif


/**
 * Debug interface for the commandline
 */
FRESULT fcscheduler_cmdline(BaseSequentialStream *chp, int argc, char *argv[]);

#endif /* End of _FCSCHEDULER_H */
