#include "ch.h"
#include "chprintf.h"

void
cmd_threads(BaseSequentialStream *chp, int argc, char *argv[])
{
  static const char *states[] =
    { THD_STATE_NAMES };
  Thread *tp;

  (void) argv;
  if (argc > 0)
    {
      chprintf(chp, "Usage: threads\r\n");
      return;
    }
  chprintf(chp, "    addr    stack prio refs     state     time      name\r\n");
  tp = chRegFirstThread();
  do
    {
      chprintf(chp, "%.8lx %.8lx %4lu %4lu %9s %8lu %15s\r\n", (uint32_t) tp,
          (uint32_t) tp->p_ctx.r13, (uint32_t) tp->p_prio,
          (uint32_t)(tp->p_refs - 1), states[tp->p_state],
          (uint32_t) tp->p_time, tp->p_name);
      tp = chRegNextThread(tp);
    }
  while (tp != NULL);
}
