/* This conatins routines to do with the connections.
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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <pthread.h>

#include <sys/vfs.h>

#include "connection.h"
#include "misc.h"
#include "connect.h"
#include "ftp.h"
#include "debug.h"
#include "runtime.h"
#include "interface.h"

/* If all the downloads have been done ok returns TRUE */

int all_dls_complete(connection_data * connections, int num_con)
{
    int i;
    for (i = 0; i < num_con; i++)
    {
	if (connections[i].status != COMPLETED)
	    return FALSE;
    }
    return TRUE;
}

/* If all the downloads have been refused to login, returns TRUE */
int all_dls_failed_login(connection_data * connections, int num_con)
{
    int i;
    for (i = 0; i < num_con; i++)
    {
	if (connections[i].status != LOGINFAIL)
	    return FALSE;
    }
    return TRUE;

}

/* If all the downloads have been refused to connect, returns TRUE */
int all_dls_connect_rejected(connection_data * connections, int num_con)
{
    int i;
    for (i = 0; i < num_con; i++)
    {
	if (connections[i].status != CONREJECT)
	    return FALSE;
    }
    return TRUE;
}

/* If all the downloads have encountered remote fatal errors returns TRUE */
int all_dls_remote_failed(connection_data * connections, int num_con)
{
    int i;
    for (i = 0; i < num_con; i++)
    {
	if (connections[i].status != REMOTEFATAL)
	    return FALSE;
    }
    return TRUE;
}


/* Returns the number of connections whose status is COMPLETED ie (completed)*/
int query_completed_conns_count(connection_data * connections, int num_con)
{
    int i;
    int count = 0;

    for (i = 0; i < num_con; i++)
    {
	if (connections[i].status == COMPLETED)
	    count++;
    }
    return count;
}

/* Returns the number of connections whose status is CONNECTING ie (connecting to server)*/
int query_connecting_conns_count(connection_data * connections,
				 int num_con)
{
    int i;
    int count = 0;

    for (i = 0; i < num_con; i++)
    {
	if (connections[i].status == CONNECTING)
	    count++;
    }
    return count;
}

/* Returns the number of connections whose status is LOGGININ ie (logging to server)*/
int query_logging_conns_count(connection_data * connections, int num_con)
{
    int i;
    int count = 0;

    for (i = 0; i < num_con; i++)
    {
	if (connections[i].status == LOGGININ)
	    count++;
    }
    return count;
}

/* Returns the number of connections whose status is DOWNLOADING */
int query_downloading_conns_count(connection_data * connections,
				  int num_con)
{
    int i;
    int count = 0;

    for (i = 0; i < num_con; i++)
    {
	if (connections[i].status == DOWNLOADING)
	    count++;
    }
    return count;
}

/* Returns the number of connections whose status is DOWNLOADING */
int query_remote_errror_conns_count(connection_data * connections,
				    int num_con)
{
    int i;
    int count = 0;

    for (i = 0; i < num_con; i++)
    {
	if (connections[i].status == REMOTEFATAL
	    || connections[i].status == TIMEDOUT)
	    count++;
    }
    return count;
}



/* Returns the number of connections whose status is LOGINFAIL */
int query_loginfail_connections_count(connection_data * connections,
				      int num_con)
{
    int i;
    int count = 0;

    for (i = 0; i < num_con; i++)
    {
	if (connections[i].status == LOGINFAIL)
	    count++;
    }
    return count;
}





/*This cats all the downloaded protions into one big file */
int join_downloads(char *output_file, connection_data * connections,
		   int num_con)
{

  const long FILE_CHUNK = 0xffff;
  FILE *fp;
  FILE *dl_file;
  char *buffer;
  int i, j;
  long total_written = 0;
  long long main_file_size = connections[0].main_file_size;

  char *prefixed_file = get_prefixed_file(output_file);
  struct statfs fs_info;
  unsigned long long free_space;

  buffer = (char *) kmalloc(FILE_CHUNK);
  if (!buffer)
    {
      free(prefixed_file);
      die("Unable to allocate %d bytes to recreate files", FILE_CHUNK);
    }

  if (!(fp = fopen(prefixed_file, "wb")))
    {
      free(buffer);
      die("Error: unable to open the file %s for writing-: %s\n",
	  prefixed_file, strerror(errno));									   
    }
  
  
  /* Stat file system */
  if(statfs(prefixed_file, &fs_info)!=0)
    {
      die("Error: unable to stat the file system for writing-: %s\n",
	  strerror(errno));
    }

  free_space =(unsigned long long)fs_info.f_bsize*(unsigned long long)fs_info.f_bavail;

  debug_prz("f_bsize = %Ld",   (unsigned long long)fs_info.f_bsize);
  debug_prz("f_bavail=%Ld",  (unsigned long long) fs_info.f_bavail);
	  
  debug_prz("Free space avaialble = %Ld", free_space);
  debug_prz("main_file_size = %ld", main_file_size);  


  /*Checking for free space */
  if(main_file_size>0)
    {
      /* TODO this currently uses f_bavail, which assumes that the user is not the superuser
	 , so one day get username and see */
 
      if(main_file_size>free_space)
	{
	  int ret;
	  if(rt.force_mode==FALSE)
	    {
	      do
		{
		  ret =
		    curses_query_user_input
		    ("Warning: You do not appear to have sufficient free space to build the file!\n"
		     "You will need to freeup %ld KB more, (A)bort,(C)ontinue?",(main_file_size-free_space));
		}
	      while (ret != 'C' && ret != 'A');

	      switch (ret)
		{
		case 'A':
		  if (unlink(prefixed_file) == -1)
		    {
		  
		      message("unable to delete the file %s before exiting. Reason-: %s",
			      prefixed_file, strerror(errno));
		  
		    }
		  die("Ok..Aborting...please free up the space and relaunch me ");
		  break;
		case 'C':	
		  break;
		}
	    }

	}
    }

  for (i = 0; i < num_con; i++)
    {
      if (!(dl_file = fopen(connections[i].localfile, "rb")))
	{
	  free(buffer);
	  free(prefixed_file);
	  die("Error: Unable to open the file %s for reading-: %s\n",
	      connections[i].localfile, strerror(errno));
	}

      while ((j = fread(buffer, sizeof(char), FILE_CHUNK, dl_file)))
	{

	  if (main_file_size > 0)
	    {
	      total_written += j;
	      message("Rebuilding file %.1f percent completed...",
		      (float) total_written * 100 /
		      (float) main_file_size);
	    } else
	      message("Rebuilding file %d percent completed...",
		      i * 100 / num_con);

	  if (fwrite(buffer, sizeof(char), j, fp) != j)
	    {

	      /*Shit! a error occured delte file before exiting */

	      if (unlink(prefixed_file) == -1)
		{
		  
		  message("unable to delete the file %s before exiting. Reason-: %s",
			  prefixed_file, strerror(errno));
		  
		}
	      free(buffer);
	      die("Error:A write error occured while writing to  %s -: %s\n", prefixed_file, strerror(errno));
	    }
	}
      fclose(dl_file);
    }

  fclose(fp);
  free(buffer);
  free(prefixed_file);
  return 1;
}



/*Cleans up the downlaoded file portions */
int delete_downloads(connection_data * connections, int num_con)
{
    int i;
    for (i = 0; i < num_con; i++)
    {
	if (unlink(connections[i].localfile) == -1)
	{
	    /*
	     * if the file is not present the continue silently 
	     */
	    if (errno == ENOENT)
		continue;
	    else
	    {
		message("unable to delete the file %s. Reason-: %s",
			connections[i].localfile, strerror(errno));
		return -1;
	    }
	}

    }
    return 1;
}

void calc_con_ratebps(connection_data * connection)
{
    struct timeval tv_cur;
    struct timeval tv_diff;
    float diff_us;

    if (connection->time_begin.tv_sec == 0
	&& connection->time_begin.tv_usec == 0)
    {
	connection->rate_bps = 0;
	return;
    } else
    {
	gettimeofday(&tv_cur, NULL);
	timeval_subtract(&tv_diff, &tv_cur, &(connection->time_begin));
	diff_us = ((float) tv_diff.tv_sec * 1000000) + tv_diff.tv_usec;

	if (diff_us == 0)
	{
	    return;
	}
	connection->rate_bps =
	    ((float) connection->remote_bytes_received * 10e5 / diff_us);
    }

    return;
}

void throttle_con_rate(connection_data * connection)
{

    struct timeval tv_cur;
    struct timeval tv_diff;
    float diff_us;
    float wtime;
    struct timeval tv_delay;
    /*fixme */
    extern pthread_mutex_t compute_throttle_mutex;

    pthread_mutex_lock(&compute_throttle_mutex);
    pthread_mutex_unlock(&compute_throttle_mutex);

    /* fixme */
    if (connection->rate_bps == 0 || connection->max_allowed_bps == 0)
	return;

    if (connection->time_begin.tv_sec == 0
	&& connection->time_begin.tv_usec == 0)
	return;

    gettimeofday(&tv_cur, NULL);
    timeval_subtract(&tv_diff, &tv_cur, &(connection->time_begin));
    diff_us = ((float) tv_diff.tv_sec * 1000000) + tv_diff.tv_usec;

    if (diff_us == 0)
    {
	return;
    }


    wtime =
	10e5 * connection->remote_bytes_received /
	connection->max_allowed_bps;

    memset(&tv_delay, 0, sizeof(tv_delay));

    if (wtime > diff_us)
    {
	/*too fast have to delay */

	if ((wtime - diff_us) > (rt.timeout * 10e5))	/* problem here */
	{
	    /*If we were to delay for wtime-diff_us we would cause a connection 
	       timeout, so rather than doing that shall we delay for a bit lesser
	       than the time for the timeout, like say 1 second less
	     */
	    const int limit_time_us = 2 * 10e5;

	    /*  Will rt,timeout - limit_time_us  be less or equal to  0?
	       If so no point in delaing just print a messagethat we cant 
	       throttle because of the connection timing out 
	     */

	    if (((rt.timeout * 10e5) - limit_time_us) <= 0)
	    {
		message
		    ("Cant throttle: Connection would timeout if done so, please try increasing the timeout value");
		return;
	    }

	    tv_delay.tv_usec = (rt.timeout * 10e5) - limit_time_us;
	    message
		("Cant throttle fully : Connection would timeout if done so, please try increasing the timeout value");

	    debug_prz("delaymaxlimit %ld sec\n", tv_delay.tv_usec);
	} else
	{
	    tv_delay.tv_usec = (wtime - diff_us);
	    debug_prz("sleeping %f secs\n", (wtime - diff_us) / 10e5);
	}

	tv_delay.tv_sec = tv_delay.tv_usec / 1000000;
	tv_delay.tv_usec = tv_delay.tv_usec % 1000000;

	if (select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &tv_delay)
	    < 0)
	{
	    debug_prz("Unable to throttle Bandwith\n");
	}
    }
}


void calc_throttle_factor(connection_data * connections,
			  int num_connections)
{
    int i;
    int num_slow_cons = 0;
    long t_slow_rates = 0;
    long limit_high_cons_rate;
    long avg_rate;
    int num_dl_cons= query_downloading_conns_count(connections, num_connections);

    if(num_dl_cons==0)
      return;

    avg_rate= rt.max_bps / num_dl_cons;


    if (rt.max_bps == 0)
      {
	for (i = 0; i < num_connections; i++)
	  connections[i].max_allowed_bps = 0;
	return;
      }

    /*MAKE IR USE THE NUMBER OF ACTIVE DOWNLOAdING CONENCTIONS: Done */

    for (i = 0; i < num_connections; i++)
    {
	if ((connections[i].status == DOWNLOADING)
	    && connections[i].rate_bps < avg_rate)
	{
	    t_slow_rates += connections[i].rate_bps;
	    num_slow_cons++;
	}
    }


    /*fixme mutex to preven this conenctions */
    if (num_slow_cons > num_dl_cons)
	num_dl_cons = num_slow_cons;


    /*If all the connections are slower then no need to do anything */

    if (num_slow_cons == num_dl_cons)
    {
	for (i = 0; i < num_connections; i++)
	{
	    connections[i].max_allowed_bps = 0;
	}
	return;
    }


    limit_high_cons_rate =
	(rt.max_bps - t_slow_rates) / (num_dl_cons - num_slow_cons);

    debug_prz("limit_high_cons_rate = %ld", limit_high_cons_rate);

    for (i = 0; i < num_connections; i++)
    {
	if ((connections[i].status == DOWNLOADING)
	    && connections[i].rate_bps >= avg_rate)
	{
	    connections[i].max_allowed_bps = limit_high_cons_rate;
	}
    }
}
