/**************************************************************************
*  Copyright (c) 2013 by Michael Fischer.
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without 
*  modification, are permitted provided that the following conditions 
*  are met:
*  
*  1. Redistributions of source code must retain the above copyright 
*     notice, this list of conditions and the following disclaimer.
*  2. Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in the 
*     documentation and/or other materials provided with the distribution.
*  3. Neither the name of the author nor the names of its contributors may 
*     be used to endorse or promote products derived from this software 
*     without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL 
*  THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS 
*  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
*  AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
*  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF 
*  THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
*  SUCH DAMAGE.
*
***************************************************************************
*  History:
*
*  18.06.2013  mifi  First Version, tested with a STM3240G-EVAL board
**************************************************************************/
#define __IO_C__

/*=======================================================================*/
/*  Includes                                                             */
/*=======================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "io.h"
#include "sys/stat.h"
#include "fs/uromfs.h"

/*=======================================================================*/
/*  All Structures and Common Constants                                  */
/*=======================================================================*/

/*=======================================================================*/
/*  Definition of all global Data                                        */
/*=======================================================================*/

/*=======================================================================*/
/*  Definition of all local Data                                         */
/*=======================================================================*/

static ROMENTRY Testimage = 
{
   NULL,
   "testimg.bin",
   (1024L*1024L),
   (const char*)0x08000000
};

/*=======================================================================*/
/*  Definition of all local Procedures                                   */
/*=======================================================================*/

/*=======================================================================*/
/*  All code exported                                                    */
/*=======================================================================*/

int _open (const char *name, int mode)
{
   int       res = -1;
   ROMENTRY *rome;
   ROMFILE  *romf = NULL;
   
   (void)mode;

   /* check for "UROM:" */
   if( (name[0] == 'U') && (name[1] == 'R') && 
       (name[2] == 'O') && (name[3] == 'M') && (name[4] == ':') )
   {
      name += 5;

      if (*name == '/') 
      {
         name++;
      }

      /* Search file */
      for (rome = romEntryList; rome; rome = rome->rome_next) 
      {
         if (strcmp(name, rome->rome_name) == 0)
         {
            break;
         }   
      }

      /* Check if file was found */
      if (rome) 
      {
         /* Create ROMFILE */
         if ((romf = calloc(1, sizeof(ROMFILE))) != 0)
         {
            romf->romf_entry = rome;
            res = (int)romf;
         }
      }     
      
   } /* end if UROM: */  
   
   
   /* Check for "testimg.bin" */   
   if (res == -1)
   {
      if (strcmp(name, "testimg.bin") == 0)
      {
         if ((romf = calloc(1, sizeof(ROMFILE))) != 0)
         {
            romf->romf_entry = &Testimage;
            res = (int)romf;
         }
      }
   } /* end if "testimg.bin" */
   
   return(res);
} /* _open */


int _close (int fd)
{
   ROMFILE *romf = (ROMFILE*)fd;

   if (romf != 0)
   {
      free(romf);
   }
   
   return(0);
} /* _close */


int _read (int fd, void *buffer, size_t size)
{
   ROMFILE  *romf = (ROMFILE*)fd;
   ROMENTRY *rome = romf->romf_entry;

   if (romf != 0)
   {
      if ((unsigned int) size > rome->rome_size - romf->romf_pos)
      {
         size = rome->rome_size - romf->romf_pos;
      }
         
      if (size) 
      {
         memcpy(buffer, (rome->rome_data + romf->romf_pos), size);
         romf->romf_pos += size;
      }
   }
   
   return((int)size);
} /* _read */

int _seek (int fd, long offset, int origin)
{
   (void)fd;
   (void)offset;
   (void)origin;
   
   return(-1);
} /* _seek */


long _filelength (int fd)
{
   long      size = 0;
   ROMFILE  *romf = (ROMFILE*)fd;
   ROMENTRY *rome = romf->romf_entry;

   if (romf != 0)
   {
      size = (long)rome->rome_size;
   }
   
   return(size);
} /* _filelength */


int fstat (int fh, struct stat *s)
{
   (void)fh;
   (void)s;
   
   return(-1);
} /* fstat */

/*** EOF ***/


