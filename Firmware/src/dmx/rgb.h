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

#define DMX_RGB_RET_OK			0x0
#define DMX_RGB_RET_INCREASE	0x1 /**< The DMX universe was increased */
#define DMX_RGB_RET_ERR_MAXBUF	0x2 /**< The requested offset was larger, than one DMX universe supports */

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
	
  /* @brief Fade a given lamp to the corresponding target color
   * (the length of the DMX universe is not increased)
   *
   * @param red		target value of red
   * @param green	target value of green
   * @param blue	target value of blue
   * @param offset	offset of the lamp (starting with zero)
   *
   * @return	DMX_RGB_RET_OK
   * @return	DMX_RGB_RET_INCREASE
   * @return	DMX_RGB_RET_ERR_MAXBUF
   */
  uint8_t dmx_rgb_fade(uint8_t offset, uint8_t red, uint8_t green, uint8_t blue, uint32_t duration,
					   BaseSequentialStream *chp /*FIXME remove debug output, so this parameter*/ );

	
  /**
   * Map a value to a rainbow color.
   * (Source: http://blog.csharphelper.com/2010/05/20/map-numeric-values-to-colors-in-a-rainbow-in-c.aspx)
   * @param[in] value current value (between 0 and 1023)
   * @param[in|out] red	minumal value (and output for red)
   * @param[in|out] blue  maximum value (also the result for blue)
   * @param[out]    green	output value for the green value
   * @return used color
   */
  void rgb_rainbowcolor(uint16_t value, uint8_t* red, uint8_t* blue, uint8_t* green);
	
#ifdef __cplusplus
}
#endif

/*@}*/
#endif /* _DMX_CMD_H_ */
