/* Declarations for HTTP handling.
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

#ifndef HTTP_H
#define HTTP_H

#include "url.h"

/* Some status code validation macros: */
#define H_20X(x)        (((x) >= 200) && ((x) < 300))
#define H_PARTIAL(x)    ((x) == HTTP_PARTIAL_CONTENTS)
#define H_REDIRECTED(x) (((x) == HTTP_MOVED_PERMANENTLY) || ((x) == HTTP_MOVED_TEMPORARILY))

/* HTTP/1.0 status codes from RFC1945, given for reference. */
/* Successful 2xx. */
#define HTTP_OK                200
#define HTTP_CREATED           201
#define HTTP_ACCEPTED          202
#define HTTP_NO_CONTENT        204
#define HTTP_PARTIAL_CONTENTS  206

/* Redirection 3xx. */
#define HTTP_MULTIPLE_CHOICES  300
#define HTTP_MOVED_PERMANENTLY 301
#define HTTP_MOVED_TEMPORARILY 302
#define HTTP_NOT_MODIFIED      304

/* Client error 4xx. */
#define HTTP_BAD_REQUEST       400
#define HTTP_UNAUTHORIZED      401
#define HTTP_FORBIDDEN         403
#define HTTP_NOT_FOUND         404

/* Server errors 5xx. */
#define HTTP_INTERNAL          500
#define HTTP_NOT_IMPLEMENTED   501
#define HTTP_BAD_GATEWAY       502
#define HTTP_UNAVAILABLE       503

/* Typedefs: */
typedef struct {
    long len;			/*
				 * Received length. 
				 */
    long contlen;		/*
				 * Expected length. 
				 */
    long restval;		/*
				 * The restart value. 
				 */
    int res;			/*
				 * The result of last read. 
				 */
    /*
     * added by grendel
     * * -1 = accept ranges not found
     * *  0 = accepts range is none
     * *  1 = accepts ranges
     */
    int accept_ranges;
    char *newloc;		/*
				 * New location (redirection). 
				 */
    char *remote_time;		/*
				 * Remote time-stamp string. 
				 */
    char *error;		/*
				 * Textual HTTP error. 
				 */
    int statcode;		/*
				 * Status code. 
				 */
    long dltime;		/*
				 * Time of the download. 
				 */
} http_stat_t;

uerr_t fetch_next_header(int fd, char **hdr);
int hparsestatline(const char *hdr, const char **rp);
int hskip_lws(const char *hdr);
long hgetlen(const char *hdr);
long hgetrange(const char *hdr);
int hgetaccept_ranges(const char *hdr);
char *hgetlocation(const char *hdr);
char *hgetmodified(const char *hdr);


uerr_t get_http_info(urlinfo * u, http_stat_t * hs);

/* Fetches the header after sending the specified command ie:"GET index.html"*/
uerr_t http_fetch_headers(int sock, urlinfo * u, http_stat_t * hs,
			  char *command);
/*Returns a comlete basic authentification string */
char *get_basic_auth_str(char *user, char *passwd);
#endif
