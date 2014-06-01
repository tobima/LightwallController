/*
 * @file ugfx_cmd.c
 *
 *  Created on: Apr 22, 2014
 *  Author: Ollo
 */

#include "ugfx_cmd.h"
#include "ugfx_util.h"
#include "fcscheduler.h"


/******************************************************************************
 * GLOBAL VARIABLES of this module
 ******************************************************************************/


/******************************************************************************
 * LOCAL VARIABLES for this module
 ******************************************************************************/


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

void ugfx_cmd_manualtesting(uint8_t status)
{
  if (status == UGFX_CMD_MANUAL_ENDED)
  {
      fcscheduler_startThread();
  }
  else if (status != UGFX_CMD_MANUAL_START)
  {
      return; /* Unkown command executed */
  }

  /********** stop normal mode and start the manual testing *********/

  /* Stop all fullcircle threads first */
  fcscheduler_stopThread();

  /*********************
   * Update the UI:
   * - Boxes are clickable to be changed
   */
}

void ugfx_cmd_cfgsave(uint16_t instance, const uint8_t *calbuf, size_t size)
{
  int i=0;
  (void)instance;

  if (gSDU1)
  {
      chprintf((BaseSequentialStream *) gSDU1, "ugfx_cmd_cfgsave %s:%d\r\n", __FILE__, __LINE__);

        chprintf((BaseSequentialStream *) gSDU1, "Setting (%d bytes): ", size);
        for(i=0; i < (int) size; i++)
        {
            chprintf((BaseSequentialStream *) gSDU1, "%02X", calbuf[i]);
        }
        chprintf((BaseSequentialStream *) gSDU1, "\r\n");
    }



    if (gSDU1)
    {
      chprintf((BaseSequentialStream *) gSDU1,
          "-------- configuration stored [%s:%d] -------\r\n", __FILE__, __LINE__);
    }


}

const char *ugfx_cmd_cfgload(uint16_t instance)
{
        char *buffer;
        buffer = NULL;

        (void)instance;

        /*FIXME buffer = gfxAlloc(bsize); */

        return buffer;
}
