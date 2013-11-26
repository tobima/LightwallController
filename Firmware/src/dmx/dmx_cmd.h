/* @file dmx_cmd.h
 * @brief Debuging tool, to modify the shared memory for DMX.
 * @author Ollo
 *
 * @date 04.10.2013 – Created
 * @defgroup DMX
 *
 */
/** @addtogroup DMX */
/*@{*/

#ifndef _DMX_CMD_H_
#define _DMX_CMD_H_ 

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
  void
  cmd_dmx_modify(BaseSequentialStream *chp, int argc, char *argv[]);

#ifdef __cplusplus
}
#endif

/*@}*/
#endif /* _DMX_CMD_H_ */
