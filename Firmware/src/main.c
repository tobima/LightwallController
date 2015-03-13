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

#ifdef WS2811_WALL
#include "ledstripe/ledstripe.h"
#endif

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


/*===========================================================================*/
/* USB related stuff.                                                        */
/*===========================================================================*/

/*
 * Endpoints to be used for USBD1.
 */
#define USBD1_DATA_REQUEST_EP           1
#define USBD1_DATA_AVAILABLE_EP         1
#define USBD1_INTERRUPT_REQUEST_EP      2

/*
 * Serial over USB Driver structure.
 */
static SerialUSBDriver SDU1;

/*
 * USB Device Descriptor.
 */
static const uint8_t vcom_device_descriptor_data[18] = {
    USB_DESC_DEVICE       (0x0110,        /* bcdUSB (1.1).                    */
                           0x02,          /* bDeviceClass (CDC).              */
                           0x00,          /* bDeviceSubClass.                 */
                           0x00,          /* bDeviceProtocol.                 */
                           0x40,          /* bMaxPacketSize.                  */
                           0x0483,        /* idVendor (ST).                   */
                           0x5740,        /* idProduct.                       */
                           0x0200,        /* bcdDevice.                       */
                           1,             /* iManufacturer.                   */
                           2,             /* iProduct.                        */
                           3,             /* iSerialNumber.                   */
                           1)             /* bNumConfigurations.              */
};

/*
 * Device Descriptor wrapper.
 */
static const USBDescriptor vcom_device_descriptor = {
    sizeof vcom_device_descriptor_data,
    vcom_device_descriptor_data
};

/* Configuration Descriptor tree for a CDC.*/
static const uint8_t vcom_configuration_descriptor_data[67] = {
    /* Configuration Descriptor.*/
    USB_DESC_CONFIGURATION(67,            /* wTotalLength.                    */
                           0x02,          /* bNumInterfaces.                  */
                           0x01,          /* bConfigurationValue.             */
                           0,             /* iConfiguration.                  */
                           0xC0,          /* bmAttributes (self powered).     */
                           50),           /* bMaxPower (100mA).               */
    /* Interface Descriptor.*/
    USB_DESC_INTERFACE    (0x00,          /* bInterfaceNumber.                */
                           0x00,          /* bAlternateSetting.               */
                           0x01,          /* bNumEndpoints.                   */
                           0x02,          /* bInterfaceClass (Communications
                                           Interface Class, CDC section
                                           4.2).                            */
                           0x02,          /* bInterfaceSubClass (Abstract
                                           Control Model, CDC section 4.3).   */
                           0x01,          /* bInterfaceProtocol (AT commands,
                                           CDC section 4.4).                */
                           0),            /* iInterface.                      */
    /* Header Functional Descriptor (CDC section 5.2.3).*/
    USB_DESC_BYTE         (5),            /* bLength.                         */
    USB_DESC_BYTE         (0x24),         /* bDescriptorType (CS_INTERFACE).  */
    USB_DESC_BYTE         (0x00),         /* bDescriptorSubtype (Header
                                           Functional Descriptor.           */
    USB_DESC_BCD          (0x0110),       /* bcdCDC.                          */
    /* Call Management Functional Descriptor. */
    USB_DESC_BYTE         (5),            /* bFunctionLength.                 */
    USB_DESC_BYTE         (0x24),         /* bDescriptorType (CS_INTERFACE).  */
    USB_DESC_BYTE         (0x01),         /* bDescriptorSubtype (Call Management
                                           Functional Descriptor).          */
    USB_DESC_BYTE         (0x00),         /* bmCapabilities (D0+D1).          */
    USB_DESC_BYTE         (0x01),         /* bDataInterface.                  */
    /* ACM Functional Descriptor.*/
    USB_DESC_BYTE         (4),            /* bFunctionLength.                 */
    USB_DESC_BYTE         (0x24),         /* bDescriptorType (CS_INTERFACE).  */
    USB_DESC_BYTE         (0x02),         /* bDescriptorSubtype (Abstract
                                           Control Management Descriptor).  */
    USB_DESC_BYTE         (0x02),         /* bmCapabilities.                  */
    /* Union Functional Descriptor.*/
    USB_DESC_BYTE         (5),            /* bFunctionLength.                 */
    USB_DESC_BYTE         (0x24),         /* bDescriptorType (CS_INTERFACE).  */
    USB_DESC_BYTE         (0x06),         /* bDescriptorSubtype (Union
                                           Functional Descriptor).          */
    USB_DESC_BYTE         (0x00),         /* bMasterInterface (Communication
                                           Class Interface).                */
    USB_DESC_BYTE         (0x01),         /* bSlaveInterface0 (Data Class
                                           Interface).                      */
    /* Endpoint 2 Descriptor.*/
    USB_DESC_ENDPOINT     (USBD1_INTERRUPT_REQUEST_EP|0x80,
                           0x03,          /* bmAttributes (Interrupt).        */
                           0x0008,        /* wMaxPacketSize.                  */
                           0xFF),         /* bInterval.                       */
    /* Interface Descriptor.*/
    USB_DESC_INTERFACE    (0x01,          /* bInterfaceNumber.                */
                           0x00,          /* bAlternateSetting.               */
                           0x02,          /* bNumEndpoints.                   */
                           0x0A,          /* bInterfaceClass (Data Class
                                           Interface, CDC section 4.5).     */
                           0x00,          /* bInterfaceSubClass (CDC section
                                           4.6).                            */
                           0x00,          /* bInterfaceProtocol (CDC section
                                           4.7).                            */
                           0x00),         /* iInterface.                      */
    /* Endpoint 3 Descriptor.*/
    USB_DESC_ENDPOINT     (USBD1_DATA_AVAILABLE_EP,       /* bEndpointAddress.*/
                           0x02,          /* bmAttributes (Bulk).             */
                           0x0040,        /* wMaxPacketSize.                  */
                           0x00),         /* bInterval.                       */
    /* Endpoint 1 Descriptor.*/
    USB_DESC_ENDPOINT     (USBD1_DATA_REQUEST_EP|0x80,    /* bEndpointAddress.*/
                           0x02,          /* bmAttributes (Bulk).             */
                           0x0040,        /* wMaxPacketSize.                  */
                           0x00)          /* bInterval.                       */
};

/*
 * Configuration Descriptor wrapper.
 */
static const USBDescriptor vcom_configuration_descriptor = {
    sizeof vcom_configuration_descriptor_data,
    vcom_configuration_descriptor_data
};

/*
 * U.S. English language identifier.
 */
static const uint8_t vcom_string0[] = {
    USB_DESC_BYTE(4),                     /* bLength.                         */
    USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
    USB_DESC_WORD(0x0409)                 /* wLANGID (U.S. English).          */
};

/*
 * Vendor string.
 */
static const uint8_t vcom_string1[] = {
    USB_DESC_BYTE(38),                    /* bLength.                         */
    USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
    'S', 0, 'T', 0, 'M', 0, 'i', 0, 'c', 0, 'r', 0, 'o', 0, 'e', 0,
    'l', 0, 'e', 0, 'c', 0, 't', 0, 'r', 0, 'o', 0, 'n', 0, 'i', 0,
    'c', 0, 's', 0
};

/*
 * Device Description string.
 */
static const uint8_t vcom_string2[] = {
    USB_DESC_BYTE(56),                    /* bLength.                         */
    USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
    'C', 0, 'h', 0, 'i', 0, 'b', 0, 'i', 0, 'O', 0, 'S', 0, '/', 0,
    'R', 0, 'T', 0, ' ', 0, 'V', 0, 'i', 0, 'r', 0, 't', 0, 'u', 0,
    'a', 0, 'l', 0, ' ', 0, 'C', 0, 'O', 0, 'M', 0, ' ', 0, 'P', 0,
    'o', 0, 'r', 0, 't', 0
};

/*
 * Serial Number string.
 */
static const uint8_t vcom_string3[] = {
    USB_DESC_BYTE(8),                     /* bLength.                         */
    USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
    '0' + CH_KERNEL_MAJOR, 0,
    '0' + CH_KERNEL_MINOR, 0,
    '0' + CH_KERNEL_PATCH, 0
};

/*
 * Strings wrappers array.
 */
static const USBDescriptor vcom_strings[] = {
    {sizeof vcom_string0, vcom_string0},
    {sizeof vcom_string1, vcom_string1},
    {sizeof vcom_string2, vcom_string2},
    {sizeof vcom_string3, vcom_string3}
};

/*
 * Handles the GET_DESCRIPTOR callback. All required descriptors must be
 * handled here.
 */
static const USBDescriptor *get_descriptor(USBDriver *usbp,
                                           uint8_t dtype,
                                           uint8_t dindex,
                                           uint16_t lang) {
    
    (void)usbp;
    (void)lang;
    switch (dtype) {
        case USB_DESCRIPTOR_DEVICE:
            return &vcom_device_descriptor;
        case USB_DESCRIPTOR_CONFIGURATION:
            return &vcom_configuration_descriptor;
        case USB_DESCRIPTOR_STRING:
            if (dindex < 4)
                return &vcom_strings[dindex];
    }
    return NULL;
}

/**
 * @brief   IN EP1 state.
 */
static USBInEndpointState ep1instate;

/**
 * @brief   OUT EP1 state.
 */
static USBOutEndpointState ep1outstate;

/**
 * @brief   EP1 initialization structure (both IN and OUT).
 */
static const USBEndpointConfig ep1config = {
    USB_EP_MODE_TYPE_BULK,
    NULL,
    sduDataTransmitted,
    sduDataReceived,
    0x0040,
    0x0040,
    &ep1instate,
    &ep1outstate,
    2,
    NULL
};

/**
 * @brief   IN EP2 state.
 */
static USBInEndpointState ep2instate;

/**
 * @brief   EP2 initialization structure (IN only).
 */
static const USBEndpointConfig ep2config = {
    USB_EP_MODE_TYPE_INTR,
    NULL,
    sduInterruptTransmitted,
    NULL,
    0x0010,
    0x0000,
    &ep2instate,
    NULL,
    1,
    NULL
};

/*
 * Handles the USB driver global events.
 */
static void usb_event(USBDriver *usbp, usbevent_t event) {
    
    switch (event) {
        case USB_EVENT_RESET:
            return;
        case USB_EVENT_ADDRESS:
            return;
        case USB_EVENT_CONFIGURED:
            chSysLockFromIsr();
            
            /* Enables the endpoints specified into the configuration.
             Note, this callback is invoked from an ISR so I-Class functions
             must be used.*/
            usbInitEndpointI(usbp, USBD1_DATA_REQUEST_EP, &ep1config);
            usbInitEndpointI(usbp, USBD1_INTERRUPT_REQUEST_EP, &ep2config);
            
            /* Resetting the state of the CDC subsystem.*/
            sduConfigureHookI(&SDU1);
            
            chSysUnlockFromIsr();
            return;
        case USB_EVENT_SUSPEND:
            return;
        case USB_EVENT_WAKEUP:
            return;
        case USB_EVENT_STALLED:
            return;
    }
    return;
}

/*
 * USB driver configuration.
 */
static const USBConfig usbcfg = {
    usb_event,
    get_descriptor,
    sduRequestsHook,
    NULL
};

/*
 * Serial over USB driver configuration.
 */
static const SerialUSBConfig serusbcfg = {
    &USBD1,
    USBD1_DATA_REQUEST_EP,
    USBD1_DATA_AVAILABLE_EP,
    USBD1_INTERRUPT_REQUEST_EP
};


/*===========================================================================*/
/* USB related stuff. */
/*===========================================================================*/

/*
* Endpoints to be used for USBD1.
*/
#define USBD1_DATA_REQUEST_EP 1
#define USBD1_DATA_AVAILABLE_EP 1
#define USBD1_INTERRUPT_REQUEST_EP 2


/* Generic large buffer.*/
static uint8_t fbuff[1024];

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


void cmd_ledctrl(BaseSequentialStream *chp, int argc, char *argv[]) {
	int i;
	if (argc >= 1 && strcmp(argv[0], "test1") == 0) {
		chprintf(chp,"Red ...\r\n");
		for(i=0; i < LEDSTRIPE_FRAMEBUFFER_SIZE; i++) {
			ledstripe_framebuffer[i].red = 255;
			ledstripe_framebuffer[i].green = 0;
			ledstripe_framebuffer[i].blue = 0;
		}
		chThdSleepMilliseconds(5000);
		chprintf(chp,"Green ...\r\n");
		for(i=0; i < LEDSTRIPE_FRAMEBUFFER_SIZE; i++) {
			ledstripe_framebuffer[i].green = 255;
			ledstripe_framebuffer[i].red = 0;
			ledstripe_framebuffer[i].blue = 0;
		}
		chThdSleepMilliseconds(5000);
		chprintf(chp,"Blue ...\r\n");
		for(i=0; i < LEDSTRIPE_FRAMEBUFFER_SIZE; i++) {
			ledstripe_framebuffer[i].blue = 255;
			ledstripe_framebuffer[i].red = 0;
			ledstripe_framebuffer[i].green = 0;
		}
		chThdSleepMilliseconds(5000);
	}
	else if (argc >= 1 && strcmp(argv[0], "on") == 0) {
		for(i=0; i < LEDSTRIPE_FRAMEBUFFER_SIZE; i++) {
			ledstripe_framebuffer[i].red = 255;
			ledstripe_framebuffer[i].green = 255;
			ledstripe_framebuffer[i].blue = 255;
		}
	} else if (argc >= 1 && strcmp(argv[0], "off") == 0) {
		for(i=0; i < LEDSTRIPE_FRAMEBUFFER_SIZE; i++) {
			ledstripe_framebuffer[i].red = 0;
			ledstripe_framebuffer[i].green = 0;
			ledstripe_framebuffer[i].blue = 0;
		}
	} else if (argc >= 4 && strcmp(argv[0], "all") == 0) {
		int red = atoi(argv[1]);
		int green = atoi(argv[2]);
		int blue = atoi(argv[3]);
		for(i=0; i < LEDSTRIPE_FRAMEBUFFER_SIZE; i++) {
			ledstripe_framebuffer[i].red = red;
			ledstripe_framebuffer[i].green = green;
			ledstripe_framebuffer[i].blue = blue;
		}
	}
	else if (strcmp(argv[0], "show") == 0)
	{
		int i, width, height = 0;
		dmx_getScreenresolution(&width, &height);

		if (width == 0 && height == 0)
		{ /* Display the complete DMX universe */
			for (i = 0; i < LEDSTRIPE_FRAMEBUFFER_SIZE; i++)
			{
				chprintf(chp, "%.2X%", dmx_fb[i]);
			}
		}
		else
		{
			/* We have valid information! Let's display the wall on the shell */
			chprintf(chp, "LED is filled with %d x %d pixel\r\n", width, height);
			for (i = 0; i < width * height; i++)
			{
				chprintf(chp, "%.2X%.2X%.2X|",
				ledstripe_framebuffer[i].red,
				ledstripe_framebuffer[i].green,
				ledstripe_framebuffer[i].blue);

				/* generate a new line after each row */
				if ((i + 1) % width == 0)
				{
					chprintf(chp, "\r\n");
				}
			}
			chprintf(chp, "\r\n");
		}
	}
	else /* Usage */
	{
		chprintf(chp, "possible arguments are:\r\n"
				"- test1\r\n"
				"- all (red) (green) (blue)\tSet the last box\r\n"
				"- on\r\n"
				"- off\r\n");
	}
}


/*===========================================================================*/
/* Manage all possible commands		*/
/*===========================================================================*/

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
#ifdef WS2811_WALL
    { "led", cmd_ledctrl },
#endif
    { "flash", cmd_flash },
    { NULL, NULL } };

static const ShellConfig shell_cfg1 =
  { (BaseSequentialStream *) &SD6, commands };

static const ShellConfig shell_cfg2 = {
    (BaseSequentialStream *)&SDU1,
    commands
};

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
    static Thread *shelltp = NULL;
    int i, red, green, blue;

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
  sduObjectInit(&SDU1);
  sduStart(&SDU1, &serusbcfg);

  /*
  * Activates the USB driver and then the USB bus pull-up on D+.
  * Note, a delay is inserted in order to not have to disconnect the cable
  * after a reset.
  */
   usbDisconnectBus(serusbcfg.usbp);
   chThdSleepMilliseconds(1500);
   usbStart(serusbcfg.usbp, &usbcfg);
   usbConnectBus(serusbcfg.usbp);

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
  /*
   * Activates the SDC driver 1 using default configuration.
   */
  sdcStart(&SDCD1, NULL);

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

#ifdef WS2811_WALL
  chprintf((BaseSequentialStream *) &SD6, "Initialazing WS2811 driver ...");
  /*
   * Initialize LedDriver
   */
  ledstripe_init();
  chprintf((BaseSequentialStream *) &SD6, " Done\r\n");
#endif

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
	  if (!shelltp && (SDU1.config->usbp->state == USB_ACTIVE))
		  shelltp = shellCreate(&shell_cfg2, SHELL_WA_SIZE, NORMALPRIO);
	  else if (chThdTerminated(shelltp)) {
		  chThdRelease(shelltp); /* Recovers memory of the previous shell. */
		  shelltp = NULL; /* Triggers spawning of a new shell. */
	  }
      chEvtDispatch(evhndl, chEvtWaitOneTimeout(ALL_EVENTS, MS2ST(500)));

#ifdef UGFX_WALL
      fcwall_processEvents(&SDU1);
#endif

        int offset=0;
		if (palReadPad(GPIOA, GPIOA_BUTTON))
		{
			if (ledstripe_framebuffer[offset].red > 0
					&& ledstripe_framebuffer[offset].green >0
					&& ledstripe_framebuffer[offset].blue >0)
			{
				red = 255;
				green = 0;
				blue = 0;
			}
			else if (ledstripe_framebuffer[offset].red > 0)
			{
				red = 0;
				green = 255;
				blue = 0;
			}
			else if (ledstripe_framebuffer[offset].green > 0)
			{
				red = 0;
				green = 0;
				blue = 255;
			}
			else if (ledstripe_framebuffer[offset].blue > 0)
			{
				red = 0;
				green = 0;
				blue = 0;
			}
			else
			{
				red = green = blue = 255;
			}
			chprintf((BaseSequentialStream *) &SD6, "Set %2X%2X%2X (RRGGBB)\r\n", red, green, blue);

			/* Update the end of the stripe */
			for(i=offset; i < LEDSTRIPE_FRAMEBUFFER_SIZE; i++) {
				ledstripe_framebuffer[i].red = red;
				ledstripe_framebuffer[i].green = green;
				ledstripe_framebuffer[i].blue = blue;
			}

		}
    }
}

