/* Functions to help with  retreiving HTTP files
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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif				/*
				 * HAVE_CONFIG_H 
				 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <assert.h>

#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#else
#include <curses.h>
#endif				/*
				 * HAVE_CURSES 
				 */

#include "connect.h"
#include "runtime.h"
#include "url.h"
#include "misc.h"
#include "resume.h"
#include "main.h"
#include "mainconf.h"
#include "connection.h"
#include "http.h"
#include "http-retr.h"
#include "debug.h"

void clean_httpsock(void *cdata)
{
    int flags;
    connection_data *connection = (connection_data *) cdata;

    debug_prz("in clean http sock\n");

    if (connection->http_sock > 0)
    {
	flags = fcntl(connection->http_sock, F_GETFD, 0);
	if (flags == -1)
	{
	    debug_prz("sock invalid\n");
	} else
	    close(connection->http_sock);
    }

}

uerr_t http_get_file_chunk(connection_data * connection)
{

    char buffer[HTTP_BUFFER_SIZE];
    char *user, *passwd, *wwwauth, *referer=0;
    int tos = IPTOS_THROUGHPUT;
    netrc_entry *netrc_ent;
    uerr_t err;
    /*
     * did we get a filename? 
     */

    assert(connection->localfile != NULL);
    assert(connection->file_mode != NULL);

    /*clear the socks */
    connection->http_sock = 0;
    /*
     * set the thread attributes 
     */
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    connection->hs.len = 0L;
    connection->hs.contlen = -1;
    connection->hs.res = -1;
    connection->hs.newloc = NULL;
    connection->hs.remote_time = NULL;
    connection->hs.error = NULL;


    /*
     * if there is nothing to download then return 
     */
    if (connection->status == COMPLETED)
    {
	gettimeofday(&connection->time_begin, NULL);
	return HOK;
    }

    /*
     * Lets test and see wether the file length to be got is zero  
     * If so then there is no point in connecting.. 
     * just create the file (zero length) and return  
     * have to resort to a ugly method to detect this :( 
     */

    if (connection->remote_startpos == 0 && connection->remote_endpos < 0)
    {
	if (!
	    (connection->fp =
	     fopen(connection->localfile, connection->file_mode)))
	{
	    message("Error opening file %s for writing: %s",
		    connection->localfile, strerror(errno));
	    connection->status = LOCALFATAL;
	    return WRITEERR;
	}
	fclose(connection->fp);
	connection->status = COMPLETED;
	return HOK;
    }

    connection->status = CONNECTING;
    err =
	connect_to_server(&(connection->http_sock), connection->u.host,
			  connection->u.port, rt.timeout);
    if (err != NOCONERROR)
    {
	message("Error connecting to %s", connection->u.host);
	connection->status = REMOTEFATAL;
	return err;
    }


    /* Authentification code */
    user = connection->u.user;
    passwd = connection->u.passwd;

    /*
     * Use .netrc if asked to do so. 
     */
    if (rt.use_netrc == TRUE)
    {
	netrc_ent = search_netrc(rt.netrc_list, connection->u.host);
	if (netrc_ent != NULL)
	{
	    user = netrc_ent->account;
	    passwd = netrc_ent->password;
	}
    }

    user = user ? user : "";
    passwd = passwd ? passwd : "";

    if (strlen(user) || strlen(passwd))
    {
	/*Construct the necessary header */
	wwwauth = get_basic_auth_str(user, passwd);
	message("Authenticating as user %s password %s", user, passwd);

	debug_prz("Authentification string=%s\n", wwwauth);
    } else
	wwwauth = 0;

    if(connection->u.referer)
      {
	referer= (char *) alloca(13+strlen(connection->u.referer));
	sprintf(referer, "Referer: %s\r\n", connection->u.referer);
      }

    /*
     * get the headers by sending GET 
     */
    sprintf(buffer,
	    "GET %s HTTP/1.0\r\nUser-Agent: %s%s\r\nHost: %s\r\nAccept: */*\r\nRange: bytes=%ld-%ld\r\n%s%s\r\n",
	    connection->u.path, PACKAGE_NAME, VERSION,
	    connection->u.host,
	    connection->remote_startpos, connection->remote_endpos,
	    referer ? referer : "",
	    wwwauth ? wwwauth : "");


    debug_prz("HTTP request= %s\n", buffer);

    err =
	http_fetch_headers(connection->http_sock, &(connection->u),
			   &(connection->hs), buffer);
    if (wwwauth)
	free(wwwauth);

    if (err != HOK)
    {
	/*Check if we authenticated using any user or password and if we 
	   were kicked out, if so return HAUTHFAIL */
	if (err == HAUTHREQ && (strlen(user) || strlen(passwd)))
	    err = HAUTHFAIL;
	/*
	 * a error occured druing the process 
	 */
	close(connection->http_sock);
	connection->status = REMOTEFATAL;
	return err;
    }

    if (!
	(connection->fp =
	 fopen(connection->localfile, connection->file_mode)))
    {
	message("Error opening file %s for writing: %s",
		connection->localfile, strerror(errno));
	close(connection->http_sock);
	connection->status = LOCALFATAL;
	return FOPENERR;
    }

    /*
     * prioritize packets 
     */
    setsockopt(connection->http_sock, IPPROTO_IP, IP_TOS, (char *) &tos,
	       sizeof(tos));

    /*
     * Make sure all writes go directly to the file 
     */
    setvbuf(connection->fp, NULL, _IONBF, 0);
    connection->status = DOWNLOADING;
    /*
     * lets store the start time in the connections structure
     * if this is the first time we have started 
     */
    if ((connection->time_begin.tv_sec == 0)
	&& (connection->time_begin.tv_usec == 0))
	gettimeofday(&connection->time_begin, NULL);

    err =
	http_retr_fsize_known(connection, connection->http_sock,
			      connection->fp);
    close(connection->http_sock);

    return err;

}


/* This function will get the complete file*/
/* Used when the server doesn't support reteving the file in chunks */

uerr_t http_get_complete_file(connection_data * connection)
{
    char buffer[HTTP_BUFFER_SIZE];
    char *user, *passwd, *wwwauth, *referer=0;
    int tos = IPTOS_THROUGHPUT;
    netrc_entry *netrc_ent;
    uerr_t err;

    /* did we get a filename? */

    assert(connection->localfile != NULL);
    assert(connection->file_mode != NULL);

    /*clear the socks */
    connection->http_sock = 0;

    /*
     * set the thread attributes 
     */
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    connection->hs.len = 0L;
    connection->hs.contlen = -1;
    connection->hs.res = -1;
    connection->hs.newloc = NULL;
    connection->hs.remote_time = NULL;
    connection->hs.error = NULL;

    /*
     * if there is nothing to download then return 
     */
    if (connection->status == COMPLETED)
    {
	gettimeofday(&(connection->time_begin), NULL);
	return HOK;
    }

    connection->status = CONNECTING;
    err =
	connect_to_server(&(connection->http_sock), connection->u.host,
			  connection->u.port, rt.timeout);
    if (err != NOCONERROR)
    {
	message("Error connecting to %s", connection->u.host);
	connection->status = REMOTEFATAL;
	return err;
    }

    /* Authentification code */
    user = connection->u.user;
    passwd = connection->u.passwd;

    /*
     * Use .netrc if asked to do so. 
     */
    if (rt.use_netrc == TRUE)
    {
	netrc_ent = search_netrc(rt.netrc_list, connection->u.host);
	if (netrc_ent != NULL)
	{
	    user = netrc_ent->account;
	    passwd = netrc_ent->password;
	}
    }


    user = user ? user : "";
    passwd = passwd ? passwd : "";

    if (strlen(user) || strlen(passwd))
    {
	/*Construct the necessary header */
	wwwauth = get_basic_auth_str(user, passwd);
	message("Authenticating as user %s password %s", user, passwd);

	debug_prz("Authentification string=%s\n", wwwauth);
    } else
	wwwauth = 0;

   if(connection->u.referer)
      {
	referer= (char *) alloca(13+strlen(connection->u.referer));
	sprintf(referer, "Referer: %s\r\n", connection->u.referer);
      }


    /*
     * lets fetch the headers and since we want to retreive the file 
     */

    /*
     * we will use the GET command 
     */

    sprintf(buffer,
	    "GET %s HTTP/1.0\r\nUser-Agent: %s%s\r\nHost: %s\r\nAccept: */*\r\n%s%s\r\n",
	    connection->u.path, PACKAGE_NAME, VERSION,
	    connection->u.host, referer ? referer : "",wwwauth ? wwwauth : "");

    debug_prz("HTTP request = %s", buffer);

    err =
	http_fetch_headers(connection->http_sock, &(connection->u),
			   &(connection->hs), buffer);

    if (wwwauth)
	free(wwwauth);

    if (err != HOK)
    {
	/*Check if we authenticated using any user or password and if we 
	   were kicked out, if so return HAUTHFAIL */
	if (err == HAUTHREQ && (strlen(user) || strlen(passwd)))
	    err = HAUTHFAIL;
	/*
	 * a error occured druing the process 
	 */
	connection->status = REMOTEFATAL;
	close(connection->http_sock);
	return err;
    }

    if (!
	(connection->fp =
	 fopen(connection->localfile, connection->file_mode)))
    {
	message("Error opening file %s for writing: %s",
		connection->localfile, strerror(errno));
	close(connection->http_sock);
	connection->status = LOCALFATAL;
	return FOPENERR;
    }

    /*
     * prioritize packets 
     */
    setsockopt(connection->http_sock, IPPROTO_IP, IP_TOS, (char *) &tos,
	       sizeof(tos));

    /*Make sure all writes go directly to the file */
    setvbuf(connection->fp, NULL, _IONBF, 0);
    connection->remote_bytes_received = 0;
    connection->status = DOWNLOADING;

    /*
     * lets store the start time in the connections structure
     * if this is the first time we have started 
     */
    if ((connection->time_begin.tv_sec == 0)
	&& (connection->time_begin.tv_usec == 0))
	gettimeofday(&connection->time_begin, NULL);

    if (connection->main_file_size != -1)
    {
	err =
	    http_retr_fsize_known(connection, connection->http_sock,
				  connection->fp);
    } else
    {
	err =
	    http_retr_fsize_notknown(connection, connection->http_sock,
				     connection->fp);
    }

    close(connection->http_sock);

    return err;
}


/* 
 * This function attemps to retrieve the requested file, if a error occurs it retries    
 * until either the file is retrieved properly or the max number of retry attemps are
 * exceeded
 */

void http_loop(connection_data * connection)
{
    uerr_t err = HERR;
    int first_attempt = TRUE;

    assert(rt.try_attempts >= 0);
    assert(connection->try_attempts >= 0);

    pthread_cleanup_push(clean_httpsock, (void *) (connection));

    do
    {
	if (first_attempt != TRUE)
	{
	    sleep(rt.retry_delay);	/*
					 * Sleep 
					 */
	    if (rt.try_attempts == 0)	/*
					 * Try infinitely 
					 */
	    {
		message("Trying infinitely...attempt %d",
			connection->try_attempts);
	    } else
		message("Trying....attempt %d of %d.",
			connection->try_attempts, rt.try_attempts);
	    delay_ms(400);
	    connection->status = IDLE;
	}

	/*
	 * Well if this connection is resumable ,and this is not the first attempt lets try to 
	 * see whether on the previous attempt any bytes were 
	 * downloaded, if so call the resume function which 
	 * will do the changes
	 */
	if ((connection->try_attempts > 0)
	    && (connection->u.resume_support == TRUE))
	{
	    if (resume_modify_http_single_connection(connection) != 0)
	    {
		connection->status = LOCALFATAL;
		pthread_exit(0);
		return;
	    }
	    /*
	     * Then change the connections file acess method to append,
	     * since we want it to append to the end when restarting
	     */
	    assert(connection->file_mode);
	    free(connection->file_mode);
	    connection->file_mode = kstrdup("ab");
	}

	if (first_attempt == TRUE)
	    first_attempt = FALSE;



	/*
	 * if connections are 1 and you have to start at the beginning and
	 * if the server doesn't support resume, only then call the 
	 * get_complete_file routine (which just downloads the file from 
	 * beginning to end) 
	 */
	if (rt.num_connections == 1 && connection->remote_startpos <= 0
	    && connection->u.resume_support == FALSE)
	{
	    err = http_get_complete_file(connection);
	} else
	    err = http_get_file_chunk(connection);


	connection->try_attempts++;

	if (err == HOK)
	    pthread_exit(0);

	switch (err)
	{
	    /*
	     * Is it a local error ie: not enough free space etc 
	     * if so display a helpful message and return
	     */
	case FWRITEERR:
	    message("Error writing to  file %s : %s",
		    connection->u.file, strerror(errno));
	    pthread_exit(0);
	    return;
	    break;
	case FOPENERR:
	    message("Error opening file %s for writing: %s",
		    connection->localfile, strerror(errno));
	    pthread_exit(0);
	    return;
	    break;

	case CONERROR:
	    if (errno == ETIMEDOUT)
		message
		    ("Connection Attempt Timed out,Retrying in %d seconds",
		     rt.retry_delay);
	    else
		message("Error while Connecting...Retrying in %d seconds",
			rt.retry_delay);
	    break;

	case CONREJECT:
	    message("Connection attempt rejected,retrying in %d seconds");
	    break;

	case SERVERCLOSECONERR:
	    message
		("Server closed the conenction prematurely!...Retrying in %d seconds",
		 rt.retry_delay);
	    break;
	case HAUTHREQ:
	    message("%s needs authentication to access  resource",
		    connection->u.host);
	    break;
	case HAUTHFAIL:
	    message("Authentication with %s failed!", connection->u.host);
	    break;
	default:
	    message
		("Error occured in connection.......Retrying in %d seconds",
		 rt.retry_delay);
	    break;

	}
    }
    while ((connection->try_attempts < rt.try_attempts)
	   || (rt.try_attempts == 0));

    assert(err != HOK);
    connection->status = MAXTRYS;

    pthread_cleanup_pop(0);
    pthread_exit(0);
}


uerr_t http_retr_fsize_known(connection_data * connection, int sock,
			     FILE * fp)
{

    long total = 0, bytes_read = 0;
    long temp = connection->remote_endpos - connection->remote_startpos;
    char buffer[HTTP_BUFFER_SIZE];

    /* bugfix */
    if (temp == 0)
	temp = 1;

    while (temp > 0)
    {
	if (temp < HTTP_BUFFER_SIZE && temp > 0)
	{
	    while (temp > 0)
	    {
		bytes_read =
		    krecv(sock, buffer, HTTP_BUFFER_SIZE, 0, rt.timeout);

		if ((bytes_read == 0 && temp > 0))
		{
		    /* The conenction as closed by the server prematurely :( */
		    message("Server closed the conenction prematurely!");
		    fclose(fp);
		    connection->status = REMOTEFATAL;
		    return SERVERCLOSECONERR;

		}

		if (bytes_read == -1)
		{
		    message("Error receving data");
		    fclose(fp);
		    if (errno == ETIMEDOUT)
		    {
			message("connection timed out");
			connection->status = TIMEDOUT;
			return HERR;
		    }
		    connection->status = REMOTEFATAL;
		    return READERR;
		}

		if (bytes_read != fwrite(buffer,
					 sizeof(char), bytes_read, fp))
		{
		    message("write failed");
		    fclose(fp);
		    connection->status = LOCALFATAL;
		    return FWRITEERR;

		}
		temp -= bytes_read;
		total += bytes_read;
		connection->remote_bytes_received += bytes_read;
		calc_con_ratebps(connection);
		throttle_con_rate(connection);
	    }
	    break;
	}

	bytes_read = krecv(sock, buffer, HTTP_BUFFER_SIZE, 0, rt.timeout);

	if ((bytes_read == 0 && temp > 0))
	{
	    /* The conenction as closed by the server prematurely :( */
	    message("Server closed the connection prematurely!");
	    fclose(fp);
	    connection->status = REMOTEFATAL;
	    return SERVERCLOSECONERR;
	}

	if (bytes_read == -1)
	{
	    message("Error receving data");
	    fclose(fp);
	    if (errno == ETIMEDOUT)
	    {
		message("connection timed out");
		connection->status = TIMEDOUT;
		return HERR;
	    }
	    connection->status = REMOTEFATAL;
	    return READERR;
	}

	if (bytes_read != fwrite(buffer, sizeof(char), bytes_read, fp))
	{
	    message("write failed");
	    fclose(fp);
	    connection->status = LOCALFATAL;
	    return FWRITEERR;
	}
	temp -= bytes_read;
	total += bytes_read;
	connection->remote_bytes_received += bytes_read;
	calc_con_ratebps(connection);
	throttle_con_rate(connection);
    }

    fclose(fp);
    connection->status = COMPLETED;
    message("download for this connection completed");
    message("%s : %ld received", connection->localfile, total);
    return HOK;
}

uerr_t http_retr_fsize_notknown(connection_data * connection, int sock,
				FILE * fp)
{
    long bytes_read;
    char buffer[HTTP_BUFFER_SIZE];

    do
    {
	bytes_read = krecv(sock, buffer, HTTP_BUFFER_SIZE, 0, rt.timeout);
	if (bytes_read > 0)
	{
	    if (fwrite(buffer, sizeof(char), bytes_read, fp) < bytes_read)
	    {
		message("Error writing to  file %s : %s",
			connection->localfile, strerror(errno));
		fclose(fp);
		connection->status = LOCALFATAL;
		return FWRITEERR;
	    }
	}
	connection->remote_bytes_received += bytes_read;
	calc_con_ratebps(connection);
	throttle_con_rate(connection);
    }
    while (bytes_read > 0);

    fclose(fp);

    if (bytes_read == -1)
    {
	if (errno == ETIMEDOUT)
	{
	    message("Connection timed out");
	    connection->status = TIMEDOUT;
	    return READERR;
	}
	connection->status = REMOTEFATAL;
	return READERR;
    }

    connection->status = COMPLETED;
    message("download for this connection completed");
    message("%s : %ld received", connection->localfile,
	    connection->remote_bytes_received);
    return HOK;
}




uerr_t fetch_http_portions(urlinfo * url_data, http_stat_t * hs,
			   int same_url, ftp_mirror * mirrors,
			   int num_servers)
{
  uerr_t err;
  int i = 0;
  interface_ret i_ret;

  do
    {
      if (same_url == FALSE)
	{
	  err = get_http_info(url_data, hs);

	  if (err == NEWLOCATION)
	    {
	      /*
	       * loop for max_redirections searching for a valid file 
	       */
	      for (i = 0; i < rt.max_redirections; ++i)
		{
		  char *constructed_newloc;

		  /*DONE : handle relative urls too */
		  constructed_newloc =
		    uri_merge(url_data->url, hs->newloc);

		  debug_prz("Redirected to %s, merged URL = %s",
			  hs->newloc, constructed_newloc);

		  err =
		    parseurl(constructed_newloc, url_data, 0);
		  if (err != URLOK)
		    {
		      die("The server returned location is syntatically wrong: %s!", constructed_newloc);
		    }

		  err = get_http_info(url_data, hs);

		  if (err == HOK)
		    {
		      break;
		    } else if (err == NEWLOCATION)
		      {
			if (i == rt.max_redirections)
			  {
			    /*
			     * redirected too many times 
			     */
			    die("Error: Too many redirections:%d\n",
				rt.max_redirections);
			  }
		      } else
			{
			  switch (err)
			    {
			    case HOSTERR:
			      die("Unable to resolve %s", url_data->host);
			      break;
			    case CONREFUSED:
			      die("%s Rejected the Connection Attempt",
				  url_data->host);
			      break;
			    case CONERROR:
			      if (errno == ETIMEDOUT)
				{
				  die("The connection Attempt to %s timed out", url_data->host);
				} else
				  die("Error while attempting to connect to %s", url_data->host);
			      break;
			    case HAUTHREQ:
			      die("%s needs authentication to access that resource -: Privileged resource!", url_data->host);
			      break;
			    case HAUTHFAIL:
			      die("Authentication with %s failed!",
				  url_data->host);
			      break;
			    default:
			      die("A error occured while trying to get info from the server\n");
			      break;
			    }

			}
		}
	    } else if (err != HOK)
	      {
		if (hs->statcode == HTTP_NOT_FOUND)
		  {
		    die("Error: The file was not found on the server!\n");
		  } else
		    {
		      switch (err)
			{
			case HOSTERR:
			  die("Unable to resolve %s", url_data->host);
			  break;
			case CONREFUSED:
			  die("%s Rejected the Connection Attempt",
			      url_data->host);
			  break;
			case CONERROR:
			  if (errno == ETIMEDOUT)
			    {
			      die("The connection Attempt to %s timed out",
				  url_data->host);
			    } else
			      die("Error while attempting to connect to %s",
				  url_data->host);
			  break;
			case HAUTHREQ:
			  die("%s needs authentication to access that resource -: Privileged resource", url_data->host);
			  break;
			case HAUTHFAIL:
			  die("Authentication with %s failed!",
			      url_data->host);
			  break;
			default:
			  die("A error occured while trying to get info from the server\n");
			  break;
			}
		    }
	      }
	}

      if (url_data->file_size != -1)
	message("file size =%d", url_data->file_size);


      /*
       * Check and see wether the server supports resuming 
       */
      if ((url_data->file_size == -1) || (hs->accept_ranges == -1)
	  || (hs->accept_ranges == 0))
	{
	  url_data->resume_support = FALSE;
	  rt.num_connections = 1;
	  if (rt.run_mode == RESUME)
	    {
	      message("Warning: This server does not support RESUME\n");
	      delay_ms(2000);
	      /*Change the run mode to normal, if a previous download is detected 
		the other routines will give the user the option of aborting or overwriting
	      */
	      rt.run_mode = NORMAL;
	    }
	} else
	  url_data->resume_support = TRUE;

      /*
       * If force mode is false, check and prompt the user if the file
       * or it's downloaded fragments are already existing
       */
      if (rt.force_mode == FALSE)
	{
	  if (query_overwrite_target(url_data->file) != 0)
	    {
	      die("Error while querying for %s :%s", url_data->file,
		  strerror(errno));
	    }
	}

      /*
       * Process the logfile if it exists 
       */
      do_logfile(url_data);

      debug_prz("HTTP MAIN File size=%ld", url_data->file_size);

      switch (rt.run_mode)
	{
	case NORMAL:
	  http_allocate_connections(url_data, url_data->file_size, "wb");
	  break;
	case RESUME:
	  http_allocate_connections(url_data, url_data->file_size, "ab");
	  break;
	default:
	  die("Error: Unsupported mode\n");
	}

      i_ret = do_downloads(mirrors, num_servers);

      if (i_ret == USER_NEXT_SERV)
	{
	  int cur_server =
	    get_mirror_list_pos(url_data->url, mirrors, num_servers);
	  freeurl(url_data, 0);
	  err = parseurl(mirrors[cur_server + 1].full_name, url_data, 0);


	  if (err != URLOK)
	    {
	      die("The  URL is syntatically wrong!\n");
	    }
	  same_url = FALSE;
	  message("Continuing from %s", url_data->host);
	  delay_ms(1000);

	} else if (i_ret == USER_PREV_SERV)
	  {
	    int cur_server =
	      get_mirror_list_pos(url_data->url, mirrors, num_servers);
	    freeurl(url_data, 0);
	    err = parseurl(mirrors[cur_server - 1].full_name, url_data, 0);
	    if (err != URLOK)
	      {
		die("The  URL is syntatically wrong!\n");
	      }
	    same_url = FALSE;
	    message("Continuing from %s", url_data->host);
	    delay_ms(1000);
	  }

    }
  while (i_ret == USER_NEXT_SERV || i_ret == USER_PREV_SERV);

  return 1;
}
