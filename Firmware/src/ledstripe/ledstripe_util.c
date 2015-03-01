/*
 * ledstripe_util.c
 *
 *  Created on: Mar 1, 2015
 *  Author: ollo
 */

#include "ledstripe/ledstripe_util.h"
#include "ledstripe/ledstripe.h"

#include "ch.h"
#include "hal.h"

#include "hwal.h"	/* Needed for memcpy */

#ifndef DISABLE_FILESYSTEM
#include "ini/ini.h"
#include <string.h>
#include <stdlib.h>
#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
#endif


/** @var wallconf_t
 *  @brief This structure contains information about the physical wall
 */
typedef struct
{
  int width; /**< Horizontal count of boxes the physical installation */
  int height; /**< Vertical count of boxes the physical installation */
  int fps; /**< Framerate, the wall uses */
  int dimmFactor; /**< In percent -> 100 no change, 50 half the brightness */
  uint32_t *pLookupTable; /**< Memory to the Lookup table, must be freed after usage */
} wallconf_t;


/******************************************************************************
 * PROTOTYPE
 ******************************************************************************/

/** @fn void readConfigurationFile( extern wallconf_t )
 * @brief Read configuration file
 *
 * The static configuration with the wall.

 * @param[out]	pConfiguration	Read configuration
 * @return 0 on success,
 */
static int readConfigurationFile(wallconf_t* pConfiguration);

/******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************/

static wallconf_t wallcfg;

/******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/


static uint8_t
dimmValue(uint8_t incoming)
{
  uint32_t tmp = incoming;
  tmp = tmp * wallcfg.dimmFactor / 100;
  if (tmp > 255)
    tmp = 255;
  return (uint8_t) tmp;
}

/** @fn static int wall_handler(void* config, const char* section, const char* name, const char* value)
 * @brief Extract the configuration for the wall
 *
 * After using, the memory of this structure must be freed!
 *
 * @param[in]	config	structure, all found values are stored
 * @param[in]	section	section, actual found
 * @param[in]	name	key
 * @param[in]	value	value
 *
 * @return < 0 on errors
 */
static int
wall_handler(void* config, const char* section, const char* name,
    const char* value)
{
  wallconf_t* pconfig = (wallconf_t*) config;
  int row = strtol(section, NULL, 10);
  int col;
  int memoryLength = 0;
  int dmxval;

  if (MATCH("global", "width"))
    {
      pconfig->width = strtol(value, NULL, 10);
    }
  else if (MATCH("global", "height"))
    {
      pconfig->height = strtol(value, NULL, 10);
    }
  else if (MATCH("global", "fps"))
    {
      pconfig->fps = strtol(value, NULL, 10);
    }
  else if (MATCH("global", "dim"))
    {
      pconfig->dimmFactor = strtol(value, NULL, 10);
    }
   else if ((row >= 0) && (row < pconfig->height))
    {
      /* when the function was called the first time, take some memory */
      if (pconfig->pLookupTable == NULL)
        {
          memoryLength = sizeof(uint32_t) * pconfig->width * pconfig->height;
          pconfig->pLookupTable = chHeapAlloc(0, memoryLength);
          if (pconfig->pLookupTable == NULL)
          {
        	  /*FCSCHED_PRINT("%s Not enough memory to allocate %d bytes \r\n", __FILE__, memoryLength); */
          }
		  /* Clean the whole memory: (dmxval is reused as index) */
		  for(dmxval=0; dmxval < memoryLength; dmxval++)
		  {
			pconfig->pLookupTable[dmxval] = 0;
		  }
        }
      col = strtol(name, NULL, 10);
      dmxval = (uint32_t) strtol(value, NULL, 10);

      if ((row * pconfig->width + col) < (pconfig->width * pconfig->height) )
      {
        pconfig->pLookupTable[row * pconfig->width + col] = dmxval;
      }

    }
  else
    {
      return 0; /* unknown section/name, error */
    }
  return 1;
}
static int readConfigurationFile(wallconf_t* pConfiguration)
{
	if (pConfiguration == NULL)
	{
		/* ERROR! No configuration memory given */
		return 1;
	}

  memset(pConfiguration, 0, sizeof(wallconf_t));
  pConfiguration->dimmFactor = 100;
  pConfiguration->fps = -1;

  /* Load the configuration */
  return ini_parse(FCSCHED_WALLCFG_FILE, wall_handler, pConfiguration);
}

/******************************************************************************
 * GLOBAL FUNCTIONS
 ******************************************************************************/

void ledstripe_util_Init(void)
{
  ledstripe_init();

  /* Load wall configuration */
  memset(&wallcfg, 0, sizeof(wallconf_t));
  readConfigurationFile(&wallcfg);
}


void ledstripe_util_button_demo(BaseSequentialStream * chp)
{
    int i, red, green, blue, offset = 0;

	if (ledstripe_framebuffer[offset].red > 0
			&& ledstripe_framebuffer[offset].green >0
			&& ledstripe_framebuffer[offset].blue >0)
	{
		red = 255;
		green = 0;
		blue = 0;
	}
	else if (ledstripe_framebuffer[offset].red > 0)
	{
		red = 0;
		green = 255;
		blue = 0;
	}
	else if (ledstripe_framebuffer[offset].green > 0)
	{
		red = 0;
		green = 0;
		blue = 255;
	}
	else if (ledstripe_framebuffer[offset].blue > 0)
	{
		red = 0;
		green = 0;
		blue = 0;
	}
	else
	{
		red = green = blue = 255;
	}
	chprintf(chp, "Set %2X%2X%2X (RRGGBB)\r\n", red, green, blue);

	/* Update the end of the stripe */
	for(i=offset; i < LEDSTRIPE_FRAMEBUFFER_SIZE; i++) {
		ledstripe_framebuffer[i].red = red;
		ledstripe_framebuffer[i].green = green;
		ledstripe_framebuffer[i].blue = blue;
	}
}

void ledstripe_util_update(uint8_t* rgb24, int width  , int height)
{
	int row, col, offset;
	/* no configuration is present, the mapping could not be done */
	if (wallcfg.pLookupTable)
	{
	  for (row = 0; row < wallcfg.height; row++)
		{
		  for (col = 0; col < wallcfg.width; col++)
			{
			  offset = (row * wallcfg.width + col);
			  ledstripe_framebuffer[offset].red = dimmValue(
					  rgb24[offset * 3 + 0]);
			  ledstripe_framebuffer[offset].green = dimmValue(
					  rgb24[offset * 3 + 1]);
			  ledstripe_framebuffer[offset].blue = dimmValue(
					  rgb24[offset * 3 + 2]);
			}
		}
	}
	else
	{
	  /* Set the LED buffer directly */
	  hwal_memcpy(ledstripe_framebuffer, rgb24, width * height * 3 /* FIXE remove the dirty hack */);
	}
}
