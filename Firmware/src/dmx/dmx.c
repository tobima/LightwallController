#include "ch.h"
#include "hal.h"
#include "dmx.h"


/**
 * Stack area for the dmx thread.
 */
WORKING_AREA(wa_dmx, DMX_THREAD_STACK_SIZE);

/**
 * Interface, to fill new data on the DMX line.
 * YOU have to secure, that only one source is filling this buffer.
 */
DMXBuffer dmx_buffer;



static Semaphore sem;

void DMXInit(void)
{
  chSemInit(&sem, 1);
	
  /* Set the initial length of DMX to one */
  dmx_buffer.length = 1;
}

/**
 * DMX thread.
 */
__attribute__((noreturn))
msg_t dmxthread(void *arg) {
  (void)arg;
  chRegSetThreadName("dmx");
	
  
  while(1)
  {
    /* Send Reset. */
    palSetPadMode(GPIOD, GPIOD_DMX_BREAK, PAL_STM32_MODE_OUTPUT | PAL_STM32_OTYPE_PUSHPULL |PAL_STM32_PUDR_PULLDOWN);
    palClearPad(GPIOD, GPIOD_DMX_BREAK);
    gptPolledDelay(&GPTD2, 25);
    palSetPad(GPIOD, GPIOD_DMX_BREAK);
    palSetPadMode(GPIOD, GPIOD_DMX_BREAK, PAL_STM32_MODE_INPUT | PAL_STM32_PUDR_FLOATING);
    gptPolledDelay(&GPTD2, 2);
    
    uartStartSend(&UARTD3, (size_t) (dmx_buffer.length), (void*) &(dmx_buffer.buffer));
    
    chSemWait(&sem);
    
    uartStopSend(&UARTD3);
  }
  
  
}
