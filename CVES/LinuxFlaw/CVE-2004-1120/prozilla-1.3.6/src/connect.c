/* Connection routines.
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif				/*
				 * HAVE_CONFIG_H 
				 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <ctype.h>
#include <assert.h>
#include "misc.h"
#include "connect.h"
#include "runtime.h"
#include "debug.h"

uerr_t connect_to_server(int *sock, char *name, int port, int timeout)
{
    unsigned int portnum;
    int status;
    struct sockaddr_in server;
    struct hostent *hp, hostbuf;
    extern int h_errno;
    /*    int opt; */
    int noblock, flags;

    char *tmphstbuf;
    size_t hstbuflen = 2048;
    tmphstbuf = kmalloc(hstbuflen);

    assert(name != NULL);

    portnum = port;
    memset((void *) &server, 0, sizeof(server));

    message("Resolving %s", name);

    hp=k_gethostname (name,&hostbuf,&tmphstbuf,&hstbuflen);

    if (hp == NULL)
    {
	message("Failed to resolve %s", name);
	return HOSTERR;
    }

	message("Resolved %s !", name);

    
    memcpy((void *) &server.sin_addr, hp->h_addr, hp->h_length);
    server.sin_family = hp->h_addrtype;
    server.sin_port = htons(portnum);

    /*
     * create socket 
     */
    if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) < 1)
    {
	message("unable to create socket\n");
	free(tmphstbuf);
	return CONSOCKERR;
    }
    /*Experimental */
    flags = fcntl(*sock, F_GETFL, 0);
    if (flags != -1)
	noblock = fcntl(*sock, F_SETFL, flags | O_NONBLOCK);
    else
	noblock = -1;

    message("Connecting to server.......");


    status = connect(*sock, (struct sockaddr *) &server, sizeof(server));

    if (status == -1 && noblock != -1 && errno == EINPROGRESS)
    {
	fd_set writefd;
	struct timeval tv;

	FD_ZERO(&writefd);
	FD_SET(*sock, &writefd);

	tv.tv_sec = timeout;
	tv.tv_usec = 0;

	status = select((*sock + 1), NULL, &writefd, NULL, &tv);
	/*do we need to retry if the err is EINTR? */

	if (status > 0)
	{
	    socklen_t arglen = sizeof(int);

	    if (getsockopt(*sock, SOL_SOCKET, SO_ERROR, &status, &arglen) <
		0)
		status = errno;

	    if (status != 0)
		errno = status, status = -1;
	    if (errno == EINPROGRESS)
		errno = ETIMEDOUT;
	} else if (status == 0)
	    errno = ETIMEDOUT, status = -1;
    }

    if (status < 0)
    {
	close(*sock);

	if (errno == ECONNREFUSED)
	{
	    free(tmphstbuf);
	    return CONREFUSED;
	} else
	{
	    free(tmphstbuf);
	    return CONERROR;
	}
    } else
    {
	flags = fcntl(*sock, F_GETFL, 0);

	if (flags != -1)
	{
	    fcntl(*sock, F_SETFL, flags & ~O_NONBLOCK);
	}
    }


    /*    setsockopt(*sock, SOL_SOCKET, SO_KEEPALIVE,
     *         (char *) &opt, (int) sizeof(opt));  
     */
    message("Connect OK!");
    free(tmphstbuf);
    return NOCONERROR;
}


uerr_t bind_socket(int *sockfd)
{

    struct sockaddr_in serv_addr;

    /*
     * Open a TCP socket (an Internet stream socket).
     */
    if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
	return CONSOCKERR;
    }

    /*
     * Fill in structure fields for binding
     */
    memset((void *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(0);	/*
					 * let system choose 
					 */

    /*
     * bind the address to the socket
     */
    if (bind(*sockfd, (struct sockaddr *) &serv_addr,
	     sizeof(serv_addr)) < 0)
    {
	perror("bind");
	close(*sockfd);
	return BINDERR;

    }

    /*
     * allow only one server 
     */

    if (listen(*sockfd, 1) < 0)
    {
	perror("listen");
	close(*sockfd);
	return LISTENERR;
    }
    return BINDOK;
}

uerr_t accept_connection(int listen_sock, int *data_sock)
{
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    int sockfd;

    sockfd = accept(listen_sock, (struct sockaddr *) &cli_addr, &clilen);
    if (sockfd < 0)
    {
	perror("accept");
	return ACCEPTERR;
    }

    *data_sock = sockfd;
    /*
     * now we can free the listen socket since it is not needed...
     * accept returnd the new socket... 
     */
    close(listen_sock);
    return ACCEPTOK;
}




int select_fd(int fd, int maxtime, int writep)
{
    fd_set fds, exceptfds;
    struct timeval timeout;

    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    FD_ZERO(&exceptfds);
    FD_SET(fd, &exceptfds);
    timeout.tv_sec = maxtime;
    timeout.tv_usec = 0;

    return (select(fd + 1, writep ? NULL : &fds, writep ? &fds : NULL,
		   &exceptfds, &timeout));

}

int krecv(int sock, char *buffer, int size, int flags, int timeout)
{
    int ret;
    int arglen;

    arglen = sizeof(int);

    assert(size >= 0);

    do
    {
	if (timeout)
	{
	    do
	    {
		ret = select_fd(sock, timeout, 0);
	    }
	    while (ret == -1 && errno == EINTR);


	    if (ret <= 0)
	    {
		/*              debug_prz("Error after select res=%d errno=%d", ret,
		 *         errno);
		 */

		/*
		 * Set errno to ETIMEDOUT on timeout. 
		 */
		if (ret == 0)
		    errno = ETIMEDOUT;

		return -1;
	    }
	}

	ret = recv(sock, buffer, size, flags);
    }
    while (ret == -1 && errno == EINTR);

    return ret;
}

int ksend(int sock, char *buffer, int size, int flags, int timeout)
{
    int ret = 0;


    /* `write' may write less than LEN bytes, thus the outward loop
       keeps trying it until all was written, or an error occurred.  The
       inner loop is reserved for the usual EINTR f*kage, and the
       innermost loop deals with the same during select().  */
    while (size)
    {
	do
	{
	    if (timeout)
	    {
		do
		{
		    ret = select_fd(sock, timeout, 1);
		}
		while (ret == -1 && errno == EINTR);
		if (ret <= 0)
		{
		    /* Set errno to ETIMEDOUT on timeout.  */
		    if (ret == 0)
			errno = ETIMEDOUT;
		    return -1;
		}
	    }
	    ret = send(sock, buffer, size, flags);
	}
	while (ret == -1 && errno == EINTR);
	if (ret <= 0)
	    break;
	buffer += ret;
	size -= ret;
    }
    return ret;
}


struct hostent * k_gethostname (const char *host,struct hostent *hostbuf,char **tmphstbuf,size_t *hstbuflen)
{

    struct hostent *hp;
    int herr,res;

    if (*hstbuflen == 0)
    {
	*hstbuflen = 2048; 
	*tmphstbuf = (char *)kmalloc (*hstbuflen);
    }

#ifdef HAVE_FUNC_GETHOSTBYNAME_R_6
    while (( res = 
	     gethostbyname_r(host,hostbuf,*tmphstbuf,*hstbuflen,&hp,&herr))
	   && (errno == ERANGE))
#endif
#ifdef HAVE_FUNC_GETHOSTBYNAME_R_5
	while ((NULL == ( hp =
			  gethostbyname_r(host,hostbuf,*tmphstbuf,*hstbuflen,&herr)))
	       && (errno == ERANGE))
#endif
	{
	    /* Enlarge the buffer. */
	    *hstbuflen *= 2;
	    *tmphstbuf = (char *)realloc (*tmphstbuf,*hstbuflen);
	}
    if (res)
	return NULL;
    return hp;
}
