bin_PROGRAMS    = proz

proz_SOURCES =		\
	connect.c	\
	connect.h	\
	connection.c	\
	connection.h	\
	curses.c	\
	ftp.c		\
        ftp.h		\
	ftp-retr.c	\
	ftp-retr.h	\
	ftpparse.c	\
	ftpparse.h	\
	ftpsearch.c	\
	ftpsearch.h	\
	getopt.c	\
	getopt.h	\
	http-retr.c	\
	http-retr.h	\
	http.c		\
	http.h		\
	init.c		\
	init.h		\
	interface.h	\
	logfile.c	\
	logfile.h	\
	main.c		\
	main.h		\
	mainconf.h	\
	misc.c		\
	misc.h		\
	netrc.c		\
	netrc.h		\
	ping.c		\
	ping.h		\
	resume.c	\
	resume.h	\
	runtime.h	\
        debug.c         \
        debug.h         \
	url.c		\
	url.h

INCLUDES        = -I. -I.. -I@includedir@
CFLAGS          = @CFLAGS@ -D_REENTRANT -Wall
CFLAGS         += -DGLOBAL_CONF_FILE="\"@sysconfdir@/prozilla.conf\""
LDADD           = -L@libdir@
LIBS            = @LIBS@ 
DEFS            = @DEFS@








