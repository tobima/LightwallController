/*
 * @file ugfx_cmd.c
 *
 *  Created on: Apr 22, 2014
 *  Author: Ollo
 */

#include "ugfx_cmd.h"
#include "ugfx_util.h"
#include "fcscheduler.h"
#include "flash.h"
#include <string.h>


/******************************************************************************
 * GLOBAL VARIABLES of this module
 ******************************************************************************/

#define FLASH_CONFIG_BASEADDR	0x8060020

#ifndef FLASH_BLOCKSIZE
#define FLASH_BLOCKSIZE 		((int) sizeof(flashdata_t))
#endif

#define CALIBRATION_SIZE		24 /**< bytes necessary for the touch screen calibration data */


#define  PARAM_CMP(X, Y)	strncmp(X, Y, sizeof(Y)
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
  unsigned int i=0;
  unsigned int j=0;
  int status;
  (void)instance;
  const char* buffer = (const char*) calbuf;

  if (gSDU1)
  {
      chprintf((BaseSequentialStream *) gSDU1, "ugfx_cmd_cfgsave %s:%d\r\n", __FILE__, __LINE__);

        chprintf((BaseSequentialStream *) gSDU1, "Setting (%d bytes): ", size);
        for(i=0; i < (unsigned int) size; i++)
        {
            chprintf((BaseSequentialStream *) gSDU1, "%02X", calbuf[i]);
        }
        chprintf((BaseSequentialStream *) gSDU1, "\r\n");
    }


  	/* write the memory into the flash */
    for (i=0; i < size / FLASH_BLOCKSIZE; i++)
    {
		status = flashWrite(FLASH_CONFIG_BASEADDR + (i * FLASH_BLOCKSIZE), buffer + (i * 4), FLASH_BLOCKSIZE);
		if (status != FLASH_RETURN_SUCCESS)
		{
			if (gSDU1)
			{
				chprintf((BaseSequentialStream *) gSDU1, "%d. block: Writing returned %d \r\n", i + 1, status);
			}
			return;
		}
		else
		{
			if (gSDU1)
			{
				chprintf((BaseSequentialStream *) gSDU1, "%d. block in sector %u : stored at 0x%x ", i + 1,
										flashSectorAt(FLASH_CONFIG_BASEADDR + (i * FLASH_BLOCKSIZE)), FLASH_CONFIG_BASEADDR + (i * FLASH_BLOCKSIZE));

				for(j= (i * 4); j < ((i + 1) * 4); j++)
				{
					chprintf((BaseSequentialStream *) gSDU1, "%02X", buffer[j]);
				}
				chprintf((BaseSequentialStream *) gSDU1, "\r\n");
			}
		}
    }

    if (gSDU1)
    {
      chprintf((BaseSequentialStream *) gSDU1,
          "-------- configuration stored [%s:%d] -------\r\n", __FILE__, __LINE__);
    }


}

const char *ugfx_cmd_cfgload(uint16_t instance)
{
    unsigned int i=0;
    int status;
	char *buffer;

	buffer = NULL;
	(void)instance;

	buffer = gfxAlloc(CALIBRATION_SIZE);

	/* read configuration from the flash */
	for (i=0; i < CALIBRATION_SIZE / FLASH_BLOCKSIZE; i++)
	{
		status = flashRead(FLASH_CONFIG_BASEADDR + (i * FLASH_BLOCKSIZE), buffer + (i * 4), FLASH_BLOCKSIZE);
		if (status != FLASH_RETURN_SUCCESS)
		{
			if (gSDU1)
			{
				chprintf((BaseSequentialStream *) gSDU1, "%d. block: Reading returned %d \r\n", i + 1, status);

				/* clean the buffer on a faulty configuration */
				gfxFree(buffer);
				buffer = NULL;

				return buffer;
			}
		}
	}

	return buffer;
}

void ugfx_cmd_shell(BaseSequentialStream *chp, int argc, char *argv[])
{
	chprintf(chp, "GUI UTIL\r\n"
			"Possible arguments are:\r\n"
			"- calibrate\tCalibrate the touchscreen\r\n");

	/* Handle warnings: */
	if (argc >= 1)
	{
		/* stop the fullcirce stuff */
		fcscheduler_stopThread();

		if (PARAM_CMP(argv[0], "calibrate")) == 0)
		{
			ginputCalibrateMouse(0);
		}
	}
}
