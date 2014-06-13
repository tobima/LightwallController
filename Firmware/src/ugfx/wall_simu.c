/** @file wall_simu.c
 * @brief Visualization of Lightwall on the display
 * @author Ollo
 *
 * @date 13.06.2014 - Created
 * @defgroup UGFX
 *
 */

#include "wall_simu.h"

msg_t
  fc_wallsimu(void *p)
{
	return RDY_OK;
}

void ugfx_wall_simu_startThread(void)
{

}

void ugfx_wall_simu_stopThread(void)
{

}

int ugfx_wall_simu_isRunning(void)
{
	return 0; /*FIXME some logic must be stolen for this line */
}
