/* Declarations for URL handling.
   Copyright (C) 1995, 1996, 1997 Free Software Foundation, Inc.
   
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


#ifndef URL_H
#define URL_H

#include "main.h"


/* Structure containing info on a URL. */
typedef struct _urlinfo {
    char *url;			/*
				 * Unchanged URL 
				 */
    uerr_t proto;		/*
				 * URL protocol 
				 */
    char *host;			/*
				 * Extracted hostname 
				 */
    unsigned short port;
    char *path, *dir, *file;	/*
				 * Path, as well as dir and file
				 * (properly decoded) 
				 */
    char *user, *passwd;	/*
				 * For FTP 
				 */
    struct _urlinfo *proxy;	/*
				 * The exact string to pass to proxy
				 * server. 
				 */
    char *referer;		/*
				 * The source from which the request
				 * URI was obtained. 
				 */
    char *local;		/*
				 * The local filename of the URL
				 * document. 
				 */
  char ftp_type;
    /*
     * Does the server support resume? 
     */
    int resume_support;
    long file_size;
} urlinfo;



/* Structure containing info on a protocol. */
  typedef struct proto {
    char *name;
    uerr_t ind;
    unsigned short port;
  } proto_t;

  enum uflags {
    URELATIVE = 0x0001,		/* Is URL relative? */
    UNOPROTO = 0x0002,		/* Is URL without a protocol? */
    UABS2REL = 0x0004,		/* Convert absolute to relative? */
    UREL2ABS = 0x0008		/* Convert relative to absolute? */
  };

/* A structure that defines the whereabouts of a URL, i.e. its
   position in an HTML document, etc. */
  typedef struct _urlpos {
    char *url;			/* URL */
    char *local_name;		/* Local file to which it was saved. */
    enum uflags flags;		/* Various flags. */
    int pos, size;		/* Rekative position in the buffer. */
    struct _urlpos *next;	/* Next struct in list. */
  } urlpos;

  int
   has_proto(const char *url);
  int
   skip_uname(const char *url);
  void
   parse_dir(const char *path, char **dir, char **file);
  void
   path_simplify(char *path);
  char *uri_merge(const char *base, const char *link);
  int
   urlpath_length(const char *url);
  int
   skip_proto(const char *url);
  char *str_url(const urlinfo * u, int hide);
uerr_t parseurl(const char *url, urlinfo * u, int strict);
void freeurl(urlinfo * u, int complete);
#endif		
