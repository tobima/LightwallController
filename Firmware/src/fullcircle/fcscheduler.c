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

#ifdef UGFX_WALL
#include "fcwall.h"
#include "ugfx_util.h"
#endif

#ifdef WS2811_WALL
#include "ledstripe/ledstripe.h"
#endif

#define FCSCHED_CONFIG_FILE     "fc/conf/controller"
#define FCSCHED_FILE_ROOT			"fc/static\0"	/**< Folder on the sdcard to check */

#define	FILENAME_LENGTH			512	/**< Including the absolut path to the file */
#define INPUT_MAILBOX_SIZE		4

#define MAXIMUM_INITIALIZATION			100
#define FCSCHED_DYNSERVER_RESETVALUE    (FRAME_ALIVE_STARTLEVEL * 2)

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

#define FCSCHED_PRINT( ... )	if (gDebugShellSched) { chprintf(gDebugShellSched, __VA_ARGS__); }

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
  FCSRC_STATE_NETWORK, /**< Someone is streaming dynamic content to us */
  FCSRC_STATE_PROCESSENDED /**< State, that is set, only if the thread was stopped */
} fcsource_state_t;

/******************************************************************************
 * GLOBAL VARIABLES for this module
 ******************************************************************************/

uint32_t* gFcBuf4DynQueue;
Mailbox * gFcMailboxDyn;

/******************************************************************************
 * LOCAL VARIABLES for this module
 ******************************************************************************/

static int configuredFps;
static int configuredDimmFactor;

static BaseSequentialStream * gDebugShellSched = NULL;

/* Mailbox, checked by the fullcircle scheduler thread */
static uint32_t buffer4mailbox2[INPUT_MAILBOX_SIZE];
static MAILBOX_DECL(mailboxIn, buffer4mailbox2, INPUT_MAILBOX_SIZE);

static fcsource_state_t gSourceState = FCSRC_STATE_NOBODY;

uint32_t gFcConnectedClients = 0;

static uint32_t gDynamicServerTimeout = FCSCHED_DYNSERVER_RESETVALUE;
static uint8_t  gSchedulerActive = TRUE;

static char path[FILENAME_LENGTH];	/**< Path of the actual played file */

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
                gDebugShellSched = (BaseSequentialStream *) msg2;
                FCSCHED_PRINT("Activated debugging\r\n");
                hwal_init((BaseSequentialStream *) msg2);
                break;
              case MSG_SETFPS:
                configuredFps = (int) msg2;
                break;
              case MSG_DIMM:
            	  configuredDimmFactor = (int) msg2;
                break;
              case MSG_STOPP:
                gSchedulerActive = FALSE;
                gFcServerActive = TRUE; /* The server has an GO */
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
	   
#ifdef UGFX_WALL
	int lineRightPx =  (FCSCHED_DYNSERVER_RESETVALUE - gDynamicServerTimeout) * gdispGetWidth() / FCSCHED_DYNSERVER_RESETVALUE;
	gdispDrawLine(0, gdispGetHeight() - 1, 
	gdispGetWidth(), gdispGetHeight() - 1, Black);

	gdispDrawLine(0, gdispGetHeight() - 1, lineRightPx, gdispGetHeight() - 1, Red);
	
	FCSCHED_PRINT("Position in px %6d / %6d   ", lineRightPx,gdispGetWidth());
#endif
	
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
msg_t fc_scheduler(void *p)
{
  int sleeptime = FCSERVER_IMPL_SLEEPTIME;
    /* File handling variables: */
    int res, resOpen;
    char *filename = NULL;
    uint32_t filenameLength = 0;
    char* root = FCSCHED_FILE_ROOT;
    schedulerconf_t schedConfiguration;
    int width, height = -1;

  #ifdef UGFX_WALL
    /* Initiaize the font */
    font_t font = gdispOpenFont("DejaVu*");
  #endif

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

    dmx_getScreenresolution(&width, &height);
    dmx_getDefaultConfiguration(&configuredFps, &configuredDimmFactor);

    chRegSetThreadName("fcscheduler");
    (void) p;

    /* Load the configuration */
    hwal_memset(&schedConfiguration, 0, sizeof(schedulerconf_t));

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

        if (resOpen)
          {
            /* pick the next file */
            res = fcstatic_getnext_file(path, FILENAME_LENGTH, &filenameLength,
                filename);
            if (res)
              {
#ifdef UGFX_WALL
		gdispPrintf(0, gdispGetHeight() - 30, font, White, 256, 
			"%d x %d , %d fps [dimmed to %d%%]", 
			width, height, configuredFps, configuredDimmFactor);
		gdispPrintf(0, gdispGetHeight() - 15, font, White, 256, 
			"File %s playing.", path);
#endif
                FCSCHED_PRINT("%s ...\r\n", path);

                /* Initialize the file for playback */
                seqRet = fcseq_load(path, &seq);
                if (seqRet == FCSEQ_RET_OK)
                  {
                    gSourceState = FCSRC_STATE_FILE;
                    /* Allocation some space for the RGB buffer */
                    rgb24 = (uint8_t*) chHeapAlloc(NULL,
                        (seq.width * seq.height * 3));
                    seq.fps = configuredFps;
                    dmx_dim(configuredDimmFactor);
                    FCSCHED_PRINT("Using %d fps and dimmed to %d %.\r\n",
                        seq.fps, configuredDimmFactor);
                    sleeptime = (1000 / seq.fps);
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
#ifdef UGFX_WALL
		gdispPrintf(0, gdispGetHeight() - 15, font, Red, 256,
			"Waiting for SDcard");
#endif

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

        /*extract filename from path for the next cycle */
        fcstatic_remove_filename(path, &filename, filenameLength);
        gSourceState = FCSRC_STATE_NOBODY;

        FCSCHED_PRINT("Check Ethernet interface %d\r\n", gFcConnectedClients);

        if (gFcConnectedClients)
          {
            /* set server status to true */
            gFcServerActive = TRUE;
            gSourceState = FCSRC_STATE_NETWORK;
            gDynamicServerTimeout = 0;
#ifdef UGFX_WALL
		gdispPrintf(0, gdispGetHeight() - 15, font, Green, 256, 
			"Using Network");
#endif
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
        	hwal_memcpy(dmx_fb, rgb24, seq.width * seq.height * DMX_RGB_COLOR_WIDTH);
        	dmx_update(seq.width , seq.height);

#ifdef WS2811_WALL
        	hwal_memcpy(ledstripe_framebuffer, rgb24, seq.width * seq.height * DMX_RGB_COLOR_WIDTH);
#endif

        	FCSCHED_PRINT("%X\r\n", rgb24[0]);
          }
        break;
      case FCSRC_STATE_NETWORK:
        if (gFcConnectedClients <= 0)
          {
            gSourceState = FCSRC_STATE_NOBODY;
#ifdef UGFX_WALL
		gdispPrintf(0, gdispGetHeight() - 15, font, Green, 256, 
			"Network disconnected");
#endif
            FCSCHED_PRINT("Client disconnected %d\r\n", gSourceState);
          }
        break;
      default:
        /*FIXME check dynamic fullcircle for a new client */
        FCSCHED_PRINT("Unkown status: %d\r\n", gSourceState);
        break;
        }

      chThdSleep(MS2ST(sleeptime /* convert milliseconds to system ticks */));

    }


    /* when playing file close it */
    if (gSourceState == FCSRC_STATE_FILE)
    {
    	/* Close the file */
		if (rgb24)
		  {
			chHeapFree(rgb24);
		  }
		rgb24 = NULL;
		fcseq_close(&seq);
		sleeptime = FCSERVER_IMPL_SLEEPTIME;
    	FCSCHED_PRINT("File closed\r\n");
    }

  gSchedulerActive = FCSRC_STATE_PROCESSENDED;
  FCSCHED_PRINT("Scheduler stopped!\r\n");

  return RDY_OK;
}

void
fcscheduler_cmdline(BaseSequentialStream *chp, int argc, char *argv[])
{
  if (argc < 1)
    {
      chprintf(chp, "Usage {debugOn, debugOff, fps (value), dim, start, stop, config}\r\n");
      return;
    }
  else if (argc >= 1)
    {
      if (strcmp(argv[0], "debugOn") == 0)
        {
          /* Activate the debugging */
          chprintf(chp, "Activate the logging for Fullcircle Scheduler\r\n");
          chSysLock();
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
          fcscheduler_stopThread();
        }
	else if (strcmp(argv[0], "start") == 0 )
	{
		chprintf(chp, "Start server again\r\n");
		fcscheduler_startThread();
	}
	else if (strcmp(argv[0], "status") == 0 )
	{
		chprintf(chp, "Server status is %d\r\n", gSchedulerActive);
	}

    }

}

void
fcscheduler_startThread(void)
{
  gSchedulerActive = TRUE;
  chThdCreateStatic(wa_fc_scheduler, sizeof(wa_fc_scheduler), NORMALPRIO + 1,
      fc_scheduler, NULL);

}

void
fcscheduler_stopThread(void)
{
  chSysLock();
  chMBPostI(&mailboxIn, (uint32_t) MSG_STOPP);
  chMBPostI(&mailboxIn, (uint32_t) 1);
  chSysUnlock();
}

int fcscheduler_isRunning(void)
{
	/* check the last variable, that is cleared in the threads (@see fc_scheduler()) */
	return (gSchedulerActive != FCSRC_STATE_PROCESSENDED);
}

int fcscheduler_getActualFile(char* pPathToFile, int* pLengthOfFile)
{
	if ((strlen(path) + 1) > (*pLengthOfFile) )
	{
		return 1;
	}

	chSysLock();
	(*pLengthOfFile) = strlen(path) + 1 /* also copy the ending zero */;
	memcpy(pPathToFile, path, strlen(path));
	chSysUnlock();
	return 0; /* Success */
}
