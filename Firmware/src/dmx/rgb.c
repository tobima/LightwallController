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


#define RGB_USAGE_HELP	"Possible commands are:\r\nfill\r\nwrite\r\n"

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

