/** @file usbcdc.h
 * @brief Module to provide an USB CDC (USB serial interface)
 *
 * @page USBusage USB CDC module - usage
 *
 * This module can be used to access all Chibios-commands via a USB-UART. <br/>
 *
 * <code>
 * static const ShellCommand commands[] = {<br/>
 *              { "mem", cmd_mem },<br/>
 *              { "threads", cmd_threads },<br/>
 *              { NULL, NULL } };<br/>
 * </code><br/>
 * This example shown above are some basic Chibios commands.<br/>
 *
 * A mini version of a <code>main.c</code> contains the following additional header:
 * <code>
 * \#include "usbcdc/usbcdc.h"
 * </code>
 *
 * A minimal main routine content would be:<br/>
 * <code>
 * halInit();<br/>
 * chSysInit();<br/>
 * <br/>
 * usbcdc_init(commands);<br/>
 * shellInit();<br/>
 * while (TRUE) {<br/>
 *      usbcdc_process();<br/>
 *      if (palReadPad(GPIOA, GPIOA_BUTTON))
 *      {<br/>
 *                  usbcdc_print("Button pressed\r\n");<br/>
 *      }<br/>
 *      //Wait some time, to give the scheduler a chance to run tasks with lower prio<br/>
 *      chThdSleep(MS2ST(200));<br/>
 *   }<br/>
 * </code>
 * <br/>
 * Don't forget to add the source <code>usbcdc.c</code> to your projects Makefile.
 * e.g.:<br/>
 * <code>
 * APPSRC = src/main.c \ <br/>
 *          src/usbcdc/usbcdc.c
 * </code>
 *
 * @author Ollo
 *
 * @date 28.06.2014
 *
 * @defgroup USB Output library (USB UART)
 * @{
 *
 * <br/>
 *
 *
 */

#ifndef _USBCDC_H_
#define _USBCDC_H_

#include "ch.h"
#include "hal.h"

#include "chprintf.h"
#include "shell.h"

#define SHELL_WA_SIZE   THD_WA_SIZE(2048)       /**< Memory for the SHELL connected via USB */

#ifdef __cplusplus
extern "C"
{
#endif

/** @fn void usbcdc_init(const ShellCommand * commands)
 * @param[in] commands     the available commands to execute.
 * @brief initialization of the USB stack. Must only executed once!
 */
void usbcdc_init(const ShellCommand * commands);

/** @fn void usbcdc_process(void)
 * @brief check for a established USB connection
 *
 * Must check must be done cyclic. In order to open a new shell, if the USB is connected.
 */
void usbcdc_process(void);

/** @fn void usbcdc_print(const char *s, ...)
 * @brief prints the given string on the USB CDC
 * @param[in]   s       string to print including formating definition
 * @param[in]   ...     All values for the formating options
 */
void usbcdc_print(const char *s, ...);

#ifdef __cplusplus
}
#endif

/*@}*/

#endif
