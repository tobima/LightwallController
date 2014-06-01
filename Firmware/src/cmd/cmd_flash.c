#include <string.h>
#include <stdlib.h>
#include "ch.h"
#include "chprintf.h"
#include "flash.h"
#include "helper.h"

#define FLASH_BASEADDR				0x08060000
#define FLASH_BLOCKSIZE 			((int) sizeof(flashdata_t))
#define FLASH_BLOCK_TESTCASE_COUNT	8

void flash_usage(BaseSequentialStream *chp)
{
	chprintf(chp, "usage: [write|read]\r\n");
}

void cmd_flash(BaseSequentialStream *chp, int argc, char *argv[])
{
	int j;
	int status, address;
	char readBufferBefore[32];
	flashsector_t i;
	chprintf(chp, "Flash test UTIL\r\n");

	/* Handle warnings: */
	if (argc >= 1)
	{
		if (strncmp(argv[0], "write", sizeof("write")) == 0)
		{
			if (argc >= 2)
			{
				address = FLASH_BASEADDR;
				readBufferBefore[0] = atoi(argv[1]);

				status = flashWrite(address, readBufferBefore, FLASH_BLOCKSIZE);
				if (status != FLASH_RETURN_SUCCESS)
				{
					chprintf(chp, "Writing returned %d\r\n", status);
					return;
				}
				chprintf(chp, "test sector %u : stored at 0x%x %d\r\n",
						flashSectorAt(address), address, readBufferBefore[0]);
			}
			else
			{
				chprintf(chp, "flash needs additional parameter\r\n");
			}
		}
		else if (strncmp(argv[0], "read", sizeof("read")) == 0)
		{
			for (i = 0; i < FLASH_BLOCK_TESTCASE_COUNT; ++i)
			{
				address = FLASH_BASEADDR + (i * FLASH_BLOCKSIZE);
				chprintf(chp, "test sector %u\t: 0x%x -> 0x%x (%u bytes)\r\n",
						flashSectorAt(address), address,
						address + FLASH_BLOCKSIZE,
						FLASH_BLOCKSIZE);

				status = flashRead(address, readBufferBefore, FLASH_BLOCKSIZE);
				if (status != FLASH_RETURN_SUCCESS)
				{
					chprintf(chp, "Reading returned %d\r\n", status);
					return;
				}

				for (j = 0; j < FLASH_BLOCKSIZE; j++)
				{
					chprintf(chp, "%0.2X", readBufferBefore[j]);
				}
				chprintf(chp, "\r\n");

			}
		}
	}
	else
	{
		flash_usage(chp);
	}

}
