/** @file fcscheduler.h
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

#define INPUT_DYNMAILBOX_SIZE	10

#ifndef _FCSCHEDULER_H
#define _FCSCHEDULER_H

#ifndef FCSCHEDULER_THREAD_STACK_SIZE
#define FCSCHEDULER_THREAD_STACK_SIZE   THD_WA_SIZE(2048) /* Increase memory for parsing */
#endif

/** @var gFcConnectedClients
 * @brief Amount of actual clients connected via the network (accessable for all, as shared memory)
 * This variable is used as input method to this module
 */
extern uint32_t gFcConnectedClients;

/**
 * Stack area for the scheduler thread.
 */
extern WORKING_AREA(wa_fc_scheduler, FCSCHEDULER_THREAD_STACK_SIZE);

/**
 * @var gFcBuf4DynQueue
 * @var gFcMailboxDyn
 * Mailbox, that must be filled from the network code.
 * When this mailbox runs empty, the network code will be stopped.
 */
extern uint32_t*	gFcBuf4DynQueue;
extern Mailbox *	gFcMailboxDyn;

#ifdef __cplusplus
extern "C"
{
#endif
  msg_t
  fc_scheduler(void *p);
#ifdef __cplusplus
}
#endif

typedef struct
{
  int netOnly; /**< Use only the network device */
} schedulerconf_t;

/**
 * Debug interface for the commandline
 */
void fcscheduler_cmdline(BaseSequentialStream *chp, int argc, char *argv[]);

/** @fn void fcscheduler_startThread(void)
 * @brief function to start a thread
 * This function is asynchron and will return immediately.
 * FIXME WARNING: There is at the moment no logic to prevent multiple starts
 */
void fcscheduler_startThread(void);

/** @fn void fcscheduler_stopThread(void)
 * @brief Stop the thread (Must be started, if needed again)
 * Sends a message to the thread. This function is asynchron and will return immediately
 */
void fcscheduler_stopThread(void);

/** @fn int fcscheduler_isRunning(void)
 * @brief status information, if the scheduler is running
 *@return TRUE: if the scheduler is runnning
 *@return FALSE: no files will be automatically displayed
 */
int fcscheduler_isRunning(void);

/**
 *@fn int fcscheduler_getActualFile(char* pPathToFile)
 *@brief Get the file including its path of the actual played file.
 *@param[out]	pPathToFile Memory, where the actual played file is stored into
 *@param[in|out]	length in bytes, that fit at maximum in the given memory (pPathToFile); Later it returns the length of the found path
 *@return 1 on failure
 *@return 0 on success
 */
int fcscheduler_getActualFile(char* pPathToFile, int* pLengthOfFile);

#endif /* End of _FCSCHEDULER_H */
