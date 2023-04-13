/* Declarations for handling the  connections.
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

#ifndef CONNECTION_H
#define CONNECTION_H
#include <sys/time.h>
#include "url.h"
#include "http.h"

typedef enum {
    IDLE = 0,
    CONNECTING,
    LOGGININ,
    DOWNLOADING,
    COMPLETED,
    LOGINFAIL,
    CONREJECT,
    REMOTEFATAL,
    LOCALFATAL,
    TIMEDOUT,
    MAXTRYS
} dl_status;

typedef struct connect {
    int listen_sock;
    int control_sock;
    int data_sock;
    int http_sock;
    /*
     * struct which contains the parsed url info 
     */
    /*
     * It includes the remote file,path,protocol etc  
     */
    urlinfo u;
    /*
     * the file name to save the data to locally 
     */
    char *localfile;
    /*
     * Pointer to file that we will be saving the data to locally 
     */
    FILE *fp;
    /*
     * tells whether to open the file for appending or for writing etc 
     */
    /*
     * Used for adding resume support 
     */
    char *file_mode;
    long remote_startpos;
    long remote_endpos;
    long remote_bytes_received;
    long main_file_size;

    /*
     * indicates the startpos of the localfile
     * * It is allways 0 in normal mode and can be any + value
     * * in resume mode
     */
    long local_startpos;
    /*the start postion at the beginning of the download */
    long orig_local_startpos;
    dl_status status;
    http_stat_t hs;
    long len;
    char ftp_buffer[FTP_BUFFER_SIZE + 1];
    /*
     * tells the download thread whether to abort the download ... 
     */
    int abort;
    /*
     * information about the connections start and end time 
     */
    struct timeval time_begin;
    struct timeval time_end;
    /*
     * info about wether to retry the thread etc 
     */
    int retry;
    /*
     * the number of attempts to try to complete a connection 
     * 0 means unlimited connection attempts 
     */
    int try_attempts;
    /*
     * the time when to try to restart the connection 
     */
    struct timeval retry_start;
    long rate_bps;
    long max_allowed_bps;

  /* this stores the time when the connections login was rejected */
     time_t ftp_login_reject_start;
} connection_data;

/*Function definitions */

/* If all the downloads have been done ok returns TRUE */
int all_dls_complete(connection_data * connections, int num_con);

/* If all the downloads have been refused to login, returns TRUE */
int all_dls_failed_login(connection_data * connections, int num_con);

/* If all the downloads have been refused to connect, returns TRUE */
int all_dls_connect_rejected(connection_data * connections, int num_con);

/* If all the downloads have failed due to remote causes, returns TRUE */
int all_dls_remote_failed(connection_data * connections, int num_con);


/* Returns the number of connections whose status is COMLPETED ie (completed)*/
int query_completed_conns_count(connection_data * connections,
				int num_con);
/* Returns the number of connections whose status is CONNECTING ie (connecting to server)*/
int query_connecting_conns_count(connection_data * connections,
				 int num_con);
/* Returns the number of connections whose status is LOGGININ ie (logging to server)*/
int query_logging_conns_count(connection_data * connections, int num_con);
/* Returns the number of connections whose status is DOWNLOADING */
int query_downloading_conns_count(connection_data * connections,
				  int num_con);
int query_remote_errror_conns_count(connection_data * connections,
				    int num_con);

/*This cats all thedownloaded protions into one big file */
int join_downloads(char *output_file, connection_data * connections,
		   int num_con);
/*Cleans up the downlaoded file portions */
int delete_downloads(connection_data * connections, int num_con);

void calc_con_ratebps(connection_data * connection);
void throttle_con_rate(connection_data * connection);

void calc_throttle_factor(connection_data * connections,
			  int num_connections);
#endif
