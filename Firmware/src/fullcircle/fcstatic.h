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

/** @fn int fcstatic_open_sdcard()
 * @brief prepare the SD card
 *
 * @return -1 when the found path does not fit into filename
 * @return 1 if the SDCard could be opend
 */
int fcstatic_open_sdcard(void);

/** @fn int fcstatic_getnext_file(char* filename, uint32_t length, char *path)
 * @brief pick the next file to process
 *
 * The memory to store the filename must already by present
 * 
 * @param[in,out]	path		ingoing path to search in; output: complete found file
 * @param[in]		length		The maximum path for the file (length of filename in byte)
 * @param[in,out]	pFilelength	length of the filename appended at last in path
 *
 * @return 0 on errors (no file found)
 * @return -1 when the found path does not fit into filename
 * @return 1 if a file was found
 */
int fcstatic_getnext_file(char* path, uint32_t length, uint32_t *pFilelength);

#endif /* End of FCSTATIC_H */