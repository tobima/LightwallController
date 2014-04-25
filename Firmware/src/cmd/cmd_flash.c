#include "ch.h"
#include "chprintf.h"
#include "flash.h"
#include "helper.h"

#define PRINTABLE_CHARACTER_FIRST 0x20
#define PRINTABLE_CHARACTER_LAST 0x7e
#define PRINTABLE_CHARACTER_COUNT (PRINTABLE_CHARACTER_LAST - PRINTABLE_CHARACTER_FIRST + 1)

#define TEST_BASE						0x08060000
#define TEST_DATASIZE 					sizeof(flashdata_t)
#define TEST_READWRITE_TESTCASE_COUNT	8

void cmd_flash(BaseSequentialStream *chp, int argc, char *argv[])
{
  int j;
  int status;
  char readBufferBefore[32];
  flashsector_t i;
  chprintf(chp, "Flash test\r\n");

  /* Handle warnings: */
  (void) argc;
  (void) argv;

  for (i = 0; i < TEST_READWRITE_TESTCASE_COUNT; ++i)
  {
	  int address = TEST_BASE + (i* TEST_DATASIZE);
	  chprintf(chp, "test sector %u\t: 0x%x -> 0x%x (%u bytes)\r\n",
	                   flashSectorAt(address),
	                   address,
	                   address + TEST_DATASIZE,
	                   TEST_DATASIZE);


	  status = flashRead(address,
	  			  readBufferBefore, TEST_DATASIZE);
	  if (status != FLASH_RETURN_SUCCESS)
	  {
		  chprintf(chp, "Reading returned %d\r\n", status);
		  return;
	  }

	  for(j=0; j < 32; j++)
	  {
		  chprintf(chp, "%0.2X", readBufferBefore[j]);
	  }
	  chprintf(chp, "\r\n");

  }

}
