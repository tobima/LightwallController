/* @file fcscheduler.h
 * @brief File-Scheduler
 * @author Ollo
 *
 * @date 07.11.2013 - Created
 * @defgroup LightwallController
 *
 */

#include "ch.h"

#ifndef FCSTATIC_H
#define FCSTATIC_H

/** @fn int fcstatic_getnext_file(char* filename, uint32_t length, char *path)
 * @brief pick the next file to process
 *
 * The memory to store the filename must already by present
 * 
 * @param[in,out]	filename	actual filename or starting with zero on startup
 * @param[in]		length		The maximum path for the file (length of filename in byte)
 * @param[in]		path		root where to search
 *
 * @return 0 on errors (no file found)
 * @return -1 when the found path does not fit into filename
 * @return 1 if a file was found
 */
int fcstatic_getnext_file(char* filename, uint32_t length, char *path);

#endif /* End of FCSTATIC_H */