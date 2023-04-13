/* Declarations for FTP handling.
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

#ifndef FTP_H
#define FTP_H

#include "url.h"
#include "ftpparse.h"
#include "connection.h"


typedef struct {
    struct ftpparse fp;
    int is_dir;			/*
				 * 1 if the object is a file 
				 */
    int is_file;		/*
				 * 1 if the object is a dir 
				 */
    int not_found;		/*
				 * 1 if the object was not found 
				 */

} ftp_stat_t;

int ftp_check_msg(int sock, char *szPtr, int len);
int ftp_read_msg(int sock, char *szBuffer, int len);
uerr_t ftp_send__msg(int control_sock, char *szBuffer, int len);

uerr_t ftp_get_line(int control_sock, char *buffer);
uerr_t ftp_binary(int sock);
uerr_t ftp_ascii(int sock);
uerr_t ftp_port(int sock, char *command);
uerr_t ftp_pasv(int sock, unsigned char *addr);
uerr_t ftp_list(int sock, char *file);
uerr_t ftp_retr(int sock, char *file);
uerr_t ftp_rest(int sock, long bytes);
uerr_t ftp_cwd(int sock, char *dir);
uerr_t ftp_pwd(int sock, char *dir);
uerr_t ftp_get_listen_socket(int control_sock, int *listen_sock);

uerr_t ftp_login(int control_sock, const char *username,
		 const char *passwd);
uerr_t ftp_connect_to_server(int *sock, char *name, int port);
uerr_t ftp_get_file_info(int control_sock, char *fname, ftp_stat_t * fs);
uerr_t ftp_get_info(urlinfo * u, ftp_stat_t * fs);
#endif
