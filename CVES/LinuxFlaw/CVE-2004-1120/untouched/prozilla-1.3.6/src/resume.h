/* Declarations for resuming
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

#ifndef RESUME_H
#define RESUME_H

#include "connection.h"
int resume_modify_http_connections(connection_data * connections,
				   int num_conns);
int resume_modify_ftp_connections(connection_data * connections,
				  int num_conns);
/* The following are for single connections */
int resume_modify_http_single_connection(connection_data * connection);
int resume_modify_ftp_single_connection(connection_data * connection);

#endif
