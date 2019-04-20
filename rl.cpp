/* New for v2.0: readline support -- daw */

/* this is the main bunch of code for readline; lots of misc stuff here */

#include <config.h>
#include "tintin.h"

#if defined(HAVE_STRING_H)
#include <string.h>
#elif defined(HAVE_STRINGS_H)
#include <strings.h>
#endif

#include <sys/types.h>

#include <errno.h>

#ifndef _WINDOWS
#include <readline/readline.h>
#include <readline/history.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "ansi.proto"
#include "llist.proto"
#include "log.proto"
#include "main.proto"
#include "net.proto"
#include "parse.proto"
#include "rl.proto"
#include "rlhist.proto"
#include "rltab.proto"
#include "rltick.proto"
#ifndef _WINDOWS
#include "rlvt100.proto"
#include "scrsize.proto"
#endif
#include "session.proto"
#include "stream.proto"
#include "utils.proto"
#include "action.proto"
#include "macro.proto"  /* PFn key binding - vastheman 2001-08-07       */

/* this gets set when user hits control-z, so we don't worry about EINTR */
int ignore_interrupt;

/* should we display the user's commands in the top window, too? */
int am_purist = DEFAULT_PURIST_MODE;

/* the user requested this screen size -- kill autodetection code if set */
int requested_scrsize = -1;

/* essentially the size of the screen; only for split mode */
static unsigned int splitline, cols;

/* what column we were at in the top window; only for split mode */
static int topwin_col = 1;  /* VT100 has origin at 1, not 0 - vastheman 2001-07-24  */

#define ISAPROMPT(x) (1)

#ifndef _WINDOWS
/* some readline stuff */
extern int _rl_last_c_pos;
extern int _rl_eof_char;

int ctrl_r_redraw()
{
	if(is_split) {
		goto_rowcol(splitline+1, 0);
		erase_toeol();
	}
	else
		printf("\n");
	fflush(stdout);
	_rl_last_c_pos = 0;
	rl_forced_update_display();
	return 1;
}
#endif

#ifdef _WINDOWS
/***************************/
/* rec'd data handler      */
/***************************/
void tinread(SOCKET rsock)
{
	fd_set sockset;
	int rvalue = 0;
	struct session *sesptr;
	TIMEVAL mytimeval =	{0, 0};
	
	FD_ZERO(&sockset);
	
	for (sesptr = sessionlist; sesptr; sesptr = sesptr->next)
		if(sesptr->socket) //check if it is dummy session
			FD_SET(sesptr->socket, &sockset);
	
	
	rvalue = select(32, &sockset, 0, 0, &mytimeval);
	
	if (rvalue == SOCKET_ERROR)
		syserr("select");
	
	for (sesptr = sessionlist; sesptr; sesptr = sesptr->next)
		if (sesptr->socket
			&& FD_ISSET(sesptr->socket, &sockset)
			&& sesptr->socket == rsock)
	{
		readmud(sesptr);
		break;
	}

/*
	//wl somehow this got called with null activesession
	if (conn_has_closed && activesession)
	{
		cleanup_session(activesession);
//		unhook_session(activesession);
//		activesession = newactive_session();
		conn_has_closed = 0;
	}
*/
}
#else
/* for use with rl_event_hook, */
void bait()
{
	fd_set readfds;
	struct session *s;
	struct timeval to;
	int rv, maybe_redraw;
	
	FD_ZERO(&readfds);
	while(!FD_ISSET(0, &readfds)) {
		FD_ZERO(&readfds);
		FD_SET(0, &readfds); /* stdin */
		for(s = sessionlist; s; s = s->next)
			FD_SET(s->socket, &readfds);
		/*  to.tv_sec = checktick(); /* 100 */
		/*  to.tv_usec = 0;*/
		checktick(&to);
		
		ignore_interrupt = 0;
		rv = select(FD_SETSIZE, &readfds, NULL, NULL, &to);
		
		if(!rv)
			continue; /* timeout */
		else if(rv < 0 && errno == EINTR && ignore_interrupt)
			continue; /* don't worry, be happy */
		else if(rv < 0)
			syserr("select");
		
		maybe_redraw = 0;
		//use valid_session double check, since session may change in the loop
		s = sessionlist;
		while(s){ //changed to while loop, since readmud might change session
			if(s->socket && FD_ISSET(s->socket, &readfds)) { /*check dummy session*/
				if(s != readmud(s) || !valid_session(s)){
					s = sessionlist;
					maybe_redraw = 1;
					continue;
				}
				maybe_redraw = 1;
			}
			s = s->next; //if s is freed, won't come to this step
		}
		if(maybe_redraw && redraw && !is_split)
			ctrl_r_redraw();
	}
}
#endif

#ifndef _WINDOWS
void initrl(void)
{
	rl_readline_name = "tintin++";
	rl_completion_entry_function = (Function *)rltab_generate;
	using_history();
	stifle_history(HISTORY_SIZE);
	rl_variable_bind("horizontal-scroll-mode", "on");
	rl_variable_bind("blink-matching-paren", "on"); //new from readline 4.1
	rl_variable_bind("meta-flag", "on");
	rl_variable_bind("convert-meta", "off");
#if ( defined(USE_ISO_LATIN_ONE) || defined(CHINESE) )  /*moved for experimental meta-key binding - vastheman 2001-08-03	*/
	rl_variable_bind("output-meta", "on");
#endif
	rl_bind_key('\022', (Function *)ctrl_r_redraw); /* control-r */
	rl_bind_key_in_map('\022', (Function *)ctrl_r_redraw, vi_movement_keymap);
	rl_bind_key_in_map('\022', (Function *)ctrl_r_redraw, vi_insertion_keymap);
	macro_init();   /* PFn key binding - vastheman 2001-08-07       */
	rl_event_hook = (Function *)bait;
}

/* turn on split mode */
void initsplit(void)
{
	int i;
	unsigned int rows;  /* Moved here as it's not used elsewhere - vastheman 2001-07-25 */
	
	/* notice the screen size and remember it */
	scrsize(&rows, &cols);
	if(requested_scrsize > 0)       /* changed so negative split specifies input line height - vastheman 2001-07-24   */
		i = requested_scrsize + 1;
	else
		i = (int)rows + requested_scrsize;

/*	Section added to improve console resize in split mode - vastheman 2001-07-24	*/
	if (i >= (int)rows)
		splitline = rows - 1;
	else if (i < 2)
		splitline = 2;
	else
		splitline = i;
/*	End added section	*/

	is_split = 1;
	topwin_col = 1;     /* VT100 has origin at 1, not 0 - vastheman 2001-07-24  */
	reset();
	erase_screen();
	scroll_region(1, splitline-1);
	goto_rowcol(splitline, 1);      /* As above - vastheman 2001-07-24  */
	for(i = cols; i; i--)
		printf("-");
	goto_rowcol(splitline+1, 1);    /* As above - vastheman 2001-07-24  */
	save_pos();
	fflush(stdout);
}


void mainloop(void)
{
	char *line;
	
	initrl();
	
	for(;;) {
		if(!(line = readline("")))
			continue;
		if(!(line = rlhist_expand(line)))
			continue;
		
		if(is_split)
			goto_rowcol(splitline-1, topwin_col);
		
		/* commands will echo to the top screen, except for purists */
		if(is_split)
		/*	Section modified to work better with split.  Previously, linefeeds were not printed after commands - vastheman 2001-07-24	*/
			if(!am_purist && readline_echoing_p) {
				int save_tag = newline_tag;
				newline_tag = TRUE;
				printline(line, 0);
				newline_tag = save_tag;
			}
		/*	End modified section	*/
		else {
			/* should I echo newlines after prompts? */
			/* printline("", 0); */
			;
		}
		
		activesession = parse_input(line, activesession);
		
		if(is_split) {
			goto_rowcol(splitline+1, 0);
			erase_toeol();
			fflush(stdout);
		}
		
		free(line);
	}
}
#endif

/* ahh, gotta love coding around hardware bugs :-(
* [this beauty does linewrapping for terminals which
* are too dumb to do it themselves -- only needed
* for split mode, i think.]
* 
* 'str' better not contain any \n or \r's.
* if 'isaprompt' is non-zero, then we won't tack
* a \r\n to the end of 'str', and try to remember that.
*/

void printline(const char *str, int isaprompt)
{
	unsigned int pos = 0, start_pos = 0, k=0, len = topwin_col;
	char buffer[BUFFER_SIZE + 32];  /* allow escape sequence to span > two packets - vastheman 2001-07-25   */
#ifndef _WINDOWS
	char stripped[BUFFER_SIZE];	/*	Modified to allow ANSI escape sequences to be split across packets - Vasantha Crabb	*/
	static char ansifrag[32] = "";	/*	Added to allow ANSI escape sequences to be split across packets - Vasantha Crabb	*/
#endif
	
#ifdef _WINDOWS
	strncpy(buffer, str, BUFFER_SIZE);
	showstr1(buffer);
	if(newline_tag) showstr1("\n");
#else

	if (*ansifrag) {    /* allow escape sequences to span > two packets - vastheman 2001-07-25  */
		strcpy(buffer, ansifrag);
		strcat(buffer, str);
	} else
		strcpy(buffer, str);

/*	str replaced with buffer to fix wrap problems - vastheman 2001-07-25	*/
	if(is_split){
		while(buffer[pos]){
			if(!(k = skip_ansi(&buffer[pos]))) {
				pos++;
				if(++len >= cols) {
					printf("%.*s\r\n", pos-start_pos, buffer+start_pos);
					start_pos = pos;
					len = 0;
				}
			}
			else pos += k;
		}
	}
	
/*	Section added to allow ANSI escape sequences to be split across packets - Vasantha Crabb	*/
	int is_frag = 0;
	char chtemp;
	len = strlen(buffer + start_pos);
	memmove(buffer, buffer + start_pos, len + 1);   /* use memmove to deal with overlap - vastheman 2001-07-25  */
	for (k = len - 1; k && !is_frag; k--) {
		if ((buffer[k] == '[') && (buffer[k - 1] == '\x1B'))
			is_frag = 1;
		else if ((k == (len - 1)) && (buffer[k] == '\x1B')) {
			k++;
			is_frag = 1;
		} else if (((buffer[k] < '0') || (buffer[k] > '9')) && (buffer[k] != ';'))
			is_frag = -1;
	}
	if (is_frag == 1) {
		strcpy(ansifrag, &buffer[k]);
		buffer[k] = 0;
	} else
		ansifrag[0] = 0;
	strip_ansi(buffer, stripped);
/*	End added section	*/
	
	if(isaprompt && is_split) {
		printf("%s", buffer); /* don't append \r\n */	/*	Modified to allow ANSI escape sequences to be split across packets - Vasantha Crabb	*/
		topwin_col += strlen(stripped);			/*	Modified to allow ANSI escape sequences to be split across packets - Vasantha Crabb	*/
	}
	else {
		if (!newline_tag) 	topwin_col += strlen(stripped);	/*	Added to stop remote prompts being overwritten - Vasantha Crabb	*/
		else			topwin_col = 1;			/* VT100 has origin at 1, not zero - vastheman 2001-07-25   */
		printf("%s%s", buffer, newline_tag ? "\r\n" : "");	/*	Modified to allow ANSI escape sequences to be split across packets - Vasantha Crabb	*/
	}
#endif
}

/* data waiting on this mud session; read & display it; do the dirty work */
/* seriuos bug was found by DasI:                     */
/* no reaction to actions in incative sessions              */
/* fixed by DasI                             */
/* btw: i don't tested my fix very much, so it's up to you.        */
struct session *readmud(struct session* s)
{
	struct session *ses = s;
	char thebuffer[2*BUFFER_SIZE+1], *buf, *line, *next_line;
	char linebuf[BUFFER_SIZE], header[BUFFER_SIZE];
	int rv, headerlen;
	int i, dbyte;
	
	buf = thebuffer + BUFFER_SIZE;
	rv = read_buffer_mud(buf, s);
	if(!rv) {
		return cleanup_session(s);
	}
	else if(rv < 0)
		syserr("readmud: read");
	buf[++rv] = '\0';
	
	/* changed by DasI */
	if( s->snoopstatus && (s != activesession))
		sprintf(header, "%s%% ", s->name);
	else
		header[0] = '\0';
	
	headerlen = strlen(header);
	
	if(!s->lograw) logit(s, buf); /* if lograw, do it in net.c */
	
	if(s->old_more_coming) {
		line = s->last_line;
		buf -= strlen(line);
		strncpy(buf, line, strlen(line));
/*
		while(*line)
			*buf++ = *line++;
		buf -= strlen(s->last_line);
*/
		s->last_line[0] = '\0';
	}
	if(GBfilter) {
		dbyte = 0;
		for(i=0; buf[i]; i++){
			if(((unsigned char)buf[i])>=0xa0) dbyte = 1-dbyte;
			else dbyte = 0;
			if(!dbyte && buf[i]==GBfilter && i>0 && ((unsigned char)buf[i-1])>=0xa0
					&& ((unsigned char)buf[i+1])>=0xa0)
				memmove(buf+i, buf+i+1, strlen(buf));
		}
	}
	
	/* added by DasI */
	if( (s == activesession) || s->snoopstatus )
	{
#ifdef _WINDOWS
		buffer_view = 1;
#else
		if(is_split) {
			save_pos();
			goto_rowcol(splitline-1, topwin_col);
		}
		#endif
	}
	
	/* separate into lines and print away */
	for(line = buf; line && *line; line = next_line) 
	{
		if(!(next_line = strchr(line, '\n'))
				&& s->more_coming)
			break;
		if(next_line) {
			newline_tag=1; /*fix for unsplit mode, extra newlines from printline(), chitchat */
			*next_line++ = '\0';
			if(*next_line == '\r')   /* ignore \r's */
				next_line++;
		}
		else
			newline_tag=0; /*added by chitchat*/
		strncpy(linebuf, line, BUFFER_SIZE);
		
#ifndef _WINDOWS
		if (is_split && ((s == activesession) || s->snoopstatus )) {
			restore_pos();
			ses = do_one_line(linebuf, s);  /* changes linebuf */
			goto_rowcol(splitline-1, topwin_col);
		}
		else
#endif
			ses = do_one_line(linebuf, s);  /* changes linebuf */
		
		if(!valid_session(s)) {
#ifdef _WINDOWS
			flush_view();
#endif
			return ses;
		}
		/* The following lines are added by yansong chen to fix miss action bug */
		if (next_line) s->act_buf[0]='\0';
		/* End of modification by yansong chen */
		
		/* added by DasI */
		if( (s == activesession) || s->snoopstatus )
		{
// get rid of the check for ".", substitute to empty string and set newline_tag to 0 in 
//			if(strcmp(linebuf, ".")) {
				strcat(header, linebuf);
				if (prompt_on == 0) {
					printline(header, !next_line && ISAPROMPT(header));
				}
#ifndef _WINDOWS
				else if (check_status(linebuf, s) == 0)
					printline(header, !next_line && ISAPROMPT(header));
				else {
					goto_rowcol(splitline+1, topwin_col);
					save_pos();
				}
#endif
				header[headerlen] = '\0';
//			} /* if strcmp -- seems it was a dirty hack for gag, now disable it, shall improve it later, chitchat, 7/28/2000 */
		}
	}
	
	if(line && *line)
		strncpy(s->last_line, line, BUFFER_SIZE);
	
	/* added by DasI */
	if( (s == activesession) || s->snoopstatus )
	{
#ifndef _WINDOWS
		if(is_split)
			restore_pos();
		/* note: for windows, fflush redefined to flush to CTinTiew, Chitchat */
		fflush(stdout);
#else
		flush_view();
#endif
	}

	return ses;
	/* q: do we need to do some sort of redraw here? i don't think so */
	/* a: no, i do it at the end of mainloop() [for now] */
}


/*
* output to screen should go through this function
* the output is NOT checked for actions or anything
*/
void tintin_puts2(const char *cptr, struct session* ses)
{
	/* The following line add by yansong chen to fix miss action bug */
	if (valid_session(ses)) ses->act_buf[0] = '\0'; //6/1/2000, valid_session here
	/* End of modification by yansong chen */
	if((ses && ses != activesession) || !puts_echoing)
		return;
#ifndef _WINDOWS
	if(is_split) {
		save_pos();
		goto_rowcol(splitline-1, topwin_col);
	}
#endif
	newline_tag = 1; /*newline, chitchat*/
	printline(cptr, 0);
	
#ifndef _WINDOWS
	if(is_split)
		restore_pos();
	// fflush(stdout);
	
	/* q: do we need to do some sort of redraw here? i don't think so */
	/* a: right now, i think so */
	if(redraw && !is_split)
		ctrl_r_redraw();
#endif
}

/*
* output to screen should go through this function
* the output IS treated as though it came from the mud
* avoid use it for ses == NULL unless really want to, 2/5/2001, chitchat
*/
struct session* tintin_puts(const char *cptr, struct session* ses)
{
	struct session *s0, *s;
	char buf[BUFFER_SIZE];
	strcpy(buf, cptr);

	if(ses) {
		ses->act_buf[0] = '\0';
		s0 = do_one_line(buf, ses);
		if(*buf && valid_session(ses))
			tintin_puts2(buf, ses);
	}
	else {
		s = sessionlist;
		while(s){ //changed to while loop, since readmud might change session
			if(!valid_session(s)){
				s = sessionlist;
				continue;
			}
			s0 = do_one_line(buf, s);
			s = s->next; //if s is freed, won't come to this step
		}
		tintin_puts2(cptr, ses);
	}
	return s;
}

/* quit tintin++ and print a message */
void quitmsg(const char *m)
{
#ifndef _WINDOWS
	tinquit();
	cleanscreen();
	if(m)
		printf("%s\n", m);
	printf("\nGoodbye from tintin++.\n");
	exit(0);
#else
//	tintin_puts2("Use windows GUI to quit nicely, do not #end under windows!", NULL);
//need figure out how to quit nicely, now either have mem leak or crash depends on the debug/release mode
	closemainwindow();
#endif
}

#ifndef _WINDOWS
/* get a clean screen without the split crap; useful for ^Z, quitting, etc */
void cleanscreen(void)
{
	system("stty echo"); /* a hack, admittedly */
	
	if(!is_split)
		return;
	scroll_region(1, splitline+1);
	erase_screen();
	reset();
	fflush(stdout);
}

/* undo cleanscreen(); useful after ^Z */
void dirtyscreen(void)
{
	if(is_split)
		initsplit();
}

/* quit tintin++ fast! for use with signal() */
RETSIGTYPE myquitsig(int no_care)
{
	quitmsg(NULL);
}

/* Put the tickcounter in the splitline */
int tickcounter_in_splitline(int ttt)
{
	if(is_split) {
		save_pos();
		goto_rowcol(splitline, cols-5);
		if(ttt >= 0)
			printf("%d--", ttt);
		else
			printf("---");
		restore_pos();
		fflush(stdout);
		return(ttt >= 0);
	}
	
	return(0);
}


/* Put the status line from a mud in the splitline */
int status_in_splitline(const char *arg, struct session* ses)
{
	char buf[BUFFER_SIZE];
	
	if(is_split) {
		save_pos();
		strcpy(buf, arg);
		
		if (strlen(buf)+6 < cols) {
			goto_rowcol(splitline, 1);
			printf("%s", buf);
			restore_pos();
			fflush(stdout);
			return(0);
		}
	}
	return(0);
}
/***********************************************/
/* the checking to print prompt on status line */
/***********************************************/
int check_status(const char *buf, struct session* ses)
{
	
	if (check_one_action(buf, prompt_line, ses) & (is_split)) { 
		status_in_splitline(buf, ses);
		return(1);
	}
	return(0);
}
#endif

/**********************/
/* the #split command */
/**********************/

/*
* if the user requests a certain screen size, who are we
* to autodetect things and ignore his request? :-)
*/
void split_command(const char *arg)
{
	#ifdef _WINDOWS
	tintin_puts2("#We do not need split in Windows!", NULL);
	#else
	char left[BUFFER_SIZE];
	
	if(is_split) {
		cleanscreen();
		is_split = 0;
	}
	
	arg = get_arg_in_braces(arg, left, 0);
	if(!*left)   /* Changed to allow split to be specified as lines from bottom of screen - vastheman 2001-07-24    */
		requested_scrsize = -2;
	else
		requested_scrsize = atoi(left);
		//requested_scrsize = -1;
	
	initsplit();
	#endif
}

/************************/
/* the #unsplit command */
/************************/

void unsplit_command(void)
{
#ifdef _WINDOWS
	tintin_puts2("#We do not need unsplit in Windows!", NULL);
#else
	requested_scrsize = -1;
	cleanscreen();
	is_split = 0;
#endif
}

/***********************/
/* the #purist command */
/***********************/

void purist_command(void)
{
	tintin_puts("#Ok, purist mode enabled.", NULL);
	am_purist = 1;
}

/*************************/
/* the #unpurist command */
/*************************/

void unpurist_command(void)
{
	tintin_puts("#Ok, purist mode disabled.", NULL);
	am_purist = 0;
}

/***************************/
/* quit handler            */
/***************************/
void unhook_session(struct session* ses)
{
	struct session *sesptr, **lastptr;
	
	sesptr = sessionlist;
	lastptr = &sessionlist;
	
	while(sesptr)
	{
		if(sesptr == ses)
		{
			*lastptr = sesptr->next;
			sesptr = sessionlist;
			lastptr = &sessionlist;
		}
		else
		{
			lastptr = &(sesptr->next);
			sesptr = sesptr->next;
		}
	}
}

void tinquit()
{
	struct session* ses;
	ignore = 1; //avoid infinite loops due to some session die messages
	while( valid_session(ses=sessionlist) ){
		cleanup_session(ses);
	}
	cleanup_session(NULL);
	clean_all_file();
	
#ifdef _WINDOWS
	WSACleanup();
#endif
}
