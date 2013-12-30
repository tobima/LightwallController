/** @file ugfx_util.h
 * @brief Some utily functions
 * @author Ollo
 *
 * @date 30.12.2013
 * @defgroup LightwallController
 *
 */

#include "gfx.h"

#ifndef UGFX_UTIL_H
#define UGFX_UTIL_H

#ifdef __cplusplus
extern "C"
{
#endif

extern void gdispPrintf(int x, int y, font_t font, color_t color, int bufferlength, char* text, ...);

#ifdef __cplusplus
}
#endif

#endif /* End of UGFX_UTIL_H */

