/*
 * @file ugfx_cmd.c
 *
 *  Created on: Apr 22, 2014
 *  Author: Ollo
 */

#include "ugfx_cmd.h"
#include "ff.h"

/******************************************************************************
 * GLOBAL VARIABLES of this module
 ******************************************************************************/


/******************************************************************************
 * LOCAL VARIABLES for this module
 ******************************************************************************/

/******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/

/******************************************************************************
 * EXTERN FUNCTIONS
 ******************************************************************************/
void ugfx_cmd_calibrate(void)
{
  ginputSetMouseCalibrationRoutines(0, ugfx_cmd_cfgsave, ugfx_cmd_cfgload, FALSE);
  ginputCalibrateMouse(0);
  return;
}

void ugfx_cmd_cfgsave(uint16_t instance, const uint8_t *calbuf, size_t size)
{
        FIL fi;
        UINT wr;
        uint8_t bsize;

        (void)instance;

        bsize = size;
/*
        if (f_open(&fi, "touchcalib", FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
          return;

        f_write(&fi, &bsize, 1, &wr);
        f_write(&fi, calbuf, size, &wr);

        f_close(&fi);
*/
}

const char *ugfx_cmd_cfgload(uint16_t instance)
{
        FRESULT ferr;
        FIL fi;
        UINT br;
        uint8_t bsize;
        char *buffer;

        (void)instance;
/*
        f_open(&fi, "touchcalib", FA_READ);
        f_read(&fi, &bsize, 1, &br);

        buffer = gfxAlloc(bsize);

        f_read(&fi, buffer, bsize, &br);
        f_close(&fi);
*/
        return buffer;
}
