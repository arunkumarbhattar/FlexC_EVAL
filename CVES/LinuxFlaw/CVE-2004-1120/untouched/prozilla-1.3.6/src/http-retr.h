/* Declarations for getting HTTP files.
   Copyright (C) 1995, 1996, 1997 Free Software Foundation, Inc.
   
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

#ifndef HTTP_RETR_H
#define HTTP_RETR_H

#include "connection.h"
#include "ftpsearch.h"

uerr_t http_get_file_chunk(connection_data * connection);
uerr_t http_get_complete_file(connection_data * connection);
void http_loop(connection_data * connection);
uerr_t http_retr_fsize_known(connection_data * connection, int sock,
			     FILE * fp);
uerr_t http_retr_fsize_notknown(connection_data * connection, int sock,
				FILE * fp);
uerr_t fetch_http_portions(urlinfo * url_data, http_stat_t * hs,
			   int same_url, ftp_mirror * mirrors,
			   int num_servers);
#endif
