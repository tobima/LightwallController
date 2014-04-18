/*
 *  dmx_cmd.c
 *
 *  Created by ollo on 14.04.14.
 *  Copyright 2013 C3MA. All rights reserved.
 *
 */

#include "rgb.h"
#include "dmx.h"

#include <string.h>
#include <stdlib.h>

#include "chprintf.h"
#include "shell.h"


#define RGB_USAGE_HELP	"Possible commands are:\r\nfill\r\nwrite\r\nfade\r\n"

#define COLORMAX	256 /**< Maximum for each color */

/** @define  CONVERT_RGB2INT(red, green, blue)
 * @brief converts the RGB24 value to one numeric value from 0 to 768 (3*COLORMAX)
 * Range for color red: 0 - 255
 * Range for color green: 256 - 511
 * Range for color blue: 512 - 768
 */
#define	CONVERT_RGB2INT(red, green, blue)	((red) + ((green) + COLORMAX) + ((blue) + COLORMAX + COLORMAX))

void dmx_rgb_modify(BaseSequentialStream *chp, int argc, char *argv[])
{
    if (argc < 1)
    {
      chprintf(chp, "Usage <command> (parameter)\r\n");
      chprintf(chp, RGB_USAGE_HELP);
      return;
    }

    if (strcmp(argv[0], "fill") == 0)
    {
     if (argc < 4)
     {
       chprintf(chp, "Usage: rgb fill (red) (green) (blue)\r\n");
     }
     else
     {
	int red = atoi(argv[1]);
	int green = atoi(argv[2]);
	int blue = atoi(argv[3]);
        dmx_rgb_fill(red, green, blue);
     }
    }
    else if (strcmp(argv[0], "write") == 0)
    {
	    if (argc < 5)
        {
           chprintf(chp, "Usage: rgb write (offset, starting from zero) (red) (green) (blue)\r\n");
        }
        else
        {
          int offset = atoi(argv[1]);
		  int red = atoi(argv[2]);
          int green = atoi(argv[3]);
          int blue = atoi(argv[4]);

          if (dmx_buffer.length < ((offset * 3) + 3) )
          {
            chprintf(chp, "Increased Universe from %d to %d bytes.\r\n",
              dmx_buffer.length, ((offset * 3) + 3) );
            dmx_buffer.length = ((offset * 3) + 3);
          }

	      /* Works perfect with an offset starting at zero */
          dmx_buffer.buffer[(offset * 3) + 0] = red;
	      dmx_buffer.buffer[(offset * 3) + 1] = green;
	      dmx_buffer.buffer[(offset * 3) + 2] = blue;
          chprintf(chp, "Set DMX at %d with 0x%2X%2X%2X\r\n", (offset * 3),           			red, green, blue);
        }
    }
	else if (strcmp(argv[0], "fade") == 0)
    {
	    if (argc < 6)
        {
			chprintf(chp, "Usage: rgb write (offset, starting from zero) (red) (green) (blue)\r\n");
        }
        else
        {
			int offset = atoi(argv[1]);
			int red = atoi(argv[2]);
			int green = atoi(argv[3]);
			int blue = atoi(argv[4]);
			int duration = atoi(argv[5]);
			
			
			dmx_rgb_fade(offset, red, green, blue, duration, chp);
        }
    }
}

void dmx_rgb_fill(uint8_t red, uint8_t green, uint8_t blue)
{
  int i;
  if (dmx_buffer.length < 3)
  {
	/* nothing to do :-) */
	return;
  }

  for (i=0; i <= (dmx_buffer.length - 3); i +=3)
  {
    dmx_buffer.buffer[i + 0] = red;
    dmx_buffer.buffer[i + 1] = green;
    dmx_buffer.buffer[i + 2] = blue;
  }
}


void rgb_rainbowcolor(uint16_t value, uint8_t* red, uint8_t* blue, uint8_t* green)
{
	// Convert into a value between 0 and 1023.
	int int_value = (int)(1023 * (value - *red) / (*blue - *red));
	
	// Map different color bands.
	if (int_value < 256)
	{
		// Red to yellow. (255, 0, 0) to (255, 255, 0).
		*red = 255;
		*green = int_value;
		*blue = 0;
	}
	else if (int_value < 512)
	{
		// Yellow to green. (255, 255, 0) to (0, 255, 0).
		int_value -= 256;
		*red = 255 - int_value;
		*green = 255;
		*blue = 0;
	}
	else if (int_value < 768)
	{
		// Green to aqua. (0, 255, 0) to (0, 255, 255).
		int_value -= 512;
		*red = 0;
		*green = 255;
		*blue = int_value;
	}
	else
	{
		// Aqua to blue. (0, 255, 255) to (0, 0, 255).
		int_value -= 768;
		*red = 0;
		*green = 255 - int_value;
		*blue = 255;
	}
}

uint8_t dmx_rgb_fade(uint8_t offset, uint8_t red, uint8_t green, uint8_t blue, uint32_t duration, 
					 BaseSequentialStream *chp)
{
	uint8_t red_start, green_start, blue_start;
	uint8_t returnValue = DMX_RGB_RET_OK;
	int value, maximum;
	
	if (((offset * 3) + 3) >= DMX_BUFFER_MAX)
	{
		return DMX_RGB_RET_ERR_MAXBUF;
	}
	
	if (dmx_buffer.length < ((offset * 3) + 3) )
	{
		chprintf(chp, "Increased Universe from %d to %d bytes.\r\n",
				 dmx_buffer.length, ((offset * 3) + 3) );
		dmx_buffer.length = ((offset * 3) + 3);
		returnValue = DMX_RGB_RET_INCREASE;
	}
	
	/* Initialize the algorithm with the actual color */
	red_start = dmx_buffer.buffer[(offset * 3) + 0];
	green_start = dmx_buffer.buffer[(offset * 3) + 1];
	blue_start = dmx_buffer.buffer[(offset * 3) + 2];
	
	/** the range, to walk through */
	maximum = CONVERT_RGB2INT(red, green, blue);
	value =	  CONVERT_RGB2INT(red_start, green_start, blue_start);
	while (value < maximum)
	{
		red_start = 0;
		blue_start = maximum;
		rgb_rainbowcolor(value, &red_start, &blue_start, &green_start);
		chprintf(chp, "Fade DMX at %d with 0x%2X%2X%2X [calculated: %d of %d]\r\n", (offset * 3),           	
				 red_start, green_start, blue_start,
				 value, maximum); /*TODO remove debug code */
		
		/*FIXME update intervall*/
		value += 10;
		chThdSleep(MS2ST(50 /* milliseconds */));
		
		/* Update the dmx buffer */
		dmx_buffer.buffer[(offset * 3) + 0] = red_start;
		dmx_buffer.buffer[(offset * 3) + 1] = green_start;
		dmx_buffer.buffer[(offset * 3) + 2] = blue_start;
	}
	
	return returnValue;
}
