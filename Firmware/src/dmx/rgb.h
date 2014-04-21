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
#define DMX_RGB_RET_ERR_PARAM	0x4 /**< necessary parameter were not set */

/**
 * @typdef RGB24Color_t
 * @brief Type for the color, red, green and blue with each an length of 256 (0-255) shades
 */
typedef struct {
	uint8_t red;	/**< amount of red in the color (0-255) */
	uint8_t green;	/**< amount of green in the color (0-255) */
	uint8_t blue;	/**< amount of blue in the color (0-255) */
} RGB24Color_t;

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

  /** @fn (*FadeCallback_t)
   * Color, that must be written out
   *
   * @param[in] red		value for red, needs to be updated
   * @param[in] green	value for green, needs to be updated
   * @param[in] blue	value for blue, needs to be updated
   * @param[in] pParam	pointer to additional parameter (optional, as needed by the user)
   *
   * @return NOTHING
   */
  typedef void (*FadeCallback_t) (uint8_t red, uint8_t green, uint8_t blue, void* pParam);	
	
  /* @brief Fade a given lamp to the corresponding target color
   * (the length of the DMX universe is not increased)
   *
   * @param[in] start			actual displayed color
   * @param[in] target			color to reach
   * @param[in] duration		time in milliseconds the fading may last
   * @param[in] onColorChange	callback function, for the algorithm
   * @param[in] pParam			pointer to additional parameter for callback (onColorChange)
   *
   * @return	DMX_RGB_RET_OK
   * @return	DMX_RGB_RET_ERR_PARAM
   */
  uint8_t dmx_rgb_fade(RGB24Color_t* start, RGB24Color_t* target, uint32_t duration, FadeCallback_t onColorChange, void* pParam);

	
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
