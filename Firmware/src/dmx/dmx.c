#include "ch.h"
#include "hal.h"
#include "dmx.h"

/* Lenght of dmxFrameBuffer,
 * that descibes one byte including start-bit, stop-bits
 * and mark between two bytes/channel (intertigit) */
#define DMX_FORMAT_MAX 20
#define DMX_FORMAT_WITHOUT_INTERFRAMESPACE 11

#define DMX_BREAK 0
#define DMX_MARK 1

/****** define some constants for the reset; IMPORTANT: on tick = 4us *******/
#define COUNTER_RESET_END 22 /* Last tick of RESET */
#define COUNTER_MARK_END (COUNTER_RESET_END + 2) /* Last tick of the MARK */
#define COUNTER_PREAMPLE_END (COUNTER_MARK_END + DMX_FORMAT_WITHOUT_INTERFRAMESPACE) + 1

#define COUNTER_POSTMARK (COUNTER_PREAMPLE_END + 10) /* first tick of the end of a frame */
#define COUNTER_POSTMARK_END (COUNTER_POSTMARK + 50) /* last tick of the end of a frame */

#define EXTR_LEVEL(a) (((a) > 0) ? DMX_MARK : DMX_BREAK)

typedef enum dmx_states_e { Idle, Reset, Stop, Start, Send } dmx_states_t;
typedef enum dmx_mode_e	{ Continous, Single, Off} dmx_mode_t;



static uint8_t dmx_frame_ready = 00;
static dmx_mode_t dmx_mode = Off;


/**
 * Stack area for the dmx thread.
 */
WORKING_AREA(wa_dmx, DMX_THREAD_STACK_SIZE);

/**
 * Interface, to fill new data on the DMX line.
 * YOU have to secure, that only one source is filling this buffer.
 */
DMXBuffer dmx_buffer;



void handler(void)
{
	
	static DMXBuffer current_buffer;
	static dmx_states_t state = Idle;
	static uint8_t counter;
	static uint16_t channel_counter = 0;
	static uint8_t current_channel;
	
	static uint8_t out = 0;

	/* set the pin */
	if (out)
	{
		palSetPad(GPIOD, GPIOD_LED5);       /* Red  */
	}
	else
	{
		palClearPad(GPIOD, GPIOD_LED5);     /* Red.  */
	}
	
	switch(state)
	{
		case(Idle):
			out = 1;
			switch(dmx_mode)
			{
				case(Single):
					if(dmx_frame_ready)
					{
						/* swap_frame */
						current_buffer = dmx_buffer;
						
						state = Reset;
						counter = 0;
					}
					break;
				case(Continous):
					/* swap_frame */
					current_buffer = dmx_buffer;
					
					state = Reset;
					counter = 0;
					break;
				default:
				case(Off):
					break;
			}
			break;
		case(Reset):
			out = 0;
			if(++counter >= COUNTER_RESET_END)
			{
				state = Stop;
				channel_counter = -1;
				counter = 0;
			}
			break;
		case(Stop):
			out = 1;
			if(counter++ >= 1)
			{
				state = Start;
				channel_counter++;
				if(channel_counter == 0)
				{
					current_channel = 0x00;
					state = Start;
				}
				else if (channel_counter > current_buffer.length)
				{
					state = Idle;
				}
				else
				{
					current_channel = current_buffer.buffer[channel_counter-1];
					state = Start;
					/*FIXME: port: trigger the uC, that can now do something else */
				}
			}
			break;
		case(Start):
			out = 0;
			state = Send;
			counter = 0;
            /*FIXME: port: NVIC_EnableIRQwod(TIMER_32_0_IRQn); */
			break;
		case(Send):
			out = current_channel & 0x1;
			current_channel >>= 1;
			if(++counter >= 8)
			{
				state = Stop;
				counter = 0;
				
                /*FIXME: prot NVIC_EnableIRQwoe(TIMER_32_0_IRQn); */
			}
			break;
	}
}


CH_IRQ_HANDLER(myIRQ)
{
	CH_IRQ_PROLOGUE();
	
	/* IRQ handling code, preemptable if the architecture supports it.*/
	static uint8_t out = 0;
	
	
	/* set the pin */
    if (out)
    {
        palSetPad(GPIOD, GPIOD_LED5);       /* Red  */
        out = 0;
    }
    else
    {
        palClearPad(GPIOD, GPIOD_LED5);     /* Red.  */
        out = 1;
    }
	
	/* More IRQ handling code, again preemptable.*/
	
	CH_IRQ_EPILOGUE();
}

static Semaphore sem;

void DMXInit(void)
{
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
	  /*FIXME: Thread is probalbly not needed */
		
	  palSetPad(GPIOD, GPIOD_LED4);
	  chThdSleepMilliseconds(250);
	  palClearPad(GPIOD, GPIOD_LED4);
	  chThdSleepMilliseconds(250);
//    chSemWait(&sem);    
  }
  
  
}
