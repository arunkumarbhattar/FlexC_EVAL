/* The  file contins routines for managing curses 
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

#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#else
#include <curses.h>
#endif				/*
				 * HAVE_CURSES 
				 */

#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include "interface.h"
#include "misc.h"
#include "runtime.h"
#include "logfile.h"
#include "ftpsearch.h"
#include "debug.h"

/* This is used to convert the connection.status enums to a user
 * friendly text message */
/* the enums are defined in connection.h and are as belor
 *typedef enum {
 *    IDLE = 0,
 *    CONNECTING,
 *    LOGGININ,
 *    DOWNLOADING,
 *    COMPLETED,
 *    LOGINFAIL,
 *    CONREJECTED,
 *    REMOTEFATAL,
 *    LOCALFATAL
 *} dl_status;
 *
 * And here are the texts for the above enums.
*/
const char *dl_status_txt[] = {
    "Idle",
    "Connecting",
    "Logging in",
    "Downloading",
    "Completed",
/* I have decided to change the login failed message 
 * to something else like "login rejected" because people
 * might get alarmed and abort the download rather than 
 * letting prozilla handle it */
    "Login Denied",
    "Connect Refused",
    "Remote Fatal",
    "Local Fatal",
    "Timed Out",
    "Max attempts reached",
};

#define CTRL(x) ((x) & 0x1F)

void curses_exit_resume(connection_data * connections,
			int num_connections);
void curses_exit_no_resume(connection_data * connections,
			   int num_connections);
void curses_draw_display(const connection_data * connections,
			 int num_connections, ftp_mirror * mirrors,
			 int num_servers);
void curses_display_est_time(int row, int col, long bytes_left,
			     float current_speed);

static int top_con = 0;		/*
				 * the connection that is on the top of the display 
				 */
#define MAX_CON_VIEWS 4

/*
 * needed for the message(...) function 
 */
pthread_mutex_t curses_msg_mutex = PTHREAD_MUTEX_INITIALIZER;
char message_buffer[MAX_MSG_SIZE];

/* Added for current_dl_speed */
time_t time_stamp;
long bytes_got_last_time;
int start = 1;
float current_dl_speed = 0.0;

void
curses_draw_display(const connection_data * connections,
		    int num_connections, ftp_mirror * mirrors,
		    int num_servers)
{
    int i, j;
    long total_bytes_got = 0;
    long file_size;
    /*
     * To get a accurate estimate of the D/L time when resuming,
     * * we need to know the number of bytes that have been already
     * * been stored to the disk in the earlier D/L session. 
     * * We will store it in variable below 
     */
    long prev_total_bytes_got = 0;
    struct timeval cur_time;
    float total_speed = 0.0;

    file_size = connections[0].main_file_size;

    attrset(COLOR_PAIR(HIGHLIGHT_PAIR) | A_BOLD);
    mvaddstr(0, 0,
	     "Connection           Status                   Received");
    attrset(COLOR_PAIR(NULL_PAIR));

    for (i = 0; i < MAX_CON_VIEWS; i++)
    {
	move(i + 1, 0);
	clrtoeol();
	if (i + top_con < num_connections)
	{
	    if (connections[top_con + i].main_file_size != -1)
	    {
		float bytes_pp =
		    (float) (connections[top_con + i].local_startpos +
			     connections[top_con + i].remote_endpos -
			     connections[top_con +
					 i].remote_startpos) / 1024;
		mvprintw(i + 1, 0,
			 "    %2d          %15s        %10.1fK of %5.1fK",
			 top_con + i + 1,
			 dl_status_txt[connections[top_con + i].status],
			 (float) (connections[top_con + i].
				  remote_bytes_received +
				  connections[top_con +
					      i].orig_local_startpos) /
			 1024, (bytes_pp < 0) ? 0 : bytes_pp);
	    } else
	    {
		mvprintw(i + 1, 0, "    %d          %15s        %15.1fK",
			 top_con + i + 1,
			 dl_status_txt[connections[top_con + i].status],
			 (float) (connections[top_con + i].
				  remote_bytes_received +
				  connections[top_con +
					      i].orig_local_startpos) /
			 1024);
	    }

	}
    }

    for (j = 0; j < num_connections; j++)
    {
	/*
	 * If even 1 connection has started DLing then set the start time
	 */
	if ((rt.dl_start_time.tv_sec == 0 && rt.dl_start_time.tv_usec == 0)
	    && connections[j].status == DOWNLOADING)
	    gettimeofday(&rt.dl_start_time, NULL);

	total_bytes_got += connections[j].remote_bytes_received;
	prev_total_bytes_got += connections[j].orig_local_startpos;
    }

/*   debug_prz("prev_total= %d", prev_total_bytes_got); */

    gettimeofday(&cur_time, NULL);


    total_speed = (total_bytes_got) /
	((cur_time.tv_sec - rt.dl_start_time.tv_sec) >
	 0 ? (cur_time.tv_sec - rt.dl_start_time.tv_sec) : 1);


    /* FIXME: Initialize this somewhere else, e.g. main.c or init.c
     * I don't want to change more than one file for now */

    /* this is only fulfilled the very first time
     * after starting prozilla */
    if (start == 1)
    {
	time_stamp = time(NULL);
	bytes_got_last_time = total_bytes_got;
	start = 0;
    }
    /* FIXME_END */

    if (time_stamp + 1 == time(NULL))
    {
	current_dl_speed = total_bytes_got - bytes_got_last_time;
	time_stamp = time(NULL);
	bytes_got_last_time = total_bytes_got;
    }


    /*
     * message("%ld bytes received\r",total_bytes_got); 
     */
    attrset(COLOR_PAIR(HIGHLIGHT_PAIR) | A_BOLD);
    mvprintw(6, 0, "%s", connections[0].u.url);
    attrset(COLOR_PAIR(HIGHLIGHT_PAIR));

    if (file_size == -1)
    {
	mvprintw(8, 0, "File Size = UNKNOWN");
	mvprintw(9, 0, "Total bytes received = %.1fK",
		 (float) (total_bytes_got + prev_total_bytes_got) / 1024);
    } else
    {
	mvprintw(8, 0, "File Size = %ldK", file_size / 1024);
	mvprintw(9, 0, "Total bytes received = %.1fK (%.2f%%)",
		 (float) (total_bytes_got + prev_total_bytes_got) / 1024,
		 (file_size >
		  0) ? (float) (total_bytes_got +
				prev_total_bytes_got) * 100 /
		 (float) file_size : 0);
    }

    /*
     * Added current speed display
     */
    move(10, 0);
    clrtoeol();
    printw("Current speed = %1.2fKb/s, Average D/L speed = %1.2fKb/s",
	   current_dl_speed / 1024, total_speed / 1024);

    /*
     * Added estimated time display 
     */
    curses_display_est_time(11, 0,
			    file_size - (total_bytes_got +
					 prev_total_bytes_got),
			    total_speed);

    attrset(COLOR_PAIR(NULL_PAIR));

    /*
     * Does the server support resume 
     */
    if (connections[0].u.resume_support)
    {
	attrset(COLOR_PAIR(HIGHLIGHT_PAIR) | A_BOLD);
	mvprintw(13, 0, "Resume Supported");
	attrset(COLOR_PAIR(NULL_PAIR));
	if(num_connections>MAX_CON_VIEWS)
	  {
	    mvprintw(14, 0, "Up and Down Arrow keys to scroll display");
	  }
	mvprintw(15, 0, "Ctrl-R to exit and resume later");
	mvprintw(16, 0, "Ctrl-X to exit without resuming later");
	mvprintw(17, 0, "Ctrl-L to repaint the screen");

	if (rt.ftp_search == TRUE && num_servers > 1)
	{
	    int cur_server =
		get_mirror_list_pos(connections[0].u.url, mirrors,
				    num_servers);

	    move(18, 0);
	    clrtoeol();
	    if (cur_server > 0 && cur_server < num_servers - 1)
		mvprintw(18, 0,
			 "Ctrl-N to try the next server or CTRL-P for previous one");
	    else if (cur_server == 0)
		mvprintw(18, 0, "Ctrl-N to try the next server");
	    else if (cur_server == num_servers - 1)
		mvprintw(18, 0, "CTRL-P for previous one");
	}
    } else
    {
	attrset(COLOR_PAIR(WARN_PAIR) | A_BOLD);
	mvprintw(13, 0, "Resume NOT Supported");
	attrset(COLOR_PAIR(NULL_PAIR));
	if(num_connections>MAX_CON_VIEWS)
	  {
	    mvprintw(15, 0, "Up and Down Arrow keys to scroll display");
	  }
	mvprintw(16, 0, "Ctrl-X to exit without resuming later");
	mvprintw(17, 0, "Ctrl-L to repaint the screen");

	if (rt.ftp_search == TRUE && num_servers > 1)
	{
	    int cur_server =
		get_mirror_list_pos(connections[0].u.url, mirrors,
				    num_servers);
	    move(18, 0);
	    clrtoeol();
	    if (cur_server > 0 && cur_server < num_servers - 1)
		mvprintw(18, 0,
			 "Ctrl-N to try the next server or CTRL-P for previous one");
	    else if (cur_server == 0)
		mvprintw(18, 0, "Ctrl-N to try the next server");
	    else if (cur_server == num_servers - 1)
		mvprintw(18, 0, "CTRL-P for previous one");
	}
    }

    /* Display whats in the message buffer */
    /* Lock the mutex */
    pthread_mutex_lock(&curses_msg_mutex);
    move(19, 0);
    clrtoeol();
    move(20, 0);
    clrtoeol();
    move(21, 0);
    clrtoeol();
    attrset(COLOR_PAIR(MSG_PAIR) | A_BOLD);
    mvprintw(19, 0, "%s", message_buffer);
    /* Unlock the mutex */
    pthread_mutex_unlock(&curses_msg_mutex);
}

interface_ret
curses_do_interface(connection_data * connections,
		    int num_connections, ftp_mirror * mirrors,
		    int num_servers)
{

    int cur_server;
    extern pthread_t *threads;


    while (1)
    {
	handle_threads();
	/*
	 * display the URL
	 */
	napms(100);
	curses_draw_display(connections, num_connections, mirrors,
			    num_servers);

	refresh();

	switch (getch())
	{
	case CTRL('R'):
	    /*
	     * if connections are ==1 then resume isn't suppported 
	     */
	    if (connections[0].u.resume_support)
	    {
		curses_exit_resume(connections, num_connections);
	    }
	    break;
	case CTRL('X'):
	    curses_exit_no_resume(connections, num_connections);
	    break;

	case CTRL('N'):
	    if (rt.ftp_search == TRUE && num_servers > 1)
	    {
		cur_server =
		    get_mirror_list_pos(connections[0].u.url, mirrors,
					num_servers);
		debug_prz("cur_server %d", cur_server);


		if ((cur_server >= 0 && cur_server < num_servers - 1)
		    && !(cur_server == num_servers - 1))
		{
		    terminate_threads(threads, num_connections);
		    return USER_NEXT_SERV;
		}
	    }
	    break;

	case CTRL('P'):
	    if (rt.ftp_search == TRUE && num_servers > 1)
	    {
		cur_server =
		    get_mirror_list_pos(connections[0].u.url, mirrors,
					num_servers);
		debug_prz("cur_server %d", cur_server);

		if ((cur_server > 0 && cur_server < num_servers - 1)
		    && !(cur_server == 0))
		{
		    terminate_threads(threads, num_connections);
		    return USER_PREV_SERV;
		}
	    }
	    break;

	case CTRL('L'):	/*
				   * Repaint the screen 
				 */
	    /*
	     * force a complete clear of the screen 
	     */
	    clear();
	    refresh();
	    /*
	     * now redraw it 
	     */
	    curses_draw_display(connections, num_connections, mirrors,
				num_servers);
	    refresh();
	    break;
	case KEY_DOWN:
	    if (num_connections <= 4)	/* no need to scroll */
		break;
	    if (top_con < num_connections - 4)	/* more than 4 connections */
		top_con++;
	    break;
	case KEY_UP:
	    if (top_con > 0)
		top_con--;
	    break;
	default:
	    break;
	}

	if (all_dls_complete(connections, num_connections) == TRUE)
	{
	    /*
	     * redraw the display again 
	     */
	    /*
	     * Delay a bit to get the proper value
	     */
	    napms(300);
	    curses_draw_display(connections, num_connections, mirrors,
				num_servers);
	    refresh();
	    break;
	}
    }

    return DONE;
}

void curses_exit_resume(connection_data * connections, int num_connections)
{
    /*
     * Ugly 
     */
    extern pthread_t *threads;

    /* indicate that we are out of the display loop */
    rt.in_curses_display_loop = FALSE;

    message("Aborting...");
    terminate_threads(threads, num_connections);
    quit("Ok..please resume later with the -r switch");
}

void
curses_exit_no_resume(connection_data * connections, int num_connections)
{
    /*
     * Ugly 
     */
    extern pthread_t *threads;

    /* indicate that we are out of the display loop */
    rt.in_curses_display_loop = FALSE;

    terminate_threads(threads, num_connections);

    if (delete_downloads(connections, num_connections) == 1)
    {
	message("Cleaned up the file parts");

    } else
    {
	message("unable to cleanup the downloaded file parts");
    }

    if (log_delete_logfile(&connections[0].u) != 0)
    {
	die("Error while deleting the logfile: %s", strerror(errno));
    }

    quit("Aborted...ok");
}




/* because of different args for different ncurses, I had to write these 
 * my self 
 */

#define kwattr_get(win,a,p,opts)	  ((void)((a) != 0 && (*(a) = (win)->_attrs)), (void)((p) != 0 && (*(p) = PAIR_NUMBER((win)->_attrs))),OK)

#define kwattr_set(win,a,p,opts) ((win)->_attrs = (((a) & ~A_COLOR) | COLOR_PAIR(p)), OK)


/* Message: prints a message to the screen */
void curses_message(const char *args, ...)
{
    char p[MAX_MSG_SIZE];
    va_list vp;
    attr_t attrs;
    short i;
    int x, y;

    /*
     * Lock the mutex 
     */
    pthread_mutex_lock(&curses_msg_mutex);
    va_start(vp, args);
    vsprintf(p, args, vp);
    va_end(vp);

    if (rt.in_curses_display_loop == TRUE)
    {
	strncpy(message_buffer, p, MAX_MSG_SIZE);
	pthread_mutex_unlock(&curses_msg_mutex);
	return;
    } else
    {
	/*
	 * get the cursor position 
	 */
	getyx(stdscr, y, x);
	kwattr_get(stdscr, &attrs, &i, NULL);
	move(19, 0);
	clrtoeol();
	move(20, 0);
	clrtoeol();
	move(21, 0);
	clrtoeol();
	attrset(COLOR_PAIR(MSG_PAIR) | A_BOLD);
	mvprintw(19, 0, "%s", p);
	attrset(COLOR_PAIR(NULL_PAIR));
	/*
	 * Unlock the mutex 
	 */
	refresh();
	kwattr_set(stdscr, attrs, i, NULL);
	/*
	 * set the cursor to whhere it was */

	move(y, x);
	pthread_mutex_unlock(&curses_msg_mutex);
    }
}


/* Displays the mesasge and gets the users input for overwriting files*/
int curses_query_user_input(const char *args, ...)
{
    char p[MAX_MSG_SIZE];
    va_list vp;
    attr_t attrs;
    int x, y;
    int ch;
    int i;

    va_start(vp, args);
    vsprintf(p, args, vp);
    va_end(vp);
    getyx(stdscr, y, x);
    kwattr_get(stdscr, &attrs, &i, NULL);

    attrset(COLOR_PAIR(PROMPT_PAIR) | A_BOLD);
    move(19, 0);
    clrtoeol();
    move(20, 0);
    clrtoeol();
    move(21, 0);
    clrtoeol();
    mvprintw(19, 0, "%s", p);
    echo();
    refresh();
    do
    {
	napms(20);
	ch = getch();
    }
    while (ch == ERR);

    refresh();
    noecho();
    kwattr_set(stdscr, attrs, i, NULL);
    /*
     * set the cursor to where it was 
     */
    move(y, x);
    /*
     * the following strange line  is for compatibility 
     */
    return islower(ch) ? toupper(ch) : ch;
}

void curses_display_est_time(int row, int col, long bytes_left,
			     float current_speed)
{
    long seconds_left;

/* If the bytes left to obtain is less than zero, or the current_speed is zero
 * then just quit without printing anything 
 */
    if (bytes_left < 0 || current_speed <= 0)
	return;
    else
	seconds_left = (long) (bytes_left / current_speed);
    move(row, col);
    clrtoeol();

    if (seconds_left >= 3600)
	mvprintw(row, col, "Time remaining %d hours %d minutes %d seconds",
		 seconds_left / 3600, seconds_left % 3600 / 60,
		 (seconds_left % 3600) % 60);
    else if (seconds_left >= 60)
	mvprintw(row, col, "Time remaining %d minutes %d seconds",
		 seconds_left % 3600 / 60, (seconds_left % 3600) % 60);
    else
	mvprintw(row, col, "Time remaining %d seconds", seconds_left);

    return;
}
