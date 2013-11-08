
#include "fcstatic.h"
#include "ff.h"
#include <string.h>
#include "hwal.h"	/* Needed for memcpy */
#include "chprintf.h"

#include "dmx/dmx.h"

#include "fcseq.h"

#define FC_SEQUENCE_EXTENSION ".fcs"
#define FC_SEQUENCE_EXTENSION_UPPER ".FCS"


#define CHP_PRINT( ... )	if (chp) { chprintf(chp, __VA_ARGS__); }

int isFcSequence(char* path)
{
	char* point;
	/* seach for the dot, describing the extension */
	if((point = strrchr(path,'.')) != NULL ) {
		if ( (strcmp(point, FC_SEQUENCE_EXTENSION) == 0)
		    || (strcmp(point, FC_SEQUENCE_EXTENSION_UPPER) == 0))
		{
			return 1;
		}
	}
	
	/* the listed file is not a sequence file */
	return 0;
}

int fcstatic_open_sdcard(void)
{
	FRESULT err;
	uint32_t clusters;
	FATFS *fsp;
	
	err = f_getfree("/", &clusters, &fsp);
	if (err != FR_OK) {
		/* FS: f_getfree() failed. */
		return -1;
	}
	return 1;
}

int fcstatic_getnext_file(char* path, uint32_t length, uint32_t *pFilelength, char *pLastFilename)
{
	FRESULT res;
	FILINFO fno;
	DIR dir;
	int i;
	char *fn;
	int	checkedResult = 0; /* Status, if the last filename was passed */
		
#if _USE_LFN
	fno.lfname = 0;
	fno.lfsize = 0;
#endif
	res = f_opendir(&dir, path);
	if (res == FR_OK) {
		i = strlen(path);
		for (;;) {
			res = f_readdir(&dir, &fno);
			if (res != FR_OK || fno.fname[0] == 0)
				break;
			if (fno.fname[0] == '.')
				continue;
			fn = fno.fname;
			if (fno.fattrib & AM_DIR) /* handle directories */
			{
				path[i++] = '/';
				strcpy(&path[i], fn);
				res = fcstatic_getnext_file(path, length, pFilelength, pLastFilename);
				if (res != FR_OK)
					break;
				path[--i] = 0;
			}
			else
			{
				/* search for fullcircle files */
				if (isFcSequence(fn) && (checkedResult 
										 || pLastFilename == NULL 
										 || (*pFilelength) == 0)
					)
				{
					/* found a new file, present it to the user */
					if (strlen(fn) + i >= length)
					{
						/* The given memory is not big enough */
						return -1;
					}
					else
					{
						path[i++] = '/';
						hwal_memcpy(path + i, fn, strlen(fn));
						(*pFilelength) = strlen(fn) + 1 /* space for the slash */;
						return 1;
					}
				}
				/* Search for the last filename */
				else if (isFcSequence(fn) && pLastFilename != NULL)
				{
					if (strcmp(fn, pLastFilename) == 0)
					{
						/* found the last result, store status, to take the next file */
						checkedResult = 1; 
					}
				}

			}
		}
	}
	return 0;
}

void fcstatic_remove_filename(char *path, char *pFilename, uint32_t filenameLength)
{
	int startOffset = 0;
	
	/* Clean th dirt of the last run */
	if (pFilename)
	{
		chHeapFree(pFilename);
		pFilename = NULL;
	}
	
	if (filenameLength)
	{
		/* create new memory for the filename */
		pFilename = chHeapAlloc(0, filenameLength);
		
		startOffset = strlen(path) - filenameLength;
		hwal_memcpy(pFilename, path + startOffset, filenameLength);
		/* Clean the filename*/
		hwal_memset(path + startOffset - 1, 0, filenameLength + 1);
	}
}

void fcstatic_playfile(char *pFilename, wallconf_t *pConfiguration , BaseSequentialStream *chp)
{
	fcsequence_t seq;
	fcseq_ret_t ret = FCSEQ_RET_NOTIMPL; 
	int x, y, ypos;
	int frame_index = 0;
	int sleeptime;
	int fixSleepTimer = -1;
	uint8_t* rgb24;

	
	ret = fcseq_load(pFilename, &seq);
	
	if (ret != FCSEQ_RET_OK)
	{
		CHP_PRINT("Could not read %s\r\nError code is %d\r\n", pFilename, ret);
		CHP_PRINT("Unable to load sequnce (%s:%d)\r\n", __FILE__, __LINE__);
		return;
	}
	
	CHP_PRINT("=== Meta information ===\r\n"
			 "fps: %d, width: %d, height: %d\r\n",seq.fps,seq.width,seq.height);
	
	if (seq.fps == 0)
	{
		CHP_PRINT("The FPS could NOT be extracted! Stopping\r\n");
		return;
	}
	
	/* Allocation some space for the RGB buffer */
	rgb24 = (uint8_t*) chHeapAlloc(NULL, (seq.width * seq.height * 3) );
	
	/* parse */
	ret = fcseq_nextFrame(&seq, rgb24);
	
	if (ret != FCSEQ_RET_OK)
	{
		CHP_PRINT("Reading the first frame failed with error code %d.\r\n", ret );
		return;
	}
	
	/* Update the fps to the wall ones */
	if (pConfiguration && pConfiguration->fps > 0)
	{
		seq.fps = pConfiguration->fps;
		CHP_PRINT("Updated fps to %d (configuration of the wall).\r\n", seq.fps );
	}
	
	/* loop to print something on the commandline */
	while (ret == FCSEQ_RET_OK)
	{
#if 0
		CHP_PRINT("=============== %d ===============\r\n", frame_index);
		for (y=0; y < seq.height; y++)
		{
			ypos = y * seq.width * 3;
			for(x=0; x < seq.width; x++)
			{
				CHP_PRINT("%.2X", rgb24[(ypos+x*3) + 0]);
				CHP_PRINT("%.2X", rgb24[(ypos+x*3) + 1]);
				CHP_PRINT("%.2X", rgb24[(ypos+x*3) + 2]);
				CHP_PRINT("|");
			}
			CHP_PRINT("\r\n");
		}
#endif
		
		/* Set the DMX buffer */
		dmx_buffer.length = seq.width * seq.height * 3;
		memcpy(dmx_buffer.buffer, rgb24, dmx_buffer.length);
		
		sleeptime = (1000 / seq.fps);
		
		chThdSleep(MS2ST(sleeptime /* convert milliseconds to system ticks */));
		/* parse the next */
		ret = fcseq_nextFrame(&seq, rgb24);
		
		/* Move the count */
		frame_index++;
	}
	
	chHeapFree(rgb24);
}
