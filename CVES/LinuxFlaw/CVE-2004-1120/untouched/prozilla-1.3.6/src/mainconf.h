/* Declarations for the accelerator and included by most sources files.
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

#ifndef MAINCONF_H
#define MAINCONF_H

#include "main.h"
#include "url.h"
#include "ftpsearch.h"

void http_allocate_connections(urlinfo * u, long file_length,
			       char *file_io_mode);
void ftp_allocate_connections(urlinfo * u, long file_length,
			      char *file_io_mode);
void do_logfile(urlinfo * u);
interface_ret do_downloads(ftp_mirror * mirrors, int num_servers);

#endif
