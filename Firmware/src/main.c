/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ch.h"
#include "hal.h"
#include "ini/ini.h"

#include "chprintf.h"
#include "shell.h"

#include "lwipthread.h"
#include "web/web.h"
#include "dmx/dmx.h"
#include "netshell/netshell.h"
#include "dmx/dmx_cmd.h"
#include "fcseq.h"
#include "fullcircle/fcserverImpl.h"
#include "fullcircle/fcscheduler.h"

#include "conf/conf.h"

#include "lwip/netif.h"
#include "hwal.h"	/* Needed for memcpy */
#include "customHwal.h"

#include "cmd/cmd.h"

#include "ff.h"

/*===========================================================================*/
/* Card insertion monitor.                                                   */
/*===========================================================================*/

#define POLLING_INTERVAL                10
#define POLLING_DELAY                   10

/**
 * @brief   Card monitor timer.
 */
static VirtualTimer tmr;

static configuration_t config;

/**
 * @brief   Debounce counter.
 */
static unsigned cnt;

/**
 * @brief   Card event sources.
 */
static EventSource inserted_event, removed_event;

/**
 * @brief   Insertion monitor timer callback function.
 *
 * @param[in] p         pointer to the @p BaseBlockDevice object
 *
 * @notapi
 */
static void tmrfunc(void *p) {
  BaseBlockDevice *bbdp = p;

  chSysLockFromIsr();
  if (cnt > 0) {
    if (blkIsInserted(bbdp)) {
      if (--cnt == 0) {
        chEvtBroadcastI(&inserted_event);
      }
    }
    else
      cnt = POLLING_INTERVAL;
  }
  else {
    if (!blkIsInserted(bbdp)) {
      cnt = POLLING_INTERVAL;
      chEvtBroadcastI(&removed_event);
    }
  }
  chVTResetI(&tmr);
  chVTSetI(&tmr, MS2ST(POLLING_DELAY), tmrfunc, bbdp);
  chSysUnlockFromIsr();
}

/**
 * @brief   Polling monitor start.
 *
 * @param[in] p         pointer to an object implementing @p BaseBlockDevice
 *
 * @notapi
 */
static void tmr_init(void *p) {
  chEvtInit(&inserted_event);
  chEvtInit(&removed_event);
  chSysLock();
  cnt = POLLING_INTERVAL;
  chVTResetI(&tmr);
  chVTSetI(&tmr, MS2ST(POLLING_DELAY), tmrfunc, p);
  chSysUnlock();
}

/*===========================================================================*/
/* FatFs related.                                                            */
/*===========================================================================*/

/**
 * @brief FS object.
 */
static FATFS SDC_FS;

/* FS mounted and ready.*/
static bool_t fs_ready = FALSE;

static void print_fsusage() {
   uint32_t clusters;
    FATFS *fsp;
    DIR dir;
    
    if(f_getfree("/", &clusters, &fsp)== FR_OK) {
      chprintf(
        (BaseSequentialStream *)&SD6,
        "FS: %lu free clusters, %lu sectors per cluster, %lu bytes free\r\n",
        clusters, (uint32_t)SDC_FS.csize,
        clusters * (uint32_t)SDC_FS.csize * (uint32_t)MMCSD_BLOCK_SIZE);
        
        f_opendir(&dir, "fc/conf");
    }
}

/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/

#define SHELL_WA_SIZE   THD_WA_SIZE(2048)

/* Generic large buffer.*/
static uint8_t fbuff[1024];

static FRESULT scan_files(BaseSequentialStream *chp, char *path) {
  FRESULT res;
  FILINFO fno;
  DIR dir;
  int i;
  char *fn;

#if _USE_LFN
  fno.lfname = 0;
  fno.lfsize = 0;
#endif
  res = f_opendir(&dir, path);
  if (res == FR_OK) {
    i = strlen(path);
    for (;;) {
      res = f_readdir(&dir, &fno);
      if (res != FR_OK || fno.fname[0] == 0)
        break;
      if (fno.fname[0] == '.')
        continue;
      fn = fno.fname;
      if (fno.fattrib & AM_DIR) {
        path[i++] = '/';
        strcpy(&path[i], fn);
        res = scan_files(chp, path);
        if (res != FR_OK)
          break;
        path[--i] = 0;
      }
      else {
        chprintf(chp, "%s/%s\r\n", path, fn);
      }
    }
  }
  return res;
}

void cmd_tree(BaseSequentialStream *chp, int argc, char *argv[]) {
  FRESULT err;
  uint32_t clusters;
  FATFS *fsp;

  if (argc > 1) {
    chprintf(chp, "Usage: tree {root path}\r\n");
    return;
  }
  if (!fs_ready) {
    chprintf(chp, "File System not mounted\r\n");
    return;
  }
  err = f_getfree("/", &clusters, &fsp);
  if (err != FR_OK) {
    chprintf(chp, "FS: f_getfree() failed. %lu\r\n", err);
    return;
  }
  
  chprintf(chp,
           "FS: %lu free clusters, %lu sectors per cluster, %lu bytes free\r\n",
           clusters, (uint32_t)SDC_FS.csize,
           clusters * (uint32_t)SDC_FS.csize * (uint32_t)MMCSD_BLOCK_SIZE);
	
	
	if (argc > 0) {
		chprintf(chp, "Searching in %s\r\n", argv[0]);		
		hwal_memcpy((char *) fbuff, argv[0], strlen(argv[0]));
		fbuff[strlen(argv[0])] = 0;
	}
	else
	{
		fbuff[0] = 0;
	}	
	scan_files(chp, (char *)fbuff);
}

static void cmd_fcat(BaseSequentialStream *chp, int argc, char *argv[]) {
  fcsequence_t seq;
  fcseq_ret_t ret = FCSEQ_RET_NOTIMPL; 
  int x, y, ypos;
  int frame_index = 0;
  int sleeptime;
  int fixSleepTimer = -1;
  uint8_t* rgb24;
	
  if(argc < 1)
  {
	chprintf(chp, "Usage <filename>\r\n");
    return;
  }
  else if(argc >= 2)
  {
    fixSleepTimer = atoi(argv[1]);
    chprintf(chp, "Sleeptime was set FIX to %d ms\r\n", fixSleepTimer);  
  }
	
#if 0
	hwal_init(chp); /* No Debug output for the sequence library */
#endif
	
	ret = fcseq_load(argv[0], &seq);

  if (ret != FCSEQ_RET_OK)
  {
	chprintf(chp, "Could not read %s\r\nError code is %d\r\n", argv[0], ret);
	chprintf(chp, "Unable to load sequnce (%s:%d)\r\n", __FILE__, __LINE__);
	return;
  }

  chprintf(chp, "=== Meta information ===\r\n"
		   "fps: %d, width: %d, height: %d\r\n",seq.fps,seq.width,seq.height);
	
	if (seq.fps == 0)
	{
		chprintf(chp, "The FPS could NOT be extracted! Stopping\r\n");
		return;
	}
	
	/* Allocation some space for the RGB buffer */
	rgb24 = (uint8_t*) chHeapAlloc(NULL, (seq.width * seq.height * 3) );
	
	/* parse */
	ret = fcseq_nextFrame(&seq, rgb24);

	if (ret != FCSEQ_RET_OK)
	{
		chprintf(chp, "Reading the first frame failed with error code %d.\r\n", ret );
		return;
	}
	
	/* loop to print something on the commandline */
	while (ret == FCSEQ_RET_OK)
	{
#if 0
		chprintf(chp, "=============== %d ===============\r\n", frame_index);
		for (y=0; y < seq.height; y++)
		{
			ypos = y * seq.width * 3;
			for(x=0; x < seq.width; x++)
			{
				chprintf(chp, "%.2X", rgb24[(ypos+x*3) + 0]);
				chprintf(chp, "%.2X", rgb24[(ypos+x*3) + 1]);
				chprintf(chp, "%.2X", rgb24[(ypos+x*3) + 2]);
				chprintf(chp, "|");
			}
			chprintf(chp, "\r\n");
		}
#endif

		/* Set the DMX buffer */
		dmx_buffer.length = seq.width * seq.height * 3;
		memcpy(dmx_buffer.buffer, rgb24, dmx_buffer.length);
		
		if (fixSleepTimer > 0)
		{
			sleeptime = fixSleepTimer;
		}
		else
		{
			sleeptime = (1000 / seq.fps);
		}
		
		chThdSleep(MS2ST(sleeptime /* convert milliseconds to system ticks */));
		/* parse the next */
		ret = fcseq_nextFrame(&seq, rgb24);
		
		/* Move the count */
		frame_index++;
	}
	
	chHeapFree(rgb24);
}


static const ShellCommand commands[] = {
  {"mem", cmd_mem},
  {"tree", cmd_tree},
  {"threads", cmd_threads},
  {"cat", cmd_cat},
  {"ifconfig", cmd_ifconfig},
  {"fcat", cmd_fcat},
  {"dmx", cmd_dmx_modify},
  {"fcdyn", fcsserverImpl_cmdline},
  {"fcsched", fcscheduler_cmdline},
  {NULL, NULL}
};

static const ShellConfig shell_cfg1 = {
  (BaseSequentialStream *)&SD6,
  commands
};

/*===========================================================================*/
/* Main and generic code.                                                    */
/*===========================================================================*/

/*
 * Card insertion event.
 */
static void InsertHandler(eventid_t id) {
  FRESULT err;
  uint32_t clusters;
  FATFS *fsp;

  (void)id;
  /*
   * On insertion SDC initialization and FS mount.
   */
  if (sdcConnect(&SDCD1))
  {
    if (sdcConnect(&SDCD1))
      return;
  }

  err = f_mount(0, &SDC_FS);
  f_getfree("/", &clusters, &fsp);
  if (err != FR_OK) {
    sdcDisconnect(&SDCD1);
    return;
  }
  fs_ready = TRUE;
}

/*
 * Card removal event.
 */
static void RemoveHandler(eventid_t id) {

  (void)id;
  sdcDisconnect(&SDCD1);
  fs_ready = FALSE;
}

static struct EventListener el0, el1;

static const evhandler_t evhndl[] = {
    InsertHandler,
    RemoveHandler
  };

/*
 * This is a periodic thread that does absolutely nothing except flashing
 * a LED.
 */
static WORKING_AREA(waThread1, 128);

__attribute__((noreturn))
static msg_t Thread1(void *arg) {

  (void)arg;
  chRegSetThreadName("blinker");
  while (TRUE) {
    palSetPad(GPIOD, GPIOD_LED3);       /* Orange.  */
    chThdSleepMilliseconds(500);
    palClearPad(GPIOD, GPIOD_LED3);     /* Orange.  */
    chThdSleepMilliseconds(500);
  }
}

/*
 * Application entry point.
 */
int main(void) {

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

    
  /*
   * Activates the serial driver 6 using the driver default configuration.
   */
  sdStart(&SD6, NULL);
  
  chprintf(
    (BaseSequentialStream *)&SD6,
    "\x1b[1J\x1b[0;0HStarting ChibiOS\r\n");
  
  chprintf(
    (BaseSequentialStream *)&SD6,
    "Initialazing SDCARD driver ...");
  
  /*
   * Activates the SDC driver 1 using default configuration.
   */
  sdcStart(&SDCD1, NULL);
  
  /*
   * Activates the card insertion monitor.
   */
  tmr_init(&SDCD1);  
  
  chEvtRegister(&inserted_event, &el0, 0);
  chEvtRegister(&removed_event, &el1, 1);
  
  chprintf(
    (BaseSequentialStream *)&SD6,
    " Done\r\n");

  chprintf(
    (BaseSequentialStream *)&SD6,
    "Initialazing DMX driver ...");
  
  DMXInit();
  
  /*
   * Creates the DMX thread.
   */
  chThdCreateStatic(wa_dmx, sizeof(wa_dmx), NORMALPRIO - 1,
                    dmxthread, NULL);
  chprintf(
    (BaseSequentialStream *)&SD6,
    " Done\r\n");
  
  chprintf(
    (BaseSequentialStream *)&SD6,
    "Start blinker thread ...");
  
  /*
   * Creates the example thread.
   */
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);
  
	
  /** 
   * Booting ...
   * - search for configuration on SD-card
   */
  chprintf(
    (BaseSequentialStream *)&SD6,
    " Done\r\n");
  
  chprintf(
    (BaseSequentialStream *)&SD6,
    "Searching filesystem ...");
  
  chEvtDispatch(evhndl, chEvtWaitOneTimeout(ALL_EVENTS, MS2ST(500)));
  chThdSleepMilliseconds(500);
  chEvtDispatch(evhndl, chEvtWaitOneTimeout(ALL_EVENTS, MS2ST(500)));
  
  if (!fs_ready) {
    chprintf(
      (BaseSequentialStream *)&SD6,
      "\x1b[31m Failed!\r\n\x1b[0m");
    chprintf(
      (BaseSequentialStream *)&SD6,
     "File System not mounted\r\n");
  } else {
    chprintf(
      (BaseSequentialStream *)&SD6,
      "\x1b[32m OK\r\n\x1b[0m");
    
    print_fsusage();
  }
  
  int use_config = 0;
  if (fs_ready) {
    chprintf(
      (BaseSequentialStream *)&SD6,
      "Loading network configuration from SDcard ...");
    
    use_config = (conf_load(&config)==0)?1:0;
    
    if (use_config) {
     chprintf(
      (BaseSequentialStream *)&SD6,
      "\x1b[32m OK\r\n\x1b[0m");
    }
    else {
      chprintf(
      (BaseSequentialStream *)&SD6,
      "\x1b[31m Failed!\r\n\x1b[0m");
    }
     
  }
  
   /*
   * Creates the LWIP threads (it changes priority internally).
   */
  chThdCreateStatic(wa_lwip_thread, LWIP_THREAD_STACK_SIZE, NORMALPRIO + 2,                    
                    lwip_thread, 
					(use_config)?&(config.network):NULL);
	
  /*
   * Creates the dynamic fullcircle thread.
   */
  chThdCreateStatic(wa_fc_server, sizeof(wa_fc_server), NORMALPRIO + 1,
					  fc_server, NULL);
	
	/*
	 * Creates the scheduler thread.
	 */
	chThdCreateStatic(wa_fc_scheduler, sizeof(wa_fc_scheduler), NORMALPRIO + 1,
					  fc_scheduler, NULL);
	
  /*
   * Creates the HTTP thread.
   */
  chThdCreateStatic(wa_http_server, sizeof(wa_http_server), NORMALPRIO + 3,
                    http_server, NULL);

  /*
   * Creates the Net Shell thread.
   */
  //chThdCreateStatic(wa_net_shell_server, sizeof(wa_net_shell_server), NORMALPRIO + 1,
  //                server_thread, NULL);

  chprintf(
    (BaseSequentialStream *)&SD6,
    "Initialazing Shell...");
  
  /*
   * Shell manager initialization.
   */
  shellInit();
  
  chprintf(
    (BaseSequentialStream *)&SD6,
    "Done\r\nCreate new Shell\r\n");
  

  shellCreate(&shell_cfg1, SHELL_WA_SIZE, NORMALPRIO);
  
  /*
   * Normal main() thread activity, in this demo it does nothing except
   * sleeping in a loop and check the button state, when the button is
   * pressed the test procedure is launched with output on the serial
   * driver 2.
   */
  while (TRUE) {  
    chEvtDispatch(evhndl, chEvtWaitOneTimeout(ALL_EVENTS, MS2ST(500)));
    //chThdSleepMilliseconds(500);
  }
}
