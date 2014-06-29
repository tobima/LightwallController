#include "fcstatic.h"
#include "ff.h"
#include <string.h>
#include "hwal.h"	/* Needed for memcpy */
#include "chprintf.h"

#include "fcseq.h"
#include "fatfsWrapper.h"

#define FC_SEQUENCE_EXTENSION ".fcs"
#define FC_SEQUENCE_EXTENSION_UPPER ".FCS"

#define CHP_PRINT( ... )	if (chp) { chprintf(chp, __VA_ARGS__); }

int
isFcSequence(char* path)
{
  char* point;
  /* seach for the dot, describing the extension */
  if ((point = strrchr(path, '.')) != NULL)
    {
      if ((strcmp(point, FC_SEQUENCE_EXTENSION) == 0)
          || (strcmp(point, FC_SEQUENCE_EXTENSION_UPPER) == 0))
        {
          return 1;
        }
    }

  /* the listed file is not a sequence file */
  return 0;
}

int
fcstatic_open_sdcard(void)
{
  FRESULT err;
  uint32_t clusters;
  FATFS *fsp;

  err = wf_getfree("/", &clusters, &fsp);
  if (err != FR_OK)
    {
      /* FS: wf_getfree() failed. */
      return -1;
    }
  return 1;
}

int
fcstatic_getnext_file(char* path, uint32_t length, uint32_t *pFilelength,
    char *pLastFilename)
{
  FRESULT res;
  FILINFO fno;
  DIR dir;
  int i;
  char *fn;
  int checkedResult = 0; /* Status, if the last filename was passed */

#if _USE_LFN
  fno.lfname = 0;
  fno.lfsize = 0;
#endif
  res = wf_opendir(&dir, path);
  if (res == FR_OK)
    {
      i = strlen(path);
      for (;;)
        {
          res = wf_readdir(&dir, &fno);
          if (res != FR_OK || fno.fname[0] == 0)
            break;
          if (fno.fname[0] == '.')
            continue;
          fn = fno.fname;
          if (fno.fattrib & AM_DIR) /* handle directories */
            {
              path[i++] = '/';
              strcpy(&path[i], fn);
              res = fcstatic_getnext_file(path, length, pFilelength,
                  pLastFilename);
              if (res != FR_OK)
                break;
              path[--i] = 0;
            }
          else
            {
              /* search for fullcircle files */
              if (isFcSequence(fn)
                  && (checkedResult || pLastFilename == NULL
                      || (*pFilelength) == 0))
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

void
fcstatic_remove_filename(char *path, char **ppFilename, uint32_t filenameLength)
{
  int startOffset = 0;

  /* Clean th dirt of the last run */
  if (ppFilename && (*ppFilename))
    {
      chHeapFree((*ppFilename));
      (*ppFilename) = NULL;
    }

  if (filenameLength)
    {
      /* create new memory for the filename */
      (*ppFilename) = chHeapAlloc(0, filenameLength);

      startOffset = strlen(path) - filenameLength
          + 1 /* +1 to ignore the "/" to seperate filename from path */;
      hwal_memcpy((*ppFilename), path + startOffset, filenameLength - 1);
      (*ppFilename)[filenameLength - 1] = 0; /* mark the end of the file */

      /* Clean the filename*/
      hwal_memset(path + startOffset - 1, 0, filenameLength);
    }
}
