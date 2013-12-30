/** @file fcwall.h
 * @brief Simulation of the Wall on the LCD
 * @author Ollo
 *
 * @date 30.12.2013
 * @defgroup LightwallController
 *
 */

#include "gfx.h"

#ifndef FCWALL_H
#define FCWALL_H

#ifdef __cplusplus
extern "C"
{
#endif
 
 void fcwall_update(void);

 void fcwall_init(int width, int height);

#ifdef __cplusplus
}
#endif

#endif /* End of FCWALL_H */
