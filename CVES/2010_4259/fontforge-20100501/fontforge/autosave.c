/* Copyright (C) 2000-2010 by George Williams */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "fontforgevw.h"
/*#include "ustring.h"*/
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>
#include <ustring.h>
#include "gfile.h"

#if !defined(__MINGW32__)
# include <pwd.h>
#endif


char *getPfaEditDir(char *buffer) {
    static char *editdir = NULL;
    char *dir;
    char olddir[1024];

    if ( editdir!=NULL )
return( editdir );

    dir = GFileGetHomeDir();
    if ( dir==NULL )
return( NULL );
#ifdef __VMS
   sprintf(buffer,"%s/_FontForge", dir);
#else
   sprintf(buffer,"%s/.FontForge", dir);
#endif
   /* We used to use .PfaEdit. So if we don't find a .FontForge look for that*/
    /*  if there is a .PfaEdit, then rename it to .FontForge */
    if ( access(buffer,F_OK)==-1 ) {
#ifdef __VMS
       snprintf(olddir,sizeof(olddir),"%s/_PfaEdit", dir);
#else
       snprintf(olddir,sizeof(olddir),"%s/.PfaEdit", dir);
#endif
       if ( access(olddir,F_OK)==0 )
	    rename(olddir,buffer);
    }
    free(dir);
    /* If we still can't find it, create it */
    if ( access(buffer,F_OK)==-1 )
	if ( GFileMkDir(buffer)==-1 )
return( NULL );
    editdir = copy(buffer);
return( editdir );
}

static char *getAutoDirName(char *buffer) {
    char *dir=getPfaEditDir(buffer);

    if ( dir==NULL )
return( NULL );
    sprintf(buffer,"%s/autosave", dir);
    if ( access(buffer,F_OK)==-1 )
	if ( GFileMkDir(buffer)==-1 )
return( NULL );
    dir = copy(buffer);
return( dir );
}

static void MakeAutoSaveName(SplineFont *sf) {
    char buffer[1025];
    char *autosavedir;
    static int cnt=0;

    if ( sf->autosavename )
return;
    autosavedir = getAutoDirName(buffer);
    if ( autosavedir==NULL )
return;
    while ( 1 ) {
	sprintf( buffer, "%s/auto%06x-%d.asfd", autosavedir, getpid(), ++cnt );
	if ( access(buffer,F_OK)==-1 ) {
	    sf->autosavename = copy(buffer);
return;
	}
    }
}

int DoAutoRecovery(int inquire) {
    char buffer[1025];
    char *recoverdir = getAutoDirName(buffer);
    DIR *dir;
    struct dirent *entry;
    int any = false;
    SplineFont *sf;
    int inquire_state=0;

    if ( recoverdir==NULL )
return( false );
    if ( (dir = opendir(recoverdir))==NULL )
return( false );
    while ( (entry=readdir(dir))!=NULL ) {
	if ( strcmp(entry->d_name,".")==0 || strcmp(entry->d_name,"..")==0 )
    continue;
	sprintf(buffer,"%s/%s",recoverdir,entry->d_name);
	fprintf( stderr, "Recovering from %s... ", buffer);
	if ( (sf = SFRecoverFile(buffer,inquire,&inquire_state)) ) {
	    any=true;
	    if ( sf->fv==NULL )		/* Doesn't work, cli arguments not parsed yet */
		FontViewCreate(sf,false);
	}
	fprintf( stderr, " Done\n" );
    }
    closedir(dir);
return( any );
}

void CleanAutoRecovery(void) {
    char buffer[1025];
    char *recoverdir = getAutoDirName(buffer);
    DIR *dir;
    struct dirent *entry;

    if ( recoverdir==NULL )
return;
    if ( (dir = opendir(recoverdir))==NULL )
return;
    while ( (entry=readdir(dir))!=NULL ) {
	if ( strcmp(entry->d_name,".")==0 || strcmp(entry->d_name,"..")==0 )
    continue;
	sprintf(buffer,"%s/%s",recoverdir,entry->d_name);
	if ( unlink(buffer)!=0 ) {
	    fprintf( stderr, "Failed to clean " );
	    perror(buffer);
	}
    }
    closedir(dir);
}

void _DoAutoSaves(FontViewBase *fvs) {
    FontViewBase *fv;
    SplineFont *sf;

    for ( fv=fvs; fv!=NULL; fv=fv->next ) {
	sf = fv->cidmaster?fv->cidmaster:fv->sf;
	if ( sf->changed_since_autosave ) {
	    if ( sf->autosavename==NULL )
		MakeAutoSaveName(sf);
	    if ( sf->autosavename!=NULL )
		SFAutoSave(sf,fv->map);
	}
    }
}

void DoAutoSaves(void) {
    _DoAutoSaves(FontViewFirst());
}
