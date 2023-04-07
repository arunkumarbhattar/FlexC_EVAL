/* Declarations for a struct contaning info about the runtime conf settings.
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

#ifndef RUNTIME_H
#define RUNTIME_H

#include <sys/time.h>
#include "netrc.h"

/*
 * enum for the programs running mode 
 */
typedef enum {
    NORMAL,
    RESUME
} big_mode;

/* for which display is running */
typedef enum {
    DISPLAY_CURSES,
    DISPLAY_GTK
} display;

struct runtime {
    int num_connections;
    int max_redirections;
    /*
     * whether to use the netrc file 
     */
    int use_netrc;
    netrc_entry *netrc_list;
    big_mode run_mode;		/*
				 * NORMAL or RESUME 
				 */
    display display_mode;
    int use_pasv;
    int force_mode;
/* Dammit I need a bool to tell me wether the user likes gtk
 * reason: currently gtk is called only in the display routine
 * so if message is using gtk before hand it wouldn't have any effect 
 * TODO :Fix this Soon...*/
#ifdef HAVE_GTK
    int use_gtk_option;
#endif
    /*
     * The start_time 
     */
    struct timeval dl_start_time;
    int try_attempts;
    int retry_delay;		/*delay in seconds */
    /*
     * The timeout period for the connections 
     */
    int timeout;
    /* Whether to die to stdout or wait for a key press */
    int die_to_stdout;
    int *main_argc;
    char ***main_argv;
    /* Indicates that the program is in the curses display loop */
    int in_curses_display_loop;
    /* Whether to output debugging info */
    int debug_mode;
    int ftp_search;
    /* The maximum number of servers to ping at once */
    int max_simul_pings;
    /* The max number of seconds to wait for a server response to ping */
    int max_ping_wait;
    /* The maximum number of servers/mirrors to request */
    int ftps_mirror_req_n;
    int max_bps;
    /* The dir to save the generated file in */
    char *output_dirprefix;
    char *ftpsearch_url;
};

extern struct runtime rt;

#endif
