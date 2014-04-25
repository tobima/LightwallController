#include "ch.h"
#include "chprintf.h"
#include "flash.h"
#include "helper.h"

#define PRINTABLE_CHARACTER_FIRST 0x20
#define PRINTABLE_CHARACTER_LAST 0x7e
#define PRINTABLE_CHARACTER_COUNT (PRINTABLE_CHARACTER_LAST - PRINTABLE_CHARACTER_FIRST + 1)

#define TEST_READWRITE_TESTCASE_COUNT 8

/**
 * @brief The flash_sector_def struct describes a flash sector
 */
struct flash_sector_def
{
    flashsector_t index; ///< Index of the sector in the flash memory
    size_t size; ///< Size of the sector in bytes
    flashaddr_t beginAddress; ///< First address of the sector in memory (inclusive)
    flashaddr_t endAddress; ///< Last address of the sector in memory (exclusive)
};

/**
 * @brief Definition of the stm32f407 sectors.
 */
const struct flash_sector_def f407_flash[FLASH_SECTOR_COUNT] =
{
    { 0, 16 * 1024, 0x08000000, 0x08004000},
    { 1, 16 * 1024, 0x08004000, 0x08008000},
    { 2, 16 * 1024, 0x08008000, 0x0800C000},
    { 3, 16 * 1024, 0x0800C000, 0x08010000},
    { 4, 64 * 1024, 0x08010000, 0x08020000},
    { 5, 128 * 1024, 0x08020000, 0x08040000},
    { 6, 128 * 1024, 0x08040000, 0x08060000},
    { 7, 128 * 1024, 0x08060000, 0x08080000},
    { 8, 128 * 1024, 0x08080000, 0x080A0000},
    { 9, 128 * 1024, 0x080A0000, 0x080C0000},
    { 10, 128 * 1024, 0x080C0000, 0x080E0000},
    { 11, 128 * 1024, 0x080E0000, 0x08100000}
};

/**
 * @brief The test_flashreadwrite_data struct defines a test case data for the read/write test.
 */
struct test_flashreadwrite_data
{
    char* testName; ///< Name of the test case
    flashaddr_t from; ///< First address to be read/write (inclusive)
    flashaddr_t to; ///< Last address to be read/write (exclusive)
};

void cmd_flash(BaseSequentialStream *chp, int argc, char *argv[])
{
  int j;
  char readBufferBefore[32];
  flashsector_t i;
  chprintf(chp, "Flash test\r\n");

  for (i = 0; i < FLASH_SECTOR_COUNT; ++i)
  {
	  const struct flash_sector_def* sector = &f407_flash[i];
	  chprintf(chp, "\r\n> test sector %u: 0x%x -> 0x%x (%u bytes)\r\n\r\n",
	                   sector->index,
	                   sector->beginAddress,
	                   sector->endAddress,
	                   sector->size);
#if 0
	  flashRead(sector->beginAddress,
	  			  readBufferBefore, sector->size);
	  for(j=0; j < 32; j++)
	  {
		  chprintf(chp, "%0.2X", readBufferBefore[j]);
	  }
	  chprintf(chp, "\r\n");
#endif
  }

}
