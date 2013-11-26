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

#include "ch.h"
#include "hal.h"

#include "chprintf.h"
#include "shell.h"

void
cmd_dmx_modify(BaseSequentialStream *chp, int argc, char *argv[])
{
#define DMX_USAGE_HELP "Possible commands are:\r\nshow\tprint content\r\nwrite (offset) (value)\r\nfill (start offset) (end) (value)\r\nshrink (size)\tUpdate the length of the universe\r\n"

  if (argc < 1)
    {
      chprintf(chp, "Usage <command> (parameter)\r\n");
      chprintf(chp, DMX_USAGE_HELP);
      return;
    }

  if (argc >= 1)
    {
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

              if (dmx_buffer.length < offset)
                {
                  chprintf(chp, "Increased Universe from %d to %d bytes.\r\n",
                      dmx_buffer.length, offset + 1);
                  dmx_buffer.length = offset + 1;
                }

              dmx_buffer.buffer[offset] = value;
              chprintf(chp, "Set DMX at %d with 0x%2X (%d)\r\n", offset, value,
                  value);
            }
        }
      else if (strcmp(argv[0], "fill") == 0)
        {
          if (argc < 4)
            {
              chprintf(chp, "Usage: dmx fill (start offset) (end) (value)\r\n");
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

              if (dmx_buffer.length < end)
                {
                  chprintf(chp, "Increased Universe from %d to %d bytes.\r\n",
                      dmx_buffer.length, end + 1);
                  dmx_buffer.length = end + 1;
                }

              length = end - offset;
              for (; offset < end; offset++)
                {
                  dmx_buffer.buffer[offset] = value;
                }
              chprintf(chp, "Filled DMX with %2X (%d times)\r\n", value,
                  length);
            }
        }
      else if (strcmp(argv[0], "show") == 0)
        {
          int i;
          chprintf(chp, "DMX is filled with %d byte\r\n", dmx_buffer.length);
          for (i = 0; i < dmx_buffer.length; i++)
            {
              chprintf(chp, "%.2X", dmx_buffer.buffer[i]);
            }
          chprintf(chp, "\r\n");
        }
      else if (strcmp(argv[0], "shrink") == 0)
        {
          if (argc < 2)
            {
              chprintf(chp, "Usage: dmx shrink (universe size)\r\n");
            }
          else
            {
              int size = atoi(argv[1]);
              chprintf(chp,
                  "Update size of the universe from %d to %d bytes.\r\n",
                  dmx_buffer.length, size);
              dmx_buffer.length = size;
            }
        }
      else if (strcmp(argv[0], "help") == 0)
        {
          chprintf(chp, "Possible commands are:\r\n"
          DMX_USAGE_HELP);
        }
    }

}
