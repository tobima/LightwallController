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

#include "chprintf.h"
#include "shell.h"

#include "lwipthread.h"
#include "web/web.h"
#include "dmx/dmx.h"
#include "fcs/fcs.h"

#include "ifconfig.h"

#include "fcseq.h"
#include "customHwal.h"

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


/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/

#define SHELL_WA_SIZE   THD_WA_SIZE(2048)

static void cmd_mem(BaseSequentialStream *chp, int argc, char *argv[]) {
  size_t n, size;

  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: mem\r\n");
    return;
  }
  n = chHeapStatus(NULL, &size);
  chprintf(chp, "core free memory : %u bytes\r\n", chCoreStatus());
  chprintf(chp, "heap fragments   : %u\r\n", n);
  chprintf(chp, "heap free total  : %u bytes\r\n", size);
}

static void cmd_threads(BaseSequentialStream *chp, int argc, char *argv[]) {
  static const char *states[] = {THD_STATE_NAMES};
  Thread *tp;

  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: threads\r\n");
    return;
  }
  chprintf(chp, "    addr    stack prio refs     state     time      name\r\n");
  tp = chRegFirstThread();
  do {
    chprintf(chp, "%.8lx %.8lx %4lu %4lu %9s %8lu %15s\r\n",
            (uint32_t)tp, (uint32_t)tp->p_ctx.r13,
            (uint32_t)tp->p_prio, (uint32_t)(tp->p_refs - 1),
            states[tp->p_state], (uint32_t)tp->p_time, tp->p_name);
    tp = chRegNextThread(tp);
  } while (tp != NULL);
}

static void cmd_tree(BaseSequentialStream *chp, int argc, char *argv[]) {
  FRESULT err;
  uint32_t clusters;
  FATFS *fsp;

  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: tree\r\n");
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
  fbuff[0] = 0;
  scan_files(chp, (char *)fbuff);
}

static void cmd_fcs(BaseSequentialStream *chp, int argc, char *argv[]) {
	FRESULT err;
	uint32_t clusters;
	FATFS *fsp;
	
	(void)argv;
	if (argc > 0) {
		chprintf(chp, "Usage: tree\r\n");
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
	fbuff[0] = 0;
	fcs_scan_files(chp, (char *)fbuff);
}

static void cmd_cat(BaseSequentialStream *chp, int argc, char *argv[]) {
  FIL fp;
  uint8_t buffer[32];
  int br;
  
  if(argc < 1)
    return;
  
  if(f_open(&fp, (TCHAR*) *argv, FA_READ) != FR_OK)
    return;
  
  do {
    if(f_read(&fp, (TCHAR*) buffer, 32,(UINT*) &br) != FR_OK)
      return;
    
    chSequentialStreamWrite(chp, buffer, br);
  } while (!f_eof(&fp));
  
  f_close(&fp);
  
  chprintf(chp, "\r\n");
}

static void cmd_dmx_modify(BaseSequentialStream *chp, int argc, char *argv[])
{
#define DMX_USAGE_HELP "Possible commands are:\r\nshow\tprint content\r\nwrite (offset) (value)\r\nfill (start offset) (end) (value)\r\nshrink (size)\tUpdate the length of the universe\r\n"
	
	if(argc < 1)
	{
		chprintf(chp, "Usage <command> (parameter)\r\n");	  
		chprintf(chp, DMX_USAGE_HELP);
		return;
	}
	
	if(argc >= 1)
	{
		if (strcmp(argv[0], "write") == 0)
		{
			if (argc < 3)
			{
				chprintf(chp, "Usage: dmx write (offset) (value)\r\n");
			}
			else
			{
				int offset = atoi(argv[1]);
				int value = atoi(argv[2]);
				
				if (dmx_buffer.length < offset)
				{
					chprintf(chp, "Increased Universe from %d to %d bytes.\r\n", dmx_buffer.length, offset + 1);
					dmx_buffer.length = offset + 1;
				}
				
				dmx_buffer.buffer[offset] = value;
			    chprintf(chp, "Set DMX at %d with 0x%2X (%d)\r\n", offset, value, value);
			}
		}
		else if (strcmp(argv[0], "fill") == 0)
		{
			if (argc < 4)
			{
				chprintf(chp, "Usage: dmx fill (start offset) (end) (value)\r\n");
			}
			else
			{
				int offset = atoi(argv[1]);
				int end = atoi(argv[2]);
				int value = atoi(argv[3]);
				int length;
				
				/* swap, if the user cannot determine the lower number */
				if (end < offset)
				{
					int tmp = offset;
					offset = end;
					end = tmp;
				}
				
				if (dmx_buffer.length < end)
				{
					chprintf(chp, "Increased Universe from %d to %d bytes.\r\n", dmx_buffer.length, end + 1);
					dmx_buffer.length = end + 1;
				}
				
				length = end - offset;
				for (; offset < end; offset++) {					
					dmx_buffer.buffer[offset] = value;	
				}
			    chprintf(chp, "Filled DMX with %2X (%d times)\r\n", value, length);
			}
		}
		else if (strcmp(argv[0], "show") == 0)
		{
			int i;
			chprintf(chp, "DMX is filled with %d byte\r\n", dmx_buffer.length);
			for (i = 0; i < dmx_buffer.length; i++) {					
				chprintf(chp, "%.2X", dmx_buffer.buffer[i]);	
			}
			chprintf(chp, "\r\n");			
		}
		else if (strcmp(argv[0], "shrink") == 0)
		{
			if (argc < 2)
			{
				chprintf(chp, "Usage: dmx shrink (universe size)\r\n");
			}
			else
			{
				int size = atoi(argv[1]);
				chprintf(chp, "Update size of the universe from %d to %d bytes.\r\n", dmx_buffer.length, size);
				dmx_buffer.length = size;
			}
		}
		else if (strcmp(argv[0], "help") == 0)
		{
			chprintf(chp, "Possible commands are:\r\n"
					 DMX_USAGE_HELP);
		}
	}
	

}

static void cmd_fcat(BaseSequentialStream *chp, int argc, char *argv[])
{
  fcsequence_t seq;
  fcseq_ret_t ret = FCSEQ_RET_NOTIMPL; 
  int x, y, ypos;
  int frame_index = 0;
  int singleframe = -1; /* position of the frame to load */
  uint8_t* rgb24;
	
  if(argc < 1)
  {
	chprintf(chp, "Usage <filename> (Frame to display)\r\n");	  
    chprintf(chp, "One parameter with the file to read is necessary, then all frames are displayed!\r\n");
    return;
  }
  else if(argc >= 2)
  {
	singleframe = atoi(argv[1]);
	chprintf(chp, "Extract frame with index %d\r\n", singleframe); 
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
	
	rgb24 = (uint8_t*) chHeapAlloc(NULL, (seq.width * seq.height * 3) );
	chprintf(chp, "Allocated buffer at %x with %d bytes\r\n", rgb24, (seq.width * seq.height * 3) );

	
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
	
		/* print all frames or the single requested one */
		if (singleframe == frame_index || singleframe < 0)
		{
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
		}
	

		/* only sent something to DMX on the single frame debug mode */
		if (singleframe == frame_index)
		{
			/* Set the DMX buffer */
			dmx_buffer.length = seq.width * seq.height * 3;
			memcpy(dmx_buffer.buffer, rgb24, dmx_buffer.length);
			chprintf(chp, "Filled DMX with %d bytes\r\n", dmx_buffer.length);
			chHeapFree(rgb24);
			return; /* The DMX buffer has new input -> exit */			
		}
		
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
  {"fcat", cmd_fcat},
  {"fcs", cmd_fcs},
  {"dmx", cmd_dmx_modify},
  {"ifconfig", cmd_ifconfig},
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
  static const evhandler_t evhndl[] = {
    InsertHandler,
    RemoveHandler
  };
  struct EventListener el0, el1;
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
   * Shell manager initialization.
   */
  shellInit();
  
  /*
   * Activates the serial driver 6 using the driver default configuration.
   */
  sdStart(&SD6, NULL);
  
  /*
   * Activates the SDC driver 1 using default configuration.
   */
  sdcStart(&SDCD1, NULL);
  
  
  /*
   * Activates the card insertion monitor.
   */
  tmr_init(&SDCD1);
  
  DMXInit();
  
  /*
   * Creates the example thread.
   */
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);
  
   /*
   * Creates the LWIP threads (it changes priority internally).
   */
  chThdCreateStatic(wa_lwip_thread, LWIP_THREAD_STACK_SIZE, NORMALPRIO + 2,
                    lwip_thread, NULL);
  
  /*
   * Creates the HTTP thread.
   */
  chThdCreateStatic(wa_http_server, sizeof(wa_http_server), NORMALPRIO + 1,
                    http_server, NULL);

  /*
   * Creates the DMX thread.
   */
  chThdCreateStatic(wa_dmx, sizeof(wa_dmx), NORMALPRIO - 1,
                    dmxthread, NULL);
  
  
  chEvtRegister(&inserted_event, &el0, 0);
  chEvtRegister(&removed_event, &el1, 1);
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
