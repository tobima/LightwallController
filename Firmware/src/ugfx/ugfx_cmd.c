/*
 * @file ugfx_cmd.c
 *
 *  Created on: Apr 22, 2014
 *  Author: Ollo
 */

#include "ugfx_cmd.h"
#include "ff.h"

#define TOUCHCALIBRATION_FILE   "touchcalib.bin"

#define UGFX_CONFIGSTORE_THREAD_STACK_SIZE      THD_WA_SIZE(128)

typedef struct UgfxConfig {
  const uint8_t *calbuf;
  uint32_t      size;
} UgfxConfig_t;

/******************************************************************************
 * GLOBAL VARIABLES of this module
 ******************************************************************************/


/******************************************************************************
 * LOCAL VARIABLES for this module
 ******************************************************************************/


/**
 * Stack area for the scheduler thread.
 */
WORKING_AREA(wa_ucfx_configstore, UGFX_CONFIGSTORE_THREAD_STACK_SIZE);

static SerialUSBDriver* gSDU1 = NULL;

/******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/

/******************************************************************************
 * EXTERN FUNCTIONS
 ******************************************************************************/
void ugfx_cmd_calibrate(SerialUSBDriver* pSDU1)
{
  gSDU1 = pSDU1;
  ginputCalibrateMouse(0);
  return;
}

msg_t ucfx_configstore(void* configuration)
{
  FIL fi;
  FRESULT ferr = FR_INVALID_PARAMETER;
  uint8_t bsize;
  int i = 0;
  const uint8_t *calbuf;

  UgfxConfig_t *config = (UgfxConfig_t *) configuration;
  bsize = config->size;
  calbuf = config->calbuf;

  chRegSetThreadName("configstore");

  ferr = f_open(&fi, (TCHAR*) TOUCHCALIBRATION_FILE, FA_CREATE_ALWAYS | FA_WRITE);
#if 0
  if (ferr != FR_OK)
    return;
#endif

  if (gSDU1)
  {
      chprintf((BaseSequentialStream *) gSDU1, "File opend %s, returned %d %s:%d\r\n", TOUCHCALIBRATION_FILE, ferr, __FILE__, __LINE__);
  }

#if 0
  f_write(&fi, &bsize, 1, &wr);
  f_write(&fi, calbuf, size, &wr);
#endif

  if (gSDU1)
  {
      chprintf((BaseSequentialStream *) gSDU1, "Stored at file: ");
      for(i=0; i < bsize; i++)
      {
          chprintf((BaseSequentialStream *) gSDU1, "%0.2X", calbuf[i]);
      }
      chprintf((BaseSequentialStream *) gSDU1, "\r\n");
  }

#if 0
  f_close(&fi);
#endif
}

void ugfx_cmd_cfgsave(uint16_t instance, const uint8_t *calbuf, size_t size)
{
  int i=0;
  UgfxConfig_t config;
  config.size = size;
  config.calbuf = calbuf;

  (void)instance;


  if (gSDU1)
  {
      chprintf((BaseSequentialStream *) gSDU1, "ugfx_cmd_cfgsave %s:%d\r\n", __FILE__, __LINE__);

        chprintf((BaseSequentialStream *) gSDU1, "Setting: ");
        for(i=0; i < size; i++)
        {
            chprintf((BaseSequentialStream *) gSDU1, "%0.2X", calbuf[i]);
        }
        chprintf((BaseSequentialStream *) gSDU1, "\r\n");
    }
  chThdCreateStatic(wa_ucfx_configstore, sizeof(wa_ucfx_configstore), NORMALPRIO + 1,
      ucfx_configstore, (void *) &config);

}

const char *ugfx_cmd_cfgload(uint16_t instance)
{
        FRESULT ferr;
        FIL fi;
        UINT br;
        uint8_t bsize;
        char *buffer;
        buffer = NULL;

        (void)instance;

        if (gSDU1)
        {
            chprintf((BaseSequentialStream *) gSDU1, "ugfx_cmd_cfgload %s:%d\r\n", __FILE__, __LINE__);
        }

        if (f_open(&fi, TOUCHCALIBRATION_FILE, FA_READ)!= FR_OK)
          return buffer;

        if (gSDU1)
        {
            chprintf((BaseSequentialStream *) gSDU1, "File opend %s\r\n", TOUCHCALIBRATION_FILE);
        }
#if 0
        ferr = f_read(&fi, &bsize, 1, &br);
        if (ferr != FR_OK)      return buffer;

        buffer = gfxAlloc(bsize);

        ferr = f_read(&fi, buffer, bsize, &br);
        if (ferr != FR_OK)      return buffer;

        f_close(&fi);
#endif
        return buffer;
}
