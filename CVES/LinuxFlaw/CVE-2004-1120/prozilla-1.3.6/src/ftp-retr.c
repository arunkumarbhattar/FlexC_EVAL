/* Functions to help with  retreiving FTP files
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
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <setjmp.h>
#include <assert.h>
#include <pthread.h>

#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#else
#include <curses.h>
#endif				/*
				 * HAVE_CURSES 
				 */

#include "connect.h"
#include "resume.h"
#include "ftp.h"
#include "ftp-retr.h"
#include "main.h"
#include "mainconf.h"
#include "netrc.h"
#include "runtime.h"
#include "misc.h"
#include "debug.h"

#include <fcntl.h>

void clean_ftpsocks(void *cdata)
{
    int flags;
    connection_data *connection = (connection_data *) cdata;

    debug_prz("in clean ftp sock\n");


    if (connection->data_sock > 0)
    {
	flags = fcntl(connection->data_sock, F_GETFD, 0);
	if (flags == -1)
	{
	    debug_prz("data sock invalid\n");
	} else
	    close(connection->data_sock);
    }

    if (connection->control_sock > 0)
    {
	flags = fcntl(connection->control_sock, F_GETFD, 0);
	if (flags == -1)
	{
	    debug_prz("control sock invalid\n");
	} else
	    close(connection->control_sock);
    }

}

/*
 * function to get a file chunk through ftp
 */

uerr_t ftp_get_file_chunk(connection_data * connection)
{

    char *user, *passwd;

    unsigned char pasv_addr[6];	/*
				 * for PASV 
				 */
    int passive_mode = FALSE;
    uerr_t err;
    netrc_entry *netrc_ent;
    int tos = IPTOS_THROUGHPUT;

    extern pthread_mutex_t status_change_mutex;
    extern pthread_cond_t connecting_cond;

    /*
     * set the thread attributes 
     */
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    /*
     * did we get a filename? 
     */
    assert(connection->localfile != NULL);
    assert(connection->file_mode != NULL);

    /*clear the socks */
    connection->control_sock = 0;
    connection->data_sock = 0;

    /*
     * if there is nothing to download then return 
     */
    if (connection->status == COMPLETED)
    {
	gettimeofday(&connection->time_begin, NULL);
	return FTPOK;
    }
    /* Broadcast the condition here */
    pthread_mutex_lock(&status_change_mutex);
    /* Change the connection status */
    connection->status = CONNECTING;
    pthread_cond_broadcast(&connecting_cond);
    pthread_mutex_unlock(&status_change_mutex);


    err = ftp_connect_to_server(&(connection->control_sock),
				connection->u.host, connection->u.port);
    if (err != FTPOK)
    {
	if (err == FTPCONREFUSED)
	{
	    close(connection->control_sock);
	    connection->status = CONREJECT;
	    /*
	     * message("Login was not allowed by the server!"); 
	     */
	    return err;
	} else
	{
	    message("Error connecting to %s", connection->u.host);
	    connection->status = REMOTEFATAL;
	    return err;
	}
    }
    message("connect ok");

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


    user = user ? user : DEFAULT_FTP_USER;
    passwd = passwd ? passwd : DEFAULT_FTP_PASSWD;
    message("Logging in as user %s password %s", user, passwd);
    connection->status = LOGGININ;

    /*
     * if the login was refused the send  FTPLOGREFUSED 
     */
    err = ftp_login(connection->control_sock, user, passwd);
    if (err != FTPOK)
    {
	if (err == FTPLOGREFUSED)
	{
	    close(connection->control_sock);
	    connection->status = LOGINFAIL;
	    /*
	     * message("Login was not allowed by the server!"); 
	     */
	    return err;
	} else
	{
	    message("login failed");
	    close(connection->control_sock);
	    connection->status = REMOTEFATAL;
	    return err;
	}
    } else
	message("login ok");

    err = ftp_binary(connection->control_sock);
    if (err != FTPOK)
    {
	message("binary failed");
	close(connection->control_sock);
	connection->status = REMOTEFATAL;
	return err;
    } else
	message("binary ok");

    /*
     * De we need to CWD 
     */
    if (*connection->u.dir)
    {
	err = ftp_cwd(connection->control_sock, connection->u.dir);
	if (err != FTPOK)
	{
	    message("CWD failed to change to directory %s",
		    connection->u.dir);
	    close(connection->control_sock);
	    connection->status = REMOTEFATAL;
	    return err;
	} else
	    message("CWD OK");
    } else
    {
	message("CWD is not needed");
    }

    /*
     * If enabled lets try PASV 
     */
    if (rt.use_pasv == TRUE)
    {
	err = ftp_pasv(connection->control_sock, pasv_addr);

	/*
	 * If the error is due to the server not supporting PASV then
	 * * set the flag and lets try PORT
	 */
	if (err == FTPNOPASV || err == FTPINVPASV)
	{
	    message("Server doesn't seem to support PASV");
	    passive_mode = FALSE;
	}

	if (err == FTPOK)	/*
				 * server supports PASV 
				 */
	{
	    char dhost[256];
	    unsigned short dport;

	    sprintf(dhost, "%d.%d.%d.%d",
		    pasv_addr[0], pasv_addr[1], pasv_addr[2],
		    pasv_addr[3]);
	    dport = (pasv_addr[4] << 8) + pasv_addr[5];
	    /*
	     * message("Connecting to %s %d",dhost,dport);          
	     * *delay_ms(500);
	     */

	    debug_prz("FTP PASV server =%s port=%d\n", dhost, dport);
	    err =
		connect_to_server(&(connection->data_sock), dhost, dport,
				  rt.timeout);
	    if (err != NOCONERROR)
	    {
		message
		    ("Error while connecting, according to servers PASV info");

		close(connection->control_sock);
		connection->status = REMOTEFATAL;
		return err;
	    }
	    /*
	     * Everything seems to be ok 
	     */
	    passive_mode = TRUE;
	} else
	{
	    message("Error while connecting to FTP server for PASV");
	    close(connection->control_sock);
	    connection->status = REMOTEFATAL;
	    return err;
	}
    } else
    {
	/*
	 * Ok..since PASV is not to be used... 
	 */
	passive_mode = FALSE;
    }

    if (passive_mode == FALSE)	/*
				 * try to use PORT instead 
				 */
    {
	err = ftp_get_listen_socket(connection->control_sock,
				    &(connection->listen_sock));
	if (err != FTPOK)
	{
	    message("listen failed");
	    close(connection->control_sock);
	    connection->status = REMOTEFATAL;
	    return err;
	}
    }

    err = ftp_rest(connection->control_sock, connection->remote_startpos);
    if (err != FTPOK)
    {
	message("REST failed");
	close(connection->control_sock);
	connection->status = REMOTEFATAL;
	return err;
    }
    message("REST ok");


    err = ftp_retr(connection->control_sock, connection->u.file);
    if (err != FTPOK)
    {
	message("RETR failed");
	close(connection->control_sock);
	connection->status = REMOTEFATAL;
	return err;
    }
    message("RETR ok");

    if (passive_mode == FALSE)	/*
				 * Then we have to accept the connection 
				 */
    {
	err = accept_connection(connection->listen_sock,
				&(connection->data_sock));
	if (err != ACCEPTOK)
	{
	    close(connection->control_sock);
	    connection->status = REMOTEFATAL;
	    return err;
	}
    }

    /*
     * Lets get the file and store it  
     * open the file with current mode 
     */

    if (!
	(connection->fp =
	 fopen(connection->localfile, connection->file_mode)))
    {
	close(connection->control_sock);
	close(connection->data_sock);
	connection->status = LOCALFATAL;
	return FOPENERR;
    }

    /*
     * prioritize packets 
     */
    setsockopt(connection->data_sock, IPPROTO_IP, IP_TOS,
	       (char *) &tos, sizeof(tos));
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

    err = ftp_retr_fsize_known(connection, connection->fp);

    close(connection->control_sock);
    close(connection->data_sock);

    /*
     * no need to close the listen_sock since it is already  
     * closed by accept connection 
     */
    return err;
}



/* a function that handles the FTP downloading of a complete file */
/* such as when the server doesn't support the REST call */

uerr_t ftp_get_file_to_end(connection_data * connection)
{

    uerr_t err;
    char *user, *passwd;
    unsigned char pasv_addr[6];	/*
				 * for PASV 
				 */
    int passive_mode = FALSE;
    int tos = IPTOS_THROUGHPUT;
    netrc_entry *netrc_ent;

    extern pthread_mutex_t status_change_mutex;
    extern pthread_cond_t connecting_cond;

    /*
     * set the thread attributes 
     */
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    /*
     * did we get a filename? 
     */
    assert(connection->localfile != NULL);
    assert(connection->file_mode != NULL);

    /*clear the socks */
    connection->control_sock = 0;
    connection->data_sock = 0;

    /*
     * if there is nothing to download then return 
     */
    if (connection->status == COMPLETED)
    {
	gettimeofday(&connection->time_begin, NULL);
	return FTPOK;
    }

    /* Broadcast the condition here */
    pthread_mutex_lock(&status_change_mutex);
    /* Change the connection status */
    connection->status = CONNECTING;
    pthread_cond_broadcast(&connecting_cond);
    pthread_mutex_unlock(&status_change_mutex);

    err = ftp_connect_to_server(&(connection->control_sock),
				connection->u.host, connection->u.port);

    if (err != FTPOK)
    {
	if (err == FTPCONREFUSED)
	{
	    close(connection->control_sock);
	    connection->status = CONREJECT;
	    /*
	     * message("Login was not allowed by the server!"); 
	     */
	    return err;
	} else
	{
	    message("Error connecting to %s", connection->u.host);
	    connection->status = REMOTEFATAL;
	    return err;
	}
    }

    message("connect ok");

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

    user = user ? user : DEFAULT_FTP_USER;
    passwd = passwd ? passwd : DEFAULT_FTP_PASSWD;
    message("Logging in as user %s password %s", user, passwd);

    connection->status = LOGGININ;
    err = ftp_login(connection->control_sock, user, passwd);
    if (err != FTPOK)
    {
	if (err == FTPLOGREFUSED)
	{
	    close(connection->control_sock);
	    connection->status = LOGINFAIL;
	    /*
	     * message("Login was not allowed by the server!"); 
	     */
	    return err;
	} else
	{
	    message("login failed");
	    close(connection->control_sock);
	    connection->status = REMOTEFATAL;
	    return err;
	}
    } else
	message("login ok");

    err = ftp_binary(connection->control_sock);
    if (err != FTPOK)
    {
	message("binary failed");
	close(connection->control_sock);
	connection->status = REMOTEFATAL;
	return err;
    } else
	message("binary ok");

    /* De we need to CWD */
    if (*connection->u.dir)
    {
	err = ftp_cwd(connection->control_sock, connection->u.dir);
	if (err != FTPOK)
	{
	    message("CWD '%s' failed", connection->u.dir);
	    close(connection->control_sock);
	    connection->status = REMOTEFATAL;
	    return err;
	} else
	    message("CWD OK");
    } else
    {
	message("CWD not needed");
    }

    /*
     * If enabled lets try PASV 
     */
    if (rt.use_pasv == TRUE)
    {
	err = ftp_pasv(connection->control_sock, pasv_addr);
	/*
	 * If the error is due to the server not supporting PASV then
	 * set the flag and lets try PORT
	 */
	if (err == FTPNOPASV || err == FTPINVPASV)
	{
	    message("Server doesn't seem to support PASV");
	    passive_mode = FALSE;
	}

	if (err == FTPOK)	/*
				 * server supports PASV 
				 */
	{
	    char dhost[256];
	    unsigned short dport;
	    sprintf(dhost, "%d.%d.%d.%d",
		    pasv_addr[0], pasv_addr[1], pasv_addr[2],
		    pasv_addr[3]);
	    dport = (pasv_addr[4] << 8) + pasv_addr[5];

	    err =
		connect_to_server(&(connection->data_sock), dhost, dport,
				  rt.timeout);
	    if (err != NOCONERROR)
	    {
		message("Error while connecting according to PASV");
		connection->status = REMOTEFATAL;
		close(connection->control_sock);
		return err;
	    }
	    /*
	     * Everything seems to be ok 
	     */
	    passive_mode = TRUE;
	} else
	{
	    message("Error while connecting to FTP server for PASV");
	    close(connection->control_sock);
	    connection->status = REMOTEFATAL;
	    return err;
	}
    } else
    {
	/*
	 * Ok..since PASV is not to be used... 
	 */
	passive_mode = FALSE;
    }

    if (passive_mode == FALSE)	/*
				 * try to use PORT instead 
				 */
    {
	err = ftp_get_listen_socket(connection->control_sock,
				    &(connection->listen_sock));
	if (err != FTPOK)
	{
	    message("listen failed");
	    close(connection->control_sock);
	    connection->status = REMOTEFATAL;
	    return err;
	}
    }

    /*
     * do we need to start from a different position? If so 
     * * does the server support REST 
     */
    if (connection->remote_startpos > 0
	&& connection->u.resume_support == TRUE)
    {

	err =
	    ftp_rest(connection->control_sock,
		     connection->remote_startpos);
	if (err != FTPOK)
	{
	    message("REST failed");
	    close(connection->control_sock);
	    connection->status = REMOTEFATAL;
	    return err;
	}
	message("REST ok");
    }

    err = ftp_retr(connection->control_sock, connection->u.file);
    if (err != FTPOK)
    {
	message("RETR failed");
	close(connection->control_sock);
	connection->status = REMOTEFATAL;
	return err;
    }


    if (passive_mode == FALSE)	/*
				 * Then we have to accept the connection 
				 */
    {
	err = accept_connection(connection->listen_sock,
				&(connection->data_sock));
	if (err != ACCEPTOK)
	{
	    close(connection->control_sock);
	    connection->status = REMOTEFATAL;
	    return err;
	}
    }

    /*
     * Lets get the file and store it 
     */

    /*
     * open the file with current mode 
     */
    if (!
	(connection->fp =
	 fopen(connection->localfile, connection->file_mode)))
    {
	close(connection->control_sock);
	close(connection->data_sock);
	connection->status = LOCALFATAL;
	return FOPENERR;
    }

    /*
     * prioritize packets 
     */
    setsockopt(connection->data_sock, IPPROTO_IP, IP_TOS,
	       (char *) &tos, sizeof(tos));

    /*Make sure all writes go directly to the file */
    setvbuf(connection->fp, NULL, _IONBF, 0);
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
	err = ftp_retr_fsize_known(connection, connection->fp);
	return err;
    } else
    {
	err = ftp_retr_fsize_notknown(connection, connection->fp);
    }

    close(connection->control_sock);
    close(connection->data_sock);

    return err;
}

/* This function attemps to retrieve the requested file, if a error occurs it retries    
 * until either the file is retrieved properly or the max number of retry attemps are
 * exceeded
 */

void ftp_loop(connection_data * connection)
{
    uerr_t err = FTPERR;
    int first_attempt = TRUE;

    assert(rt.try_attempts >= 0);
    assert(connection->try_attempts >= 0);

    pthread_cleanup_push(clean_ftpsocks, (void *) connection);
    
    connection->ftp_login_reject_start=0;

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
		message("Trying....attempt %d of %d",
			connection->try_attempts, rt.try_attempts);
	    delay_ms(400);
	    connection->status = IDLE;
	}


	/*
	 * Well if this connection is resumable, and this is not the first attempt,lets try to 
	 * see whether on the previous attempt any bytes were 
	 * downloaded, if so call the resume function which 
	 * will do the changes
	 */

	if ((connection->try_attempts > 0)
	    && (connection->u.resume_support == TRUE))
	{
	    /*
	     * So the conenctions first attempt failed somehow 
	     * however the connection may have downloaded some data
	     * before terminating, so lets see whether the download
	     */
	    if (resume_modify_ftp_single_connection(connection) != 0)
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

	if (rt.num_connections == 1)
	    err = ftp_get_file_to_end(connection);
	else
	    err = ftp_get_file_chunk(connection);

	connection->try_attempts++;

	if (err == FTPOK)
	    pthread_exit(0);

	/*
	 * So some error has occured 
	 * Lets figure out whether it was a error which has to be handled
	 * at this level or passed to the thread which launched this
	 */

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
		message("Error while connecting..retrying in %d seconds",
			rt.retry_delay);
	    break;

	case FTPLOGREFUSED:
	    /*
	     * This will be handled by the handle_threads func in main.c
	     */
	    pthread_exit(0);
	    return;

	    break;

	case FTPCONREFUSED:
	    /*
	     * This will be handled by the handle_threads func in main.c
	     */
	    pthread_exit(0);
	    return;
	    break;

	case SERVERCLOSECONERR:
	    message
		("Server closed the conenction prematurely!...Retrying in %d seconds",
		 rt.retry_delay);
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

    assert(err != FTPOK);
    connection->status = MAXTRYS;

    pthread_cleanup_pop(0);
    pthread_exit(0);
}


/* 
 * This will download till it receives the required bytes, if the server 
 * closes the connection prematurely, this routine will return a error if the 
 * required number of bytes have not been got.   
 */

uerr_t ftp_retr_fsize_known(connection_data * connection, FILE * fp)
{
    long total = 0, bytes_read = 0;
    long temp = connection->remote_endpos - connection->remote_startpos;

    while (temp > 0)
    {
	if (temp < FTP_BUFFER_SIZE && temp > 0)
	{
	    while (temp > 0)
	    {
		bytes_read = ftp_read_msg(connection->data_sock,
					  connection->ftp_buffer, temp);

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
			return READERR;
		    }
		    connection->status = REMOTEFATAL;
		    return READERR;
		}

		if (bytes_read != fwrite(connection->ftp_buffer,
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

	bytes_read =
	    ftp_read_msg(connection->data_sock, connection->ftp_buffer,
			 FTP_BUFFER_SIZE);

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
		return READERR;
	    }
	    connection->status = REMOTEFATAL;
	    return READERR;
	}

	if (bytes_read !=
	    fwrite(connection->ftp_buffer, sizeof(char), bytes_read, fp))
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
    return FTPOK;
}

/* This will download till it receives a EOF */

uerr_t ftp_retr_fsize_notknown(connection_data * connection, FILE * fp)
{
    long bytes_read;
    do
    {
	bytes_read =
	    krecv(connection->data_sock, connection->ftp_buffer,
		  FTP_BUFFER_SIZE, 0, rt.timeout);
	if (bytes_read > 0)
	{
	    if (fwrite
		(connection->ftp_buffer, sizeof(char), bytes_read,
		 fp) < bytes_read)
	    {
		fclose(fp);
		message("write failed");
		connection->status = LOCALFATAL;
		return FWRITEERR;
	    }
	    connection->remote_bytes_received += bytes_read;

	    calc_con_ratebps(connection);
	    throttle_con_rate(connection);
	}
    }
    while (bytes_read > 0);

    fclose(fp);

    if (bytes_read == -1)
    {
	if (errno == ETIMEDOUT)
	{
	    message("connection timed out");
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
    return FTPOK;
}


uerr_t fetch_ftp_portions(urlinfo * url_data, ftp_stat_t * fs,
			  int same_url, ftp_mirror * mirrors,
			  int num_servers)
{
    uerr_t err;
    interface_ret i_ret;

    do
    {
	if (same_url == FALSE)
	{
	    err = ftp_get_info(url_data, fs);

	    if (err == FTPOK)
	    {
		if (fs->fp.flagtryretr == 0 && fs->fp.flagtrycwd == 1)
		    die("Error:  %s is a directory, not a file",
			url_data->file);
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
		case FTPCONREFUSED:
		    die("%s Rejected the Connection Attempt",
			url_data->host);
		    break;
		case FTPLOGREFUSED:
		    die("The FTP server has rejected the login attempt.");
		    break;
		case FTPNSFOD:
		    die("Error: A file called %s was not found on the server", url_data->file);
		    break;
		default:
		    die("A error occured while trying to get info from the server\n");
		    break;
		}

	    }
	}
	/*
	 * If resume is not supported default to one connection
	 */
	if (url_data->resume_support == FALSE)
	{
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
	}

	/*
	 *  If force mode is false, check and prompt the user if the file
	 *  or it's downloaded fragments are already existing
	 */
	if (rt.force_mode == FALSE)
	{
	    if (query_overwrite_target(url_data->file) != 0)
	    {
		die("Error while querying for %s", url_data->file);
	    }
	}

	/*
	 * Process the logfile if it exists 
	 */
	do_logfile(url_data);

	switch (rt.run_mode)
	{
	case NORMAL:
	    ftp_allocate_connections(url_data, url_data->file_size, "wb");
	    break;
	case RESUME:
	    ftp_allocate_connections(url_data, url_data->file_size, "ab");
	    break;
	default:
	    die("Error: unsupported mode\n");
	}

	i_ret = do_downloads(mirrors, num_servers);

	if (i_ret == USER_NEXT_SERV)
	{
	    int cur_server =
		get_mirror_list_pos(url_data->url, mirrors, num_servers);
	    freeurl(url_data, 0);

	    debug_prz("restarting %s", mirrors[cur_server + 1].full_name);

	    err = parseurl(mirrors[cur_server + 1].full_name, url_data, 0);
	    if (err != URLOK)
	    {
		die("The  URL is syntatically wrong!\n");
	    }
	    same_url = FALSE;
	    rt.run_mode = RESUME;
	    message("Continuing from %s", url_data->host);
	    delay_ms(1000);
	    erase();
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
	    rt.run_mode = RESUME;
	    message("Continuing from %s", url_data->host);
	    delay_ms(1000);
	    erase();
	}

    }
    while (i_ret == USER_NEXT_SERV || i_ret == USER_PREV_SERV);

    return 1;
}
