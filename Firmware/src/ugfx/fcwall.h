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
 
void setBox(int x, int y, uint8_t red, uint8_t green, uint8_t blue);

 void fcwall_init(int width, int height);

#ifdef __cplusplus
}
#endif

#endif /* End of FCWALL_H */
