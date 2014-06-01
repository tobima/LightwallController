#include <string.h>
#include <stdlib.h>
#include "ch.h"
#include "chprintf.h"
#include "flash.h"
#include "helper.h"

#define FLASH_BASEADDR				0x08060000
#define FLASH_BLOCKSIZE 			((int) sizeof(flashdata_t))
#define FLASH_BLOCK_TESTCASE_COUNT	16

void flash_usage(BaseSequentialStream *chp)
{
	chprintf(chp, "usage: [write|read]\r\n"
			"Parameter description\r\n"
			"---------------------\r\n"
			"read:  no additional parameter necessary\r\n"
			"write: (block offset) (value)\r\n");
}

void cmd_flash(BaseSequentialStream *chp, int argc, char *argv[])
{
	int j, blockoffset;
	int status, address;
	char readBufferBefore[32];
	flashsector_t i;
	chprintf(chp, "Flash test UTIL\r\n");

	/* Handle warnings: */
	if (argc >= 1)
	{
		if (strncmp(argv[0], "write", sizeof("write")) == 0)
		{
			if (argc >= 3)
			{
				blockoffset = atoi(argv[1]);
				if (blockoffset < 0 || blockoffset > FLASH_BLOCK_TESTCASE_COUNT)
				{
					chprintf(chp, "Offset must between %d and %d. Actual one was: %d \r\n",
							0, FLASH_BLOCK_TESTCASE_COUNT, blockoffset);
					return;
				}

				/* use the fourth memory block for the tests */
				address = FLASH_BASEADDR + (blockoffset * FLASH_BLOCKSIZE);

				/* extract the new value to store into it */
				readBufferBefore[0] = atoi(argv[2]);

				status = flashWrite(address, readBufferBefore, FLASH_BLOCKSIZE);
				if (status != FLASH_RETURN_SUCCESS)
				{
					chprintf(chp, "Writing returned %d \r\n", status);
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
					chprintf(chp, "%02X ", readBufferBefore[j]);
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
