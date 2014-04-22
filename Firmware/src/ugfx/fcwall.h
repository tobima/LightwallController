/** @file fcwall.h
 * @brief Simulation of the Wall on the LCD
 * @author Ollo
 *
 * @date 30.12.2013
 * @defgroup LightwallController
 *
 */


#ifndef FCWALL_H
#define FCWALL_H

#include "gfx.h"
#include "chprintf.h"

#ifdef __cplusplus
extern "C"
{
#endif
 
void setBox(int x, int y, uint8_t red, uint8_t green, uint8_t blue);

void fcwall_init(int width, int height);

/** @fn void fcwall_initWindow(void)
 * Initialize the window system (necessary for input)
 */
void fcwall_initWindow();

/**
 * @fn void fcwall_processEvents(SerialUSBDriver* pSDU1)
 * @param[in] pSDU1     (optinal) parameter necessary to print debug messages on the serial console
 */
void fcwall_processEvents(SerialUSBDriver* pSDU1);

#ifdef __cplusplus
}
#endif

#endif /* End of FCWALL_H */
