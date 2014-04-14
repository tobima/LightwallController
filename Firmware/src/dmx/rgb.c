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


#define RGB_USAGE_HELP	"Possible commands are:\r\nfill\r\nset\r\n"

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
    else if (strcmp(argv[0], "set") == 0)
    {
	
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

