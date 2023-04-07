/* Declarations for the log file
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

#ifndef LOGFILE_H
#define LOGFILE_H

typedef struct {
/* the number of connections that this download was started with */
    int num_connections;

} logfile;

int log_create_logfile(urlinfo * u);
int log_read_logfile(urlinfo * u, logfile * lf);
int log_delete_logfile(urlinfo * u);
int log_logfile_exists(urlinfo * u);
#endif
