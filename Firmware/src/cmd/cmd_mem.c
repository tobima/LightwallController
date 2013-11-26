#include "ch.h"
#include "chprintf.h"

void
cmd_mem(BaseSequentialStream *chp, int argc, char *argv[])
{
  size_t n, size;

  (void) argv;
  if (argc > 0)
    {
      chprintf(chp, "Usage: mem\r\n");
      return;
    }
  n = chHeapStatus(NULL, &size);
  chprintf(chp, "core free memory : %u bytes\r\n", chCoreStatus());
  chprintf(chp, "heap fragments   : %u\r\n", n);
  chprintf(chp, "heap free total  : %u bytes\r\n", size);
}
