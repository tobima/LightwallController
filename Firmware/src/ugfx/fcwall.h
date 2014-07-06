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


#define MENU_BUTTON_WIDTH       75     /**< offset for the info text, aka size of the button */

/** @type gGWdefault
 * @brief Window, where the visualization of the complete wall is present
 */
extern GHandle gGWdefault;

/** @fn void fcwall_setBox(int x, int y, uint8_t red, uint8_t green, uint8_t blue)
 * @brief Draw the given box on the LCD
 * @param[in] x		position from the left side in a row	range: 0 - width of the wall (exclusive the amount itself)
 * @param[in] y		row	to use range: 0 - height of the wall (exclusive the amount itself)
 * @param[in] red	range: 0 - 255
 * @param[in] green	range: 0 - 255
 * @param[in] blue	range: 0 - 255
 */
void fcwall_setBox(int x, int y, uint8_t red, uint8_t green, uint8_t blue);

/** @fn void fcwall_init(int width, int height)
 * @brief Initialize the resolution of the wall to display on the LCD
 * @param[in]	width	of the wall in boxes
 * @param[in]	height	of the wall in boxes
 *
 * This function MUST be called before @see fcwall_setBox can be used.
 */
void fcwall_init(int width, int height);

/** @fn void fcwall_initWindow(void)
 * Initialize the window system (necessary for input)
 */
void fcwall_initWindow(void);

/**
 * @fn void fcwall_processEvents(SerialUSBDriver* pSDU1)
 * @param[in] pSDU1     (optional) parameter necessary to print debug messages on the serial console
 * @brief Handle the events from the touchscreen
 */
void fcwall_processEvents(SerialUSBDriver* pSDU1);

#ifdef __cplusplus
}
#endif

#endif /* End of FCWALL_H */
