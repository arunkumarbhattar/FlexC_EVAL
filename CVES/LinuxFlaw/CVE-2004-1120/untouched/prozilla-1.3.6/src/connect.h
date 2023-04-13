/* Declarations for connection handling.
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

#ifndef CONNECT_H
#define CONNECT_H

#include "main.h"

uerr_t connect_to_server(int *sock, char *name, int port, int timeout);
uerr_t get_listen_socket(int control_sock, int *listen_sock);
uerr_t accept_connection(int listen_sock, int *data_sock);
uerr_t bind_socket(int *sockfd);
int select_fd(int fd, int maxtime, int writep);

struct hostent * k_gethostname (const char *host,struct hostent *hostbuf,char **tmphstbuf,size_t *hstbuflen);

/* special functions to read and write to sockets */
/* which have a time delay incorparated in them... */

int krecv(int sock, char *buffer, int size, int flags, int timeout);
int ksend(int sock, char *buffer, int size, int flags, int timeout);

#endif
