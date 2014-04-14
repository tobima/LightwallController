/* @file rgb.h
 * @brief Specific functions to modify the dmx buffer in the fullcircle world
 * @author Ollo
 *
 * @date 14.04.2014 – Created
 * @defgroup DMX
 *
 */
/** @addtogroup DMX */
/*@{*/

#ifndef _RGB_H_
#define _RGB_H_ 

#include <stdio.h>

#include "ch.h"
#include "chprintf.h"

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C"
{
#endif

  /* @brief Hook for Chibios to the commandline logic
   * @param chp	input and output stream for the Chibios
   * @param argc	amount of arguments
   * @param argv	the arguments themself
   * @return nothing, even on problems and errors
   */
  void dmx_rgb_modify(BaseSequentialStream *chp, int argc, char *argv[]);

  /* @brief Set the complete universe with the following color values
   * (the length of the DMX universe is not increased)
   * @param red		value for all red LEDs
   * @param green	value for all green LEDs
   * @param blue	value for all blue LEDs
   */
  void dmx_rgb_fill(uint8_t red, uint8_t green, uint8_t blue);

#ifdef __cplusplus
}
#endif

/*@}*/
#endif /* _DMX_CMD_H_ */
