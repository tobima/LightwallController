/** @file fcscheduler.c
 * @brief Brain of Lightwall controller
 * @author Ollo
 *
 * @date 07.11.2013 - Created
 * @defgroup LightwallController
 *
 */

#include "fcscheduler.h"
#include <stdio.h>
#include <string.h>
#include "chprintf.h"
#include "ch.h"

#include "ini/ini.h"
#include <string.h>
#include <stdlib.h>

#include "hwal.h"	/* Needed for memcpy */
#include "fcstatic.h"
#include "customHwal.h"	/* Needed to activate debugging in server implementation */

#include "fcseq.h"
#include "fcserverImpl.h"

#include "fcserver.h" /* Necessary the timing supervision */
#include "dmx/dmx.h"

#define FCSCHED_WALLCFG_FILE	"fc/conf/wall"
#define FCSCHED_CONFIG_FILE     "fc/conf/controller"
#define FCSCHED_FILE_ROOT			"fc/static\0"	/**< Folder on the sdcard to check */

#define	FILENAME_LENGTH			512	/**< Including the absolut path to the file */
#define INPUT_MAILBOX_SIZE		4

#define MAXIMUM_INITIALIZATION			10
#define FCSCHED_DYNSERVER_RESETVALUE    (FRAME_ALIVE_STARTLEVEL * 2)

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

#define FCSCHED_PRINT( ... )	if (gDebugShell) { chprintf(gDebugShell, __VA_ARGS__); }

#define	MSG_ACTIVATE_SHELL	1
#define	MSG_SETFPS		2
#define	MSG_DIMM		3
#define MSG_STOPP               4

/** @typedef fcsource_state_t
 * @brief Status of the actual used source.
 *
 *
 * @dot
 digraph G {
 edge [fontsize=10];
 start [shape = rect];
 start -> nobody [label="Starting"];
 nobody -> file [label="Nobody is sending DMX"];
 file -> fileended [label="Last Frame sent"];
 fileended -> file [label="Goto next file"];
 fileended -> network [label = "Client is waiting"];
 network -> file [label = "Client disconnected"];
 }
 @enddot
 */
typedef enum
{
  FCSRC_STATE_NOBODY = 0, /**< Nobody is writing into the DMX buffer  */
  FCSRC_STATE_FILE, /**< Actual sending frames from a file  */
  FCSRC_STATE_FILEENDED, /**< Need to find the next file to play */
  FCSRC_STATE_NETWORK /**< Someone is streaming dynamic content to us */
} fcsource_state_t;

/******************************************************************************
 * GLOBAL VARIABLES for this module
 ******************************************************************************/

uint32_t* gFcBuf4DynQueue;
Mailbox * gFcMailboxDyn;

/******************************************************************************
 * LOCAL VARIABLES for this module
 ******************************************************************************/

static BaseSequentialStream * gDebugShell = NULL;

/* Mailbox, checked by the fullcircle scheduler thread */
static uint32_t buffer4mailbox2[INPUT_MAILBOX_SIZE];
static MAILBOX_DECL(mailboxIn, buffer4mailbox2, INPUT_MAILBOX_SIZE);

static wallconf_t wallcfg;

static fcsource_state_t gSourceState = FCSRC_STATE_NOBODY;

uint32_t gFcConnectedClients = 0;

static uint32_t gDynamicServerTimeout = FCSCHED_DYNSERVER_RESETVALUE;
static uint8_t  gSchedulerActive = TRUE;

/******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/

/**
 * @fn static int fcsched_handleInputMailbox(void)
 *
 * Handle all incoming messages from the mailbox
 * @return 1 if the loop must be skipped completely
 */
static int fcsched_handleInputMailbox(void)
{
  msg_t msg1, msg2, status;
  int newMessages;
  int retStatus = 0;
  /* Use nonblocking function to count incoming messages */
  newMessages = chMBGetUsedCountI(&mailboxIn);

  if (newMessages >= 2)
    {
      /* First retrieve the given pointer */
      status = chMBFetch(&mailboxIn, &msg1, TIME_INFINITE);
      if (status == RDY_OK)
        {
          status = chMBFetch(&mailboxIn, &msg2, TIME_INFINITE);
          if (status == RDY_OK)
            {
              chSysLock()
              ;
              switch ((uint32_t) msg1)
                {
              case MSG_ACTIVATE_SHELL:
                gDebugShell = (BaseSequentialStream *) msg2;
                hwal_init((BaseSequentialStream *) msg2);
                break;
              case MSG_SETFPS:
                wallcfg.fps = (int) msg2;
                break;
              case MSG_DIMM:
                wallcfg.dimmFactor = (int) msg2;
                break;
              case MSG_STOPP:
                gSchedulerActive = 0;
                gFcServerActive = TRUE; /* The server has an GO */
                palSetPad(GPIOD, GPIOD_LED4); /* Green.  */
                palClearPad(GPIOD, GPIOD_LED5); /* Red.  */
                retStatus = 1;
              default:
                break;
                }
              chSysUnlock();
            }
        }
    }
   return retStatus;
}

/** @fn static void fcsched_handleFcMailboxDyn(uint32_t sleeptime)
 * Check if the server code is still acting and the client sends a new frame
 *
 * @param[in] sleeptime cycle this function is called again, to update the timeout supervision correctly
 */
static void fcsched_handleFcMailboxDyn(uint32_t sleeptime)
{
  msg_t msg1, status;
  int newMessages;

  /* Use nonblocking function to count incoming messages */
  newMessages = chMBGetUsedCountI(gFcMailboxDyn);

  if (newMessages)
    {
      status = chMBFetch(gFcMailboxDyn, &msg1, TIME_INFINITE);
      if (status == RDY_OK)
        {
          chSysLock();
          FCSCHED_PRINT("%d\r\n", ((uint32_t ) msg1));
          chSysUnlock();
          gDynamicServerTimeout = FCSCHED_DYNSERVER_RESETVALUE;
        }
    }
  else
    {
      if (gSourceState == FCSRC_STATE_NETWORK)
        {
          FCSCHED_PRINT("FcDyn timeout %6d / %6d [ms]\r\n",
              FCSCHED_DYNSERVER_RESETVALUE - gDynamicServerTimeout, FCSCHED_DYNSERVER_RESETVALUE);

          if (gDynamicServerTimeout == 0)
            {
              /* Monitoring is deactivated */
            }
          /* Check if the counter is outside of its borders */
          else if (gDynamicServerTimeout <= sleeptime
              || gDynamicServerTimeout > FCSCHED_DYNSERVER_RESETVALUE)
          {
              gDynamicServerTimeout = 0; /* deactivate monitoring */
              FCSCHED_PRINT("%d wall is dead. (actual %d clients connected)\r\n",
                  gSourceState, gFcConnectedClients);
              gSourceState = FCSRC_STATE_NOBODY;

              /* Update the client amount, as one client died */
              if (gFcConnectedClients > 0)
                {
                  gFcConnectedClients--;
                }
          }
          else
          {
              gDynamicServerTimeout -= sleeptime;
          }
        }
    }
}

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
  uint32_t dmxval;

  if (MATCH("global", "width"))
    {
      pconfig->width = strtol(value, NULL, 10);
    }
  else if (MATCH("global", "height"))
    {
      pconfig->height = strtol(value, NULL, 10);
    }
  else if (MATCH("global", "fps"))
    {
      pconfig->fps = strtol(value, NULL, 10);
    }
  else if (MATCH("global", "dim"))
    {
      pconfig->dimmFactor = strtol(value, NULL, 10);
    }
  if ((row >= 0) && (row < pconfig->height))
    {
      /* when the function was called the first time, take some memory */
      if (pconfig->pLookupTable == NULL)
        {
          col = sizeof(uint32_t) * pconfig->width * pconfig->height;
          pconfig->pLookupTable = chHeapAlloc(0,
              col /* reused for memory length */);
        }
      col = strtol(name, NULL, 10);
      dmxval = (uint32_t) strtol(value, NULL, 10);
      pconfig->pLookupTable[row * pconfig->width + col] = dmxval;
    }
  else
    {
      return 0; /* unknown section/name, error */
    }
  return 1;
}

/** @fn static int configuration_handler(void* config, const char* section, const char* name, const char* value)
 * @brief Extract the configuration for the scheduler
 *
 * @param[in]   config  structure, all found values are stored
 * @param[in]   section section, actual found
 * @param[in]   name    key
 * @param[in]   value   value
 *
 * @return < 0 on errors
 */
static int
configuration_handler(void* config, const char* section, const char* name,
    const char* value)
{
  schedulerconf_t* pconfig = (schedulerconf_t*) config;
  if (MATCH("scheduler", "netonly"))
    {
      pconfig->netOnly = strtol(value, NULL, 10);
    }
  else
    {
      return 0; /* unknown section/name, error */
    }
  return 1;
}

/******************************************************************************
 * GLOBAL FUNCTIONS
 ******************************************************************************/

/**
 * Stack area for the scheduler thread.
 */
WORKING_AREA(wa_fc_scheduler, FCSCHEDULER_THREAD_STACK_SIZE);

/**
 * Scheduling thread.
 */
msg_t
fc_scheduler(void *p)
{
  int sleeptime = FCSERVER_IMPL_SLEEPTIME;
  /* File handling variables: */
  int res, resOpen;
  char path[FILENAME_LENGTH];
  char *filename = NULL;
  uint32_t filenameLength = 0;
  char* root = FCSCHED_FILE_ROOT;
  schedulerconf_t schedConfiguration;

  /* SD card initing variables */
  FRESULT err;
  uint32_t clusters;
  FATFS *fsp;

  /* File reading variables */
  fcsequence_t seq;
  fcseq_ret_t seqRet = FCSEQ_RET_NOTIMPL;
  uint8_t* rgb24 = NULL;

  /* small hack to initialize the global variables */
  gFcBuf4DynQueue = (uint32_t*) hwal_malloc(
      sizeof(uint32_t) * INPUT_DYNMAILBOX_SIZE);
  MAILBOX_DECL(fcMailboxDyn, gFcBuf4DynQueue, INPUT_DYNMAILBOX_SIZE);
  gFcMailboxDyn = &fcMailboxDyn;

  hwal_memset(&seq, 0, sizeof(fcsequence_t));

  chRegSetThreadName("fcscheduler");
  (void) p;

  /* Initialize the SDcard */
  for (res = 0 /* reuse res to avoid endless loop*/;
      res < MAXIMUM_INITIALIZATION; res++)
    {
      err = f_getfree("/", &clusters, &fsp);
      chThdSleep(MS2ST(100));
    }
  while (err != FR_OK);

  /* Load wall configuration */
  readConfigurationFile(&wallcfg);

    /* Load the configuration */
    hwal_memset(&schedConfiguration, 0, sizeof(wallconf_t));

    ini_parse(FCSCHED_CONFIG_FILE, configuration_handler, &schedConfiguration);

  /* Prepare Mailbox to communicate with the others */
  chMBInit(&mailboxIn, (msg_t *) buffer4mailbox2, INPUT_MAILBOX_SIZE);
  path[0] = 0;

  /* Deactivate this thread, if necessary */
  if (schedConfiguration.netOnly)
    {
      FCSCHED_PRINT("Deactivating Scheduler");
      chSysLock();
      chMBPostI(&mailboxIn, (uint32_t) MSG_STOPP);
      chMBPostI(&mailboxIn, (uint32_t) 1);
      chSysUnlock();
    }

  resOpen = fcstatic_open_sdcard();

  /* initialize the folder to search in */
  hwal_memcpy(path, root, strlen(FCSCHED_FILE_ROOT));

  while ( gSchedulerActive )
    {
      if (fcsched_handleInputMailbox())
        {
        continue;
        }
      fcsched_handleFcMailboxDyn(sleeptime);

      switch (gSourceState)
        {
      case FCSRC_STATE_NOBODY:
        /* Deactivate network code stuff */

        FCSCHED_PRINT("Net ONLY ?!? %d\r\n",        schedConfiguration.netOnly);

        /*set server inactive */
        gFcServerActive = 0;
        palClearPad(GPIOD, GPIOD_LED4); /* Green.  */

        if (resOpen)
          {
            /* pick the next file */
            res = fcstatic_getnext_file(path, FILENAME_LENGTH, &filenameLength,
                filename);
            if (res)
              {
                FCSCHED_PRINT("%s ...\r\n", path);

                /* Initialize the file for playback */
                seqRet = fcseq_load(path, &seq);
                if (seqRet == FCSEQ_RET_OK)
                  {
                    gSourceState = FCSRC_STATE_FILE;
                    /* Allocation some space for the RGB buffer */
                    rgb24 = (uint8_t*) chHeapAlloc(NULL,
                        (seq.width * seq.height * 3));
                    seq.fps = wallcfg.fps;
                    FCSCHED_PRINT("Using %d fps and dimmed to %d %.\r\n",
                        seq.fps, wallcfg.dimmFactor);
                    sleeptime = (1000 / seq.fps);
                    palSetPad(GPIOD, GPIOD_LED5); /* Red.  */
                  }
                else
                  {
                    gSourceState = FCSRC_STATE_FILEENDED;
                  }
              }
            else
              {
                /*Reset all and start with rootfolder again */
                hwal_memcpy(path, root, strlen(FCSCHED_FILE_ROOT));
                chHeapFree(filename);
                filename = NULL;
              }
          }
        else
          {
            FCSCHED_PRINT("Reopen SDcard\r\n");
            resOpen = fcstatic_open_sdcard();
          }
        break;
      case FCSRC_STATE_FILEENDED:
        /* Close the file */
        if (rgb24)
          {
            chHeapFree(rgb24);
          }
        rgb24 = NULL;
        fcseq_close(&seq);
        sleeptime = FCSERVER_IMPL_SLEEPTIME;
        palClearPad(GPIOD, GPIOD_LED5); /* Red.  */

        /*extract filename from path for the next cycle */
        fcstatic_remove_filename(path, &filename, filenameLength);
        gSourceState = FCSRC_STATE_NOBODY;

        FCSCHED_PRINT("Check Ethernet interface %d\r\n", gFcConnectedClients)
        ;

        if (gFcConnectedClients)
          {
            /* set server status to true */
            gFcServerActive = TRUE;
            palSetPad(GPIOD, GPIOD_LED4); /* Green.  */
            gSourceState = FCSRC_STATE_NETWORK;
            gDynamicServerTimeout = 0;
          }
        break;
      case FCSRC_STATE_FILE:
        if (rgb24 == NULL)
          {
            gSourceState = FCSRC_STATE_FILEENDED;
          }

        /* Read a frame */
        seqRet = fcseq_nextFrame(&seq, rgb24);
        if (seqRet != FCSEQ_RET_OK)
          {
            gSourceState = FCSRC_STATE_FILEENDED;
          }
        else
          {
            /* Write the DMX buffer */
            fcsched_printFrame(rgb24, seq.width, seq.height, &wallcfg);
          }
        break;
      case FCSRC_STATE_NETWORK:
        if (gFcConnectedClients <= 0)
          {
            gSourceState = FCSRC_STATE_NOBODY;
            FCSCHED_PRINT("CLient disconnected %d\r\n", gSourceState);
          }
        break;
      default:
        /*FIXME check dynamic fullcircle for a new client */
        FCSCHED_PRINT("Unkown status: %d\r\n", gSourceState)
        ;
        break;
        }

      chThdSleep(MS2ST(sleeptime /* convert milliseconds to system ticks */));

    }

  /* clean the memory of the configuration */
  if (wallcfg.pLookupTable)
    {
      hwal_free(wallcfg.pLookupTable);
    }

  FCSCHED_PRINT("Scheduler stopped!\r\n");

  return RDY_OK;
}

static uint8_t
dimmValue(uint8_t incoming, int factor)
{
  uint32_t tmp = incoming;
  tmp = tmp * factor / 100;
  if (tmp > 255)
    tmp = 255;
  return (uint8_t) tmp;
}

extern void
readConfigurationFile(wallconf_t* pConfiguration)
{
  hwal_memset(pConfiguration, 0, sizeof(wallconf_t));
  pConfiguration->dimmFactor = 100;
  pConfiguration->fps = -1;

  /* Load the configuration */
  ini_parse(FCSCHED_WALLCFG_FILE, wall_handler, pConfiguration);
}

extern void
fcsched_printFrame(uint8_t* pBuffer, int width, int height,
    wallconf_t* pWallcfg)
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
      /* Set the DMX buffer directly */
      memcpy(dmx_buffer.buffer, pBuffer, dmx_buffer.length);
    }
}

void
fcscheduler_cmdline(BaseSequentialStream *chp, int argc, char *argv[])
{
  if (argc < 1)
    {
      chprintf(chp, "Usage {debugOn, debugOff, fps (value), dim, stop}\r\n");
      return;
    }
  else if (argc >= 1)
    {
      if (strcmp(argv[0], "debugOn") == 0)
        {
          /* Activate the debugging */
          chprintf(chp, "Activate the logging for Fullcircle Scheduler\r\n");
          chSysLock()
          ;
          chMBPostI(&mailboxIn, (uint32_t) MSG_ACTIVATE_SHELL);
          chMBPostI(&mailboxIn, (uint32_t) chp);
          chSysUnlock();
        }
      else if (strcmp(argv[0], "debugOff") == 0)
        {
          /* Activate the debugging */
          chprintf(chp, "Deactivate the logging for Fullcircle Scheduler\r\n");
          chSysLock()
          ;
          chMBPostI(&mailboxIn, (uint32_t) MSG_ACTIVATE_SHELL);
          chMBPostI(&mailboxIn, (uint32_t) 0);
          chSysUnlock();
        }
      else if (strcmp(argv[0], "fps") == 0 && (argc >= 2))
        {
          /* Activate the debugging */
          chprintf(chp, "Fullcircle Scheduler - Update FPS %d\r\n",
              atoi(argv[1]));
          chSysLock()
          ;
          chMBPostI(&mailboxIn, (uint32_t) MSG_SETFPS);
          chMBPostI(&mailboxIn, (uint32_t) atoi(argv[1]));
          chSysUnlock();
        }
      else if (strcmp(argv[0], "dim") == 0 && (argc >= 2))
        {
          /* Activate the debugging */
          chprintf(chp, "Fullcircle Scheduler - Update dimming %d\r\n",
              atoi(argv[1]));
          chSysLock();
          chMBPostI(&mailboxIn, (uint32_t) MSG_DIMM);
          chMBPostI(&mailboxIn, (uint32_t) atoi(argv[1]));
          chSysUnlock();
        }
      else if (strcmp(argv[0], "stop") == 0)
        {
          /* Activate the debugging */
          chprintf(chp, "Fullcircle Scheduler - Stopped\r\n");
          chSysLock();
          chMBPostI(&mailboxIn, (uint32_t) MSG_STOPP);
          chMBPostI(&mailboxIn, (uint32_t) 1);
          chSysUnlock();
        }
    }

}
