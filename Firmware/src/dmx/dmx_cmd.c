/*
 *  dmx_cmd.c
 *
 *  Created by ollo on 04.10.13.
 *  Copyright 2013 C3MA. All rights reserved.
 *
 */

#include "dmx_cmd.h"
#include "dmx.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "ch.h"
#include "hal.h"

#include "chprintf.h"
#include "shell.h"

void
cmd_dmx_modify(BaseSequentialStream *chp, int argc, char *argv[])
{
#define DMX_USAGE_HELP "Possible commands are:\r\nshow\tprint content\r\nwrite (offset) (value)\r\nfill (start offset) (end) (value)\r\nfill (start offset) (hex to send, without spaces!)\r\n"

  if (argc < 1)
    {
      chprintf(chp, "Usage <command> (parameter)\r\n");
      chprintf(chp, DMX_USAGE_HELP);
      return;
    }

  if (argc >= 1)
    {
	  /* Always deactivate the mapping logic to convert framebuffer offsets to DMX offsets */
	  dmx_update(0, 0);

      if (strcmp(argv[0], "write") == 0)
        {
          if (argc < 3)
            {
              chprintf(chp, "Usage: dmx write (offset) (value)\r\n");
            }
          else
            {
              int offset = atoi(argv[1]);
              int value = atoi(argv[2]);

	      /*Offset adaption: */
	      offset--;

              dmx_fb[offset] = value;
              chprintf(chp, "Set DMX at %d with 0x%2X (%d)\r\n", offset, value,
                  value);
            }
        }
      else if (strcmp(argv[0], "fill") == 0)
        {
	  if (argc < 3)
          {
              chprintf(chp, "Usage: dmx fill (start offset) (end) (value)\r\n");
              chprintf(chp, "Usage: dmx fill (start offset) (hex values, without spaces)\r\n");
	      return;
	  }
          else if (argc < 4)
            {
		char tmpHex[3];
		uint8_t* tmpDMX;
		int i=0;
		int offset = atoi(argv[1]);
		int strLength = strlen(argv[2]);
		int length = strLength / 2; /* HEX values -> therefore divided by two) */
		long value;
		char* end;
		if (length <= 0 || (offset + length) >= DMX_BUFFER_MAX || (strLength % 2 != 0))
		{
		 chprintf(chp, "Could not extract HEX value, maximum of %d (got %d, %s, must be even)\r\n", DMX_BUFFER_MAX, (offset + length), (strLength % 2) ? "odd" : "even" );
		 chprintf(chp, "Usage: dmx fill (start offset) (hex values, without spaces)\r\n");		
		 return;
		}
		
		chprintf(chp, "Updating %d bytes beginning at %d\r\n", length, offset);
		/* allocate working buffer */		
		tmpDMX = chHeapAlloc(NULL, length);
		if (tmpDMX == 0)
		{
		  chprintf(chp, "Could allocate memory\r\n");
		  return;
		}

		/* valid data to write on the buffer */
		for(i = 0; i < length; i++)
		{
		  memcpy( tmpHex, (argv[2] + (i*2)), 2);
		  tmpHex[2] = 0; /* mark the last as end */	
		  
		  value =  strtol(tmpHex, &end, 16);
		  tmpDMX[i] = (uint8_t) value;
		  if ( *end )
		  {
		    chprintf(chp, "Could not extract '%s'\r\n", tmpHex); 
		    chHeapFree(tmpDMX);
		    return;
		  }
		}

		/* Update the DMX buffer */
		memcpy( &(dmx_fb[offset]), tmpDMX, length);

		/* clean the memory again */
		chHeapFree(tmpDMX);
            }
          else
            {
              int offset = atoi(argv[1]);
              int end = atoi(argv[2]);
              int value = atoi(argv[3]);
              int length;

              /* swap, if the user cannot determine the lower number */
              if (end < offset)
                {
                  int tmp = offset;
                  offset = end;
                  end = tmp;
                }

              length = end - offset;
              for (; offset < end; offset++)
                {
                  dmx_fb[offset] = value;
                }
              chprintf(chp, "Filled DMX with %2X (%d times)\r\n", value,
                  length);
            }
        }
      else if (strcmp(argv[0], "show") == 0)
        {
          int i, width, height = 0;
          dmx_getScreenresolution(&width, &height);

          if (width == 0 && height == 0)
          { /* Display the complete DMX universe */
        	  for (i = 0; i < DMX_BUFFER_MAX; i++)
			  {
				  chprintf(chp, "%.2X%", dmx_fb[i]);
			  }
          }
          else
          {	  /* We have valid information! Let's display the wall on the shell */
			  chprintf(chp, "DMX is filled with %d x %d pixel\r\n", width, height);
			  for (i = 0; i < width * height; i++)
			  {
				  chprintf(chp, "%.2X%.2X%.2X|",
						  dmx_fb[i * DMX_RGB_COLOR_WIDTH + 0],
						  dmx_fb[i * DMX_RGB_COLOR_WIDTH + 1],
						  dmx_fb[i * DMX_RGB_COLOR_WIDTH + 2]);

				  /* generate a new line after each row */
				  if ((i + 1) % width == 0)
				  {
					  chprintf(chp, "\r\n");
				  }
			  }
			  chprintf(chp, "\r\n");
          }
        }
      else if (strcmp(argv[0], "help") == 0)
        {
          chprintf(chp, "Possible commands are:\r\n"
          DMX_USAGE_HELP);
        }
    }

}
