/** @file fcserverImpl.h
 * @brief Dynamic fullcircle implementation for Chibios
 * @author Ollo
 *
 * @date 26.09.2013
 * @defgroup LightwallController
 *
 */

#include "ch.h"
#include "hal.h"
#include "ff.h"

#ifndef FCSERVER_IMPL_H
#define FCSERVER_IMPL_H

#ifndef FCSERVERIMPL_THREAD_STACK_SIZE
#define FCSERVERIMPL_THREAD_STACK_SIZE   THD_WA_SIZE(4096)
#endif

#ifndef FCSERVER_IMPL_SLEEPTIME
#define FCSERVER_IMPL_SLEEPTIME 10	/* time in milliseconds before starting endless loop again */
#endif

/**
 * Commands that can be sent via the mailbox @see gFcServerMailbox
 */
#define FCSERVER_CMD_DEBUG_ON				1	/**< Command to deactivate debugging */
#define FCSERVER_CMD_DEBUG_OFF				2	/**< Command to activate debugging */
#define FCSERVER_CMD_MODIFY_ACTIVE			3	/**< Command to modify if the server can send something to the wall */

/** @var gFcServerActive
 * @brief block clients or let clients communicate with the wall (accessible for all, as shared memory)
 *
 * The server is active on a positive number (deactivated on zero)
 */
extern uint32_t gFcServerActive;

/**
 * @var gFcServerMailboxBuffer
 * @var gFcServerMailbox
 * External mailbox interface to communicate with the dynamic server
 */
extern uint32_t*	gFcServerMailboxBuffer;
extern Mailbox *	gFcServerMailbox;

extern WORKING_AREA(wa_fc_server, FCSERVERIMPL_THREAD_STACK_SIZE);

#ifdef __cplusplus
extern "C"
{
#endif
  msg_t
  fc_server(void *p);
#ifdef __cplusplus
}
#endif

/**
 * Debug interface for the commandline
 */
void
fcsserverImpl_cmdline(BaseSequentialStream *chp, int argc, char *argv[]);

#endif /* End of FCSERVER_IMPL_H */
