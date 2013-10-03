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
  
void DMXInit(void) {
  chSemInit(&sem, 1);
	
  dmx_buffer.length = 513;
	
  /* Debug: fill the buffer with random values */
  dmx_buffer.buffer[0] = 0x00;
  dmx_buffer.buffer[1] = 0xff;
}

/**
 * DMX thread.
 */
__attribute__((noreturn))
msg_t dmxthread(void *arg) {
  (void)arg;
  chRegSetThreadName("dmx");
  
  while(TRUE) {	  
	  palSetPad(GPIOD, GPIOD_LED5);       /* Red  */
	  chThdSleepMilliseconds(250);
	  palClearPad(GPIOD, GPIOD_LED5);     /* Red.  */
	  chThdSleepMilliseconds(250);
//    chSemWait(&sem);    
  }
  
  
}
