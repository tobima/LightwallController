#include "ch.h"
#include "hal.h"
#include "dmx.h"

/**
 * Stack area for the dmx thread.
 */
WORKING_AREA(wa_dmx, DMX_THREAD_STACK_SIZE);

#define FCSCHED_WALLCFG_FILE	"fc/conf/wall"


/** @var wallconf_t
 *  @brief This structure contains information about the physical wall
 */
typedef struct
{
  int width; /**< Horizontal count of boxes the physical installation */
  int height; /**< Vertical count of boxes the physical installation */
  int fps; /**< Framerate, the wall uses */
  int dimmFactor; /**< In percent -> 100 no change, 50 half the brightness */
  uint32_t *pLookupTable; /**< Memory to the Lookup table, must be freed after usage */
} wallconf_t;

/******************************************************************************
 * PROTOTYPE
 ******************************************************************************/

/** @fn void readConfigurationFile( extern wallconf_t )
 * @brief Read configuration file
 *
 * The static configuration with the wall.

 * @param[out]	pConfiguration	Read configuration
 * @return 0 on success,
 */
int readConfigurationFile(wallconf_t* pConfiguration);

/******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************/

/**
 * Interface, to fill new data on the DMX line.
 * YOU have to secure, that only one source is filling this buffer.
 */
static DMXBuffer dmx_buffer;

static Semaphore sem;



/*
 * GPT5 callback.
 */
static void
gpt5cb(GPTDriver *gptp)
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
static const UARTConfig uart1cfg =
  { txend1, txend2, rxend, rxchar, rxerr, 250000 /* 250kbaud */, 0,
      USART_CR2_STOP2_BITS, 0 };

static const GPTConfig gpt5cfg =
  { 200000, /* 200KHz timer clock.*/
  gpt5cb, /* Timer callback.*/
  0 };

/******************************************************************************
 * GLOBAL FUNCTIONS
 ******************************************************************************/

void
DMXInit(void)
{
  chSemInit(&sem, 1);
  gptStart(&GPTD5, &gpt5cfg); // Another

  /* Use UART 1 (UART3 is needed for the LCD) */
  uartStart(&UARTD1, &uart1cfg);

  /* Set the initial length of DMX to one */
  dmx_buffer.length = 1;

  /* Load wall configuration */
  readConfigurationFile(&wallcfg);
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
      palSetPadMode(GPIOD, GPIOD_DMX_BREAK, PAL_STM32_MODE_OUTPUT | PAL_STM32_OTYPE_PUSHPULL |PAL_STM32_PUDR_PULLDOWN);
      palClearPad(GPIOD, GPIOD_DMX_BREAK);
      gptPolledDelay(&GPTD5, 25); /* wait for 125 us */
      palSetPad(GPIOD, GPIOD_DMX_BREAK);
      palSetPadMode(GPIOD, GPIOD_DMX_BREAK, PAL_STM32_MODE_INPUT | PAL_STM32_PUDR_FLOATING);
      gptPolledDelay(&GPTD5, 2); /* Mark and Reset of 10 us */

      /* Send the startbyte */
      uartStartSend(&UARTD1, (size_t) 1, (void*) &(dmx_buffer.startbyte));

      /* send the universe */
      uartStartSend(&UARTD1, (size_t)(dmx_buffer.length),
          (void*) &(dmx_buffer.buffer));

      chSemWait(&sem);

      uartStopSend(&UARTD1);

    }

}

void dmx_getScreenresolution(int *pWidth, int *pHeight)
{

}

/******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/

/** @fn static int wall_handler(void* config, const char* section, const char* name, const char* value)
 * @brief Extract the configuration for the wall
 *
 * After using, the memory of this structure must be freed!
 *
 * @param[in]	config	structure, all found values are stored
 * @param[in]	section	section, actual found
 * @param[in]	name	key
 * @param[in]	value	value
 *
 * @return < 0 on errors
 */
static int
wall_handler(void* config, const char* section, const char* name,
    const char* value)
{
  wallconf_t* pconfig = (wallconf_t*) config;
  int row = strtol(section, NULL, 10);
  int col;
  int memoryLength = 0;
  int dmxval;

  if (MATCH("global", "width"))
    {
      pconfig->width = strtol(value, NULL, 10);
      FCSCHED_PRINT("Config: width: %3d\r\n", pconfig->width);
    }
  else if (MATCH("global", "height"))
    {
      pconfig->height = strtol(value, NULL, 10);
      FCSCHED_PRINT("Config: height: %3d\r\n", pconfig->height);
    }
  else if (MATCH("global", "fps"))
    {
      pconfig->fps = strtol(value, NULL, 10);
      FCSCHED_PRINT("Config: fps: %3d\r\n", pconfig->fps);
    }
  else if (MATCH("global", "dim"))
    {
      pconfig->dimmFactor = strtol(value, NULL, 10);
    }
   else if ((row >= 0) && (row < pconfig->height))
    {
      /* when the function was called the first time, take some memory */
      if (pconfig->pLookupTable == NULL)
        {
          memoryLength = sizeof(uint32_t) * pconfig->width * pconfig->height;
          pconfig->pLookupTable = chHeapAlloc(0, memoryLength);
          if (pconfig->pLookupTable == NULL)
          {
        	  FCSCHED_PRINT("%s Not enough memory to allocate %d bytes \r\n", __FILE__, memoryLength);
          }
		  /* Clean the whole memory: (dmxval is reused as index) */
		  for(dmxval=0; dmxval < memoryLength; dmxval++)
		  {
			pconfig->pLookupTable[dmxval] = 0;
		  }
        }
      col = strtol(name, NULL, 10);
      dmxval = (uint32_t) strtol(value, NULL, 10);
      FCSCHED_PRINT("Updated row: %3d\tcol: %3d\tdmx: %3d\r\n", row, col, dmxval);
      if ((row * pconfig->width + col) < (pconfig->width * pconfig->height) )
      {
        pconfig->pLookupTable[row * pconfig->width + col] = dmxval;
      }
      else
      {
    	  FCSCHED_PRINT("ERROR could not set dmxvalue %d\r\n", dmxval);
      }
    }
  else
    {
      return 0; /* unknown section/name, error */
    }
  return 1;
}


static int readConfigurationFile(wallconf_t* pConfiguration)
{
	if (pConfiguration == NULL)
	{
		/* ERROR! No configuration memory given */
		return 1;
	}

  FCSCHED_PRINT("Reading configuration file\r\n");
  hwal_memset(pConfiguration, 0, sizeof(wallconf_t));
  pConfiguration->dimmFactor = 100;
  pConfiguration->fps = -1;

  /* Load the configuration */
  return ini_parse(FCSCHED_WALLCFG_FILE, wall_handler, pConfiguration);
}

/**
 * Uses the configuration to update the DMX buffer out of the framebuffer with the current configuration.
 */
static void updateDMXbuffer()
{
	int row, col, offset;
	  dmx_buffer.length = width * height * 3;


	  if (pWallcfg && pWallcfg->height == height && pWallcfg->width == width)
	    {
	      for (row = 0; row < pWallcfg->height; row++)
	        {
	          for (col = 0; col < pWallcfg->width; col++)
	            {
	              offset = (row * pWallcfg->width + col);
	              dmx_buffer.buffer[pWallcfg->pLookupTable[offset] + 0] = dimmValue(
	                  pBuffer[offset * 3 + 0], pWallcfg->dimmFactor);
	              dmx_buffer.buffer[pWallcfg->pLookupTable[offset] + 1] = dimmValue(
	                  pBuffer[offset * 3 + 1], pWallcfg->dimmFactor);
	              dmx_buffer.buffer[pWallcfg->pLookupTable[offset] + 2] = dimmValue(
	                  pBuffer[offset * 3 + 2], pWallcfg->dimmFactor);
	            }
	        }
	    }
	  else
	    {

		  /*
				"WRONG Resolution: Wall has %d x %d, but file is %d x %d",
				wallcfg.width, wallcfg.height, width, height);
		   */

	      /* Set the DMX buffer directly */
	      memcpy(dmx_buffer.buffer, pBuffer, dmx_buffer.length);
	    }
	}

}

