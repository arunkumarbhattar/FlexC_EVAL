/* Declarations for miscelaneous routines.
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

#ifndef MISC_H
#define MISC_H


void *kmalloc(size_t size);
void kfree(void *ptr);
void *krealloc(void *ptr, size_t new_size);

int is_number(char *str);
char *kstrdup(const char *s);
int numdigit(long a);
char *strdupdelim(const char *beg, const char *end);
void prnum(char *where, long num);

void message(const char *args, ...);
int setargval(char *optstr, int *num);
void base64_encode(const char *s, char *store, int length);
/* Return the user's home directory (strdup-ed), or NULL if none is
   found.  */
char *home_dir(void);
int timeval_subtract(struct timeval *result, struct timeval *x,
		     struct timeval *y);
void delay_ms(int ms);
char *get_prefixed_file(char *file);
#endif
