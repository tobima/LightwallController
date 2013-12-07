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

/** @var wallconf_t
 *  @brief This structure contains information about the physical wall
 */
typedef struct
{
  int width; /**< Horizontal count of boxes the phyical installation */
  int height; /**< Vertical count of boxes the phyical installation */
  int fps; /**< Framerate, the wall uses */
  int dimmFactor; /**< In percent -> 100 no change, 50 half the brightness */
  uint32_t *pLookupTable; /**< Memory to the Loopuptable, must be freed after usage */
} wallconf_t;


typedef struct
{
  int netOnly; /**< Use only the network device */
} schedulerconf_t;

/** @fn void readConfigurationFile( extern wallconf_t )
 * @brief Read configuration file
 *
 * The static configuration with the wall.
 
 * @param[out]	pConfiguration	Read configuration
 */
extern void
readConfigurationFile(wallconf_t* pConfiguration);

/** @fn void fcsched_printFrame(uint8_t* pBuffer, int width, int height, wallconf_t* pWallcfg)
 * @brief Print a frame to the DMX memory, that will be send to the wall.
 *
 * THIS function MUST only be used by one source.
 * Multiple sources will be disturb each other ;-)
 *
 * @param[in]	pBuffer		RGB24 memory with the actual frame
 * @param[in]	width		of the actual frame
 * @param[in]	height		of the actual frame
 * @param[in]	pWallcfg	Configuration
 *
 */
extern void
fcsched_printFrame(uint8_t* pBuffer, int width, int height,
    wallconf_t* pWallcfg);

/**
 * Debug interface for the commandline
 */
void
fcscheduler_cmdline(BaseSequentialStream *chp, int argc, char *argv[]);

#endif /* End of _FCSCHEDULER_H */
