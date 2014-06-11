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
#include "hwal.h"	/* necessary for custom created memset */
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
	int status;
	gSDU1 = pSDU1;

	if (gSDU1)
	{
		chprintf((BaseSequentialStream *) gSDU1, "Erase FLASH... \r\n");
	}

	status = flashErase(FLASH_CONFIG_BASEADDR, CALIBRATION_SIZE);
	if (status != FLASH_RETURN_SUCCESS)
	{
		if (gSDU1)
		{
			chprintf((BaseSequentialStream *) gSDU1, "Erasing returned %d \r\n", status);
		}
		return;
	}
	if (gSDU1)
	{
		chprintf((BaseSequentialStream *) gSDU1, "Erased at 0x%x %d bytes \r\n", FLASH_CONFIG_BASEADDR, CALIBRATION_SIZE);
	}

	ginputCalibrateMouse(0);
	return;
}

void ugfx_cmd_manualtesting(uint8_t status)
{
	/* Initiaize the font */
	font_t font = gdispOpenFont("DejaVu*");

	PRINT("%s Manual testing %d\r\n", __FILE__, status);
	if (status == UGFX_CMD_MANUAL_ENDED)
	{
		fcscheduler_startThread();
		ugfx_cmd_manualtesting_stop();
	}
	else if (status != UGFX_CMD_MANUAL_START)
	{
		return; /* Unkown command executed */
	}

	/********** stop normal mode and start the manual testing *********/

	/* Stop all fullcircle threads first */
	fcscheduler_stopThread();

	/* Wait until the thread is dead ... */
	gdispPrintf(0, gdispGetHeight() - 15, font, Red, 256,
				"Wait for deamon to stop.");
	while ( fcscheduler_isRunning() )
	{
		chThdSleep(MS2ST(50));
	}
	gdispPrintf(0, gdispGetHeight() - 15, font, Green, 256,
					"Stopped.");

	/* now start the manual tests */
	ugfx_cmd_manualtesting_init();

	/*********************
	 * Update the UI:
	 * - FIXME Boxes are clickable to be changed
	 */
}

void ugfx_cmd_cfgsave(uint16_t instance, const uint8_t *calbuf, size_t size)
{
  int status;
  (void)instance;
  const char* buffer = (const char*) calbuf;

  if (gSDU1)
  {
      chprintf((BaseSequentialStream *) gSDU1, "ugfx_cmd_cfgsave %s:%d\r\n", __FILE__, __LINE__);

        chprintf((BaseSequentialStream *) gSDU1, "Setting (%d bytes): ", size);
        for(status=0; status < (int) size; status++) /* reuse status as counter */
        {
            chprintf((BaseSequentialStream *) gSDU1, "%02X", calbuf[status]);
        }
        chprintf((BaseSequentialStream *) gSDU1, "\r\n");
    }

	status = flashWrite(FLASH_CONFIG_BASEADDR , buffer, size);
	if (status != FLASH_RETURN_SUCCESS)
	{
		if (gSDU1)
		{
			chprintf((BaseSequentialStream *) gSDU1, "Writing returned %d \r\n", status);
		}
		return;
	}
	else
	{
		if (gSDU1)
		{
			chprintf((BaseSequentialStream *) gSDU1, "Sector %u : stored at 0x%x ",
									flashSectorAt(FLASH_CONFIG_BASEADDR), FLASH_CONFIG_BASEADDR);

			chprintf((BaseSequentialStream *) gSDU1, "\r\n");
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
    int status;
	char *buffer;

	buffer = NULL;
	(void)instance;

	buffer = gfxAlloc(CALIBRATION_SIZE);

	status = flashRead(FLASH_CONFIG_BASEADDR, buffer, CALIBRATION_SIZE);
	if (status != FLASH_RETURN_SUCCESS)
	{
		if (gSDU1)
		{
			chprintf((BaseSequentialStream *) gSDU1, "Reading returned %d \r\n", status);

			/* clean the buffer on a faulty configuration */
			gfxFree(buffer);
			buffer = NULL;

			return buffer;
		}
	}


	return buffer;
}

void ugfx_cmd_shell(BaseSequentialStream *chp, int argc, char *argv[])
{
	int status;
	chprintf(chp, "GUI UTIL\r\n"
			"Possible arguments are:\r\n"
			"- calibrate\tCalibrate the touchscreen\r\n"
			"- erase\tErase the necessary flash for the calibration data\r\n");

	/* Handle warnings: */
	if (argc >= 1)
	{
		/* stop the fullcircel stuff */
		fcscheduler_stopThread();

		if (PARAM_CMP(argv[0], "calibrate")) == 0)
		{
			ginputCalibrateMouse(0);
		}
		else if (PARAM_CMP(argv[0], "erase")) == 0)
		{
			chprintf(chp, "Erase FLASH... \r\n");
			status = flashErase(FLASH_CONFIG_BASEADDR, CALIBRATION_SIZE);
			if (status != FLASH_RETURN_SUCCESS)
			{
				chprintf(chp, "Erasing returned %d \r\n", status);
				return;
			}
			chprintf(chp, "Erased at 0x%x %d bytes \r\n", FLASH_CONFIG_BASEADDR, CALIBRATION_SIZE);
		}
	}
}
