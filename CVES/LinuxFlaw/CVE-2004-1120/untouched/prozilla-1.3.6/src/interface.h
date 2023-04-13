/* Declarations for the interface
   
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

#ifndef INTERFACE_H

#include "connection.h"
#include "ftpsearch.h"

#ifdef HAVE_GTK
int gtk_do_interface(long f_size);
#endif

interface_ret curses_do_interface(connection_data * cons,
				  int num_cons, ftp_mirror * mirrors,
				  int num_servers);
void curses_message(const char *args, ...);
/* Displays the mesasge and gets the users input for overwriting files*/
int curses_query_user_input(const char *args, ...);

void gtk_message(const char *args, ...);

#define INTERFACE_H
#endif
