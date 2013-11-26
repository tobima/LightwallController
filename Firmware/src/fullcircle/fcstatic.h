/* @file fcscheduler.h
 * @brief File-Scheduler
 * @author Ollo
 *
 * @date 07.11.2013 - Created
 * @defgroup LightwallController
 *
 */

#include "ch.h"
#include "fullcircle/fcscheduler.h"

#ifndef FCSTATIC_H
#define FCSTATIC_H

/** @fn int fcstatic_open_sdcard()
 * @brief prepare the SD card
 *
 * @return -1 when the found path does not fit into filename
 * @return 1 if the SDCard could be opend
 */
int
fcstatic_open_sdcard(void);

/** @fn int fcstatic_getnext_file(char* filename, uint32_t length, char *path)
 * @brief pick the next file to process
 *
 * The memory to store the filename must already by present
 * 
 * @param[in,out]	path		ingoing path to search in; output: complete found file
 * @param[in]		length		The maximum path for the file (length of filename in byte)
 * @param[in,out]	pFilelength	length of the filename appended at last in path
 * @param[in]		pLastFilename	the last result or NULL
 *
 * @return 0 on errors (no file found)
 * @return -1 when the found path does not fit into filename
 * @return 1 if a file was found
 */
int
fcstatic_getnext_file(char* path, uint32_t length, uint32_t *pFilelength,
    char *pLastFilename);

/** @fn void fcstatic_remove_filename(char *path, char *pFilename, uint32_t filenameLength)
 * @brief Extract the filename from the path.
 *
 * If necessary, the given storage for the filename is first freed
 * 
 * @param[in,out]	path		ingoing: absolute path to an result ; output: path to search in
 * @param[in]		ppFilename	storage for the filename, will be "free"d if necessary
 * @param[in,out]	pFilelength	length of the filename appended in path
 */
void
fcstatic_remove_filename(char *path, char **ppFilename,
    uint32_t filenameLength);

/** @fn void fcstatic_playfile(char *pFilename, wallconf_t *pConfiguration , BaseSequentialStream *chp)
 * @brief Plays a given sequence to the wall.
 *
 * @param[in]	pFilename		the sequence file to play
 * @param[in]	pConfiguration	configuration of the wall, mappingtable, fps & dimmfactor are needed (optiinal)
 * @param[in]	chp				Stream for debug outputs (optional)
 */
int
fcstatic_playfile(char *pFilename, wallconf_t *pConfiguration,
    BaseSequentialStream *chp);

#endif /* End of FCSTATIC_H */
