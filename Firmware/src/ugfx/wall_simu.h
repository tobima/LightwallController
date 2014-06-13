/** @file wall_simu.h
 * @brief Visualization of Lightwall on the display
 * @author Ollo
 *
 * @date 13.06.2014 - Created
 * @defgroup UGFX
 *
 */

#include "ch.h"
#include "hal.h"

#ifndef _UGFX_WALL_SIMU_H
#define _UGFX_WALL_SIMU_H

#ifndef UGFX_WALL_SIMU_THREAD_STACK_SIZE
#define UGFX_WALL_SIMU_THREAD_STACK_SIZE   THD_WA_SIZE(2048) /* Increase memory for parsing */
#endif

/**
 * Stack area for the scheduler thread.
 */
extern WORKING_AREA(wa_fc_wallsimu, UGFX_WALL_SIMU_THREAD_STACK_SIZE);


#ifdef __cplusplus
extern "C"
{
#endif
  msg_t
  fc_wallsimu(void *p);
#ifdef __cplusplus
}
#endif

/** @fn void WALL_SIMU_startThread(void)
 * @brief function to start a thread
 * This function is asynchron and will return immediately.
 * FIXME WARNING: There is at the moment no logic to prevent multiple starts
 */
void ugfx_wall_simu_startThread(void);

/** @fn void WALL_SIMU_stopThread(void)
 * @brief Stop the thread (Must be started, if needed again)
 * Sends a message to the thread. This function is asynchron and will return immediately
 */
void ugfx_wall_simu_stopThread(void);

/** @fn int WALL_SIMU_isRunning(void)
 * @brief status information, if the scheduler is running
 *@return TRUE: if the scheduler is runnning
 *@return FALSE: no files will be automatically displayed
 */
int ugfx_wall_simu_isRunning(void);

#endif /* End of _UGFX_WALL_SIMU_H */
