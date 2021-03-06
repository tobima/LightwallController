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

#ifdef UGFX_WALL
#include "gfx.h"
#include "fcwall.h"
#include "ugfx_util.h"
#include "ugfx_cmd.h"
#include "wall_simu.h"
#endif

#include "ini/ini.h"

#include "chprintf.h"
#include "shell.h"

#include "usbcdc/usbcdc.h"

#include "lwipthread.h"
#include "web/web.h"
#include "netshell/netshell.h"
#include "dmx/dmx.h"
#include "dmx/dmx_cmd.h"
#include "dmx/rgb.h"
#include "fullcircle/fcserverImpl.h"
#include "fullcircle/fcscheduler.h"
#include "fullcircle/fcstatic.h"

#include "conf/conf.h"

#include "lwip/netif.h"
#include "hwal.h"	/* Needed for memcpy */
#include "customHwal.h"

#include "cmd/cmd.h"

#include "fatfsWrapper.h"

/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/
#ifdef UGFX_WALL
#define SHELL_WA_SIZE   THD_WA_SIZE(8192)
#else
#define SHELL_WA_SIZE   THD_WA_SIZE(2048)
#endif

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
static void tmrfunc(void *p)
{
  BaseBlockDevice *bbdp = p;

  chSysLockFromIsr()
  ;
  if (cnt > 0)
    {
      if (blkIsInserted(bbdp))
        {
          if (--cnt == 0)
            {
              chEvtBroadcastI(&inserted_event);
            }
        }
      else
        cnt = POLLING_INTERVAL;
    }
  else
    {
      if (!blkIsInserted(bbdp))
        {
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
static void tmr_init(void *p)
{
  chEvtInit(&inserted_event);
  chEvtInit(&removed_event);
  chSysLock()
  ;
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

static void print_fsusage(BaseSequentialStream *chp, int argc, char *argv[])
{
  uint32_t clusters;
  FATFS *fsp;
  DIR dir;

  (void) argc;
  (void) argv;

  if (wf_getfree("/", &clusters, &fsp) == FR_OK)
    {
      chprintf(chp,
          "FS: %lu free clusters, %lu sectors per cluster, %lu bytes free\r\n",
          clusters, (uint32_t) SDC_FS.csize,
          clusters * (uint32_t) SDC_FS.csize * (uint32_t) MMCSD_BLOCK_SIZE);

      wf_opendir(&dir, "fc/conf");
    }
}


static FRESULT
scan_files(BaseSequentialStream *chp, char *path)
{
  FRESULT res;
  FILINFO fno;
  DIR dir;
  int i;
  char *fn;

#if _USE_LFN
  fno.lfname = 0;
  fno.lfsize = 0;
#endif
  res = wf_opendir(&dir, path);
  if (res == FR_OK)
    {
      i = strlen(path);
      for (;;)
        {
          res = wf_readdir(&dir, &fno);
          if (res != FR_OK || fno.fname[0] == 0)
            break;
          if (fno.fname[0] == '.')
            continue;
          fn = fno.fname;
          if (fno.fattrib & AM_DIR)
            {
              path[i++] = '/';
              strcpy(&path[i], fn);
              res = scan_files(chp, path);
              if (res != FR_OK)
                break;
              path[--i] = 0;
            }
          else
            {
              chprintf(chp, "%s/%s\r\n", path, fn);
            }
        }
    }
  return res;
}

void
cmd_tree(BaseSequentialStream *chp, int argc, char *argv[])
{
  FRESULT err;
  uint32_t clusters;
  FATFS *fsp;
  /* Generic large buffer.*/
  static uint8_t fbuff[1024];

  if (argc > 1)
    {
      chprintf(chp, "Usage: tree {root path}\r\n");
      return;
    }
  if (!fs_ready)
    {
      chprintf(chp, "File System not mounted\r\n");
      return;
    }
  err = wf_getfree("/", &clusters, &fsp);
  if (err != FR_OK)
    {
      chprintf(chp, "FS: wf_getfree() failed. %lu\r\n", err);
      return;
    }

  chprintf(chp,
      "FS: %lu free clusters, %lu sectors per cluster, %lu bytes free\r\n",
      clusters, (uint32_t) SDC_FS.csize,
      clusters * (uint32_t) SDC_FS.csize * (uint32_t) MMCSD_BLOCK_SIZE);

  if (argc > 0)
    {
      chprintf(chp, "Searching in %s\r\n", argv[0]);
      hwal_memcpy((char *) fbuff, argv[0], strlen(argv[0]));
      fbuff[strlen(argv[0])] = 0;
    }
  else
    {
      fbuff[0] = 0;
    }
  scan_files(chp, (char *) fbuff);
}

static const ShellCommand commands[] =
  {
    { "mem", cmd_mem },
    { "threads", cmd_threads },
    { "dmx", cmd_dmx_modify },
    { "rgb" , dmx_rgb_modify },
#ifndef DISABLE_FILESYSTEM
    { "tree", cmd_tree },
    { "cat", cmd_cat },
    { "ifconfig", cmd_ifconfig },
    { "fcdyn", fcserverImpl_cmdline },
    { "fcsched", fcscheduler_cmdline },
#ifdef UGFX_WALL
    { "ugfx", ugfx_cmd_shell },
#endif
#endif
    { "flash", cmd_flash },
    { NULL, NULL } };

static const ShellConfig shell_cfg1 =
  { (BaseSequentialStream *) &SD6, commands };

/*===========================================================================*/
/* Main and generic code.                                                    */
/*===========================================================================*/

/*
 * Card insertion event.
 */
static void
InsertHandler(eventid_t id)
{
  FRESULT err;
  uint32_t clusters;
  FATFS *fsp;

  (void) id;
  /*
   * On insertion SDC initialization and FS mount.
   */
  if (sdcConnect(&SDCD1))
    {
      if (sdcConnect(&SDCD1))
        return;
    }

  err = wf_mount(0, &SDC_FS);
  wf_getfree("/", &clusters, &fsp);
  if (err != FR_OK)
    {
      sdcDisconnect(&SDCD1);
      return;
    }
  fs_ready = TRUE;
}

/*
 * Card removal event.
 */
static void
RemoveHandler(eventid_t id)
{

  (void) id;
  sdcDisconnect(&SDCD1);
  fs_ready = FALSE;
}

static struct EventListener el0, el1;

static const evhandler_t evhndl[] =
  { InsertHandler, RemoveHandler };

/*
 * This is a periodic thread that does absolutely nothing except flashing
 * a LED.
 */
static WORKING_AREA(waThreadBlink, 128);
static msg_t blinkerThread(void *arg) {

  (void)arg;
  chRegSetThreadName("blinker");
  while (TRUE) {
    palSetPad(GPIOD, GPIOD_LED4);       /* Green.  */
    chThdSleepMilliseconds(500);
    palClearPad(GPIOD, GPIOD_LED4);     /* Green.  */
    chThdSleepMilliseconds(500);
  }
  return RDY_OK;
}

/*
 * Application entry point.
 */
int
main(void)
{
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
   * Initializes a serial-over-USB CDC driver.
   */
  usbcdc_init(commands);

  /*
  * Shell manager initialization.
  */
  shellInit();
  chThdCreateStatic(waThreadBlink, sizeof(waThreadBlink), NORMALPRIO, blinkerThread, NULL);


  /*
  * Activates the serial driver 6 and SDC driver 1 using default
  * configuration.
  */
  sdStart(&SD6, NULL);

  chprintf((BaseSequentialStream *) &SD6,
      "\x1b[1J\x1b[0;0HStarting ChibiOS\r\n");

  /*************************************
   * SDCard
   */
  chprintf((BaseSequentialStream *) &SD6, "Initialazing SDCARD driver ...");



  /* start the thread for the wrapping module */
  wf_init(NORMALPRIO - 2);

  /*
   * Activates the card insertion monitor.
   */
  tmr_init(&SDCD1);

  chEvtRegister(&inserted_event, &el0, 0);
  chEvtRegister(&removed_event, &el1, 1);

  chprintf((BaseSequentialStream *) &SD6, " Done\r\n");

  /**************************************
   * Screen and Touchscr.-Driver
   */
#ifdef UGFX_WALL
  chprintf((BaseSequentialStream *) &SD6, "Wait for SD card ...");
  chprintf((BaseSequentialStream *) &SD6, " Done\r\nInitialazing GFX driver ...");
  gfxInit();

  fcwall_initWindow();
  chprintf((BaseSequentialStream *) &SD6, " Done\r\n");
#endif


  chprintf((BaseSequentialStream *) &SD6, "Start blinker thread ...");

  /**************************************
   * Booting ...
   * - search for configuration on SD-card
   */
  chprintf((BaseSequentialStream *) &SD6, " Done\r\n");

#ifndef DISABLE_FILESYSTEM
  chprintf((BaseSequentialStream *) &SD6, "Searching filesystem ...");

  chEvtDispatch(evhndl, chEvtWaitOneTimeout(ALL_EVENTS, MS2ST(500)));
  chThdSleepMilliseconds(500);
  chEvtDispatch(evhndl, chEvtWaitOneTimeout(ALL_EVENTS, MS2ST(500)));

  if (!fs_ready)
    {
      chprintf((BaseSequentialStream *) &SD6, "\x1b[31m Failed!\r\n\x1b[0m");
    }
  else
    {
      chprintf((BaseSequentialStream *) &SD6, "\x1b[32m OK\r\n\x1b[0m");

      print_fsusage((BaseSequentialStream *) &SD6, 0, NULL);
    }

  int use_config = 0;
  if (fs_ready)
    {
      chprintf((BaseSequentialStream *) &SD6,
          "Loading network configuration from SDcard ...");

      use_config = (conf_load(&config) == 0) ? 1 : 0;

      if (use_config)
        {
          chprintf((BaseSequentialStream *) &SD6, "\x1b[32m OK\r\n\x1b[0m ");
        }
      else
        {
          chprintf((BaseSequentialStream *) &SD6,
              "\x1b[31m Failed!\r\n\x1b[0m");
        }
      chprintf((BaseSequentialStream *) &SD6, "\r\n");
    }


  /**************************************
   * Creates the LWIP threads (it changes priority internally).
   */
  chThdCreateStatic(wa_lwip_thread, LWIP_THREAD_STACK_SIZE, NORMALPRIO + 2,
      lwip_thread, (use_config) ? &(config.network) : NULL);

  /**************************************
   * Creates the HTTP thread.
   */
  chThdCreateStatic(wa_http_server, sizeof(wa_http_server), NORMALPRIO + 3,
      http_server, NULL);
#endif

  chprintf((BaseSequentialStream *) &SD6, "Initialazing DMX driver ...");

  /* test only the initialization */
  DMXInit();

  /*************************************
   * Creates the DMX thread.
   */
  chThdCreateStatic(wa_dmx, sizeof(wa_dmx), NORMALPRIO - 1, dmxthread, NULL);
  chprintf((BaseSequentialStream *) &SD6, " Done\r\n");

#ifndef DISABLE_FILESYSTEM
  /**************************************
   * Creates the Fullcircle server thread.
   */
  chThdCreateStatic(wa_fc_server, sizeof(wa_fc_server), NORMALPRIO + 1,
      fc_server, NULL);
  /**************************************
   * Creates the scheduler thread.
   */
  chThdSleep(MS2ST(50));
  fcscheduler_startThread();
#endif

#if WITH_TELNET
    /*
	* Creates the Telnet Server thread (it changes priority internally).
	*/
    chThdCreateStatic(wa_telnet_server, sizeof(wa_telnet_server), NORMALPRIO + 1,
                      telnet_server, (void *) commands);
#endif

#ifdef UGFX_WALL

  /**************************************
     * Creates the DMX buffer visualization thread.
     */
  chThdSleep(MS2ST(100));
  ugfx_wall_simu_startThread();
#endif

  chprintf((BaseSequentialStream *) &SD6, "Initializing Shell...");

  /**************************************
   * Shell manager initialization.
   */
  shellInit();

  shellCreate(&shell_cfg1, SHELL_WA_SIZE, NORMALPRIO);

  /*
   * Normal main() thread activity, in this demo it does nothing except
   * sleeping in a loop and check the button state, when the button is
   * pressed the test procedure is launched with output on the serial
   * driver 2.
   */
  while (TRUE)
    {
      usbcdc_process();

      /* Wait some time, to make the scheduler running tasks with lower prio */
      chEvtDispatch(evhndl, chEvtWaitOneTimeout(ALL_EVENTS, MS2ST(500)));

#ifdef UGFX_WALL
      fcwall_processEvents(NULL /*FIXME Debugging the GUI no longer possible */);
#endif
    }
}

