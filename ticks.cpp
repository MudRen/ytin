/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: ticks.c - functions for the ticker stuff          */
/*               TINTIN III              */
/*     (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t       */
/*           coded by peter unold 1992           */
/*********************************************************************/

#include <config.h>
#include "tintin.h"

#if defined(HAVE_STRING_H)
#include <string.h>
#elif defined(HAVE_STRINGS_H)
#include <strings.h>
#endif

#include <ctype.h>
#include <signal.h>

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
#define BADSIG (RETSIGTYPE (*)(int))-1
#endif

#include "queue.proto"
#include "rl.proto"
#include "rltick.proto"
#include "ticks.proto"
#include "variables.proto"

/* local globals */
int time0 = -1, tick_size = 75;

/*********************************************************************/
/* Add by yansong chen(YChen@uh.edu) to reimplement per-session tick */
/*********************************************************************/

void tickreset(struct session* ses)
{
	struct timeval now;
	gettimeofday(&now, 0);
	/* printf("tickreset:tick=%d\n", ses->tick);
	printf("tickreset:tickwarn=%d\n", ses->tickwarn);*/
	if (ses->tick) {remove_queue(ses->tick); ses->tick = 0;}
	if (ses->tickwarn) {remove_queue(ses->tickwarn); ses->tickwarn = 0;}
	/* printf("tickreset:OK\n");*/
	if (!ses->tickstatus) return;
	if (ses->ticksize.tv_sec > 10) {
		struct timeval tickwarn;
		tickwarn.tv_sec = now.tv_sec + ses->ticksize.tv_sec - 10;
		tickwarn.tv_usec = now.tv_usec;
		add_queue(ses, EVENT_TICKWARN, &tickwarn, 0);
	}
	ses->nexttick.tv_sec = now.tv_sec + ses->ticksize.tv_sec;
	ses->nexttick.tv_usec = now.tv_usec;
	add_queue(ses, EVENT_TICK, &(ses->nexttick), 0);
}

/*********************/
/* the #tick command */
/*********************/

void tick_command(struct session* ses)
{
	if(ses)
		if(ses->tickstatus) {
		char buf[BUFFER_SIZE];
		
		sprintf(buf, "#THERE'S ABOUT %d SECONDS TO THE TICK.", timetilltick(ses));
		tintin_puts(buf, ses);
	}
	else
		tintin_puts("#MY TICKER IS OFF! DUNNO SECONDS TO TICK", ses);
	else
		tintin_puts("#NO SESSION ACTIVE => NO TICKER!", ses);
}

/************************/
/* the #tickoff command */
/************************/

void tickoff_command(struct session* ses)
{
	if(ses) {
		ses->tickstatus = FALSE;
		tickreset(ses);
		tintin_puts("#TICKER IS NOW OFF.", ses);
	}
	else
		tintin_puts("#NO SESSION ACTIVE => NO TICKER!", ses);
}

/***********************/
/* the #tickon command */
/***********************/

void tickon_command(struct session* ses)
{
	if(ses) {
		ses->tickstatus = TRUE;
		/*  if (time0 < 0)
		time0 = time(NULL); */
		tickreset(ses);     /* Add by yansong chen(YChen@uh.edu) */
		tintin_puts("#TICKER IS NOW ON.", ses);
	}
	else
		tintin_puts("#NO SESSION ACTIVE => NO TICKER!", ses);
}

/************************/
/* the #tickset command */
/************************/

void tickset_command(struct session* ses)
{
	if(ses) 
		/*  time0=time(NULL); we don't prompt! too many ticksets... */
		tickreset(ses);
	else
		tintin_puts("#NO SESSION ACTIVE => NO TICKER!", ses);
}

/*************************/
/* the #ticksize command */
/*************************/

void ticksize_command(const char *arg, struct session* ses)
{
	int tsize;
	char buff[BUFFER_SIZE];
	if(*arg){
		substitute_myvars(arg, buff, ses); /*added by chitchat*/
		if(isdigit(*buff)) {
			tsize = atoi(buff);
			if (ses) {
				ses->ticksize.tv_sec=tsize;
				tickreset(ses);
			}
			else
				tick_size = tsize;
			tintin_puts("#OK NEW TICKSIZE SET", ses);    
		}
		else
			tintin_puts("#SPECIFY A NUMBER!!!! TRYING TO CRASH ME, EH?", ses);
	}
	else
		tintin_puts("#SET THE TICK-SIZE TO WHAT?", ses);
}   
