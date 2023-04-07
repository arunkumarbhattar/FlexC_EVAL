/* Declarations for retreiving and parsing mirror information 
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

#ifndef FTPSEARCH_H
#define FTPSEARCH_H

#include <netdb.h>
#include "url.h"
#include "http.h"

#define LYCOS_FTPSEARCH_LOC "http://download.lycos.com/swadv/AdvResults.asp"

typedef enum {
    RESPONSEOK = 1, NORESPONSE, ERROR
} ftp_mirror_stat;


typedef struct ftp_mirror {
    char *server_name;
    char *path;
    char *file_name;
    char *full_name;
    char *file_size;
    struct timeval tv;
    int milli_secs;
    ftp_mirror_stat status;
} ftp_mirror;


uerr_t get_mirror_info(urlinfo * u, http_stat_t * hs, char **ret_buf);
uerr_t my_connect_to_server(int *sock, struct hostent *hp, int port,
			    int timeout);
uerr_t get_complete_mirror_list(urlinfo * url_data,
				struct ftp_mirror **pmirrors,
				int *num_servers);
void display_list(struct ftp_mirror *pmirrors, int num_servers);
interface_ret curses_ftpsearch_interface(struct ftp_mirror *ftp_mirrors,
					 int num_servers,
					 int *selected_server);
int get_mirror_list_pos(char *full_name, ftp_mirror * list,
			int num_servers);

#endif
