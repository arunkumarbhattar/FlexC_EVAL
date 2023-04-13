/*
   FTP support. Copyright (C) 2000 Kalum Somaratna 
  
   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the Free 
   Software Foundation; either version 2 of the License, or (at your option)
   any later version.
  
   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.
   
   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   675 Mass Ave, Cambridge, MA 02139, USA.  
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif				/*
				 * HAVE_CONFIG_H 
				 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <signal.h>
#include <setjmp.h>
#include <assert.h>
#include "main.h"
#include "connect.h"
#include "misc.h"
#include "url.h"
#include "ftp.h"
#include "netrc.h"
#include "runtime.h"
#include "debug.h"

#define UNIMPLEMENTED_CMD(a)          ((a == 500) || (a == 502) || (a == 504))

/*
 * return the numeric response of the ftp server by reading the first 3
 * characters in the buffer 
 */
int get_ftp_return(const char *ftp_buffer)
{
    char code[4];

    strncpy(code, ftp_buffer, 3);
    code[3] = '\0';
    return atoi(code);

}

int ftp_check_msg(int sock, char *szBuffer, int len)
{
    int ret;

    if ((ret = krecv(sock, szBuffer, len, MSG_PEEK, rt.timeout)) == -1)
    {
	message("Error checking for ftp data-: %s", strerror(errno));
	return ret;
    }

    return ret;
}


int ftp_read_msg(int sock, char *szBuffer, int len)
{
    int ret;

    if ((ret = krecv(sock, szBuffer, len, 0, rt.timeout)) == -1)
    {
	message("Error Receiving ftp  data-: %s", strerror(errno));
	return ret;
    }

    return ret;
}

uerr_t ftp_send_msg(int sock, char *szBuffer, int len)
{
    if (ksend(sock, szBuffer, len, 0, rt.timeout) == -1)
    {
	message("Error Sending ftp  data-: %s", strerror(errno));
	return FTPERR;
    }

    return FTPOK;
}


uerr_t ftp_get_line(int control_sock, char *buffer)
{
    int iLen, iBuffLen = 0, ret = 0;
    char *szPtr = buffer, ch = 0;

    while ((iBuffLen < FTP_BUFFER_SIZE) &&
	   ((ret = ftp_check_msg(control_sock, &ch, 1)) > 0))
    {
	iLen = ftp_read_msg(control_sock, &ch, 1);
	if (iLen != 1)
	{
	    return FTPERR;
	}
	iBuffLen += iLen;
	*szPtr = ch;
	szPtr += iLen;
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


uerr_t ftp_get_reply(int sock, char *szBuffer)
{
    int cont = 0;
    int code;
    char * tmp = (char *)alloca(FTP_BUFFER_SIZE);

    memset(szBuffer, 0, FTP_BUFFER_SIZE);

    if (ftp_get_line(sock, szBuffer) != FTPOK)
    {
	return FTPERR;
    }

    debug_prz("message =%s\n", szBuffer);

    if (!isdigit(*szBuffer))
	return FTPERR;

    if (*szBuffer == '\0')
	return FTPERR;

    code = get_ftp_return(szBuffer);

    if (szBuffer[3] == '-')
	cont = 1;
    else
	cont = 0;

    (void) strtok_r(szBuffer, "\r\n",&tmp);

    while (cont)
    {
	if (ftp_get_line(sock, szBuffer) != FTPOK)
	{
	    return FTPERR;
	}
	debug_prz("message =%s\n", szBuffer);

	/* Server closed the connection */
	if (*szBuffer == '\0')
	    return FTPERR;

	if ((get_ftp_return(szBuffer) == code) && (szBuffer[3] == ' '))
	    cont = 0;

	(void) strtok_r(szBuffer, "\r\n", &tmp);
    }
    return FTPOK;
}


uerr_t ftp_ascii(int sock)
{
    char szBuffer[FTP_BUFFER_SIZE];
    uerr_t err;

    sprintf(szBuffer, "TYPE A\r\n");

    err = ftp_send_msg(sock, szBuffer, strlen(szBuffer));
    if (err != FTPOK)
    {
	return err;
    }

    err = ftp_get_reply(sock, szBuffer);

    if (err != FTPOK)
    {
	return err;
    }

    if (*szBuffer != '2')
    {
	return FTPUNKNOWNTYPE;
    }

    return FTPOK;
}

uerr_t ftp_binary(int sock)
{
    char szBuffer[FTP_BUFFER_SIZE];
    uerr_t err;

    sprintf(szBuffer, "TYPE I\r\n");
    err = ftp_send_msg(sock, szBuffer, strlen(szBuffer));
    if (err != FTPOK)
    {
	return err;
    }

    err = ftp_get_reply(sock, szBuffer);
    if (err != FTPOK)
    {
	return err;
    }

    if (*szBuffer != '2')
    {
	return FTPUNKNOWNTYPE;
    }

    return FTPOK;
}


uerr_t ftp_port(int sock, char *command)
{
    char szBuffer[FTP_BUFFER_SIZE];
    uerr_t err;

    strcpy(szBuffer, command);
    err = ftp_send_msg(sock, szBuffer, strlen(szBuffer));
    if (err != FTPOK)
    {
	return err;
    }

    err = ftp_get_reply(sock, szBuffer);

    if (err != FTPOK)
    {
	return err;
    }

    if (*szBuffer != '2')
    {
	return FTPPORTERR;
    }

    return FTPOK;

}


uerr_t ftp_list(int sock, char *file)
{
    char szBuffer[FTP_BUFFER_SIZE];
    uerr_t err;

    sprintf(szBuffer, "LIST %s\r\n", file);
    err = ftp_send_msg(sock, szBuffer, strlen(szBuffer));
    if (err != FTPOK)
    {
	return err;
    }

    err = ftp_get_reply(sock, szBuffer);

    if (err != FTPOK)
    {
	return err;
    }

    if (*szBuffer == '5')
    {
	return FTPNSFOD;
    }

    if (*szBuffer != '1')
    {
	return FTPERR;
    }
    return FTPOK;
}

uerr_t ftp_retr(int sock, char *file)
{
    char szBuffer[FTP_BUFFER_SIZE];
    uerr_t err;

    sprintf(szBuffer, "RETR %s\r\n", file);
    err = ftp_send_msg(sock, szBuffer, strlen(szBuffer));
    if (err != FTPOK)
    {
	return err;
    }

    err = ftp_get_reply(sock, szBuffer);

    if (err != FTPOK)
    {
	return err;
    }

    if (*szBuffer == '5')
    {
	return FTPNSFOD;
    }

    if (*szBuffer != '1')
    {
	return FTPERR;
    }
    return FTPOK;

}

/*
 * does the PASV command 
 */

uerr_t ftp_pasv(int sock, unsigned char *addr)
{
    char szBuffer[FTP_BUFFER_SIZE];
    uerr_t err;
    unsigned char *p;
    int i;

    sprintf(szBuffer, "PASV\r\n");
    err = ftp_send_msg(sock, szBuffer, strlen(szBuffer));
    if (err != FTPOK)
    {
	return err;
    }

    err = ftp_get_reply(sock, szBuffer);

    debug_prz("FTP PASV Header =%s\n", szBuffer);

    if (err != FTPOK)
    {
	return err;
    }
    if (*szBuffer != '2')
    {
	return FTPNOPASV;
    }
    /*
     * parse it 
     */
    p = (unsigned char *) szBuffer;
    for (p += 4; *p && !isdigit(*p); p++);
    if (!*p)
	return FTPINVPASV;

    for (i = 0; i < 6; i++)
    {
	addr[i] = 0;
	for (; isdigit(*p); p++)
	    addr[i] = (*p - '0') + 10 * addr[i];
	if (*p == ',')
	    p++;
	else if (i < 5)
	{
	    return FTPINVPASV;
	}
    }
    return FTPOK;
}

uerr_t ftp_rest(int sock, long bytes)
{
    char szBuffer[FTP_BUFFER_SIZE];
    uerr_t err;

    sprintf(szBuffer, "REST %ld\r\n", bytes);
    err = ftp_send_msg(sock, szBuffer, strlen(szBuffer));
    if (err != FTPOK)
    {
	return err;
    }
    err = (ftp_get_reply(sock, szBuffer));
    if (err != FTPOK)
    {
	return err;
    }
    if (*szBuffer != '3')
    {

	return FTPRESTFAIL;
    }

    return FTPOK;
}

/*
 * CWD command 
 */
uerr_t ftp_cwd(int sock, char *dir)
{
    char szBuffer[FTP_BUFFER_SIZE];
    uerr_t err;

    sprintf(szBuffer, "CWD %s\r\n", dir);
    err = ftp_send_msg(sock, szBuffer, strlen(szBuffer));
    if (err != FTPOK)
    {
	return err;
    }

    err = (ftp_get_reply(sock, szBuffer));
    if (err != FTPOK)
    {
	return err;
    }


    /*
     * check and see wether the CWD succeeded 
     */
    if (*szBuffer == '5')
    {
	return FTPNSFOD;
    }

    if (*szBuffer != '2')
    {
	return CWDFAIL;
    }

    /*
     * all ok 
     */
    return FTPOK;
}


/*
 *  Returns the Current working directory in dir 
 */
uerr_t ftp_pwd(int sock, char *dir)
{
    char szBuffer[FTP_BUFFER_SIZE];
    uerr_t err;
    char *r, *l;

    sprintf(szBuffer, "PWD");
    err = ftp_send_msg(sock, szBuffer, strlen(szBuffer));
    if (err != FTPOK)
    {
	return err;
    }

    err = (ftp_get_reply(sock, szBuffer));
    if (err != FTPOK)
    {
	return err;
    }

    /*
     * check and see wether the PWD succeeded 
     */
    if (*szBuffer == '5')
    {
	return FTPPWDERR;
    }

    if (*szBuffer != '2')
    {
	return PWDFAIL;
    }

    if ((r = strrchr(szBuffer, '"')) != NULL)
    {
	l = strchr(szBuffer, '"');
	if ((l != NULL) && (l != r))
	{
	    *r = '\0';
	    ++l;
	    strcpy(dir, l);
	    *r = '"';
	}
    } else
    {
	if ((r = strchr(szBuffer, ' ')) != NULL)
	{
	    *r = '\0';
	    strcpy(dir, szBuffer);
	    *r = ' ';
	}
    }

    /*
     * all ok 
     */
    return FTPOK;
}


/*
 * connects to the ftp server 
 */
uerr_t ftp_connect_to_server(int *sock, char *name, int port)
{
    char szBuffer[FTP_BUFFER_SIZE];
    uerr_t err;

    err = connect_to_server(sock, name, port, rt.timeout);
    if (err != NOCONERROR)
    {
	return err;
    }

    debug_prz("Entering ftp_get_reply\n");

    err = ftp_get_reply(*sock, szBuffer);
    if (err != FTPOK)
    {
	return err;
    }

    debug_prz("Exiting ftp_get_reply\n");

    if (*szBuffer != '2')
    {
	return FTPCONREFUSED;
    }

    return FTPOK;
}


/*
 * this function will call bind to return a bound socket then the  ftp server 
 * will be connected with a port request and asked to connect 
 */

uerr_t ftp_get_listen_socket(int control_sock, int *listen_sock)
{

    /*
     * get a fixed value 
     */
    char command[MAX_MSG_SIZE];
    int sockfd;
    socklen_t len;
    struct sockaddr_in TempAddr;
    char *port, *ipaddr;
    struct sockaddr_in serv_addr;
    uerr_t err;
    /*
     * call bind to bind the sock 
     */


    if (bind_socket(&sockfd) != BINDOK)
    {
	return LISTENERR;
    }


    len = sizeof(serv_addr);
    if (getsockname(sockfd, (struct sockaddr *) &serv_addr, &len) < 0)
    {
	perror("getsockname");
	close(sockfd);
	return CONPORTERR;
    }


    /*
     * now get hosts info 
     */
    len = sizeof(TempAddr);

    if (getsockname(control_sock, (struct sockaddr *) &TempAddr, &len) < 0)
    {
	perror("getsockname");
	close(sockfd);
	return CONPORTERR;
    }

    ipaddr = (char *) &TempAddr.sin_addr;

    /*
     * the port we obtained 
     */

    port = (char *) &serv_addr.sin_port;

#define  UC(b)  (((int)b)&0xff)

    sprintf(command, "PORT %d,%d,%d,%d,%d,%d\r\n",
	    UC(ipaddr[0]), UC(ipaddr[1]), UC(ipaddr[2]), UC(ipaddr[3]),
	    UC(port[0]), UC(port[1]));

    /*
     * handle error 
     */

    err = ftp_port(control_sock, command);
    if (err != FTPOK)
    {
	return err;
    }
    *listen_sock = sockfd;
    return FTPOK;
}



uerr_t ftp_login(int control_sock, const char *username,
		 const char *passwd)
{
    uerr_t err;
    char szBuffer[FTP_BUFFER_SIZE];
    sprintf(szBuffer, "USER %s\r\n", username);

    err = ftp_send_msg(control_sock, szBuffer, strlen(szBuffer));
    if (err != FTPOK)
    {
	return err;
    }

    err = ftp_get_reply(control_sock, szBuffer);
    if (err != FTPOK)
    {
	return err;
    }

    /*
     * An unprobable possibility of logging without a password. 
     */
    if (*szBuffer == '2')
    {
	return FTPOK;
    }
    /*
     * Else, only response 3 is appropriate. 
     */
    if (*szBuffer != '3')
    {
	return FTPLOGREFUSED;
    }

    /*
     * send password 
     */

    sprintf(szBuffer, "PASS %s\r\n", passwd);
    err = ftp_send_msg(control_sock, szBuffer, strlen(szBuffer));

    if (err != FTPOK)
    {
	return err;
    }

    err = ftp_get_reply(control_sock, szBuffer);

    if (err != FTPOK)
    {
	return err;
    }

    if (*szBuffer != '2')
    {
	return FTPLOGREFUSED;
    }
    /*
     * looks like everything is ok 
     */
    return FTPOK;
}



/*
 * Gets info about the file from the ftp server
 */
uerr_t ftp_get_file_info(int control_sock, char *fname, ftp_stat_t * fs)
{
    char szBuffer[FTP_BUFFER_SIZE];
    char *tmp;
    unsigned char pasv_addr[6];	/*
				 * for PASV 
				 */
    int listen_sock;
    /*
     * this will indicate whether we are using PASV or PORT 
     */
    int passive_mode = FALSE;
    int data_sock;
    uerr_t err;

    /*
     * did we get a filename?
     */
    if (!fname || !(*fname))
    {
	message("No file specified.\n");
	return -1;
    }

    /*
     * Lets see whether it really is a file 
     */
    err = ftp_cwd(control_sock, fname);
    if (err == FTPOK)
    {
	/*
	 * So fname  is a directory and not a file
	 */
	fs->fp.flagtrycwd = 1;
	fs->fp.flagtryretr = 0;
	return FTPOK;
    } else
	fs->fp.flagtrycwd = 0;	/*
				 * can't be a directory 
				 */

    /*
     * Hmmm.. so fname may be a file or may not exist atl all 
     */


    /*
     * It has allways been hard to get the file size in FTP * lets try the
     * SIZE command and if the server doesn't support it * we fall back on
     * the LIST command..... 
     */
    sprintf(szBuffer, "SIZE %s\r\n", fname);
    err = ftp_send_msg(control_sock, szBuffer, strlen(szBuffer));
    if (err != FTPOK)
    {
	return err;
    }

    err = ftp_get_reply(control_sock, szBuffer);
    if (err != FTPOK)
    {
	return err;
    }

    /*
     * Ok lets see what hapenned..
     */
    if (*szBuffer == '2')	/*
				 * everything ok... 
				 */
    {
	sscanf(szBuffer + 3, "%ld", &(fs->fp.size));
	fs->fp.flagtryretr = 1;
	return FTPOK;
    } else if (*szBuffer == '5')  /** A error hapenned */
    {
	/*
	 * is it due to the file being not found....
	 */
	if (strstr(szBuffer, "o such file")
	    || strstr(szBuffer, "o Such File")
	    || strstr(szBuffer, "ot found")
	    || strstr(szBuffer, "ot Found"))
	    return FTPNSFOD;
    }

    /*
     * The server did not support the SIZE command, *  so lets try to get the 
     * file size through the LIST command 
     */

    /*
     * If enabled lets try PASV 
     */
    if (rt.use_pasv == TRUE)
    {
	err = ftp_pasv(control_sock, pasv_addr);
	/*
	 * If the error is due to the server not supporting PASV then * set the
	 * flag and lets try PORT 
	 */
	if (err == FTPNOPASV || err == FTPINVPASV)
	{
	    message("Server doesn't seem to support PASV");
	    passive_mode = FALSE;
	} else if (err == FTPOK)	/*
					 * server supports PASV 
					 */
	{
	    char dhost[256];
	    unsigned short dport;
	    sprintf(dhost, "%d.%d.%d.%d",
		    pasv_addr[0], pasv_addr[1], pasv_addr[2],
		    pasv_addr[3]);
	    dport = (pasv_addr[4] << 8) + pasv_addr[5];

	    err = connect_to_server(&data_sock, dhost, dport, rt.timeout);
	    if (err != NOCONERROR)
	    {
		return err;
	    }
	    /*
	     * Everything seems to be ok 
	     */
	    passive_mode = TRUE;
	} else
	{
	    return err;
	}
    } else
    {
	/*
	 * Ok..since PASV is not to be used... 
	 */
	passive_mode = FALSE;
    }

    if (passive_mode == FALSE)	/*
				 * try to use PORT instead 
				 */
    {
	/*
	 * obtain a listen socket
	 */
	err = ftp_get_listen_socket(control_sock, &listen_sock);
	if (err != FTPOK)
	{
	    return err;
	}
    }

    err = ftp_ascii(control_sock);
    if (err != FTPOK)
    {
	return err;
    }

    err = ftp_list(control_sock, fname);
    if (err != FTPOK)
    {
	return err;
    }

    if (passive_mode == FALSE)	/*
				 * Then we have to accept the connection 
				 */
    {
	err = accept_connection(listen_sock, &data_sock);
	if (err != ACCEPTOK)
	{
	    return err;
	}
    }

    /*
     * Clear the data buffer
     */
    memset(szBuffer, 0, sizeof(szBuffer));

    if (ftp_read_msg(data_sock, szBuffer, FTP_BUFFER_SIZE) == -1)
	return FTPERR;

    while ((tmp = strrchr(szBuffer, '\n'))
	   || (tmp = strrchr(szBuffer, '\r')))
    {
	*tmp = 0;
    };

    /*
     * close the data_socket  
     */
    close(data_sock);

    if (ftpparse(&(fs->fp), szBuffer, strlen(szBuffer)) == 1)
	return FTPOK;
    else
	return FTPNSFOD;
}


/* connects to the server and checks wether the file exist  
 * it also checks wether REST is supported 
 * and if everything is OK it stores the length of the file in *len 
 */

uerr_t ftp_get_info(urlinfo * u, ftp_stat_t * fs)
{
    int control_sock;
    uerr_t err;
    char *user, *passwd;
    netrc_entry *netrc_ent;

    memset(fs, 0, sizeof(ftp_stat_t));

    err = ftp_connect_to_server(&control_sock, u->host, u->port);
    if (err != FTPOK)
    {
	message("Error connecting to %s", u->host);
	return err;
    }

    message("connect ok");

    user = u->user;
    passwd = u->passwd;

    /*
     * Use .netrc if asked to do so. 
     */
    if (rt.use_netrc == TRUE)
    {
	netrc_ent = search_netrc(rt.netrc_list, u->host);
	if (netrc_ent != NULL)
	{
	    user = netrc_ent->account;
	    passwd = netrc_ent->password;
	}
    }

    user = user ? user : DEFAULT_FTP_USER;
    passwd = passwd ? passwd : DEFAULT_FTP_PASSWD;
    message("Logging in as user %s password %s", user, passwd);

    err = ftp_login(control_sock, user, passwd);
    if (err != FTPOK)
    {
	message("login failed");
	close(control_sock);
	return err;
    } else
	message("login ok");

    err = ftp_binary(control_sock);
    if (err != FTPOK)
    {
	message("binary failed");
	close(control_sock);
	return err;
    } else
	message("Type set to binary ok");

    /*
     * De we need to CWD 
     */
    if (*u->dir)
    {
	err = ftp_cwd(control_sock, u->dir);
	if (err != FTPOK)
	{
	    message("CWD failed to change to directory '%s'", u->dir);
	    close(control_sock);
	    return err;
	} else
	    message("CWD ok");
    } else
    {
	message("CWD not needed");
    }

    err = ftp_rest(control_sock, 0);
    if (err != FTPOK)
    {
	u->resume_support = FALSE;
	message("REST failed");
    } else
    {
	u->resume_support = TRUE;
	message("REST ok");
    }

    err = ftp_get_file_info(control_sock, u->file, fs);

    if (err != FTPOK)
    {
	message("Get file info failed");
	close(control_sock);
	return err;
    }

    u->file_size = fs->fp.size;
    close(control_sock);
    return FTPOK;
}
