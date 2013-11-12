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

/*
 * This file is a modified version of the lwIP web server demo. The original
 * author is unknown because the file didn't contain any license information.
 */

/**
 * @file web.c
 * @brief HTTP server wrapper thread code.
 * @addtogroup WEB_THREAD
 * @{
 */

#include <string.h>
#include "ch.h"

#include "pro/uhttp/mediatypes.h"
#include "pro/uhttp/modules/mod_redir.h"

#include "web.h"


/**
 * Stack area for the http thread.
 */
WORKING_AREA(wa_http_server, WEB_THREAD_STACK_SIZE);

/**
 * HTTP server thread.
 */
msg_t http_server(void *p) {
  chRegSetThreadName("httpd");
  (void)p;
  
  /* 
    * Initialize the TCP socket interface. 
    */
   StreamInit();

   /* 
    * Register media type defaults. These are configurable
    * in include/cfg/http.h or the Nut/OS Configurator.
    */
   MediaTypeInitDefaults();
   HttpRegisterRedir("", "/index.html", 301);  
   
   /* Goes to the final priority after initialization.*/
   chThdSetPriority(WEB_THREAD_PRIORITY);


   /* 
    * Wait for a client (browser) and handle its request.
    * This function will only return on unrecoverable errors. 
    */
   StreamClientAccept(HttpdClientHandler, NULL);
   
   /* Typically this point will be never reached. */
   while(1)
   {
      chThdSleepMilliseconds(500);
   }        
  
  return RDY_OK;
  /* HttpTask */
}

/** @} */
