/* Functions to help with  retreiving and parsing mirror information 
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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <ctype.h>
#include <pthread.h>
#include <assert.h>
#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#else
#include <curses.h>
#endif				/*
				 * HAVE_CURSES 
				 */
#include "main.h"
#include "http.h"
#include "netrc.h"
#include "connect.h"
#include "runtime.h"
#include "url.h"
#include "misc.h"
#include "ftpsearch.h"
#include "ping.h"
#include "debug.h"


typedef struct {
    struct ftp_mirror *ftp_mirrors;
    int num_servers;
    int ping_run_done;
} ping_mthread_data;


uerr_t get_mirror_info(urlinfo * u, http_stat_t * hs, char **ret_buf);
void ping_the_list(ping_mthread_data * ping_mt_data);

char *find_ahref(char *buf)
{

    return (strstr(buf, "<A HREF="));

}

char *find_end(char *buf)
{
    return (strstr(buf, ">"));
}

char *find_closed_a(char *buf)
{
    return (strstr(buf, "</A"));
}

char *get_string_ahref(char *buf, char *out)
{
    char *p1, *p2, *p3;

    p1 = find_ahref(buf);
    assert(p1 != NULL);

    p2 = find_end(p1);
    assert(p2 != NULL);

    p3 = find_closed_a(p2);
    assert(p3 != NULL);

    strncpy(out, p2 + 1, p3 - p2 - 1);
    out[p3 - p2 - 1] = 0;
    return p3;

}


char *prepare_lycos_url(urlinfo * url_data, char * ftps_loc)
{

    char *lycos_url_buf;
    int lycos_url_len = strlen(ftps_loc) +
	strlen
	("?form=advanced&query=%s&doit=Search&type=Exact+search&hits=%d&matches=&hitsprmatch=&limdom=&limpath=&limsize1=%d&limsize2=%d&limtime1=&limtime2=&f1=Host&f2=Path&f3=Size&f4=-&f5=-&f6=-&header=none&sort=none&trlen=20");

    /*    int len,len1;
       char *p, *p1,*lycos_dir;
       int num_dirs, use_file=0, use_dir=0;
     */


    /* Okay lets now construct the URL we want to do lycos */
    lycos_url_buf =
	(char *) kmalloc(lycos_url_len + strlen(url_data->file) + 300);

    sprintf(lycos_url_buf,
	    "%s?form=advanced&query=%s&doit=Search&type=Exact+search&hits=%d&matches=&hitsprmatch=&limdom=&limpath=&limsize1=%ld&limsize2=%ld&f1=Host&f2=Path&f3=Size&f4=-&f5=-&f6=-&header=none&sort=none&trlen=20",
	    ftps_loc, url_data->file, rt.ftps_mirror_req_n,
	    url_data->file_size, url_data->file_size);

/* if we have a dir
   if(strcmp(url_data->dir,"pub")==0)
   {
   use_file=TRUE;
   }
   p=kstrdup(url_data->dir);
   p1=strtok(p,"/");
*/

    /*    while(p=kstrdup(url_data->dir))
       {

       }
       p=strdup(url_data->dir);
       p1=strtok(p,"/");

       sprintf(lycos_url_buf,"localhost/search3.html"); 
     */
    /*     sprintf(lycos_url_buf,"localhost/search3.html"); */

    debug_prz("ftpsearch url= %s\n", lycos_url_buf);
    return lycos_url_buf;
}

uerr_t parse_html_mirror_list(urlinfo * url_data,char *p,struct ftp_mirror ** pmirrors, int *num_servers)
{

   char  *p1, *p2, *i = 0, *j;
    ftp_mirror *ftp_mirrors;
    int k, num_ah = 0, num_pre = 0;
    char buf[1024];


    if (strstr(p, "No hits") != 0)
    {
	*num_servers = 0;
	return MIRINFOK;
    }

/*Check the number of PRE tags */
    p1 = p;
    while (((p1 = strstr(p1, "<PRE>")) != NULL) && p1)
    {
	num_pre++;
	p1 += 5;
    }

    debug_prz("Number of PRE tags found = %d\n", num_pre);

    if (num_pre == 1)
    {

	if ((i = strstr(p, "<PRE>")) == NULL)
	{
	    debug_prz("nomatches found");
	    return MIRPARSEFAIL;
	}

	debug_prz("match at %d found", i - p);

	if ((j = strstr(p, "</PRE>")) == NULL)
	{
	    debug_prz("nomatches found");
	    return MIRPARSEFAIL;
	}
    } else
    {
	/*search for the reported hits text */
	char *rep_hits;
	int prior_pres = 0;

	if ((rep_hits = strstr(p, "reported hits")) == NULL)
	{
	    debug_prz("no reported hits found");
	    return MIRPARSEFAIL;
	}

	/* Okay so we got the position after the results, lets see how many PRE tags were there before it */

	p1 = p;
	while (((p1 = strstr(p1, "<PRE>")) < rep_hits) && p1)
	{
	    prior_pres++;
	    p1 += 5;
	}
	/* now get the location of the PRE before the output */

	p1 = p;
	i = 0;
	while (prior_pres--)
	{
	    p1 = strstr(p1, "<PRE>");
	    p1 += 5;
	}
	i = p1 - 5;

	/*now find the </PRE>  tag which is after the results */
	j = strstr(i, "</PRE>");

	if (j == NULL)
	{
	    debug_prz("The expected </PRE> tag was not found!\n");
	    return MIRPARSEFAIL;
	}
    }

    p1 = kmalloc((j - i - 5) + 100);
    strncpy(p1, i + 5, j - i - 5);

    debug_prz("\nstring len= %ld", strlen(p1));
    p1[j - i - 5 + 1] = 0;

    p2 = p1;

    while ((i = strstr(p1, "<A HREF=")) != NULL)
    {
	num_ah++;
	p1 = i + 8;
    }

    debug_prz("\n%d ahrefs found\n", num_ah);

    if (num_ah == 0)
    {
	*num_servers = 0;
	return MIRINFOK;
    }



    *num_servers = num_ah / 3;
    debug_prz("%d servers found\n", *num_servers);

    /* Allocate +1 because we need to add the user specified server as well
     */
    ftp_mirrors =
	(ftp_mirror *) kmalloc(sizeof(ftp_mirror) * ((*num_servers) + 1));
    assert(ftp_mirrors != 0);

    for (k = 0; k < *num_servers; k++)
    {
	memset(&(ftp_mirrors[k]), 0, sizeof(ftp_mirror));
	p2 = get_string_ahref(p2, buf);
	ftp_mirrors[k].server_name = kstrdup(buf);
	p2 = get_string_ahref(p2, buf);

	/*Strip any leading slash in the path name if preent */
	if (*buf == '/')
	    ftp_mirrors[k].path = kstrdup(buf + 1);
	else
	    ftp_mirrors[k].path = kstrdup(buf);

	p2 = get_string_ahref(p2, buf);
	ftp_mirrors[k].file_name = kstrdup(buf);
    }

    /* add the users server to the end if it is a ftp server */
    if (url_data->proto == URLFTP)
    {
	memset(&(ftp_mirrors[k]), 0, sizeof(ftp_mirror));
	ftp_mirrors[k].server_name = kstrdup(url_data->host);
	ftp_mirrors[k].path = kstrdup(url_data->dir);
	ftp_mirrors[k].file_name = kstrdup(url_data->file);
	*num_servers += 1;
    }

    debug_prz("%d servers found\n", *num_servers);

    for (k = 0; k < *num_servers; k++)
    {

	ftp_mirrors[k].full_name =
	    (char *) kmalloc(strlen(ftp_mirrors[k].server_name) +
			     strlen(ftp_mirrors[k].path) +
			     strlen(ftp_mirrors[k].file_name) + 13);
	sprintf(ftp_mirrors[k].full_name, "%s%s:21/%s%s%s", "ftp://",
		ftp_mirrors[k].server_name, ftp_mirrors[k].path, "/",
		ftp_mirrors[k].file_name);

	debug_prz("%s\n", ftp_mirrors[k].full_name);
    }


    *pmirrors = ftp_mirrors;
    return MIRINFOK;

}

uerr_t get_complete_mirror_list(urlinfo * url_data,
				struct ftp_mirror ** pmirrors,
				int *num_servers)
{
  uerr_t err=HERR;
  char *lycos_url_buf;
  urlinfo lycos_url_data;
  char *p;
  http_stat_t hs;

  char *ftps_servers[]={
    LYCOS_FTPSEARCH_LOC,
    "http://ftpsearch.uniovi.es:8000/ftpsearch"
  };
  int num_ftps_servers=2;
  int scount;

  for(scount=0;scount<num_ftps_servers+1;scount++)
    {
	
      if(scount==0)
	{
	  /*Lets first try the server specified at runtime */ 
	  lycos_url_buf = prepare_lycos_url(url_data, rt.ftpsearch_url);
	  if (!lycos_url_buf)
	    {
	      die("error in preparing URL %s",rt.ftpsearch_url);
	    }

	}
      else
	{
	  /* Well it appears that the default ftp server did not work 
	     well lets fall back on the ones listed above */

	  /* If the originally tried url was the same as the ones specified above 
	     then skip it
	  */
	  if(strcmp(ftps_servers[scount-1],rt.ftpsearch_url)==0)
	    continue;
	  else
	    {
	      lycos_url_buf = prepare_lycos_url(url_data, ftps_servers[scount-1]);
	      if (!lycos_url_buf)
		{
		  die("error in preparing URL %s",ftps_servers[scount-1]);
		}
	    }

	}

      memset(&lycos_url_data, 0, sizeof(lycos_url_data));

      err = parseurl(lycos_url_buf, &lycos_url_data, 0);
      if (err != URLOK)
	{
	  message("problem with URL\n");
	  continue;
	}

      err = get_mirror_info(&lycos_url_data, &hs, &p);
      if (err != HOK)
	{
	  message("A error occured while talking to %s\n",lycos_url_data.host);
	  freeurl(&lycos_url_data, 0);
   
	  if(scount==0)
	    {
	      napms(1000);
	      message("Will try backup ftpsearch servers instead");
	      napms(1000);
	    }

	  continue;
	}

  
      /*free the lycos url data struct */
      freeurl(&lycos_url_data, 0);

      err=parse_html_mirror_list(url_data,p,pmirrors, num_servers); 
      if (err==MIRINFOK)
	break;

      if(scount==0)
	    {
	      message("Unable to Parse the output of the server will try backup servers instead");
	      napms(1000);
	    }

    }

  return err;
}

void ping_one_server(ftp_mirror * server)
{
    uerr_t err;


    debug_prz("Pinging = %s....\n", server->server_name);

    err = myftp_ping(server->server_name, &(server->tv));

    debug_prz("err returned in pos =%d", err);

    if (err != FTPOK)
    {
	server->status = NORESPONSE;
/*	    message("%s No response/Error\n", server->server_name); */
    } else
    {
	/* ok it succeded */
	server->status = RESPONSEOK;
	server->milli_secs =
	    ((server->tv.tv_sec * 1000000) + server->tv.tv_usec) / 1000;
/*	    message("%s %dms\n",server->server_name, server->milli_secs); */
	debug_prz("%s %dms\n", server->server_name, server->milli_secs);
    }

}


void ping_the_list(ping_mthread_data * ping_mt_data)
{
    int k = 0, i, j;
    int num_iter;
    int num_left;
    pthread_t *ping_threads;

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    ping_threads =
	(pthread_t *) kmalloc(sizeof(pthread_t) * rt.max_simul_pings);

    num_iter = ping_mt_data->num_servers / rt.max_simul_pings;
    num_left = ping_mt_data->num_servers % rt.max_simul_pings;
    debug_prz("Max simul pings=%d", rt.max_simul_pings);

    k = 0;

    for (i = 0; i < num_iter; i++)
    {
	for (j = 0; j < rt.max_simul_pings; j++)
	{
	    if (pthread_create(&ping_threads[j], NULL,
			       (void *) &ping_one_server,
			       (void *) (&ping_mt_data->ftp_mirrors[k])) !=
		0)
		die("Error: Not enough system resources"
		    "to create thread!\n");
	    k++;
	}

	for (j = 0; j < rt.max_simul_pings; j++)
	{
	    /*Wait till the end of each thread. */
	    pthread_join(ping_threads[j], NULL);
	}

    }

    for (j = 0; j < num_left; j++)
    {
	if (pthread_create(&ping_threads[j], NULL,
			   (void *) &ping_one_server,
			   (void *) (&ping_mt_data->ftp_mirrors[k])) != 0)
	    die("Error: Not enough system resources"
		"to create thread!\n");

	k++;
    }

    for (j = 0; j < num_left; j++)
    {
	/*Wait till the end of each thread. */
	pthread_join(ping_threads[j], NULL);
    }

    ping_mt_data->ping_run_done = TRUE;
}


static int top_item = 0;

interface_ret curses_ftpsearch_interface(struct ftp_mirror *ftp_mirrors,
					 int num_servers,
					 int *selected_server)
{
#define CTRL(x) ((x) & 0x1F)

    pthread_t ping_main_thread;
    ping_mthread_data ping_mt_data;

    assert(ftp_mirrors != NULL);
    top_item = 0;
    *selected_server = 0;

    message("Starting ping\n");
    /* now that we have got the servers lets "ping" them and see */

    memset(&ping_mt_data, 0, sizeof(ping_mthread_data));
    ping_mt_data.ftp_mirrors = ftp_mirrors;
    ping_mt_data.num_servers = num_servers;
    ping_mt_data.ping_run_done = FALSE;

    /*launch the main thread */
    if (pthread_create(&ping_main_thread, NULL,
		       (void *) &ping_the_list,
		       (void *) (&ping_mt_data)) != 0)
	die("Error: Not enough system resources" "to create thread!\n");


    /* while(1) */
    while (ping_mt_data.ping_run_done == FALSE)
    {
	switch (getch())
	{
	case KEY_DOWN:
	    top_item += 1;
	    if (top_item >= num_servers)
		top_item = num_servers - 1;
	    break;
	case KEY_UP:
	    top_item -= 1;
	    if (top_item < 0)
		top_item = 0;
	    break;
	case CTRL('A'):
	    /*Abort */
	    pthread_cancel(ping_main_thread);
	    pthread_join(ping_main_thread, NULL);
	    /* fixme set a status flag...to indicate we aborted */
	    refresh();
	    delay_ms(500);
	    erase();
	    return USER_ABORTED;
	    break;

	case 13:
	case 10:
	    /*return balue user selected */
	    pthread_cancel(ping_main_thread);
	    pthread_join(ping_main_thread, NULL);
	    /*fixme set a status flag...to indicate we aborted */
	    *selected_server = top_item;
	    refresh();
	    delay_ms(500);
	    erase();
	    return USER_SELECTED;
	    break;

	default:
	    break;
	}

	delay_ms(30);
	display_list(ftp_mirrors, num_servers);
	refresh();
    }

    pthread_join(ping_main_thread, NULL);
    /*display again before leaving */

    delay_ms(30);
    display_list(ftp_mirrors, num_servers);
    refresh();
    delay_ms(1000);
    erase();
    return AUTO;

}

void display_list(struct ftp_mirror *pmirrors, int num_servers)
{
    const int max_lines = 13;
    int i;

    attrset(COLOR_PAIR(HIGHLIGHT_PAIR) | A_BOLD);
    mvaddstr(0, 0, "Server                         Ping Time");
    attrset(COLOR_PAIR(NULL_PAIR));

    for (i = 0; i < max_lines; i++)
    {
	move(i + 1, 0);
	clrtoeol();

	if (i == 0)
	    attrset(COLOR_PAIR(SELECTED_PAIR) | A_BOLD);

	if (i + top_item < num_servers)
	{
	    switch (pmirrors[top_item + i].status)
	    {
	    case RESPONSEOK:
		mvprintw(i + 1, 0, "%2d %-25.25s %12dms", top_item + i + 1,
			 pmirrors[top_item + i].server_name,
			 pmirrors[top_item + i].milli_secs);
		break;
	    case NORESPONSE:
		mvprintw(i + 1, 0, "%2d %-25.25s %12s", top_item + i + 1,
			 pmirrors[top_item + i].server_name,
			 "No Response");
		break;
	    case ERROR:
		mvprintw(i + 1, 0, "%2d %-25.25s %12s", top_item + i + 1,
			 pmirrors[top_item + i].server_name, "Error");
		break;
	    default:
		mvprintw(i + 1, 0, "%2d %-25.25s %12s", top_item + i + 1,
			 pmirrors[top_item + i].server_name, "Untested");
		break;
	    }
	}

	if (i == 0)
	    attrset(COLOR_PAIR(NULL_PAIR));

    }

    attrset(COLOR_PAIR(HIGHLIGHT_PAIR) | A_BOLD);
    mvprintw(max_lines + 2, 0,
	     "Up and Down arrows to scroll, CTRL+A to Abort");
    mvprintw(max_lines + 3, 0,
	     "Enter to abort and start from the highlighted server");
    attrset(COLOR_PAIR(NULL_PAIR));

    move(max_lines + 4, 0);
    clrtoeol();
    move(max_lines + 5, 0);
    clrtoeol();
    attrset(COLOR_PAIR(SELECTED_PAIR) | A_BOLD);
    mvprintw(max_lines + 4, 0, "%s", pmirrors[top_item].full_name);
    attrset(COLOR_PAIR(NULL_PAIR));
}




char *grow_buffer(char *buf_start, char *cur_pos, int *buf_len,
		  int data_len)
{
    const int INIT_SIZE = 4048;
    int bytes_left;
    char *p;

    /* find how many bytes are left */
    bytes_left = *buf_len - (cur_pos - buf_start);
    assert(bytes_left >= 0);
    assert(data_len <= INIT_SIZE);

    if (bytes_left < data_len + 1)
    {
	/* time to realloc the buffer buffer */
	p = krealloc(buf_start, *buf_len + INIT_SIZE);
	*buf_len += INIT_SIZE;
	return p;
    } else
    {
	return buf_start;
    }
}


uerr_t get_mirror_info(urlinfo * u, http_stat_t * hs, char **ret_buf)
{

    int sock, i;
    char buffer[HTTP_BUFFER_SIZE];
    char *user, *passwd, *wwwauth;
    char *p, *p1, *p2;
    uerr_t err;
    netrc_entry *netrc_ent;
    int p_len, ret, total;

    /* adding redirect support */

    err = get_http_info(u, hs);
    if (err == NEWLOCATION)
    {
	/*
	 * loop for max_redirections searching for a valid file 
	 */
	for (i = 0; i < rt.max_redirections; ++i)
	{
 char *constructed_newloc;

		  /*DONE : handle relative urls too */
		  constructed_newloc =
		    uri_merge(u->url, hs->newloc);

		  debug_prz("Redirected to %s, merged URL = %s",
			  hs->newloc, constructed_newloc);

		  err =
		    parseurl(constructed_newloc, u, 0);
		  if (err != URLOK)
		    {
		      die("The server returned location is syntatically wrong: %s!", constructed_newloc);
		    }


	    message("=> %s", hs->newloc);
	    debug_prz("=> %s", hs->newloc);

	    assert(u->proto == URLHTTP);

	    err = get_http_info(u, hs);

	    if (err == HOK)
	    {
		break;
	    } else if (err == NEWLOCATION)
	    {
		if (i == rt.max_redirections)
		{
		    /*
		     * redirected too many times 
		     */
		    die("Error: Too many redirections:%d\n",
			rt.max_redirections);
		}
	    } else
	    {

		message
		    ("A error occured while trying to get info from the server\n");
		return err;
	    }

	}
    } else if (err != HOK)
    {
	if (hs->statcode == HTTP_NOT_FOUND)
	{
	    message
		("Error: Uanable to get mirror info from the server: Not found!\n");
	} else
	{
	    message
		("A error occured while trying to get info from the server\n");
	    return err;
	}
    }



    err = connect_to_server(&sock, u->host, u->port, rt.timeout);
    if (err != NOCONERROR)
    {
	message("Error connecting to %s", u->host);
	return err;
    }

    /* Authentification code */

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

    /* We will get http info about the file by calling http_fetch_headers 
       with HEAD */


    sprintf(buffer,
	    "GET %s HTTP/1.0\r\nUser-Agent: %s%s\r\nHost: %s\r\nAccept: */*\r\n%s\r\n",
	    u->path, PACKAGE_NAME, VERSION, u->host,
	    wwwauth ? wwwauth : "");

    err = http_fetch_headers(sock, u, hs, buffer);

    if (wwwauth)
	free(wwwauth);


    /*Check if we authenticated using any user or password and if we 
       were kicked out, if so return HAUTHFAIL */

    if (err != HOK)
    {
	if (err == HAUTHREQ && (strlen(user) || strlen(passwd)))
	    return HAUTHFAIL;

	return err;
    }

    /* Ok start fetching the data */
    p1 = p = (char *) kmalloc(HTTP_BUFFER_SIZE + 1);
    p_len = HTTP_BUFFER_SIZE + 1;
    total = 0;
    do
    {

	ret = krecv(sock, buffer, HTTP_BUFFER_SIZE, 0, rt.timeout);
	if (ret > 0)
	{
	    p2 = grow_buffer(p, p1, &p_len, ret);
	    memcpy(p2 + (p1 - p), buffer, ret);
	    p1 = (p1 - p) + ret + p2;
	    p = p2;
	}
	total += ret;
    }
    while (ret > 0);

    if (ret == -1)
    {
	if (errno == ETIMEDOUT)
	{
	    close(sock);
	    return READERR;
	}
	close(sock);
	return READERR;
    }

    p[total] = 0;
    *ret_buf = p;
    close(sock);
    return HOK;
}


int get_mirror_list_pos(char *full_name, ftp_mirror * list, int num_servers)
{
    int i;


    for (i = 0; i < num_servers; i++)
    {
debug_prz("list = %s path = %s\n", full_name, list[i].full_name);

	    
	if (strcmp(full_name, list[i].full_name) == 0)
	    return i;
    }

    /* Not found: return -1 */
    return -1;
}
