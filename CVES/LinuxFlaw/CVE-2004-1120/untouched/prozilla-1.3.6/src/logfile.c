/* Functions to help with the log file
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
#include <string.h>
#include <unistd.h>
#include "connection.h"
#include "misc.h"
#include "main.h"
#include "runtime.h"
#include "logfile.h"


/*creates the log file and stores the info */

int log_create_logfile(urlinfo * u)
{
    char buffer[MAXPATHLEN];
    FILE *fp = NULL;

    /*
     * Compute the name of the logfile 
     */
    sprintf(buffer, "%s%s.log", u->file, DEFAULT_FILE_EXT);
    if (!(fp = fopen(buffer, "wb")))
    {

	/*
	 * fixme add the error displaing to the main function 
	 */
	message("Error opening file %s for writing: %s",
		buffer, strerror(errno));
	return -1;
    }

    if (fwrite(&rt.num_connections, 1, sizeof(rt.num_connections), fp)
	!= sizeof(rt.num_connections))
    {
	message("Error writing to file %s: %s", buffer, strerror(errno));
	fclose(fp);
	return -1;
    }

    fclose(fp);
    return 0;
}

/* returns 1 if the logfile exists, 0 if it doesn't and -1 on error*/
int log_logfile_exists(urlinfo * u)
{
    char buffer[MAXPATHLEN];
    int ret;
    struct stat st_buf;

    /*
     * Compute the name of the logfile 
     */
    sprintf(buffer, "%s%s.log", u->file, DEFAULT_FILE_EXT);

    ret = stat(buffer, &st_buf);
    if (ret == -1)
    {
	if (errno == ENOENT)
	    return 0;
	else
	    return -1;
    } else
	return 1;
}

/* delete the log file */
int log_delete_logfile(urlinfo * u)
{
    char buffer[MAXPATHLEN];
    int ret;

    sprintf(buffer, "%s%s.log", u->file, DEFAULT_FILE_EXT);
    ret = unlink(buffer);
    if (ret == -1)
    {
	if (errno == ENOENT)
	{
	    message("logfile doesn't exist");
	    return 0;
	} else
	    return -1;
    }

    return 0;
}

/* Read the logfile into the logfile structure */

int log_read_logfile(urlinfo * u, logfile * lf)
{
    char buffer[MAXPATHLEN];
    FILE *fp = NULL;

    /*
     * Compute the name of the logfile 
     */
    sprintf(buffer, "%s%s.log", u->file, DEFAULT_FILE_EXT);

    if (!(fp = fopen(buffer, "rb")))
    {
	/*
	 * fixme add the error displaing to the main function 
	 */
	message("Error opening file %s for reading: %s",
		buffer, strerror(errno));
	return -1;
    }
    if (fread(&lf->num_connections, sizeof(int), 1, fp) != 1)
    {
	fclose(fp);
	return -1;
    }

    fclose(fp);
    return 0;
}
