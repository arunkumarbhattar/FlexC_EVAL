/* Miscelaneous routines that don't quite fit anywhere else :).
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
#include <stdarg.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include <curses.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <limits.h>
#include "main.h"
#include "misc.h"
#include "debug.h"
#include "runtime.h"


/* a malloc which aborts if not enough memory is present */

void *kmalloc(size_t size)
{
    void *ret;

    ret = malloc(size);
    if (!ret)
    {
	die("Not enough memory to continue: malloc failed\n");
    }
    return ret;
}

/* a Wrapper for realloc which aborts if not enough memory is present */

void *krealloc(void *ptr, size_t new_size)
{
    void *ret;

    ret = realloc(ptr, new_size);
    if (!ret)
    {
	die("Not enough memory to continue: realloc failed\n");
    }
    return ret;
}



/******************************************************************************
 A wrapper for free() which handles NULL pointers.
******************************************************************************/
void kfree(void *ptr)
{
  if (ptr != NULL)
    free(ptr);
}



/* checks and sees wether the specifed string is a number */
/* TODO move to misc.c and add to misc.h */

int is_number(char *str)
{
    int i = 0;

    while (str[i])
    {
	if (isdigit(str[i]) == 0)
	{
	    return 0;
	}
	i++;
    }
    return 1;
}



/* TODO port these functions */
char *kstrdup(const char *s)
{
    char *s1;

    s1 = strdup(s);
    if (!s1)
    {
	die("Not enough memory to continue: strdup failed");
    }
    return s1;

}


/* How many digits in a (long) integer? */
int numdigit(long a)
{
    int res;

    for (res = 1; (a /= 10) != 0; res++);
    return res;
}


/* Copy the string formed by two pointers (one on the beginning, other
   on the char after the last char) to a new, malloc-ed
   location. 0-terminate it. */

char *strdupdelim(const char *beg, const char *end)
{
    char *res;

    res = (char *) kmalloc(end - beg + 1);
    memcpy(res, beg, end - beg);
    res[end - beg] = '\0';
    return res;
}


/* Print a long integer to the string buffer.  The digits are first
   written in reverse order (the least significant digit first), and
   are then reversed.  */

void prnum(char *where, long num)
{
    char *p;
    int i = 0, l;
    char c;

    if (num < 0)
    {
	*where++ = '-';
	num = -num;
    }
    p = where;
    /*
     * Print the digits to the string. 
     */
    do
    {
	*p++ = num % 10 + '0';
	num /= 10;
    }
    while (num);
    /*
     * And reverse them. 
     */
    l = p - where - 1;
    for (i = l / 2; i >= 0; i--)
    {
	c = where[i];
	where[i] = where[l - i];
	where[l - i] = c;
    }
    where[l + 1] = '\0';
}

/* Extracts a numurical argument from a option,
   when it has been specified for example as -l=3 or, -l3 
   returns 1 on success or 0 on a error (non nemrical argument etc 
*/

int setargval(char *optstr, int *num)
{
    if (*optstr == '=')
    {
	if (is_number(optstr + 1) == 1)
	{
	    *num = atoi(optstr + 1);
	    return 1;
	} else
	{
	    return 0;
	}
    } else
    {
	if (is_number(optstr) == 1)
	{
	    *num = atoi(optstr);
	    return 1;

	} else
	{
	    return 0;
	}
    }

}



/* Encode the string S of length LENGTH to base64 format and place it
   to STORE.  STORE will be 0-terminated, and must point to a writable
   buffer of at least 1+BASE64_LENGTH(length) bytes.  
   Note: Routine stolen from wget (grendel) 
*/

void base64_encode(const char *s, char *store, int length)
{
    /* Conversion table.  */
    char tbl[64] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/'
    };
    int i;
    unsigned char *p = (unsigned char *) store;

    /* Transform the 3x8 bits to 4x6 bits, as required by base64.  */
    for (i = 0; i < length; i += 3)
    {
	*p++ = tbl[s[0] >> 2];
	*p++ = tbl[((s[0] & 3) << 4) + (s[1] >> 4)];
	*p++ = tbl[((s[1] & 0xf) << 2) + (s[2] >> 6)];
	*p++ = tbl[s[2] & 0x3f];
	s += 3;
    }
    /* Pad the result if necessary...  */
    if (i == length + 1)
	*(p - 1) = '=';
    else if (i == length + 2)
	*(p - 1) = *(p - 2) = '=';
    /* ...and zero-terminate it.  */
    *p = '\0';
}



/* Return the user's home directory (strdup-ed), or NULL if none is
   found.  */
char *home_dir(void)
{
    char *home = getenv("HOME");
    if (!home)
    {
/* If HOME is not defined, try getting it from the password
   file.  */
	struct passwd *pwd = getpwuid(getuid());
	if (!pwd || !pwd->pw_dir)
	    return NULL;
	home = pwd->pw_dir;
    }
    return home ? kstrdup(home) : NULL;
}



/* Subtract the `struct timeval' values X and Y,
*  storing the result in RESULT.
*  Return 1 if the difference is negative, otherwise
*  0. 
*/

int timeval_subtract(struct timeval *result, struct timeval *x,
		     struct timeval *y)
{
    /* Perform the carry for the later
     * subtraction by updating Y. */
    if (x->tv_usec < y->tv_usec)
    {
	int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
	y->tv_usec -= 1000000 * nsec;
	y->tv_sec += nsec;
    }
    if (x->tv_usec - y->tv_usec > 1000000)
    {
	int nsec = (x->tv_usec - y->tv_usec) / 1000000;
	y->tv_usec += 1000000 * nsec;
	y->tv_sec -= nsec;
    }

    /* Compute the time
     * remaining to wait.
     *           `tv_usec'
     *           is
     *           certainly
     *           positive.
     *           */
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_usec = x->tv_usec - y->tv_usec;

    /* Return
     * 1 if
     * result
     * is
     * negative.
     * */
    return x->tv_sec < y->tv_sec;
}


void delay_ms(int ms)
{
    struct timeval tv_delay;

    memset(&tv_delay, 0, sizeof(tv_delay));

    tv_delay.tv_sec = ms / 1000;
    tv_delay.tv_usec = (ms * 1000) % 1000000;

    if (select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &tv_delay) < 0)
    {
	debug_prz("Warning Unable to delay\n");
    }
}

char *get_prefixed_file(char *file)
{
    char *prefixed_output_file;

    if ((strcmp(rt.output_dirprefix, ".") != 0)
	&& (strcmp(rt.output_dirprefix, "") != 0))
    {

	prefixed_output_file = kmalloc(PATH_MAX);
	   
	snprintf(prefixed_output_file, PATH_MAX,"%s/%s", rt.output_dirprefix, file);
	debug_prz("prefixed output_file = %s\n", prefixed_output_file);
	return prefixed_output_file;
    }
    return strdup(file);
}
