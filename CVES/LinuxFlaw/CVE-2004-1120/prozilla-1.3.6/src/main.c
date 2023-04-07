/* The main file 
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
#endif

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <signal.h>
#include <setjmp.h>
#include <assert.h>
#include <pthread.h>
#include <sys/param.h>
#include <sys/stat.h>

#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#else
#include <curses.h>
#endif
#include <pwd.h>
#include <ctype.h>

#include "connect.h"
#include "ftp.h"
#include "url.h"
#include "misc.h"
#include "http.h"
#include "main.h"
#include "mainconf.h"
#include "connection.h"
#include "interface.h"
#include "resume.h"
#include "http-retr.h"
#include "ftp-retr.h"
#include "runtime.h"
#include "getopt.h"
#include "netrc.h"
#include "logfile.h"
#include "ftpsearch.h"
#include "debug.h"
#include "init.h"


#define RETRY_LOGIN_TIME 300


/*
 * pointer to array of connections which is allocated in allocate_connections 
 * according to the value of int num_connections above
 */

connection_data *connections = NULL;
/*
 * the threads 
 */
pthread_t *threads = NULL;
struct runtime rt;

/* Mutex for the changing of the state */
pthread_mutex_t status_change_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Mutex for the setting of the throttling of the  rates per connection */
pthread_mutex_t compute_throttle_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Condition which is broadcast when a thread changes its status 
 *to connecting 
 */
pthread_cond_t connecting_cond = PTHREAD_COND_INITIALIZER;

/*
 * structure for options parsing
 */

struct option long_opts[] = {
    /*
     * { name  has_arg  *flag  val } 
     */
    {"resume", no_argument, NULL, 'r'},
/*    {"connections", required_argument, NULL, 'c'},*/
    {"license", no_argument, NULL, 'L'},
    {"help", no_argument, NULL, 'h'},
    {"gtk", no_argument, NULL, 'g'},
    {"no-netrc", no_argument, NULL, 'n'},
    {"tries", required_argument, NULL, 't'},
    {"force", no_argument, NULL, 'f'},
    {"version", no_argument, NULL, 'v'},
    {"directory-prefix", required_argument, NULL, 'P'},
    {"use-port", no_argument, NULL, 129},
    {"retry-delay", required_argument, NULL, 130},
    {"timeout", required_argument, NULL, 131},
    {"no-getch", no_argument, NULL, 132},
    {"debug", no_argument, NULL, 133},
    {"ftpsearch", no_argument, NULL, 's'},
    {"no-search", no_argument, NULL, 135},
    {"pt", required_argument, NULL, 136},
    {"pao", required_argument, NULL, 137},
    {"max-ftps-servers", required_argument, NULL, 138},
    {"max-bps", required_argument, NULL, 139},
    {0, 0, 0, 0}
};


/*
 * func prototypes 
 */


/* creates the threads */
interface_ret do_downloads(ftp_mirror * mirrors, int num_servers);
/* The following funcs display the license and help*/
void help(void);
void license(void);

/* Checks wether a file exists and if it does 
 * get the users input
 */
int query_overwrite_target(char *fname);
void query_resume_old_download(urlinfo * u);
void delete_file_portions(urlinfo * u);
/*Routines to handle the logfile */
void do_log_file_normal(urlinfo * u);
void do_log_file_resume(urlinfo * u);


/* displays the software license */
void license(void)
{
    fprintf(stderr,
	    "   Copyright (C) 2000 Kalum Somaratna\n"
	    "\n"
	    "   This program is free software; you can redistribute it and/or modify\n"
	    "   it under the terms of the GNU General Public License as published by\n"
	    "   the Free Software Foundation; either version 2, or (at your option)\n"
	    "   any later version.\n"
	    "\n"
	    "   This program is distributed in the hope that it will be useful,\n"
	    "   but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	    "   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
	    "   GNU General Public License for more details.\n"
	    "\n"
	    "   You should have received a copy of the GNU General Public License\n"
	    "   along with this program; if not, write to the Free Software\n"
	    "   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n");
}


/* displays the help message */

void help(void)
{
    fprintf(stderr,
	    "Usage: proz [OPTIONS] file_url\n"
	    "\n"
	    "Ex: proz http://gnu.org/gnu.jpg\n"
	    "\n"
	    "Options:\n"
	    "      -h, --help        Give this help\n"
	    "      -r, --resume      Resume an interrupted download\n"
	    "      -f, --force       Never prompt the user when overwriting files\n"
	    "      -1                Force a single connection only\n"
	    "      -g, --gtk         Use GTK interface, instead of curses (broken for now)\n"
	    "      -n, --no-netrc    Don't use .netrc, get the user/password\n"
	    "                        from the command line,otherwise use the\n"
	    "                        anonymous login for FTP sessions\n"
	    "      --no-getch        Instead of waiting for the user pressing a key,\n"
	    "                        print the error to stdout and quit\n"
	    "      --debug           Log debugging info to a file (default is debug.log)\n"
	    "\n"
	    "Directories:\n"
	    "      -P,  --directory-prefix=DIR  save the generated file to DIR/\n"
	    "\n"
	    "FTP Options:\n"
	    "      --use-port        Force usage of PORT insted of PASV (default)\n"
	    "                        for ftp transactions\n"
	    "\n"
	    "Download Options:\n"
	    "      -s,  --ftpsearch  Do a ftpsearch for faster mirrors\n"
	    "      --no-search       Do a direct download (no ftpsearch)\n"
	    "      -k=n              Use n connections instead of the default(4)\n"
	    "      --timeout=n       Set the timeout for connections to n seconds\n"
	    "                        (default 180)\n"
	    "      -t, --tries=n     Set number of attempts to n (default(200), 0=unlimited)\n"
	    "      --retry-delay=n   Set the time between retrys to n seconds\n"
	    "                        (default 15 seconds)\n"
	    "      --max-bps=n       Limit bandwith consumed to n bps (0=unlimited)\n"
	    "\n"
	    "FTP Search Options:\n"
	    "      --pt=n            Wait 2*n seconds for a server response (default 2*4)\n"
	    "      --pao=n           Ping n servers at once(default 5 servers at once)\n"
	    "      --max-ftps-servers=n  Request a max of n servers from ftpsearch (default 40)\n"
	    "\n"
	    "      -L, --license     Display software license\n"
	    "      -v, --version     Display version number\n"
	    "\n"
	    "ProZilla homepage: http://prozilla.delrom.ro\n"
	    "Please report bugs to <prozilla-dev@delrom.ro>\n");
}


/* Displays the version */

void version(void)
{
    fprintf(stderr, "%s. Version: %s\n", PACKAGE_NAME, VERSION);
}


/*
 * This function allocates the FTP connections   
 */

void ftp_allocate_connections(urlinfo * u, long file_length,
			      char *file_io_mode)
{
    int i;
    long bytes_per_connection;
    long bytes_left;

    /*
     * Find the max path in GNU C
     */
    char buffer[MAXPATHLEN];

    connections = (connection_data *) kmalloc(sizeof(connection_data) *
					      rt.num_connections);

    if (file_length == -1)
    {
	assert(rt.num_connections == 1);
	bytes_per_connection = -1;
	bytes_left = -1;

    } else
    {
	bytes_per_connection = file_length / rt.num_connections;
	bytes_left = file_length % rt.num_connections;
    }

    for (i = 0; i < rt.num_connections; i++)
    {
	memset(&connections[i], 0, sizeof(connection_data));
	memcpy(&(connections[i].u), u, sizeof(urlinfo));
	sprintf(buffer, "%s%s%d", u->file, DEFAULT_FILE_EXT, i);
	connections[i].localfile = kstrdup(buffer);
	connections[i].file_mode = kstrdup(file_io_mode);
	connections[i].retry = TRUE;

	if (file_length == -1)
	{
	    connections[i].main_file_size = -1;
	    connections[i].remote_startpos = 0;
	    connections[i].remote_endpos = -1;
	} else
	{
	    connections[i].main_file_size = file_length;
	    connections[i].remote_startpos = i * bytes_per_connection;
	    connections[i].remote_endpos =
		i * bytes_per_connection + bytes_per_connection;
	}

	connections[i].local_startpos = 0;
	connections[i].orig_local_startpos = 0;
	/*
	 * set the number of times a connection will be retried 
	 */
	connections[i].status = IDLE;
    }
    /*
     * add the remaining bytes to the last connection 
     */
    connections[--i].remote_endpos += bytes_left;

    if (rt.run_mode == RESUME)
    {
	if (resume_modify_ftp_connections(connections, rt.num_connections)
	    != 0)
	    die("A file error while accessing the DL'ed files %s",
		strerror(errno));
    }
}

/*
 * This function allocates the HTTP connections   
 */

void http_allocate_connections(urlinfo * u, long file_length,
			       char *file_io_mode)
{
    int i;
    long bytes_per_connection;
    long bytes_left;

    /*
     * Find the max path in GNU C
     */
    char buffer[MAXPATHLEN];


    connections = (connection_data *) kmalloc(sizeof(connection_data) *
					      rt.num_connections);

    if (file_length == -1)
    {
	assert(rt.num_connections == 1);
	bytes_per_connection = -1;
	bytes_left = -1;
    } else
    {
	bytes_per_connection = file_length / rt.num_connections;
	bytes_left = file_length % rt.num_connections;
    }

    for (i = 0; i < rt.num_connections; i++)
    {
	memset(&connections[i], 0, sizeof(connection_data));
	memcpy(&(connections[i].u), u, sizeof(urlinfo));

	sprintf(buffer, "%s%s%d", u->file, DEFAULT_FILE_EXT, i);
	connections[i].localfile = kstrdup(buffer);
	connections[i].file_mode = kstrdup(file_io_mode);
	connections[i].retry = TRUE;

	if (file_length == -1)
	{
	    connections[i].main_file_size = -1;
	    connections[i].remote_startpos = 0;
	    connections[i].remote_endpos = -1;
	} else
	{
	    connections[i].main_file_size = file_length;
	    connections[i].remote_startpos = i * bytes_per_connection;
	    connections[i].remote_endpos =
		i * bytes_per_connection + bytes_per_connection - 1;
	}
	connections[i].local_startpos = 0;
	connections[i].orig_local_startpos = 0;
	connections[i].status = IDLE;
    }
    /*
     * add the remaining bytes to the last connection 
     */
    connections[--i].remote_endpos += bytes_left;

    if (rt.run_mode == RESUME)
    {
	if (resume_modify_http_connections(connections, rt.num_connections)
	    != 0)
	    die("A file error while accessing the DL'ed files %s",
		strerror(errno));
    }
}


/*
 * The function that handles the creation and handling of the threads 
 */


interface_ret do_downloads(ftp_mirror * mirrors, int num_servers)
{
    int i;
    interface_ret ret;

    /* set the download start time to zero */

    memset(&(rt.dl_start_time), 0, sizeof(struct timeval));

    if (threads == NULL)
	threads =
	    (pthread_t *) kmalloc(sizeof(pthread_t) * rt.num_connections);


    for (i = 0; i < rt.num_connections; i++)
    {
	switch (connections[i].u.proto)
	{
	case URLFTP:
	    if (pthread_create(&threads[i], NULL,
			       (void *) &ftp_loop,
			       (void *) (&connections[i])) != 0)
		die("Error: Not enough system resources");

	    break;

	case URLHTTP:
	    if (pthread_create(&threads[i], NULL,
			       (void *) &http_loop,
			       (void *) (&connections[i])) != 0)
		die("Error: Not enough system resources"
		    "to create thread!\n");

	    break;

	default:
	    die("Error: Unsupported Protocol was specified");
	    break;
	}
    }

#ifdef HAVE_GTK
    if (rt.use_gtk_option == TRUE)
    {
	rt.display_mode = DISPLAY_GTK;
	gtk_do_interface(file_size);
    } else
    {
#endif
	rt.in_curses_display_loop = TRUE;

	ret =
	    curses_do_interface(connections, rt.num_connections, mirrors,
				num_servers);
	rt.in_curses_display_loop = FALSE;
#ifdef HAVE_GTK
    }
#endif

    for (i = 0; i < rt.num_connections; i++)
    {
	pthread_join(threads[i], NULL);
    }

    return ret;
}


/* A important function which the interface routines call
 * This function will handle the state of the threads while
 * they are doing the work 
 */

void handle_threads(void)
{
    int i;
    static int max_simul_conns = 0;


    /*
     *If the server disallows a ftp connection, *
     * The reason maybe that the server has being asked to disallow *
     * more than a certain number of logins per ip address (the f**ers!) *
     * so then when a thread returns with LOGINFAIL then we will wait until *
     * another thread has finished before attempting 
     *to reconnect by restarting the failed thread 
     */

    for (i = 0; i < rt.num_connections; i++)
    {

	/*
	 * Let's check and see the status of each connection 
	 */
	switch (connections[i].status)
	{
	    /*fixme */
	case MAXTRYS:
	    die("Connection %d has been tried %d time(s) and has failed. "
		"Aborting!", i + 1, rt.try_attempts);
	    break;

	case LOGINFAIL:
	  
	    /*shit we have been kicked out,  is this the firs time*/
	    if( connections[i].ftp_login_reject_start==0)
		connections[i].ftp_login_reject_start=time(0);
	  

	    /*
	     * First check if the ftp server did not allow any thread 
	     * to login at all, then  retry the curent thread 
	     */
	    if (all_dls_failed_login(connections, rt.num_connections) ==
		TRUE)
	    {
		/*
		 * Well the FTP server has disallowed all the connections
		 * login attempts...so we will continue to retry this thread 
		 */
		message("All logins rejected!.Retrying connection");
		/*
		 * make sure this thread has terminated 
		 */
		pthread_join(threads[i], NULL);
		/*
		 * Relaunch it
		 */
		connections[i].status = IDLE;
		if (pthread_create(&threads[i], NULL,
				   (void *) &ftp_loop,
				   (void *) (&connections[i])) != 0)
		    die("Error: Not enough system resources to create thread!\n");

		break;
	    } else
	    {
		/*
		 * Ok so at least there is one download whos login has not been rejected, 
		 * so lets see if it has completed, if so we can relaunch this connection, 
		 * as the commonest reason for a ftp login being rejected is because, the 
		 * ftp server has a limit on the number of logins permitted from the same 
		 * IP address. 
		 */

		/*
		 * Query the number of threads that are downloading 
		 * if it is zero then relaunch this connection
		 */

		int dling_conns_count =
		    query_downloading_conns_count(connections,
						  rt.num_connections);

		if (dling_conns_count == 0
		    &&
		    (query_connecting_conns_count
		     (connections, rt.num_connections) == 0)
		    &&
		    (query_logging_conns_count
		     (connections, rt.num_connections) == 0))
		{
		    /*
		     * make sure this thread has terminated 
		     */
		    pthread_join(threads[i], NULL);
		    /*
		     * Relaunch it
		     */
		    connections[i].status = IDLE;
		    pthread_mutex_lock(&status_change_mutex);
		    if (pthread_create(&threads[i], NULL,
				       (void *) &ftp_loop,
				       (void *) (&connections[i])) != 0)
			die("Error: Not enough system resources to create thread!\n");

		    pthread_cond_wait(&connecting_cond,
				      &status_change_mutex);
		    pthread_mutex_unlock(&status_change_mutex);
		    break;
		} else
		{
		    if (dling_conns_count > max_simul_conns)
		    {
			max_simul_conns = dling_conns_count;
			break;
		    }

		    if ((dling_conns_count < max_simul_conns)
			&&
			(query_connecting_conns_count
			 (connections, rt.num_connections) == 0)
			&&
			(query_logging_conns_count
			 (connections, rt.num_connections) == 0))
		    {
			/*
			 * make sure this thread has terminated 
			 */
			pthread_join(threads[i], NULL);
			/*
			 * Relaunch it
			 */
			connections[i].status = IDLE;
			pthread_mutex_lock(&status_change_mutex);
			if (pthread_create(&threads[i], NULL,
					   (void *) &ftp_loop,
					   (void *) (&connections[i])) !=
			    0)
			    die("Error: Not enough system resources to create thread!\n");
			pthread_cond_wait(&connecting_cond,
					  &status_change_mutex);
			pthread_mutex_unlock(&status_change_mutex);
			break;
		    }
		    
		    /* Or else  is it time to retry again, ie to check and see 
		       whether a user disconnected :-)
		    */ 
		    if(time(0)>= connections[i].ftp_login_reject_start+RETRY_LOGIN_TIME)
		    {
			/* try loggin in again */
			/*
			 * make sure this thread has terminated 
			 */
			pthread_join(threads[i], NULL);
			/*
			 * Relaunch it
			 */
			connections[i].status = IDLE;
			pthread_mutex_lock(&status_change_mutex);
			if (pthread_create(&threads[i], NULL,
					   (void *) &ftp_loop,
					   (void *) (&connections[i])) !=
			    0)
			    die("Error: Not enough system resources to create thread!\n");
			pthread_cond_wait(&connecting_cond,
					  &status_change_mutex);
			pthread_mutex_unlock(&status_change_mutex);
			break;
		    }
		}
	    }

	    break;

	case CONREJECT:
	    /*
	     * First check if the ftp server did not allow any thread 
	     * to connect at all, then retry the curent thread 
	     */
	    if (all_dls_connect_rejected(connections,
					 rt.num_connections) == TRUE)
	    {
		message("FTP server rejected all the connections!"
			"Retrying conection");
		/*
		 * make sure this thread has terminated 
		 */
		pthread_join(threads[i], NULL);
		/*
		 * Relaunch it
		 */
		connections[i].status = IDLE;
		if (pthread_create(&threads[i], NULL,
				   (void *) &ftp_loop,
				   (void *) (&connections[i])) != 0)
		    die("Error: Not enough system resources to create thread!\n");

		break;
	    } else
	    {
		/*
		 * Ok so at least there is one download who's connection attempt has not
		 * been rejected, so lets see if it has completed, if so we can relaunch 
		 * this connection, as the commonest reason for a ftp connection request 
		 * being rejected is because, the ftp server has a limit on the number of 
		 * connection requests permitted from the same IP address. 
		 */

		/*
		 * Query the number of threads that are downloading 
		 * if it is zero then relaunch this connection
		 */

		int dling_conns_count =
		    query_downloading_conns_count(connections,
						  rt.num_connections);
		if ((dling_conns_count == 0)
		    &&
		    (query_connecting_conns_count
		     (connections, rt.num_connections) == 0)
		    &&
		    (query_logging_conns_count
		     (connections, rt.num_connections) == 0))

		{
		    /*
		     * make sure this thread has terminated 
		     */
		    pthread_join(threads[i], NULL);
		    /*
		     * Relaunch it
		     */
		    connections[i].status = IDLE;
		    pthread_mutex_lock(&status_change_mutex);
		    if (pthread_create(&threads[i], NULL,
				       (void *) &ftp_loop,
				       (void *) (&connections[i])) != 0)
			die("Error: Not enough system resources to create thread!\n");

		    pthread_cond_wait(&connecting_cond,
				      &status_change_mutex);
		    pthread_mutex_unlock(&status_change_mutex);
		    break;
		} else
		{
		    if (dling_conns_count > max_simul_conns)
		    {
			max_simul_conns = dling_conns_count;
			break;
		    }

		    if ((dling_conns_count < max_simul_conns)
			&&
			(query_connecting_conns_count
			 (connections, rt.num_connections) == 0)
			&&
			(query_logging_conns_count
			 (connections, rt.num_connections) == 0))
		    {
			/*
			 * make sure this thread has terminated 
			 */
			pthread_join(threads[i], NULL);
			/*
			 * Relaunch it
			 */
			connections[i].status = IDLE;
			pthread_mutex_lock(&status_change_mutex);
			if (pthread_create(&threads[i], NULL,
					   (void *) &ftp_loop,
					   (void *) (&connections[i])) !=
			    0)
			    die("Error: Not enough system resources to create thread!\n");

			pthread_cond_wait(&connecting_cond,
					  &status_change_mutex);
			pthread_mutex_unlock(&status_change_mutex);
			break;
		    }
		}
	    }
	    break;

	case LOCALFATAL:
	    die("A local  error occured in connection %d, aborting..\n",
		i + 1);
	    break;
	default:
	    break;
	}
    }

    pthread_mutex_lock(&compute_throttle_mutex);
    calc_throttle_factor(connections, rt.num_connections);
    pthread_mutex_unlock(&compute_throttle_mutex);

}


/* 
 * terminates the running threads
 */
void terminate_threads(pthread_t * threads, int num_threads)
{
    int i;

    for (i = 0; i < num_threads; i++)
    {
	pthread_cancel(threads[i]);
    }

    for (i = 0; i < num_threads; i++)
    {
	pthread_join(threads[i], NULL);
    }
}


/* 
 * This function will check for the target file and if it is present will ask
 * the user what action he/she wishes to do
 */

int query_overwrite_target(char *fname)
{

    char buffer[MAXPATHLEN + 1];
    struct stat st_buf;
    int ret;
    char *p_file;
    /* get the prefixed name */

    p_file = get_prefixed_file(fname);

    ret = stat(p_file, &st_buf);

    if (ret == -1)
    {
	if (errno == ENOENT)
	    return 0;
	else
	    return -1;
    }

    /*
     * The File exists, ask the user what to do 
     */
    do
    {
	ret =
	    curses_query_user_input
	    ("Warning: The file %s already exists!!\nWould you like"
	     " to (O)vewrite it, (B)ackup it, (A)bort : ", p_file);
    }
    while (ret != 'O' && ret != 'A' && ret != 'B');

    switch (ret)
    {
    case 'O':
	if (unlink(p_file) != 0)
	{
	    if (errno == ENOENT)
	    {
		/*
		 * somehow the file which was supposed to be overwritten
		 * does not seem to exist???  anyway return since it is,
		 * not a problem
		 */
		return 0;
	    } else
	    {
		die("Error while trying to delete %s :",
		    p_file, strerror(errno));
	    }
	}
	break;
    case 'A':
	die("Ok..Aborting...");
	break;
    case 'B':
	strcpy(buffer, p_file);
	strcat(buffer, "~");
	message("Backing up %s to %s", p_file, buffer);
	debug_prz("Backing up %s to %s", p_file, buffer);
	if (rename(p_file, buffer) == -1)
	{
	    if (errno == ENOENT)
	    {
		/*
		 * somehow the file which was supposed to be backed up
		 * does not seem to exist???  Anyway return since it is,
		 * not a problem
		 */
		return 0;
	    } else
	    {
		die("Error trying to rename %s to %s : %s",
		    p_file, buffer, strerror(errno));
	    }
	}
	break;
    }

    return 0;
}

/* If the logfile is present then it indicates that the previous download
 * has not been completed
 */
void query_resume_old_download(urlinfo * u)
{
    int ret;

    if (u->resume_support == TRUE)
    {

	do
	{

	    ret =
		curses_query_user_input
		("Warning: Previous uncompleted download of %s detected!\n"
		 "Would you like to (A)bort, (R)esume it, (O)verwrite it?",
		 u->file);
	}
	while (ret != 'A' && ret != 'R' && ret != 'O');
    } else
    {
	do
	{

	    ret =
		curses_query_user_input
		("Warning: Previous uncompleted download of %s detected!\n"
		 "The Server does not support resuming so would you like to (A)bort,(O)verwrite it?",
		 u->file);
	}
	while (ret != 'A' && ret != 'O');
    }

    switch (ret)
    {
    case 'A':
	die("Ok..Aborting...");
	break;
    case 'R':			/*
				 *  turn the resume mode on 
				 */
	rt.run_mode = RESUME;
	break;
    case 'O':
	delete_file_portions(u);
	break;
    }

    return;
}


/* Message: calls the appropriate routine to display the msg */
void message(const char *args, ...)
{
    char p[MAX_MSG_SIZE];
    va_list vp;
    va_start(vp, args);
    vsprintf(p, args, vp);
    va_end(vp);

    switch (rt.display_mode)
    {
    case DISPLAY_CURSES:
	curses_message(p);
	break;
#ifdef HAVE_GTK
    case DISPLAY_GTK:
	gtk_message(p);
	break;
#endif
    default:
	die("Unsupported display mode");
    }
}


/* Deletes any downloaded file portions if present.
 * It determines the number of file portions by checking for 
 * a logfile and reading the number of conenctions from it.
 * If the logfile is not present then it assumes that there are 4 portions
 */


void delete_file_portions(urlinfo * u)
{
    logfile lf;
    int ret;
    int i;
    int num_connections = 0;
    char buffer[MAXPATHLEN];

    memset(&lf, 0, sizeof(logfile));

    ret = log_logfile_exists(u);
    if (ret == -1)
    {
	/*
	 * Something wrong hapenned 
	 */
	die("Error while checking for a log file Reason: %s",
	    strerror(errno));
    } else if (ret == 1)
    {
	/*
	 * Logfile from a previous session exists, lets get the number 
	 * of connections from it
	 */
	if (log_read_logfile(u, &lf) != 0)
	    die("Unable to read the log file Reason: %s", strerror(errno));
	else
	{
	    num_connections = lf.num_connections;
	}
    } else if (ret == 0)
    {
	/*
	 * No logfile found, assume 4 connections 
	 */
	num_connections = 4;
    }

/* now delete the portions */
    for (i = 0; i < num_connections; i++)
    {
	/*
	 *  calculate the name of the download
	 *  file fragment from the target file
	 */
	sprintf(buffer, "%s%s%d", u->file, DEFAULT_FILE_EXT, i);
	ret = unlink(buffer);
	/*
	 *  did we encounter a error? 
	 */
	if (ret == -1)
	{
	    if (errno == ENOENT)
		continue;
	    else
		die("Error while trying to delete %s :",
		    buffer, strerror(errno));
	}
    }

}

void do_log_file_normal(urlinfo * u)
{
    assert(rt.run_mode == NORMAL);

    if (log_create_logfile(u) != 0)
	die("Unable to create log file : %s", strerror(errno));

}


void do_log_file_resume(urlinfo * u)
{
    logfile lf;

    assert(rt.run_mode == RESUME);
    if (log_read_logfile(u, &lf) != 0)
	die("Unable to read the log file Reason: %s", strerror(errno));
    else
    {
	rt.num_connections = lf.num_connections;
    }
}

/* Creates the logfile and stores the data to it if the run mode is NORMAL,
 * Else if the run mode is RESUME, it loads the logfile from the previous 
 * session, and does the necessary modifications.
 */
void do_logfile(urlinfo * u)
{
    logfile lf;
    int ret;

    int i, portion_found;
    char buffer[MAXPATHLEN];
    struct stat st_buf;

    memset(&lf, 0, sizeof(logfile));

    ret = log_logfile_exists(u);
    if (ret == -1)
    {
	/*
	 * Something wrong hapenned 
	 */
	die("Error while checking for a log file Reason: %s",
	    strerror(errno));
    } else if (ret == 1 && rt.run_mode != RESUME && rt.force_mode == FALSE)
    {
	query_resume_old_download(u);
    } else if (ret == 1 && rt.run_mode != RESUME && rt.force_mode == TRUE)
	delete_file_portions(u);
    else if (ret == 0)
    {
	/*
	 * No logfile was found so it maybe a aborted download from 
	 * a earlier version of prozilla or the user may just 
	 *  have added the -r switch at the start 
	 */
	/*
	 * to see which is which lets search for the download portions
	 * * If any is found we will resume with four connections
	 * * Otherwise well start from scratch with the user specified 
	 * * number of connections
	 */
	portion_found = FALSE;

	for (i = 0; i < 4; i++)
	{
	    /*
	     *  calculate the name of the download
	     *  file fragment from the target file
	     */
	    sprintf(buffer, "%s%s%d", u->file, DEFAULT_FILE_EXT, i);
	    ret = stat(buffer, &st_buf);
	    /*
	     *  did we encounter a error? 
	     */
	    if (ret == -1)
	    {
		if (errno == ENOENT)	/*
					 *  the file doesn't exist..do nothing
					 */
		    continue;
		else
		    die("Error while searching for downloaded"
			"file portions: %s", strerror(errno));
	    } else
	    {
		portion_found = TRUE;
		break;
	    }
	}

	if (portion_found == TRUE)
	{
	    if (rt.run_mode != RESUME && rt.force_mode == FALSE)
		query_resume_old_download(u);
	    else if (rt.run_mode != RESUME && rt.force_mode == TRUE)
		delete_file_portions(u);

	    /*
	     * Does the user now want to resume 
	     */
	    if (rt.run_mode == RESUME)
	    {
		/*
		 * Resume with 4 connections as the old version did 
		 */
		message("No logfile from previous session found,"
			"so I will resume with the default of 4 connections");
		rt.num_connections = 4;
		delay_ms(400);
	    }
	}
	/*
	 * And then lets create the log too 
	 */
	if (log_create_logfile(u) != 0)
	    die("Unable to create log file Reason: %s", strerror(errno));
    }


    switch (rt.run_mode)
    {
    case NORMAL:
	do_log_file_normal(u);
	break;
    case RESUME:
	do_log_file_resume(u);
	break;
    default:
	die("Unsupported run mode in file %s, line %d", __FILE__,
	    __LINE__);
    }
    return;
}


/*
 * function is called when the application wants to abort 
 * TODO: Handle to freeing of resources 
 */

int die(const char *args, ...)
{
    char p[MAX_MSG_SIZE];
    va_list vp;
    va_start(vp, args);
    vsprintf(p, args, vp);
    va_end(vp);

    /* indicate that we are out of the display loop */
    rt.in_curses_display_loop = FALSE;

    /*
     * terminate any threads if running 
     */
    if (threads)
    {
	terminate_threads(threads, rt.num_connections);
    }
    /*
     * free the connections array if allocated 
     */
    if (connections)
	free(connections);
    /*
     * free the url structure 
     */

    if (rt.die_to_stdout == TRUE)
    {
	/*just dump error to stdout and quit */
	endwin();
	printf("\n%s\n", p);
	exit(1);
    }

    attrset(COLOR_PAIR(MSG_PAIR) | A_BOLD);
    move(LINES-3, 0);
    clrtoeol();
    move(LINES-2, 0);
    clrtoeol();
    move(LINES-1, 0);
    clrtoeol();
    mvprintw(LINES-3, 0, "%s", p);
    attrset(COLOR_PAIR(NULL_PAIR));
    mvprintw(LINES-1, 0, "Press any key to exit..\n");
    refresh();

    flushinp();
    /*
     * wait until a key is pressed 
     */
    do
    {
	delay_ms(20);
    }
    while (getch() == ERR);

    endwin();
    exit(1);
}

void quit(const char *args, ...)
{
    char p[MAX_MSG_SIZE];
    va_list vp;
    va_start(vp, args);
    vsprintf(p, args, vp);
    va_end(vp);

    endwin();
    printf("\n%s\n", p);
    exit(0);
}


/*fixme do something about this, move to a better file rather than main.c  */

int compare_two_servers(const void *a, const void *b)
{
    const ftp_mirror *ma = (const ftp_mirror *) a;
    const ftp_mirror *mb = (const ftp_mirror *) b;

    int milli_sec_a;
    int milli_sec_b;

    if (ma->status != RESPONSEOK && (mb->status != RESPONSEOK))
	return 1000000;


    milli_sec_a = (ma->tv.tv_sec * 1000) + (ma->tv.tv_usec / 1000);
    if (ma->status != RESPONSEOK)
    {
	milli_sec_a = 1000000;
    }

    milli_sec_b = (mb->tv.tv_sec * 1000) + (mb->tv.tv_usec / 1000);
    if (mb->status != RESPONSEOK)
    {
	milli_sec_b = 1000000;
    }


    return (milli_sec_a - milli_sec_b);
}



int main(int argc, char **argv)
{
    http_stat_t hs;
    ftp_stat_t fs;
    urlinfo url_data, ftps_url_data;
    uerr_t err;
    struct ftp_mirror *ftp_mirrors;
    int i;
    int c;
    int num_servers = 0;
    int use_server = -1;
    int same_url = FALSE;
    char *conf_file;
    char global_conf_file[] = GLOBAL_CONF_FILE;
    /*
     * set the default runtime configuration 
     */
    set_defaults(&argc, &argv);

    conf_file = prozrc_file_name();
    set_preferences(global_conf_file);
    set_preferences(conf_file);
    debug_init();

    while ((c =
	    getopt_long(argc, argv, "?hrfk:1Lt:vgsP:", long_opts,
			NULL)) != EOF)
    {
	switch (c)
	{
	case 'L':
	    license();
	    exit(0);
	case 'h':
	    help();
	    exit(0);
	case 'v':
	    version();
	    exit(0);
	case 'r':
	    rt.run_mode = RESUME;
	    break;
	case 'f':
	    rt.force_mode = TRUE;
	    break;
	case 'k':
	    if (setargval(optarg, &rt.num_connections) != 1)
	    {
		/*
		 * The call failed  due to a invalid arg
		 */
		printf("Error: Invalid arguments for the -k option\n"
		       "Please type proz --help for help\n");
		exit(0);
	    }

	    if (rt.num_connections == 0)
	    {
		printf("Hey! How can I download anything with 0 (Zero)"
		       " connections!?\n"
		       "Please type proz --help for help\n");
		exit(0);
	    }

	    break;
	case 't':
	    if (setargval(optarg, &rt.try_attempts) != 1)
	    {
		/*
		 * The call failed  due to a invalid arg
		 */
		printf
		    ("Error: Invalid arguments for the -t or --tries option(s)\n"
		     "Please type proz --help for help\n");
		exit(0);
	    }
	    break;
	case 'n':
	    /*
	     * Don't use ~/.netrc" 
	     */
	    rt.use_netrc = FALSE;
	    break;

	case 'P':
	    /*
	     * Save the downloaded file to DIR 
	     */
	    rt.output_dirprefix = kstrdup(optarg);
	    break;
	case '?':
	    help();
	    exit(0);
	    break;
	case '1':
	    rt.num_connections = 1;
	    break;

	case 'g':
	    /*
	     * TODO solve this soon 
	     */
#ifdef HAVE_GTK
	    rt.use_gtk_option = TRUE;
#else
	    printf
		("Error: GTK interface is not supported in "
		 "the development version currently\n");
	    exit(0);
#endif
	    break;

	case 129:
	    /*
	     * lets use PORT as the default then 
	     */
	    rt.use_pasv = FALSE;
	    break;
	case 130:
	    /*
	     * retry-delay option 
	     */
	    if (setargval(optarg, &rt.retry_delay) != 1)
	    {
		/*
		 * The call failed  due to a invalid arg
		 */
		printf
		    ("Error: Invalid arguments for the --retry-delay option\n"
		     "Please type proz --help for help\n");
		exit(0);
	    }
	    break;
	case 131:
	    /*--timout option */
	    if (setargval(optarg, &rt.timeout) != 1)
	    {
		/*
		 * The call failed  due to a invalid arg
		 */
		printf
		    ("Error: Invalid arguments for the --timeout option\n"
		     "Please type proz --help for help\n");
		exit(0);
	    }
	    break;
	case 132:
	    /* --no-getch option */
	    rt.die_to_stdout = TRUE;
	    break;

	case 133:
	    /* --debug option */
	    rt.debug_mode = TRUE;
	    break;

	case 's':
	    /* --ftpsearch option */
	    rt.ftp_search = TRUE;
	    break;

	case 135:
	    /* --no-search option */
	    rt.ftp_search = FALSE;
	    break;

	case 136:
	    /* --pt option */
	    if (setargval(optarg, &rt.max_ping_wait) != 1)
	    {
		/*
		 * The call failed  due to a invalid arg
		 */
		printf("Error: Invalid arguments for the --pt option\n"
		       "Please type proz --help for help\n");
		exit(0);
	    }

	    if (rt.max_ping_wait == 0)
	    {
		printf
		    ("Hey! Does waiting for a server response for Zero(0)"
		     " seconds make and sense to you!?\n"
		     "Please type proz --help for help\n");
		exit(0);
	    }

	    break;
	case 137:
	    /* --pao option */
	    if (setargval(optarg, &rt.max_simul_pings) != 1)
	    {
		/*
		 * The call failed  due to a invalid arg
		 */
		printf("Error: Invalid arguments for the --pao option\n"
		       "Please type proz --help for help\n");
		exit(0);
	    }

	    if (rt.max_simul_pings == 0)
	    {
		printf("Hey you! Will pinging Zero(0) servers at once"
		       " achive anything for me!?\n"
		       "Please type proz --help for help\n");
		exit(0);
	    }

	    break;

	case 138:
	    /* --max-ftp-servers option */
	    if (setargval(optarg, &rt.ftps_mirror_req_n) != 1)
	    {
		/*
		 * The call failed  due to a invalid arg
		 */
		printf("Error: Invalid arguments for the --pao option\n"
		       "Please type proz --help for help\n");
		exit(0);
	    }

	    if (rt.ftps_mirror_req_n == 0)
	    {
		printf("Hey! Will requesting Zero(0) servers at once"
		       "from tpsearch.lycos.com achive anything!?\n"
		       "Please type proz --help for help\n");
		exit(0);
	    }

	    break;
	case 139:
	    /* --max-bps */
	    if (setargval(optarg, &rt.max_bps) != 1)
	    {
		/*
		 * The call failed  due to a invalid arg
		 */
		printf
		    ("Error: Invalid arguments for the --max-bps option\n"
		     "Please type proz --help for help\n");
		exit(0);
	    }
	    break;

	default:
	    printf("Error: Invalid  option\n");
	    exit(0);
	}
    }

    if (optind == argc)
    {
	printf("Error: You haven't specified a URL\n"
	       "Please type proz --help for help\n");
	exit(0);
    }

    /*
     * Parse the netrc file if needed 
     */
    if (rt.use_netrc == TRUE)
    {
	char *home = home_dir();
	if (home != NULL)
	{
	    char *netrc_file = malloc(strlen(home) + strlen(".netrc") + 2);
	    sprintf(netrc_file, "%s/%s", home, ".netrc");
	    rt.netrc_list = parse_netrc(netrc_file);
	}
    }

    /*
     * Ok.. so we have a URL 
     * Setup curses 
     */
    initscr();
    keypad(stdscr, TRUE);
    start_color();
    noecho();
    nodelay(stdscr, TRUE);
    init_pair(HIGHLIGHT_PAIR, COLOR_GREEN, COLOR_BLACK);
    init_pair(MSG_PAIR, COLOR_YELLOW, COLOR_BLUE);
    init_pair(WARN_PAIR, COLOR_RED, COLOR_BLACK);
    init_pair(PROMPT_PAIR, COLOR_RED, COLOR_BLACK);
    init_pair(SELECTED_PAIR, COLOR_WHITE, COLOR_BLUE);

    memset(&url_data, 0, sizeof(url_data));
    err = parseurl(argv[optind], &url_data, 0);
    if (err != URLOK)
    {
	die("The URL syntatically wrong!\n");
    }

    if (!*url_data.file)
    {
	die("Error: You haven't specified a file name.");
    }


    if (url_data.proto == URLHTTP)
    {
	err = get_http_info(&url_data, &hs);
	if (err == NEWLOCATION)
	{
	  char * referer = 0;
	    /*
	     * loop for max_redirections searching for a valid file 
	     */
	    for (i = 0; i < rt.max_redirections; ++i)
	    {
	char *constructed_newloc;
		

/*DONE : handle relative urls too */
		  constructed_newloc =
		    uri_merge(url_data.url, hs.newloc);

		  debug_prz("Redirected to %s, merged URL = %s",
			  hs.newloc, constructed_newloc);

		  err =
		    parseurl(constructed_newloc, &url_data, 0);
		  if (err != URLOK)
		    {
		      die("The server returned location is syntatically wrong: %s!", constructed_newloc);
		    }

		  referer= url_data.host;

		url_data.referer=strdup(referer);
		/*If we have been directed to a FTP site lets continue 
		   from the FTP routine 
		 */
		if (url_data.proto != URLHTTP)
		    break;

		err = get_http_info(&url_data, &hs);

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
			die("Unable to resolve %s", url_data.host);
			break;
		    case CONREFUSED:
			die("%s Rejected the Connection Attempt",
			    url_data.host);
			break;
		    case CONERROR:
			if (errno == ETIMEDOUT)
			{
			    die("The connection Attempt to %s timed out",
				url_data.host);
			} else
			    die("Error while attempting to connect to %s",
				url_data.host);
			break;
		    case HAUTHREQ:
			die("%s needs authentication to access that resource -: Privileged resource!", url_data.host);
			break;
		    case HAUTHFAIL:
			die("Authentication with %s failed!",
			    url_data.host);
			break;
		    default:
			die("A error occured while trying to get info from the server\n");
			break;
		    }

		}
	    }
	} else if (err != HOK)
	{
	    if (hs.statcode == HTTP_NOT_FOUND)
	    {
		die("Error: The file was not found on the server!\n");
	    } else
	    {
		switch (err)
		{
		case HOSTERR:
		    die("Unable to resolve %s", url_data.host);
		    break;
		case CONREFUSED:
		    die("%s Rejected the Connection Attempt",
			url_data.host);
		    break;
		case CONERROR:
		    if (errno == ETIMEDOUT)
		    {
			die("The connection Attempt to %s timed out",
			    url_data.host);
		    } else
			die("Error while attempting to connect to %s",
			    url_data.host);
		    break;
		case HAUTHREQ:
		    die("%s needs authentication to access that resource -: Privileged resource", url_data.host);
		    break;
		case HAUTHFAIL:
		    die("Authentication with %s failed!", url_data.host);
		    break;
		default:
		    die("A error occured while trying to get info from the server\n");
		    break;
		}
	    }
	}

    }


    if (url_data.proto == URLFTP)
    {
	err = ftp_get_info(&url_data, &fs);

	if (err == FTPOK)
	{
	    if (fs.fp.flagtryretr == 0 && fs.fp.flagtrycwd == 1)
		die("Error:  %s is a directory, not a file",
		    url_data.file);
	} else
	{
	    switch (err)
	    {
	    case HOSTERR:
		die("Unable to resolve %s", url_data.host);
		break;
	    case CONREFUSED:
		die("%s Rejected the Connection Attempt", url_data.host);
		break;
	    case CONERROR:
		if (errno == ETIMEDOUT)
		{
		    die("The connection Attempt to %s timed out",
			url_data.host);
		} else
		    die("Error while attempting to connect to %s",
			url_data.host);
		break;
	    case FTPCONREFUSED:
		die("%s Rejected the Connection Attempt", url_data.host);
		break;
	    case FTPLOGREFUSED:
		die("The FTP server has rejected the login attempt.");
		break;
	    case FTPNSFOD:
		die("Error: A file called %s was not found on the server",
		    url_data.file);
		break;
	    default:
		die("A error occured while trying to get info from the server\n");
		break;
	    }

	}
    }

    if ((url_data.proto != URLHTTP) && (url_data.proto != URLFTP))
    {
	die("Error: Unsupported Protocol was specified\n");
    }
    /* now we got the info lets do our ftpsearch here... */

    message("File size=%ld", url_data.file_size);
    if (url_data.file_size == -1)
    {
	message
	    ("Unable to get file size, Will download from original server\n");
	rt.ftp_search = FALSE;
    }

    if (rt.ftp_search == TRUE)
    {
	err =
	    get_complete_mirror_list(&url_data, &ftp_mirrors,
				     &num_servers);

	if (err != MIRINFOK)
	{
	    message
		("Unable to get mirror info, Will download from original server\n");
	    same_url = TRUE;
	    delay_ms(2000);
	} else
	{
	    if (num_servers == 0)
	    {
		message
		    ("No mirrors were found, Will download from original server\n");
		same_url = TRUE;
		delay_ms(2000);
	    } else
	    {
		int user_sel_server;
		interface_ret i_ret;

		/*Ok we got the mirrors..lets ping them */
		/*Clear any other keys in the key buffer */
		flushinp();
		i_ret =
		    curses_ftpsearch_interface(ftp_mirrors, num_servers,
					       &user_sel_server);
		/*Clear any other keys in the key buffer */
		flushinp();
		/* ok we exited */

		/* We got some mirrors, now lets select the fastest one */
		use_server = -1;

		if (i_ret == AUTO || i_ret == USER_ABORTED)
		{

		    qsort(ftp_mirrors, num_servers,
			  sizeof(struct ftp_mirror), compare_two_servers);
		    use_server = 0;

		    debug_prz("dumping sorted list\n");

		    for (i = 0; i < num_servers; i++)
		    {
			debug_prz("%s\n", ftp_mirrors[i].server_name);

		    }
		    /* is the "fastest" server one which has not been tested? 
		       If so download from the original server instead 
		     */
		    if (ftp_mirrors[use_server].status != RESPONSEOK)
		    {
			message
			    ("None of the servers appear to be fast enough, therefore will use original server \n");
			same_url = TRUE;
			/*fixme */
			use_server = -1;
			delay_ms(2000);
		    }

		} else if (i_ret == USER_SELECTED)
		{

		  /*get the name */
		    char *sel_loc= ftp_mirrors[user_sel_server].full_name;
		    debug_prz("selloc %s", sel_loc);
		    /*now sort the list according to  speed */
		    qsort(ftp_mirrors, num_servers,
			  sizeof(struct ftp_mirror), compare_two_servers);
		    /* now get the new localtion of the user selected server */
		    use_server =
			get_mirror_list_pos(sel_loc, ftp_mirrors,
					    num_servers);
		}

		if (use_server != -1)
		{
		    message("Will get from %s\n",
			    ftp_mirrors[use_server].server_name);
		    delay_ms(1000);
		    message("Location is %s\n",
			    ftp_mirrors[use_server].full_name);
		    delay_ms(2000);
		    debug_prz("getting from %s\n",
			      ftp_mirrors[use_server].server_name);
		    debug_prz("gettinglocation is %s\n",
			      ftp_mirrors[use_server].full_name);

		    /*now copy that servers pathname  to url_data */

		    memset(&ftps_url_data, 0, sizeof(ftps_url_data));
		    debug_prz("mem setted");
		    err =
			parseurl(ftp_mirrors[use_server].full_name,
				 &ftps_url_data, 0);
		    debug_prz("ftpsearch returned URL parsed");
		    if (err != URLOK)
		    {
			die("The ftp search returned URL is syntatically wrong!\n");
		    }
		}
	    }
	}
    }


    /* Did the ftpsearch return the same location as the original url as the fastest server? */
    if (rt.ftp_search == TRUE && same_url == FALSE)
    {
	if (strcmp(ftps_url_data.path, url_data.path) == 0)
	{
	    same_url = TRUE;
	    freeurl(&ftps_url_data, 0);
	} else
	{
	    freeurl(&url_data, 0);
	    memcpy(&url_data, &ftps_url_data, sizeof(urlinfo));
	    same_url = FALSE;
	}
    } else
	same_url = TRUE;


    /* Now lets switch based on the protocol */
    switch (url_data.proto)
    {

    case URLHTTP:
	fetch_http_portions(&url_data, &hs, same_url, ftp_mirrors,
			    num_servers);
	break;

    case URLFTP:
	fetch_ftp_portions(&url_data, &fs, same_url, ftp_mirrors,
			   num_servers);
	break;

    default:
	die("Error: Unsupported Protocol was specified\n");
    }


    message("File Succesfully Retreived, Now joining the sections");
    join_downloads(url_data.file, connections, rt.num_connections);

    message("Deleting the unwanted files");

    if (delete_downloads(connections, rt.num_connections) != 1)
	die("Unable to cleanup the downloaded file parts");

    if (log_delete_logfile(&url_data) != 0)
    {
	die("Error while deleting the logfile: %s", strerror(errno));
	return 1;
    } else
	quit("All Done: Download Succesfull!");


    /*
     *  just to keep gcc quiet 
     */
    return 1;
}
