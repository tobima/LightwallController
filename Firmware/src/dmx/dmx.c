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
static uint8_t gLedToggle = 0;

/*
 * GPT3 callback.
 */
static void
gpt2cb(GPTDriver *gptp)
{
  (void) gptp;
}

/*
 * This callback is invoked when a transmission buffer has been completely
 * read by the driver.
 */
static void
txend1(UARTDriver *uartp)
{
  (void) uartp;
}

/*
 * This callback is invoked when a transmission has physically completed.
 */
static void
txend2(UARTDriver *uartp)
{
  (void) uartp;

  chSysLockFromIsr()
  ;
  if (chSemGetCounterI(&sem) < 0)
    chSemSignalI(&sem);
  chSysUnlockFromIsr();
}

/*
 * This callback is invoked on a receive error, the errors mask is passed
 * as parameter.
 */
static void
rxerr(UARTDriver *uartp, uartflags_t e)
{
  (void) uartp;
  (void) e;
}

/*
 * This callback is invoked when a character is received but the application
 * was not ready to receive it, the character is passed as parameter.
 */
static void
rxchar(UARTDriver *uartp, uint16_t c)
{
  (void) uartp;
  (void) c;
}

/*
 * This callback is invoked when a receive buffer has been completely written.
 */
static void
rxend(UARTDriver *uartp)
{
  (void) uartp;
}

/**
 * Use the third UART to spread DMX into the world.
 */
static const UARTConfig uart3cfg =
  { txend1, txend2, rxend, rxchar, rxerr, 250000 /* 250kbaud */, 0,
      USART_CR2_STOP2_BITS, 0 };

static const GPTConfig gpt2cfg =
  { 200000, /* 200KHz timer clock.*/
  gpt2cb, /* Timer callback.*/
  0 };

void
DMXInit(void)
{
  chSemInit(&sem, 1);
  gptStart(&GPTD2, &gpt2cfg); // Another
  uartStart(&UARTD3, &uart3cfg);

  /* Set the initial length of DMX to one */
  dmx_buffer.length = 1;
}

/**
 * DMX thread.
 */
__attribute__((noreturn))
 msg_t
dmxthread(void *arg)
{
  (void) arg;
  chRegSetThreadName("dmx");

  /* Analysis:
   *
   *	Min		Max		Custom	Working reference
   * 1: 88		-		126 us	180
   * 2: 8		1s		14 us	
   * 3: 43,12	44,48	32 us
   * 8: 0		1s		14 us 
   *
   * More values from the working reference:
   * between to frames: 22 us
   * 78 us = start bit & start byte
   *
   * Source for numbers: http://www.soundlight.de/techtips/dmx512/dmx512.htm
   */

  while (1)
    {
      /* Send Reset. */
      palSetPadMode(GPIOD, GPIOD_DMX_BREAK, PAL_STM32_MODE_OUTPUT | PAL_STM32_OTYPE_PUSHPULL |PAL_STM32_PUDR_PULLDOWN); palClearPad(GPIOD, GPIOD_DMX_BREAK);
      gptPolledDelay(&GPTD2, 25); /* wait for 125 us */
      palSetPad(GPIOD, GPIOD_DMX_BREAK); palSetPadMode(GPIOD, GPIOD_DMX_BREAK, PAL_STM32_MODE_INPUT | PAL_STM32_PUDR_FLOATING);
      gptPolledDelay(&GPTD2, 2); /* Mark and Reset of 10 us */

      /* Send the startbyte */
      uartStartSend(&UARTD3, (size_t) 1, (void*) &(dmx_buffer.startbyte));

      /* send the universe */
      uartStartSend(&UARTD3, (size_t)(dmx_buffer.length),
          (void*) &(dmx_buffer.buffer));

      chSemWait(&sem);

      uartStopSend(&UARTD3);

      if (gLedToggle)
        {
          palSetPad(GPIOD, GPIOD_LED6); /* Blue.  */
          gLedToggle = 0;
        }
      else
        {
          palClearPad(GPIOD, GPIOD_LED6); /* Blue.  */
          gLedToggle = 1;
        }

    }

}
