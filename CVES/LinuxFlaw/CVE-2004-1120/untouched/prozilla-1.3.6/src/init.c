/* Initialising runtime variables

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

   Flower <floweros@golia.ro> added the configuration routines.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif				/*
				 * HAVE_CONFIG_H 
				 */

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "runtime.h"
#include "getopt.h"
#include "main.h"
#include "misc.h"


/* Imported from wget. */
#define CMD_DECLARE(func) static int func (const char *, const char *, void *)

CMD_DECLARE(cmd_number);


/* Sets the default config */
void set_defaults(int *argc, char ***argv)
{
    /*
     * Zero the structure which holds the config data 
     */
    memset(&rt, 0, sizeof(rt));
    /*
     * The default no of connections and maximum redirections allowed 
     */
    rt.num_connections = 4;
    rt.max_redirections = 10;
    /*
     * the programs running mode by default NORMAL 
     */
    rt.run_mode = NORMAL;
    /*
     * Uses PASV by default 
     */
    rt.use_pasv = TRUE;
    /*
     * display mode is curses by default 
     */
    rt.display_mode = DISPLAY_CURSES;
    /*Prompt the user to press any key on error */
    rt.die_to_stdout = FALSE;
    /*
     * The force option, off by default when enabled 
     * cause Prozilla not to prompt the user about overwriting existent
     * files etc..
     */
    rt.force_mode = FALSE;
    /*
     * .netrc options 
     */
    rt.use_netrc = TRUE;
    rt.netrc_list = NULL;
    /*
     * The timeout period for connections in seconds
     */
    rt.timeout = 180;
    /*
     * The max number of trys and the delay between each 
     */
    rt.try_attempts = 200;
    rt.retry_delay = 15;	/*
				 * delay in seconds 
				 */
#ifdef HAVE_GTK
    rt.main_argc = argc;
    rt.main_argv = argv;
    rt.use_gtk_option = FALSE;
#endif
    /* We are not in the curses display loop */
    rt.in_curses_display_loop = FALSE;
    /*Default is to not log any debug info */
    rt.debug_mode = FALSE;
    rt.ftp_search = FALSE;
    rt.max_simul_pings = 5;
    rt.max_ping_wait = 4;
    rt.ftps_mirror_req_n = 40;

    rt.max_bps = 0;		/* 0= No throttling */
    /*Output the file to the directory ( "./" by default) */
    rt.output_dirprefix = kstrdup(".");
    rt.ftpsearch_url =
	kstrdup("http://download.lycos.com/swadv/AdvResults.asp");
}

/* Store the boolean value from VAL to CLOSURE.  COM is ignored,
   except for error messages.  */
int cmd_boolean(const char *com, const char *val, void *closure)
{
    int bool_value;

    if (!strcasecmp(val, "on") || (*val == '1' && !*(val + 1)))
	bool_value = 1;
    else if (!strcasecmp(val, "off") || (*val == '0' && !*(val + 1)))
	bool_value = 0;
    else
    {
	fprintf(stderr, "Prozilla: %s: Please specify on or off.\n", com);
	return 0;
    }

    *(int *) closure = bool_value;
    return 1;
}

/* Set the non-negative integer value from VAL to CLOSURE.  With
   incorrect specification, the number remains unchanged.  */
int cmd_number(const char *com, const char *val, void *closure)
{
    int num = atoi(val);

    if (num <= -1)
    {
	fprintf(stderr, "Prozilla: %s: Invalid specification `%s'.\n", com,
		val);
	return 0;
    }

    if ((strcmp(com, "threads") == 0) && num == 0)
    {
	fprintf(stderr, "Prozilla: %s: Invalid specification `%s'.\n", com,
		val);
	return 0;
    }

    *(int *) closure = num;
    return 1;
}

int cmd_string(const char *com, const char *val, void *closure)
{
    char **pstring = (char **) closure;
    *pstring = kstrdup(val);
    return 1;
}



/* List of recognized commands, each consisting of name, closure and
   function.  When adding a new command, simply add it to the list,
   but be sure to keep the list sorted alphabetically, as comind()
   depends on it.  */
struct {
    char *name;
    void *closure;
    int (*action) (const char *, const char *, void *);
} commands[] = {
    {
    "debug", &rt.debug_mode, cmd_boolean},
    {
    "forcemode", &rt.force_mode, cmd_boolean},
    {
    "ftpsearch", &rt.ftp_search, cmd_boolean},
    {
    "ftpsearchurl", &rt.ftpsearch_url, cmd_string},
    {
    "mainoutputdir", &rt.output_dirprefix, cmd_string},
    {
    "maxbps", &rt.max_bps, cmd_number},
    {
    "maxredirs", &rt.max_redirections, cmd_number},
    {
    "mirrors", &rt.ftps_mirror_req_n, cmd_number},
    {
    "netrc", &rt.use_netrc, cmd_boolean},
    {
    "nogetch", &rt.die_to_stdout, cmd_boolean},
    {
    "pasv", &rt.use_pasv, cmd_boolean},
    {
    "pingatonce", &rt.max_simul_pings, cmd_number},
    {
    "pingtimeout", &rt.max_ping_wait, cmd_number},
    {
    "retrydelay", &rt.retry_delay, cmd_number},
    {
    "threads", &rt.num_connections, cmd_number},
    {
    "timeout", &rt.timeout, cmd_number},
    {
    "tries", &rt.try_attempts, cmd_number}

};

/* Return index of COM if it is a valid command, or -1 otherwise.  COM
   is looked up in `commands' using binary search algorithm.  */
int comind(const char *com)
{
    int min = 0, max = sizeof(commands) / sizeof(*(commands));

    do
    {
	int i = (min + max) / 2;
	int cmp = strcasecmp(com, commands[i].name);
	if (cmp == 0)
	    return i;
	else if (cmp < 0)
	    max = i - 1;
	else
	    min = i + 1;
    }
    while (min <= max);
    return -1;
}

/* Read a line from FP.  The function reallocs the storage as needed
   to accomodate for any length of the line.  Reallocs are done
   storage exponentially, doubling the storage after each overflow to
   minimize the number of calls to realloc().                                                       
 
   It is not an exemplary of correctness, since it kills off the
   newline (and no, there is no way to know if there was a newline at
   EOF).  */
char *read_whole_line(FILE * fp)
{
    char *line;
    int i, bufsize, c;

    i = 0;
    bufsize = 40;
    line = (char *) kmalloc(bufsize);
/* Construct the line.  */
    while ((c = getc(fp)) != EOF && c != '\n')
    {
	if (i > bufsize - 1)
	    line = (char *) krealloc(line, (bufsize <<= 1));
	line[i++] = c;
    }
    if (c == EOF && !i)
    {
	free(line);
	return NULL;
    }
/* Check for overflow at zero-termination (no need to double the
   buffer in this case.  */
    if (i == bufsize)
	line = (char *) krealloc(line, i + 1);
    line[i] = '\0';
    return line;
}

/* Parse the line pointed by line, with the syntax:
   <sp>* command <sp>* = <sp>* value <newline>
   Uses kmalloc to allocate space for command and value.
   If the line is invalid, data is freed and 0 is returned.

   Return values:
    1 - success
    0 - failure
   -1 - empty */
int parse_line(const char *line, char **com, char **val)
{
    const char *p = line;
    const char *orig_comptr, *end;
    char *new_comptr;

    /* Skip spaces.  */
    while (*p == ' ' || *p == '\t')
	++p;

    /* Don't process empty lines.  */
    if (!*p || *p == '\n' || *p == '#')
	return -1;

    for (orig_comptr = p; isalpha(*p) || *p == '_' || *p == '-'; p++)
	;
    /* The next char should be space or '='.  */
    if (!isspace(*p) && (*p != '='))
	return 0;
    *com = (char *) kmalloc(p - orig_comptr + 1);
    for (new_comptr = *com; orig_comptr < p; orig_comptr++)
    {
	if (*orig_comptr == '_' || *orig_comptr == '-')
	    continue;
	*new_comptr++ = *orig_comptr;
    }
    *new_comptr = '\0';
    /* If the command is invalid, exit now.  */
    if (comind(*com) == -1)
    {
	free(*com);
	return 0;
    }

    /* Skip spaces before '='.  */
    for (; isspace(*p); p++);
    /* If '=' not found, bail out.  */
    if (*p != '=')
    {
	free(*com);
	return 0;
    }
    /* Skip spaces after '='.  */
    for (++p; isspace(*p); p++);
    /* Get the ending position.  */
    for (end = p; *end && *end != '\n'; end++);
    /* Allocate *val, and copy from line.  */
    *val = strdupdelim(p, end);
    return 1;
}

/* Asociate prozrc commands w/ internal variables. */
int setval(const char *com, const char *val)
{
    int ind;
    if (!com || !val)
	return 0;
    ind = comind(com);

    if (ind == -1)
    {
	/* #### Should I just abort()?  */
	return 0;
    }
    return ((*commands[ind].action) (com, val, commands[ind].closure));
}

/* Add options from ~/.prozrc */
void set_preferences(const char *file)
{
    FILE *fp;
    char *line;
    int ln;

    fp = fopen(file, "rb");
    if (!fp)
    {
	/*  fprintf (stderr, "Prozilla: Cannot read %s (%s).\n", file, strerror (errno));
	 */
	return;
    }
    /* Reset line number.  */
    ln = 1;
    while ((line = read_whole_line(fp)))
    {
	char *com, *val;
	int status;
	int length = strlen(line);
	if (length && line[length - 1] == '\r')
	    line[length - 1] = '\0';
	/* Parse the line.  */
	status = parse_line(line, &com, &val);
	free(line);
	/* If everything is OK, set the value.  */
	if (status == 1)
	{
	    if (!setval(com, val))
		fprintf(stderr, "Prozilla: Error in %s at line %d.\n",
			file, ln);
	    free(com);
	    free(val);
	} else if (status == 0)
	    fprintf(stderr, "Prozilla: Error in %s at line %d.\n", file,
		    ln);
	++ln;
    }
    fclose(fp);
}



/* Return the path to the user's .prozrc.  This is either the value of
   `PROZRC' environment variable, or `$HOME/.prozrc'.
   If the `PROZRC' variable exists but the file does not exist, the
   function will exit().  */
char *prozrc_file_name(void)
{
    char *env, *home;
    char *file = NULL;

    /* Try the environment.  */
    env = getenv("PROZRC");
    if (env && *env)
    {
	if (!access(env, F_OK))
	{
	    fprintf(stderr, "Prozilla: %s: %s.\n", file, strerror(errno));
	    exit(1);
	}
	return kstrdup(env);
    }
    home = home_dir();
    if (home)
    {
	file = (char *) kmalloc(strlen(home) + 1 + strlen(".prozrc") + 1);
	sprintf(file, "%s/.prozrc", home);
    }

    free(home);
    if (!file)
	return NULL;
    if (access(file, F_OK) != 0)
    {
	free(file);
	return NULL;
    }
    return file;
}
