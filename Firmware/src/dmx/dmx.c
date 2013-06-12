#include "ch.h"
#include "hal.h"

#include "dmx.h"

/**
 * Stack area for the http thread.
 */
WORKING_AREA(wa_dmx, DMX_THREAD_STACK_SIZE);
DMXBuffer dmx_buffer;


    /*
    * GPT3 callback.
    */
    static void gpt2cb(GPTDriver *gptp) {
       (void) gptp;
    }
    
static Semaphore sem;
    
/*
 * This callback is invoked when a transmission buffer has been completely
 * read by the driver.
 */
static void txend1(UARTDriver *uartp) {
  (void)uartp;
}

/*
 * This callback is invoked when a transmission has physically completed.
 */
static void txend2(UARTDriver *uartp) {
  (void)uartp;
  
  chSysLockFromIsr();
  if (chSemGetCounterI(&sem) < 0)
    chSemSignalI(&sem);
  chSysUnlockFromIsr();
}

/*
 * This callback is invoked on a receive error, the errors mask is passed
 * as parameter.
 */
static void rxerr(UARTDriver *uartp, uartflags_t e) {
  (void)uartp;
  (void)e;
}

/*
 * This callback is invoked when a character is received but the application
 * was not ready to receive it, the character is passed as parameter.
 */
static void rxchar(UARTDriver *uartp, uint16_t c) {
  (void)uartp;
  (void)c;
}

/*
 * This callback is invoked when a receive buffer has been completely written.
 */
static void rxend(UARTDriver *uartp) {
  (void)uartp;
}
    
static const UARTConfig uart3cfg = {
  txend1,
  txend2,
  rxend,
  rxchar,
  rxerr,
  250000 /* 250kbaud */,
  0,
  USART_CR2_STOP2_BITS | USART_CR2_LINEN,
  0
};

    static const GPTConfig gpt2cfg = { 200000, /* 100KHz timer clock.*/
      gpt2cb /* Timer callback.*/
    };

void DMXInit(void) {
  chSemInit(&sem, 1);
  gptStart(&GPTD2, &gpt2cfg);
  uartStart (&UARTD3, &uart3cfg);
  
  dmx_buffer.length = 513;
  dmx_buffer.buffer[0] = 0x00;
  dmx_buffer.buffer[1] = 0xff;
}

/**
 * HTTP server thread.
 */
__attribute__((noreturn))
msg_t dmxthread(void *arg) {
  (void)arg;
  chRegSetThreadName("dmx");
  
  while(1) {
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
