/*
^ *  dmx_cmd.c
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

uint8_t dmx_rgb_fade_cmd(uint8_t offset, uint8_t red, uint8_t green, uint8_t blue, uint32_t duration, 
					 BaseSequentialStream *chp);

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

	      /* Works perfect with an offset starting at zero */
          dmx_fb[(offset * DMX_RGB_COLOR_WIDTH) + 0] = red;
          dmx_fb[(offset * DMX_RGB_COLOR_WIDTH) + 1] = green;
          dmx_fb[(offset * DMX_RGB_COLOR_WIDTH) + 2] = blue;
          chprintf(chp, "Set DMX at %d with 0x%2X%2X%2X\r\n", (offset * 3),           			red, green, blue);
        }
    }
	else if (strcmp(argv[0], "fade") == 0)
    {
	    if (argc < 6)
        {
			chprintf(chp, "Usage: rgb fade (offset, starting from zero) (red) (green) (blue) (duration)\r\n");
        }
        else
        {
			int offset = atoi(argv[1]);
			int red = atoi(argv[2]);
			int green = atoi(argv[3]);
			int blue = atoi(argv[4]);
			int duration = atoi(argv[5]);
			
			
			dmx_rgb_fade_cmd(offset, red, green, blue, duration, chp);
        }
    }
}

void dmx_rgb_fill(uint8_t red, uint8_t green, uint8_t blue)
{
  int i;

  for (i=0; i <= DMX_BUFFER_MAX; i +=3)
  {
	  dmx_fb[i + 0] = red;
	  dmx_fb[i + 1] = green;
	  dmx_fb[i + 2] = blue;
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

typedef struct {
	int dmxOffset;				/**< For the algorithm, to know which address to update */
	BaseSequentialStream *chp;	/**< Necessary for debugging output */
} FadeParam_t;

void updateBuffer(uint8_t red, uint8_t green, uint8_t blue, void* pParam)
{
	FadeParam_t* p = (FadeParam_t*) pParam;
	
	if (pParam == NULL)
	{
		return; /* Fatal error, no parameter were given */
	}
	
	chprintf(p->chp, "Fade DMX at %d with 0x%2X%2X%2X \r\n", (p->dmxOffset),           	
			 red, green, blue); /*TODO remove debug code */
	
	/* Update the dmx buffer */
	dmx_fb[(p->dmxOffset) + 0] = red;
	dmx_fb[(p->dmxOffset) + 1] = green;
	dmx_fb[(p->dmxOffset) + 2] = blue;
}

uint8_t dmx_rgb_fade_cmd(uint8_t offset, uint8_t red, uint8_t green, uint8_t blue, uint32_t duration, 
					 BaseSequentialStream *chp)
{
	RGB24Color_t start;
	RGB24Color_t target;
	uint8_t returnValue = DMX_RGB_RET_OK;
	/* store some parameter in the structure for debuggin purposes */
	FadeParam_t param;
	param.chp = chp;
	
	if (((offset * 3) + 3) >= DMX_BUFFER_MAX)
	{
		return DMX_RGB_RET_ERR_MAXBUF;
	}
	
	param.dmxOffset = (offset * 3);
	

	/* Initialize the algorithm with the actual color */
	start.red   = dmx_fb[(offset * 3) + 0];
	start.green = dmx_fb[(offset * 3) + 1];
	start.blue  = dmx_fb[(offset * 3) + 2];
	
	
	/* prepare the struct for target color */
	target.red = red;
	target.green = green;
	target.blue = blue;
	
	dmx_rgb_fade(&start, &target, duration, &updateBuffer, (void *) &param);
	
	chprintf(chp, "-----------\r\n" );
	return returnValue;
}


uint8_t dmx_rgb_fade(RGB24Color_t* start, RGB24Color_t* target, uint32_t duration, FadeCallback_t onColorChange, void* pParam)
{
	uint8_t red, green, blue;
	int value, maximum;
	
	if (onColorChange == NULL || start == NULL || target == NULL)
	{
		return DMX_RGB_RET_ERR_PARAM;
	}
	
	/* initialize */
	red = start->red;
	green = start->green;
	blue = start->blue;
	
	/** the range, to walk through */
	maximum = CONVERT_RGB2INT(target->red, target->green, target->blue);
	value =	  CONVERT_RGB2INT(start->red, start->green, start->blue);
	while (value < maximum)
	{
		red = 0;
		blue = maximum;
		
		/* calculate the next color to show */
		rgb_rainbowcolor(value, &red, &blue, &green);
		
		/* inform the user what he needs to draw */
		onColorChange(red, green, blue, pParam);
		
		/*FIXME update intervall*/
		value += 10;
		chThdSleep(MS2ST(50 /* milliseconds */));
	}
	
	return DMX_RGB_RET_OK;
}
