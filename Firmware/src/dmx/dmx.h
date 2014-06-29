/* @file dmx.h
 * @brief Interface to send DMX frames
 * @author tobima
 * @author Ollo
 *
 * @date 29.06.2014 Added new abstraction: a framebuffer
 * @date 12.06.2013 Working test system
 * @defgroup DMX
 *
 * Shared memory, that is sent via an adapted UART -> DMX is born.
 * 
 * This module also maps each pixel to the corresponding DMX address.
 */

#ifndef _DMX_H_
#define _DMX_H_ 

/** @addtogroup DMX */
/*@{*/

#ifndef DMX_THREAD_STACK_SIZE
#define DMX_THREAD_STACK_SIZE   THD_WA_SIZE(128)
#endif

#ifndef DMX_THREAD_PRIORITY
#define DMX_THREAD_PRIORITY     (LOWPRIO + 3)
#endif

#define DMX_BUFFER_MAX	513

#define DMX_RGB_COLOR_WIDTH			3				/**< Amount of bytes for the 3 colors in each box */

extern uint8_t dmx_fb[DMX_BUFFER_MAX]; /**< Framebuffer that is converted and sent via DMX */

extern WORKING_AREA(wa_dmx, DMX_THREAD_STACK_SIZE);

#ifdef __cplusplus
extern "C"
{
#endif

  msg_t
  dmxthread(void *p);

  void DMXInit(void);

  /** @fn void dmx_getScreenresolution(int *pWidth, int *pHeight)
   * @brief get the standard configuration
   * @param[out] pWidth	The pixel horizontal
   * @param[out] pHeight	The amount of pixel-rows
   */
  void dmx_getScreenresolution(int *pWidth, int *pHeight);

  /** @fn void dmx_getDefaultConfiguration(int *pFPS, int *pDim)
   * @brief Further configuration about the outgoing protocol
   * The configuration provides further parameter, than @see dmx_getScreenresolution.
   * @param[out]	pFPS	Refresh rate in Frames per second, that should be used to get the best performance
   * @param[out]	pDim	valid value range is 0 - 100 (as this value represents a percentage value)
   */
  void dmx_getDefaultConfiguration(int *pFPS, int *pDim);

  void dmx_dim(int value);

#ifdef __cplusplus
}
#endif

/*@}*/
#endif /*_DMX_H_*/
