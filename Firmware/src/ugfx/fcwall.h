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

/** @fn void fcwall_initWindow(void)
 * Initialize the window system (necessary for input)
 */
void fcwall_initWindow(void);

void fcwall_processEvents(void);

#ifdef __cplusplus
}
#endif

#endif /* End of FCWALL_H */
