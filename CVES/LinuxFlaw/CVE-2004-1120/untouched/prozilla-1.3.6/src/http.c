/* HTTP support.
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
# include <config.h>
#endif				/*
				 * HAVE_CONFIG_H 
				 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <assert.h>
#include "connect.h"
#include "url.h"
#include "misc.h"
#include "main.h"
#include "connection.h"
#include "http.h"
#include "debug.h"
#include "netrc.h"
#include "runtime.h"

#define DYNAMIC_LINE_BUFFER 40

int buf_readchar(int fd, char *ret)
{
    int res;

    res = krecv(fd, ret, 1, 0, rt.timeout);
    if (res <= 0)
	return res;

    return 1;
}

/* This is similar to buf_readchar, only it doesn't move the buffer
   position.  */
int buf_peek(int fd, char *ret)
{
    int res;

    res = krecv(fd, ret, 1, MSG_PEEK, rt.timeout);
    if (res <= 0)
	return res;

    return 1;

}


/* Function to fetch a header from socket/file descriptor fd. The
   header may be of arbitrary length, since the function allocates as
   much memory as necessary for the header to fit. Most errors are
   handled.

   The header may be terminated by LF or CRLF.  If the character after
   LF is SP or HT (horizontal tab), the header spans to another line
   (continuation header), as per RFC2068.

   The trailing CRLF or LF are stripped from the header, and it is
   zero-terminated. */

uerr_t fetch_next_header(int fd, char **hdr)
{
    int i, bufsize, res;
    char next;

    bufsize = DYNAMIC_LINE_BUFFER;
    *hdr = (char *) kmalloc(bufsize);
    for (i = 0; 1; i++)
    {
	if (i > bufsize - 1)
	    *hdr = (char *) realloc(*hdr, (bufsize <<= 1));
	res = buf_readchar(fd, *hdr + i);
	if (res == 1)
	{
	    if ((*hdr)[i] == '\n')
	    {
		if (!(i == 0 || (i == 1 && (*hdr)[0] == '\r')))
		{
		    /*
		     * If the header is non-empty, we need to check if it
		     * continues on to the other line.  We do that by
		     * getting the next character without actually
		     * downloading it (i.e. peeking it).  
		     */
		    res = buf_peek(fd, &next);
		    if (res == 0)
			return HEOF;
		    else if (res == -1)
			return HERR;
		    /*
		     * If the next character is SP or HT, just continue.  
		     */
		    if (next == '\t' || next == ' ')
			continue;
		}
		/*
		 * The header ends.  
		 */
		(*hdr)[i] = '\0';
		/*
		 * Get rid of '\r'. 
		 */
		if (i > 0 && (*hdr)[i - 1] == '\r')
		    (*hdr)[i - 1] = '\0';
		break;
	    }
	} else if (res == 0)
	    return HEOF;
	else
	    return HERR;
    }

    return HOK;
}

int hparsestatline(const char *hdr, const char **rp)
{
    int mjr, mnr;		/*
				 * HTTP major and minor version. 
				 */
    int statcode;		/*
				 * HTTP status code. 
				 */
    const char *p;

    *rp = NULL;
    /*
     * The standard format of HTTP-Version is:
     * HTTP/x.y, where x is major version, and y is minor version. 
     */
    if (strncmp(hdr, "HTTP/", 5) != 0)
	return -1;
    hdr += 5;
    p = hdr;
    for (mjr = 0; isdigit(*hdr); hdr++)
	mjr = 10 * mjr + (*hdr - '0');
    if (*hdr != '.' || p == hdr)
	return -1;
    ++hdr;
    p = hdr;
    for (mnr = 0; isdigit(*hdr); hdr++)
	mnr = 10 * mnr + (*hdr - '0');
    if (*hdr != ' ' || p == hdr)
	return -1;
    /*
     * Wget will accept only 1.0 and higher HTTP-versions. The value
     * of minor version can be safely ignored. 
     */
    if (mjr < 1)
	return -1;
    /*
     * Skip the space. 
     */
    ++hdr;
    if (!(isdigit(*hdr) && isdigit(hdr[1]) && isdigit(hdr[2])))
	return -1;
    statcode = 100 * (*hdr - '0') + 10 * (hdr[1] - '0') + (hdr[2] - '0');
    /*
     * RFC2068 requires a SPC here, even if there is no reason-phrase.
     * As some servers/CGI are (incorrectly) setup to drop the SPC,
     * we'll be liberal and allow the status line to end here.  
     */
    if (hdr[3] != ' ')
    {
	if (!hdr[3])
	    *rp = hdr + 3;
	else
	    return -1;
    } else
	*rp = hdr + 4;
    return statcode;
}


/* Skip LWS (linear white space), if present.  Returns number of
   characters to skip.  */
int hskip_lws(const char *hdr)
{
    int i;

    for (i = 0;
	 *hdr == ' ' || *hdr == '\t' || *hdr == '\r' || *hdr == '\n';
	 ++hdr)
	++i;
    return i;
}

/* Return the content length of the document body, if this is
   Content-length header, -1 otherwise. */
long hgetlen(const char *hdr)
{
    const int l = 15;		/*
				 * strlen("content-length:") 
				 */
    long len;

    if (strncasecmp(hdr, "content-length:", l))
	return -1;
    hdr += (l + hskip_lws(hdr + l));
    if (!*hdr)
	return -1;
    if (!isdigit(*hdr))
	return -1;
    for (len = 0; isdigit(*hdr); hdr++)
	len = 10 * len + (*hdr - '0');
    return len;
}


/* Return the content-range in bytes, as returned by the server, if
   this is Content-range header, -1 otherwise. */
long hgetrange(const char *hdr)
{
    const int l = 14;		/*
				 * strlen("content-range:") 
				 */
    long len;

    if (strncasecmp(hdr, "content-range:", l))
	return -1;
    hdr += (l + hskip_lws(hdr + l));
    if (!*hdr)
	return -1;
    /*
     * Nutscape proxy server sends content-length without "bytes"
     * specifier, which is a breach of HTTP/1.1 draft. But heck, I must
     * support it... 
     */
    if (!strncasecmp(hdr, "bytes", 5))
    {
	hdr += 5;
	hdr += hskip_lws(hdr);
	if (!*hdr)
	    return -1;
    }
    if (!isdigit(*hdr))
	return -1;
    for (len = 0; isdigit(*hdr); hdr++)
	len = 10 * len + (*hdr - '0');
    return len;
}


/* Returns a malloc-ed copy of the location of the document, if the
   string hdr begins with LOCATION_H, or NULL. */
char *hgetlocation(const char *hdr)
{
    const int l = 9;		/*
				 * strlen("location:") 
				 */

    if (strncasecmp(hdr, "location:", l))
	return NULL;
    hdr += (l + hskip_lws(hdr + l));
    return kstrdup(hdr);
}

/* Returns a malloc-ed copy of the last-modified date of the document,
   if the hdr begins with LASTMODIFIED_H. */
char *hgetmodified(const char *hdr)
{
    const int l = 14;		/*
				   * strlen("last-modified:") 
				 */

    if (strncasecmp(hdr, "last-modified:", l))
	return NULL;
    hdr += (l + hskip_lws(hdr + l));
    return kstrdup(hdr);
}

/* Returns 0 if the header is accept-ranges, and it contains the word
   "none", -1 if there is no accept ranges, 1 is there is accept-ranges
   and it is not none  */

int hgetaccept_ranges(const char *hdr)
{
    const int l = 14;		/*
				   * strlen("accept-ranges:") 
				 */

    if (strncasecmp(hdr, "accept-ranges:", l))
	return -1;
    hdr += (l + hskip_lws(hdr + l));
    if (strstr(hdr, "none"))
	return 0;
    else
	return 1;
}



uerr_t get_http_info(urlinfo * u, http_stat_t * hs)
{

    int sock;
    char buffer[HTTP_BUFFER_SIZE];
    char *user, *passwd, *wwwauth, *referer = NULL;
    uerr_t err;
    netrc_entry *netrc_ent;

    err = connect_to_server(&sock, u->host, u->port, rt.timeout);
    if (err != NOCONERROR)
    {
	message("Error connecting to %s", u->host);
	return err;
    }

/* Authentification code*/

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

    user = user ? user : "";
    passwd = passwd ? passwd : "";

    if (strlen(user) || strlen(passwd))
    {
	/*Construct the necessary header */
	wwwauth = get_basic_auth_str(user, passwd);
	message("Authenticating as user %s password %s", user, passwd);

	debug_prz("Authentification string=%s\n", wwwauth);

    } else
	wwwauth = 0;

/* Handle referer */
   if(u->referer)
      {
	referer= (char *) alloca(13+strlen(u->referer));
	sprintf(referer, "Referer: %s\r\n", u->referer);
      }



/* We will get http info about the file by calling http_fetch_headers 
   with HEAD */


    sprintf(buffer,
	    "HEAD %s HTTP/1.0\r\nUser-Agent: %s%s\r\nHost: %s\r\nAccept: */*\r\n%s%s\r\n",
	    u->path, PACKAGE_NAME, VERSION, u->host, 
	    referer ? referer : "",
	    wwwauth ? wwwauth : "");

    debug_prz("HTTP request= %s\n", buffer);

    err = http_fetch_headers(sock, u, hs, buffer);
    close(sock);

    if (wwwauth)
	free(wwwauth);
    /*Check if we authenticated using any user or password and if we 
       were kicked out, if so return HAUTHFAIL */
    if (err == HAUTHREQ && (strlen(user) || strlen(passwd)))
	return HAUTHFAIL;

    return err;
}

/* function to fetch the http headers from the socket */
/* to a specific command string */

uerr_t http_fetch_headers(int sock, urlinfo * u, http_stat_t * hs,
			  char *command)
{
    int num_written, hcount, statcode;
    uerr_t err;
    char *hdr, *type;
    char *all_headers;
    long contlen, contrange;
    int all_length;
    const char *error;

    hs->len = 0L;
    hs->contlen = -1;
    hs->accept_ranges = -1;
    hs->res = -1;
    hs->newloc = NULL;
    hs->remote_time = NULL;
    hs->error = NULL;

    /*
     * send the command to the server 
     */
    num_written = ksend(sock, command, strlen(command), 0, rt.timeout);
    if (num_written != strlen(command))
    {
	message("Failed writing HTTP request");
	return WRITEERR;
    }

    all_headers = NULL;
    all_length = 0;
    contlen = contrange = -1;
    statcode = -1;
    type = NULL;
    /*
     * Header-fetching loop. 
     */
    hcount = 0;

    for (;;)
    {
	++hcount;
	/*
	 * Get the header. 
	 */
	err = fetch_next_header(sock, &hdr);

	debug_prz("Header =%s\n", hdr);

	if (err == HEOF)
	{
	    message("End of file while parsing headers");

	    free(hdr);
	    if (type)
		free(type);
	    if (all_headers)
		free(all_headers);
	    return HEOF;
	}

	else if (err == HERR)
	{
	    message("Read error in headers");

	    free(hdr);
	    if (type)
		free(type);
	    if (all_headers)
		free(all_headers);
	    return HERR;
	}
	/*
	 * Exit on empty header. 
	 */
	if (!*hdr)
	{
	    free(hdr);
	    break;
	}

	/*
	 * print the header for debugging purposes 
	 */

	/*        message( "\n%d %s", hcount, hdr); */

	/* Check for errors documented in the first header. */
	if (hcount == 1)
	{
	    statcode = hparsestatline(hdr, &error);
	    hs->statcode = statcode;
	    /*
	     * Store the descriptive response. 
	     */
	    if (statcode == -1)	/*
				 * malformed request 
				 */
		hs->error = kstrdup("UNKNOWN");
	    else if (!*error)
		hs->error = kstrdup("(no description)");
	    else
		hs->error = kstrdup(error);
	}
	if (contlen == -1)
	{
	    contlen = hgetlen(hdr);
	    u->file_size = hs->contlen = contlen;
	}

	/*
	 * if the server specified a new location then lets store it 
	 */

	if (!hs->newloc)
	    hs->newloc = hgetlocation(hdr);

	if (!hs->remote_time)
	    hs->remote_time = hgetmodified(hdr);

	if (hs->accept_ranges == -1)
	{
	    hs->accept_ranges = hgetaccept_ranges(hdr);
	}

	if (!hs->newloc)
	    hs->newloc = hgetlocation(hdr);
	free(hdr);
    }

    if (H_20X(statcode))
	return HOK;

    if (H_REDIRECTED(statcode) || statcode == HTTP_MULTIPLE_CHOICES)
    {
	/*
	 * RFC2068 says that in case of the 300 (multiple choices)
	 * response, the server can output a preferred URL through
	 * `Location' header; otherwise, the request should be treated
	 * like GET.  So, if the location is set, it will be a
	 * redirection; otherwise, just proceed normally.  
	 */
	if (statcode == HTTP_MULTIPLE_CHOICES && !hs->newloc)
	    return HOK;
	else
	{
	    if (all_headers)
		free(all_headers);
	    if (type)
		free(type);
	    return NEWLOCATION;
	}
    }

    if (statcode == HTTP_UNAUTHORIZED)
    {
	return HAUTHREQ;
    }

    return HERR;
}


/*Routine returns a valid Authorization request in 
  which the username:passwd is in base64 */

char *get_basic_auth_str(char *user, char *passwd)
{
    char *p1, *p2, *ret;
    int len = strlen(user) + strlen(passwd) + 1;
    int b64len = 4 * ((len + 2) / 3);
    char auth_header[] = "Authorization";

    p1 = (char *) kmalloc(sizeof(char) * len + 1);
    sprintf(p1, "%s:%s", user, passwd);
    p2 = (char *) kmalloc(sizeof(char) * b64len + 1);

    /*Encode username:passwd to base 64 */
    base64_encode(p1, p2, len);
    ret =
	(char *) kmalloc((sizeof(char) * strlen(auth_header)) + b64len +
			 11);
    sprintf(ret, "%s: Basic %s\r\n", auth_header, p2);
    free(p1);
    free(p2);

    return ret;
}
