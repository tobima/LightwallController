#include "fcs.h"
#include <stdio.h>
#include <string.h>
#include "chprintf.h"

#define FC_SEQUENCE_EXTENSION ".fcs"
#define FC_SEQUENCE_EXTENSION_UPPER ".FCS"

int isFcSequence(char* path)
{
	char* point;
	/* seach for the dot, describing the extension */
	if((point = strrchr(path,'.')) != NULL ) {
		if ( (strcmp(point, FC_SEQUENCE_EXTENSION) == 0)
		    || (strcmp(point, FC_SEQUENCE_EXTENSION_UPPER) == 0)) {
			return 1;
		}
	}
	
	/* the listed file is not a sequence file */
	return 0;
}

FRESULT fcs_scan_files(BaseSequentialStream *chp, char *path)
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
				res = fcs_scan_files(chp, path);
				if (res != FR_OK)
					break;
				path[--i] = 0;
			}
			else
			{
				/* search for fullcircle files */
				if (isFcSequence(fn))
				{
					chprintf(chp, "%s/%s\r\n", path, fn);
				}
			}
		}
	}
	return res;
}
