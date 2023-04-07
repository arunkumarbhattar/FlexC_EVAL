/* The  file contins routines for managing debugging 
   Copyright (C) 2000 Kalum Somaratna 
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif				/*
				 * HAVE_CONFIG_H 
				 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "runtime.h"
#include "debug.h"




/******************************************************************************
Initialises the debug system, deletes any prior debug.log file if present 
******************************************************************************/void debug_init()
{

 if (unlink("debug.log") == -1)
    {
      /*
       * if the file is not present the continue silently 
       */
      if (errno == ENOENT)
	return;
      else
      {
	debug_prz("unable to delete the file %s. Reason-: %s",
			      strerror(errno));       
      }
    }
      return;
}

/*
 * needed for the debug_prz(...) function 
 */
pthread_mutex_t debug_mutex = PTHREAD_MUTEX_INITIALIZER;

void debug_prz(const char *args, ...)
{
    char p[MAX_MSG_SIZE];
    FILE *fp;

    /*lock mutex */
    pthread_mutex_lock(&debug_mutex);

    if (rt.debug_mode == TRUE)
    {
	va_list vp;
	va_start(vp, args);
	vsprintf(p, args, vp);
	va_end(vp);

	if (!(fp = fopen("debug.log", "at")))
	{
	    pthread_mutex_unlock(&debug_mutex);
	    return;
	}


	if (fwrite(p, 1, strlen(p), fp) != strlen(p))
	{
	    pthread_mutex_unlock(&debug_mutex);
	    fclose(fp);
	    return;
	}

	fclose(fp);
    }

    /*unlock mutes */
    pthread_mutex_unlock(&debug_mutex);
    return;
}
