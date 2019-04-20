/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: session.c.c - funtions related to sessions                  */
/*                             TINTIN III                            */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by peter unold 1992                     */
/*********************************************************************/

#include <config.h>
#include "tintin.h"

#if defined(HAVE_STRING_H)
#include <string.h>
#elif defined(HAVE_STRINGS_H)
#include <strings.h>
#endif

#include <ctype.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "llist.proto"
#include "main.proto"
#include "net.proto"
#include "parse.proto"
#include "queue.proto"
#include "rl.proto"
#include "session.proto"
#include "ticks.proto"
#include "utils.proto"
#include "variables.proto"

/************************/
/* the #session command */
/************************/

struct session *session_command(const char *arg, struct session* ses)
{
	char left[BUFFER_SIZE], right[BUFFER_SIZE], arg0[BUFFER_SIZE];
	struct session *sesptr;

	substitute_myvars(arg, arg0, ses);
	arg = get_arg_in_braces(arg0, left, 0);
	arg = get_arg_in_braces(arg, right, 1);

	if(!*left) {
		tintin_puts("#THESE SESSIONS HAS BEEN DEFINED:", ses);
		for(sesptr = sessionlist; sesptr; sesptr = sesptr->next)
			show_session(sesptr, ses);
		prompt(ses);
	}
	else if(*left && !*right) {
		for(sesptr = sessionlist; sesptr; sesptr = sesptr->next)
			if(!strcmp(sesptr->name, left)) {
			show_session(sesptr, ses);
			break;
		}
		if(!sesptr) {
			sprintf(right, "#SESSION %s IS NOT DEFINED.", left);
			tintin_puts(right, ses);
			prompt(NULL);
		}
	}
	else {
		for(sesptr = sessionlist; sesptr; sesptr = sesptr->next)
			if(!strcmp(sesptr->name, left)) {
			sprintf(right, "#THERE'S A SESSION WITH THE NAME %s ALREADY.", left);
			tintin_puts(right, ses);
			prompt(NULL);
			return(ses);
		}
		ses = new_session(left, right, ses);
	}

	return(ses);
}


/******************/
/* show a session */
/******************/

void show_session(struct session *ses, struct session* active_ses)
{
	char temp[BUFFER_SIZE];

	sprintf(temp, "%-10s%s", ses->name, ses->address);

	if(ses == activesession)
		strcat(temp, " (active)");
	if(ses->snoopstatus)
		strcat(temp, " (snooped)");
	if(ses->logfiles)
		strcat(temp, " (logging)");
	tintin_puts2(temp, active_ses);
	prompt(NULL);
}

/**********************************/
/* find a new session to activate */
/**********************************/

struct session *newactive_session(void)
{
	if(sessionlist) {
		char buf[BUFFER_SIZE];

		activesession = sessionlist;
		sprintf(buf, "#SESSION '%s' ACTIVATED.", sessionlist->name);
		tintin_puts(buf, NULL);
#ifdef _WINDOWS
		setTitle(activesession->name);
#endif
	}
	else {
		activesession = NULL;
		tintin_puts("#THERE'S NO ACTIVE SESSION NOW.", NULL);
	}
	prompt(NULL);
	return(sessionlist);
}

/**********************/
/* open a new session */
/**********************/

struct session *new_session(const char *name, const char *address, struct session* ses)
{
	int i=0, sock;
	char *host, *port;
	struct session *newsession;
/*
#ifdef _WINDOWS
	char temp[BUFFER_SIZE];
	int b1, b2;
	DWORD dwFlag=0, dwBytes=0, dwConns=0;
	RASCONN RasConn;
	RasConn.dwSize = sizeof(RASCONN);
#endif
/* get rid of this RAS and WININET from 1.80n1 now, 7/19/2000*/
	port = host = mystrdup(space_out(address)); //only free host later

	if(!*host) {
		tintin_puts("#HEY! SPECIFY AN ADDRESS WILL YOU?", ses);
		free(host);
		return(ses);
	}

	while(*port && *port!=' ' && *port!='\t')
		port++;
	if(*port) {//yep, we stop at space/tab here
		*port++ = '\0';
		port = (char *)space_out(port);
	}
	else i=1; //no port specified

	if(i || !*port) {
		tintin_puts("#HEY! SPECIFY A PORT NUMBER WILL YOU?", ses);
		free(host);
		return(ses);
	}

	if(*port=='0' && port[1]==0){ /*port=0 -> dummy session*/
		sock = 0;
	}
/*
#ifdef _WINDOWS
	else if(checkInternet && (!(b1=InternetGetConnectedState(&dwFlag, 0))
		  || ((dwFlag&INTERNET_RAS_INSTALLED && !(dwFlag&INTERNET_CONNECTION_LAN)) && //upgrade with Plateform SDK for new wininet.h et al !!!
			((b2=RasEnumConnections(&RasConn, &dwBytes, &dwConns))==-1 || dwConns<1)))){ //dunno why, BUFFER_TOO_SMALL occurs and shall be OK
		char temp[BUFFER_SIZE];
		tintin_puts("#No internet connection! (u can toggle this check in Options menu.)", ses);
		sprintf(temp, "#debug: InternetGetConnectedState=%d (requires IE4+), dwFlag=%d (INTERNET_CONNECTION_MODEM:%d, INTERNET_CONNECTION_LAN:%d, INTERNET_CONNECTION_PROXY:%d, INTERNET_RAS_INSTALLED:%d),\
RasEnumConnections=%d, dwConns=%d", b1, dwFlag, dwFlag&INTERNET_CONNECTION_MODEM, dwFlag&INTERNET_CONNECTION_LAN, dwFlag&INTERNET_CONNECTION_PROXY, INTERNET_RAS_INSTALLED, b2, dwConns);
		tintin_puts(temp, ses);
		free(host);
		return(ses);
	}
#endif
*/
	else if(!(sock = connect_mud(host, port, ses))) {
		free(host);
		return(ses);
	}

	newsession = (struct session *)malloc(sizeof(struct session));
	newsession->name = mystrdup(name);
	newsession->address = mystrdup(address);
	newsession->tickstatus = FALSE;
	newsession->decrypt = 0;
	newsession->ping2 = 0;

	newsession->ticksize.tv_sec=tick_size;
	newsession->ticksize.tv_usec=0;
	newsession->tick=0;
	newsession->tickwarn=0;
	newsession->pausestatus=FALSE;
	newsession->lastwrite.tv_sec = 0;

	newsession->remote_echo = 0;
	newsession->slowstatus = 0;
	newsession->slowsent = 0;
	newsession->slow = NULL;

	newsession->snoopstatus = FALSE;
	newsession->logfiles = NULL;
	newsession->lograw = 0;
	newsession->ignore = DEFAULT_IGNORE;
	newsession->aliases = copy_list(common_aliases, ALPHA);
	newsession->actions = copy_list(common_actions, PRIORITY);
	newsession->subs = copy_list(common_subs, ALPHA);
	newsession->myvars = copy_list(common_myvars, ALPHA);
	newsession->highs = copy_list(common_highs, ALPHA);
	newsession->myfuncs = copy_list(common_functions, ALPHA);
#ifndef _WINDOWS
	newsession->macros = copy_list(common_macros, ALPHA);   /* meta-key binding - vastheman 2001-08-07      */
#endif
	newsession->pathdirs = copy_list(common_pathdirs, ALPHA);
	newsession->socket = sock;
	newsession->antisubs = copy_list(common_antisubs, ALPHA);
	newsession->socketbit = 1<<sock;
	newsession->next = sessionlist;
	for(i = 0; i < HISTORY_SIZE; i++)
		newsession->history[i] = NULL;
	newsession->path = init_list();
	newsession->path_list_size = 0;
	newsession->path_length = 0;
	newsession->more_coming = 0;
	newsession->old_more_coming = 0;
	newsession->last_line[BUFFER_SIZE] = 0;
	/* The following line added by yansong chen to fix miss action bug */
	newsession->act_buf[0]='\0';
	/* End of modification by yansong chen */
	sessionlist = newsession;
	activesession = newsession;
	sessionsstarted++;

	if(!sock) { /*dummy session*/
		if( (port=strchr(host, ' ')) )
			*port=0;
		tintin_puts(host, newsession); /*print out the string host as message*/
	}

	free(host);
#ifdef _WINDOWS
	setTitle(newsession->name);
#endif
	return(newsession);
}

/*****************************************************************************/
/* cleanup after session died. if session=activesession, try find new active */
/*****************************************************************************/

struct session *cleanup_session(struct session* ses)
{
	int i;
	char buf[BUFFER_SIZE];
	struct session *sesptr, *s=ses;

	kill_all_event(ses);
	sessionsstarted--;
	kill_all(ses, END);

	if(!valid_session(ses))	return NULL; //chitchat

	/* printf("DEBUG: Hist: %d \n\r",HISTORY_SIZE); */
	/* CHANGED to fix a possible memory leak
	for(i = 0; i < HISTORY_SIZE; i++)
		ses->history[i] = NULL;
	*/
	for(i = 0; i < HISTORY_SIZE; i++)
		if(ses->history[i])
		free(ses->history[i]);
	sprintf(buf, "#SESSION '%s' DIED.", ses->name);
	/*  if(write(ses->socket, "ctld\n", 5) < 5)
	syserr("write in cleanup"); */    /* can't do this, cozof the peer stuff in net.c */
#ifdef _WINDOWS
	closesocket(ses->socket);
#else
	if(ses->socket && close(ses->socket) == -1) /*handle dummy session*/
		syserr("close in cleanup");
#endif

  if(ses->logfiles){
		struct log_list *ptnext, *pt = ses->logfiles;
		do {
			fclose(pt->hd);
			ptnext = pt->next;
			free(pt);
			pt = ptnext;
		} while(pt);
    ses->logfiles=NULL;
	}

	free(ses->address);
	free(ses->name);
	if(ses == sessionlist)
		sessionlist=ses->next;
	else {
		for(sesptr = sessionlist; sesptr->next != ses; sesptr = sesptr->next);
		sesptr->next = ses->next;
	}
	tintin_puts(buf, NULL);
	if(ses == activesession)
		s = newactive_session();
	free(ses);
	return s;
}

/************************************************************/
/* check if the session is already dead from other routines */
/* fix for crashes, chitchat, 1/18/2000                     */
/************************************************************/
int valid_session(struct session* ses)
{
	struct session* sesptr;
	for(sesptr = sessionlist; sesptr; sesptr = sesptr->next) {
		if(sesptr == ses)
			return 1;
	}
	return 0;
}
