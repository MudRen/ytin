#include <config.h>
#ifdef HAVE_STRING_H
#include <string.h>
#elif defined(HAVE_STRINGS_H)
#include <strings.h>
#endif

#include <signal.h>
#include <ctype.h>
#include "tintin.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif

#ifndef BADSIG
#define BADSIG (void (*)())-1
#endif

#include "pause.proto"
#include "queue.proto"
#include "rl.proto"
#include "variables.proto"

/************************/
/* the #pauseoff command */
/************************/
/*this command is now obsolete, can be replaced by #pause 0
void pauseoff_command(struct session* ses)
{
	if(ses) {
		if(ses->pausestatus != FALSE){
			ses->pausestatus=FALSE;
			tintin_puts("#PAUSE IS NOW OFF.", ses);
		}
	}
	else
		tintin_puts("#NO SESSION ACTIVE => NO PAUSE!", ses);
}
*/

void purge_command(const char *arg0, struct session* ses)
{
	int purgemode = 1;
	char arg[BUFFER_SIZE];
	if (ses) {
		if(*arg0){
			substitute_myvars(arg0, arg, ses);
			sscanf(arg, "%d", &purgemode);
		}
		kill_all_pending_command(purgemode, ses);
		purge_slow(ses);
		sprintf(arg, "OK, ALL PENDING COMMANDS ARE PURGED!(mode %d)", purgemode);
		tintin_puts(arg, ses);
	} else
	tintin_puts("#NO SESSION ACTIVE!", ses);
}

/*************************/
/* the #pause command */
/*************************/
void pause_command(const char *arg0, struct session* ses)
{
	float psize;
	long psec, pusec;
	char buff[BUFFER_SIZE];
	
	if(!ses) {
		tintin_puts("#NO SESSION ACTIVE => NO PAUSE!", ses);
		return;
	}
	if(*arg0!='\0') {
		char arg[BUFFER_SIZE];
		substitute_myvars(arg0, arg, ses);
		sscanf(arg, "%f", &psize);
		if (psize<=0) {
			if(ses->pausestatus == TRUE){
				tintin_puts("#OK PAUSE IS TURNED OFF", ses);    
			}
			ses->pausestatus=FALSE;
		}
		else {
			psec = (long) psize; pusec = (long) ((psize - (float)psec) * 1e6);
			ses->writepause.tv_sec=psec;
			ses->writepause.tv_usec=pusec;
			if(ses->pausestatus == FALSE){
				sprintf(buff, "#OK PAUSE is turned ON(%g).",
					(float)((double)ses->writepause.tv_sec + (double)ses->writepause.tv_usec/1e6));
				tintin_puts(buff, ses);    
			}
			ses->pausestatus=TRUE;
		}
	}
	else {
		if(ses->pausestatus)
			sprintf(buff, "#PAUSE is currently ON(%g).",
			(float)((double)ses->writepause.tv_sec + (double)ses->writepause.tv_usec/1e6));
		else
			sprintf(buff, "#PAUSE is currently OFF.");
		tintin_puts(buff, ses);
	}
}   

/*************************/
/* the #slow command     */
/*************************/
/* #slow N will buffer the commands, and send at most N commands per receiving packet */
void slow_command(const char *arg0, struct session* ses)
{
	int status;
	char buff[BUFFER_SIZE];
	
	if(!ses) {
		tintin_puts("#NO SESSION ACTIVE => NO SLOW!", ses);
		return;
	}
	if(*arg0!='\0') {
		char arg[BUFFER_SIZE];
		substitute_myvars(arg0, arg, ses);
		sscanf(arg, "%d", &status);
		if (status<=0) {
			if(ses->slowstatus > 0){
				tintin_puts("#OK SLOW IS TURNED OFF", ses);    
			}
			ses->slowstatus = 0;
			ses->slowsent = 0;
			check_queue_slow(ses); /*send unsent commands out*/
		}
		else {
			if(ses->slowstatus == 0){
				sprintf(buff, "#OK SLOW is turned ON(%d).", status);
				tintin_puts(buff, ses);    
			}
			ses->slowstatus = status;
		}
	}
	else {
		if(ses->slowstatus)
			sprintf(buff, "#SLOW is currently ON(%d).", ses->slowstatus);
		else
			sprintf(buff, "#SLOW is currently OFF.");
		tintin_puts(buff, ses);
	}
}   
