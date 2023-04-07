/* Declarations for getting FTP files.
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

#ifndef FTP_RETR_H
#define FTP_RETR_H

#include "connection.h"
#include "ftpsearch.h"


uerr_t ftp_get_file_chunk(connection_data * connection);
uerr_t ftp_get_file_to_end(connection_data * connection);
void ftp_loop(connection_data * connection);
uerr_t ftp_retr_fsize_known(connection_data * connection, FILE * fp);
uerr_t ftp_retr_fsize_notknown(connection_data * connection, FILE * fp);

uerr_t fetch_ftp_portions(urlinfo * url_data, ftp_stat_t * fs,
			  int same_url, ftp_mirror * mirrors,
			  int num_servers);
#endif
