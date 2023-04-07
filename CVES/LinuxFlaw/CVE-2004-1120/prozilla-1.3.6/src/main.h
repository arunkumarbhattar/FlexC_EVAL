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

#ifndef MAIN_H
#define MAIN_H

#include <pthread.h>

typedef enum { NOCONERROR, HOSTERR, CONSOCKERR, CONERROR,
    CONREFUSED, NEWLOCATION, NOTENOUGHMEM, CONPORTERR,
    BINDERR, BINDOK, LISTENERR, LISTENOK, ACCEPTERR, ACCEPTOK,
    CONCLOSED, FTPOK, FTPLOGINC, FTPLOGREFUSED, FTPPORTERR,
    FTPNSFOD, FTPRETROK, FTPUNKNOWNTYPE, FTPUNKNOWNCMD, FTPSIZEFAIL,
    FTPERR, FTPREXC, FTPSRVERR, FTPRETRINT, FTPRESTFAIL,
    URLOK, URLHTTP, URLFTP, URLFILE, URLUNKNOWN, URLBADPORT,
    URLBADHOST, FOPENERR, FWRITEERR, HOK, HLEXC, HEOF,
    HERR, RETROK, RECLEVELEXC, FTPACCDENIED, WRONGCODE,
    FTPINVPASV, FTPNOPASV, FTPCONREFUSED,
    RETRFINISHED, READERR, TRYLIMEXC, URLBADPATTERN,
    FILEBADFILE, CWDFAIL, RANGEERR, RETRBADPATTERN, RETNOTSUP,
    ROBOTSOK, NOROBOTS, PROXERR, QUOTEXC, WRITEERR, RESTFAILED, FILESZFAIL,
    HACCEPTFAIL, FTPPWDERR, PWDFAIL, FILEISDIR, SERVERCLOSECONERR,
    HAUTHREQ, HAUTHFAIL, MIRINFOK, MIRPARSEOK, MIRPARSEFAIL
} uerr_t;

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef TRUE
#define TRUE    (!FALSE)
#endif

#define PACKAGE_NAME "Prozilla - Download accelerator for Linux"

#define FTP_BUFFER_SIZE 2048
#define HTTP_BUFFER_SIZE 2048
#define MAX_MSG_SIZE 1248
#define DEFAULT_FTP_PORT 21
#define DEFAULT_HTTP_PORT 80
/* This is used when no password is found or specified in */
#define DEFAULT_FTP_USER "anonymous"
#define DEFAULT_FTP_PASSWD "billg@hotmail.com"

/* The D/L ed fragments will be saved to files with this extension
 * ie: gnu.jpg.prz0  gnu.jpg.prz1 etc..
 */
#define DEFAULT_FILE_EXT ".prz"

/*the extension for the log file created */
#define DEFAULT_LOG_EXT ".log"

/*definnitions about the ncurse colors that the app will use*/
enum {
    NULL_PAIR = 0,
    HIGHLIGHT_PAIR,
    MSG_PAIR,
    WARN_PAIR,
    PROMPT_PAIR,
    SELECTED_PAIR
};

typedef enum {
    AUTO = 0,
    DONE,
    USER_SELECTED,
    USER_ABORTED,
    USER_NEXT_SERV,
    USER_PREV_SERV
} interface_ret;


/* function to call is program wants to abort, it differs from the quit
 * function because this waits for a key press from the user before terminating*/
int die(const char *args, ...);

/* the nextfunc is quite self explanatory, isn't it ;), it doesn't wait 
 * for a keypress from the user at the end
 */
void quit(const char *args, ...);

int query_overwrite_target(char *fname);

/* A important function which the interface routines call*/
/* This function will handle the state of the threads while*/
/*they are doing the work */
void handle_threads(void);
/* terminate_threads: terminates the running threads
 */
void terminate_threads(pthread_t * threads, int num_threads);

/*this function will call the appropriate routine*/
/*for the display ie gtk or curses*/
void message(const char *args, ...);

#endif
