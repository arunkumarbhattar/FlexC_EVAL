/* The source file for the modified ping code
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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <ctype.h>
#include <assert.h>
#include "main.h"
#include "http.h"
#include "netrc.h"
#include "connect.h"
#include "runtime.h"
#include "misc.h"
#include "ftpsearch.h"
#include "debug.h"

int ping_ftp_read_msg(int sock, char *szBuffer, int len);
int ping_ftp_check_msg(int sock, char *szBuffer, int len);
uerr_t ping_ftp_get_reply(int sock, char *szBuffer);
uerr_t ping_ftp_get_line(int control_sock, char *buffer);
int ping_get_ftp_return(const char *ftp_buffer);
uerr_t ping_connect_to_server(int *sock, struct hostent *hp, int port,
			      int timeout);



/* modified "ping algorithm for ftp servers*/

uerr_t myftp_ping(char *name, struct timeval *tv)
{

    struct timeval start_time;
    struct timeval end_time;
    int sock;
    struct hostent *hp, hostbuf;
    char szBuffer[FTP_BUFFER_SIZE];
    uerr_t err;

    char *tmphstbuf;
    size_t hstbuflen = 2048;
    tmphstbuf = malloc(hstbuflen);

    hp=k_gethostname (name,&hostbuf,&tmphstbuf,&hstbuflen);
    
     if (hp == NULL)
    {
	message("Failed to resolve %s", name);
	free(tmphstbuf);
	return HOSTERR;
    }


    /* get start time */
    gettimeofday(&start_time, 0);

    /* lets try connecting */
    err = ping_connect_to_server(&sock, hp, 21, rt.max_ping_wait);

    if (err != NOCONERROR)
    {
	free(tmphstbuf);
	return err;
    }


    /* ok so we have a connection now time to get the servers reponse message back which would compelte the "ping */

    err = ping_ftp_get_reply(sock, szBuffer);

    if (err != FTPOK)
    {
	close(sock);
	free(tmphstbuf);
	return err;
    }
    if (*szBuffer != '2')
    {
	close(sock);
	free(tmphstbuf);
	return err;
    }

    /* the end time */
    gettimeofday(&end_time, 0);

    timeval_subtract(tv, &end_time, &start_time);
    close(sock);
    free(tmphstbuf);
    return err;
}


uerr_t ping_connect_to_server(int *sock, struct hostent * hp, int port,
			      int timeout)
{
    unsigned int portnum;
    int status;
    struct sockaddr_in server;
    extern int h_errno;
    int noblock, flags;

    portnum = port;
    memset((void *) &server, 0, sizeof(server));

    memcpy((void *) &server.sin_addr, hp->h_addr, hp->h_length);
    server.sin_family = hp->h_addrtype;
    server.sin_port = htons(portnum);

    /*
     * create socket 
     */
    if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) < 1)
    {
	perror("socket");
	return CONSOCKERR;
    }
    /*Experimental */
    flags = fcntl(*sock, F_GETFL, 0);
    if (flags != -1)
	noblock = fcntl(*sock, F_SETFL, flags | O_NONBLOCK);
    else
	noblock = -1;


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
	/* do we need to retry if the err is EINTR */

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
	    return CONREFUSED;
	else
	    return CONERROR;
    } else
    {
	flags = fcntl(*sock, F_GETFL, 0);
	if (flags != -1)
	    fcntl(*sock, F_SETFL, flags & ~O_NONBLOCK);
    }

    return NOCONERROR;
}




uerr_t ping_ftp_get_line(int control_sock, char *buffer)
{
    int iLen, iBuffLen = 0, ret = 0;
    char *szPtr = buffer, ch = 0;

    while ((iBuffLen < FTP_BUFFER_SIZE) &&
	   ((ret = ping_ftp_check_msg(control_sock, &ch, 1)) > 0))
    {
	iLen = ping_ftp_read_msg(control_sock, &ch, 1);
	if (iLen != 1)
	{
	    return FTPERR;
	}
	iBuffLen += iLen;
	*szPtr = ch;
	szPtr += iLen;

	if ((szPtr - buffer) >= 4)
	    break;
	if (ch == '\n')
	    break;		/*
				 * we have a line: return 
				 */
    }
    /*
     * Check for error returned in ftp_check_message 
     */
    if (ret == -1)
	return FTPERR;

    *(szPtr + 1) = '\0';
    return FTPOK;
}



uerr_t ping_ftp_get_reply(int sock, char *szBuffer)
{

    memset(szBuffer, 0, FTP_BUFFER_SIZE);

    if (ping_ftp_get_line(sock, szBuffer) != FTPOK)
    {
	return FTPERR;
    }

    debug_prz("message =%s\n", szBuffer);

    if (!isdigit(*szBuffer))
	return FTPERR;

    if (*szBuffer == '\0')
	return FTPERR;
    return FTPOK;
}



int ping_ftp_check_msg(int sock, char *szBuffer, int len)
{
    int ret;

    if ((ret =
	 krecv(sock, szBuffer, len, MSG_PEEK, rt.max_ping_wait)) == -1)
    {
	/*  message("Error checking for ftp data-: %s", strerror(errno)); */
	return ret;
    }

    return ret;
}


int ping_ftp_read_msg(int sock, char *szBuffer, int len)
{
    int ret;

    if ((ret = krecv(sock, szBuffer, len, 0, rt.max_ping_wait)) == -1)
    {
	/*   message("Error Receiving ftp  data-: %s", strerror(errno)); */
	return ret;
    }

    return ret;
}

int ping_get_ftp_return(const char *ftp_buffer)
{
    char code[4];

    strncpy(code, ftp_buffer, 3);
    code[3] = '\0';
    return atoi(code);

}
