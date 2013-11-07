
#include "fcstatic.h"
#include "ff.h"
#include <string.h>
#include "hwal.h"	/* Needed for memcpy */

#define FC_SEQUENCE_EXTENSION ".fcs"
#define FC_SEQUENCE_EXTENSION_UPPER ".FCS"

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

int fcstatic_getnext_file(char* path, uint32_t length, uint32_t *pFilelength)
{
	FRESULT res;
	FILINFO fno;
	DIR dir;
	int i;
	char *fn;
	
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
				res = fcstatic_getnext_file(path, length, pFilelength);
				if (res != FR_OK)
					break;
				path[--i] = 0;
			}
			else
			{
				/* search for fullcircle files */
				if (isFcSequence(fn))
				{
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
			}
		}
	}
	return 0;
}
