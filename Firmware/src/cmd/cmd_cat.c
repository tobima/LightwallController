#include "ch.h"
#include "chprintf.h"
#include "fatfsWrapper.h"

void
cmd_cat(BaseSequentialStream *chp, int argc, char *argv[])
{
  FIL fp;
  uint8_t buffer[32];
  int br;

  if (argc < 1)
    return;

  if (wf_open(&fp, (TCHAR*) *argv, FA_READ) != FR_OK)
    return;

  do
    {
      if (wf_read(&fp, (TCHAR*) buffer, 32, (UINT*) &br) != FR_OK)
        return;

      chSequentialStreamWrite(chp, buffer, br);
    }
  while (!wf_eof(&fp));

  wf_close(&fp);

  chprintf(chp, "\r\n");
}
