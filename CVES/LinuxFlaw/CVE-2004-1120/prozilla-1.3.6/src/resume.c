/* Functions to help resuming downloads
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
#include <sys/param.h>
#include <sys/stat.h>
#include <errno.h>
#include "connection.h"
#include "resume.h"
#include "misc.h"
#include "main.h"
#include "runtime.h"
#include "debug.h"


/* This function modifies a single connections download start and 
*   end info it returns 0 on sucess and -1 on error 
* returns -1 on error occuring like file to found
*/

int resume_modify_ftp_single_connection(connection_data * connection)
{
    struct stat buffer;

    if (stat(connection->localfile, &buffer) == -1)
    {
	/*
	 * the call failed 
	 */
	/*
	 * if the error is due to the file not been present then there is no need to do anything..just continue, otherwise return error (-1)
	 */
	if (errno == ENOENT)
	    return 0;
	else
	    return -1;
    }


    if (buffer.st_size ==
	(connection->remote_endpos - connection->remote_startpos))
    {
	/*
	 * this means that the download for this connection
	 * has been complete earlier so just flag the connection as done 
	 */
	connection->status = COMPLETED;
    }

    /*
     * now change the remote connections start  position 
     */
    connection->remote_startpos +=
	(buffer.st_size - connection->local_startpos);
    /*
     * The startpos of the localfile 
     */
    connection->local_startpos = buffer.st_size;

    if (connection->try_attempts == 0)
	connection->orig_local_startpos = buffer.st_size;

    debug_prz("orig start pos= %d\n", connection->orig_local_startpos);

    return 0;
}


int resume_modify_ftp_connections(connection_data * connections,
				  int num_conns)
{
    int i;

    for (i = 0; i < num_conns; i++)
    {
	/*
	 * Modify the single connection 
	 */
	if (resume_modify_ftp_single_connection(connections + i) != 0)
	{
	    return -1;
	}
    }
    return 0;
}




/* 
 * This function modifies a single connections download start and 
 * end info it returns 0 on sucess and -1 on error 
 * returns -1 on error occuring like file to found
*/

int resume_modify_http_single_connection(connection_data * connection)
{
    struct stat buffer;

    if (stat(connection->localfile, &buffer) == -1)
    {
	/*
	 * the call failed 
	 */

	/*
	 * if the error is due to the file not been present then there 
	 * is no need to do anything..just continue, otherwise return error (-1)
	 */
	if (errno == ENOENT)
	    return 0;
	else
	    return -1;
    }

    /*
     * I have to add 1  hear because in HTTP 0-0 would mean down loading 1 byte 
     */

    if (buffer.st_size ==
	(connection->remote_endpos - connection->remote_startpos + 1))
    {
	/*
	 * this means that the download for this connection
	 * has been complete earlier so just flag the connection as done 
	 */
	connection->status = COMPLETED;
    }


    /* now change the remote connections start  position */
    connection->remote_startpos +=
	(buffer.st_size - connection->local_startpos);
    /* The startpos of the localfile */
    connection->local_startpos = buffer.st_size;

    if (connection->try_attempts == 0)
	connection->orig_local_startpos = buffer.st_size;

    debug_prz("orig start pos= %d\n", connection->orig_local_startpos);

    return 0;
}


int resume_modify_http_connections(connection_data * connections,
				   int num_conns)
{
    int i;

    for (i = 0; i < num_conns; i++)
    {
	if (resume_modify_http_single_connection(connections + i) != 0)
	{
	    return -1;
	}
    }
    return 0;

}
