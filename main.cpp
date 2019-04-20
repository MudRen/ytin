/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: main.c - main module - signal setup/shutdown etc            */
/*                             TINTIN++                              */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by peter unold 1992                     */
/*********************************************************************/

/* note: a bunch of changes were made here to add readline support -- daw */

#include <config.h>
#include "tintin.h"

#if defined(HAVE_STRING_H)
#include <string.h>
#elif defined(HAVE_STRINGS_H)
#include <strings.h>
#endif

#include <ctype.h>
#include <signal.h>
#include <fcntl.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef SOCKS
#include <socks.h>
#endif

#ifdef HAVE_TIME_H
#include <time.h>
#endif

#include "files.proto"
#include "llist.proto"
#include "main.proto"
#include "misc.proto"
#include "rl.proto"
#include "rltab.proto"
#include "utils.proto"

#ifndef BADSIG
#define BADSIG (RETSIGTYPE (*)(int))-1
#endif

/*************** globals ******************/
char GBfilter = 0;
int echo = DEFAULT_ECHO;
int ignore = 0; //controls global ignore, currently only used b4 quit yTin
int big5 = 0;
int skiptelnetseq = 0;
int speedwalk = DEFAULT_SPEEDWALK;
int togglesubs = DEFAULT_TOGGLESUBS;
int presub = DEFAULT_PRESUB;
int redraw = DEFAULT_REDRAW;
int ticker_in_split = 1;
int sessionsstarted;
int puts_echoing = TRUE;
int tcp_nodelay = TRUE; //TRUE: disable Nagle algorithm
int verbose = FALSE;
int stripcolor = 0;
int toggle_raw = 0;
int alnum = 0;
int acnum = 0;
int subnum = 0;
int varnum = 0;
int hinum = 0;
int pdnum = 0;
int antisubnum = 0;
int verbatim = 0;
int prompt_on = 0;
int funcnum = 0;
char home[BUFFER_SIZE];
char prompt_line[BUFFER_SIZE];
int newline_tag=0; /*fix for extra newlines in unsplit mode, chitchat*/

/* sth new from yTin */
#ifdef _WINDOWS
DWORD _my_tv_msec;
int buffer_view=0;
WSADATA WSAData;
BOOL started = FALSE;
int dim[2] = {192, 255};
int COLOR[8][3] = {
	{0, 0, 0},
	{1, 0, 0},
	{0, 1, 0},
	{1, 1, 0},
	{0, 0, 1},
	{1, 0, 1},
	{0, 1, 1},
	{1, 1, 1}
};
int checkInternet;
#endif
char PROMPT_FOR_PW_TEXT[80];
char TICK_PROMPT[80];
char DELIMITER[80];
char nlTail[3];

struct session *sessionlist, *activesession;
struct listnode *common_aliases, *common_actions, *common_subs, *common_myvars, *common_macros;
struct listnode *common_highs, *common_antisubs, *common_pathdirs, *common_functions;
char vars[10][BUFFER_SIZE]; /* the %0, %1, %2,....%9 variables */
char tintin_char = DEFAULT_TINTIN_CHAR;
char verbatim_char = DEFAULT_VERBATIM_CHAR;    
char system_com[80] = SYSTEM_COMMAND_DEFAULT;
char system2_com[80] = SYSTEM2_COMMAND_DEFAULT;
int mesvar[N_MESS_TYPES];
int display_row, display_col, input_row, input_col;
int split_line, term_columns;
char k_input[BUFFER_SIZE];
char done_input[BUFFER_SIZE], prev_command[BUFFER_SIZE];
int hist_num;

int is_split;

int text_came;
int time_started;
struct timeval tv_started;
int conn_has_closed=0;

#ifndef _WINDOWS
/* when the screen size changes, take note of it */
RETSIGTYPE winchhandler(int no_care)
{
	/*
	* select() will see a "syscall interrupted" error;
	* remember not to worry
	*/
	ignore_interrupt = 1;
	
	if(is_split)
		initsplit();
	//  tintin_puts("#SCREEN SIZE RESET (FROM SIGWINCH).", NULL);
	
	/* we haveta reinitialize the signals for sysv machines */
	if(signal(SIGWINCH, winchhandler) == BADSIG)
		syserr("signal SIGWINCH");
}

/* CHANGED to get rid of double-echoing bug when tintin++ gets suspended */
RETSIGTYPE tstphandler(int no_care)
{
	/* select() will see a "syscall interrupted" error; remember not to worry */
	ignore_interrupt = 1;
	
	cleanscreen();
	kill(getpid(), SIGSTOP);
	dirtyscreen();
	tintin_puts("#RETURNING BACK TO TINTIN++.", NULL);
	
	/* we haveta reinitialize the signals for sysv machines */
	if(signal(SIGTSTP, tstphandler) == BADSIG)
		syserr("signal SIGTSTP");
}

/****************************************************************************/
/* main() - show title - setup signals - init lists - readcoms - mainloop() */
/****************************************************************************/

int main(int argc, char **argv)
{
	struct session *ses = NULL;
	char *strptr, temp[BUFFER_SIZE];
	int arg_num;
	
	if(signal(SIGTERM, myquitsig) == BADSIG)
		syserr("signal SIGTERM");
	if(signal(SIGINT, myquitsig) == BADSIG)
		syserr("signal SIGINT");
	/* CHANGED to get rid of double-echoing bug when tintin++ gets suspended */
	if(signal(SIGTSTP, tstphandler) == BADSIG)
		syserr("signal SIGTSTP");
	if(signal(SIGWINCH, winchhandler) == BADSIG)
		syserr("signal SIGWINCH");
	
	arg_num = 1;
	if(argc > 1 && argv[1]) {
		if(*argv[1] == '-' && argv[1][1] == 'v') {
			arg_num = 2;
			verbose = TRUE;
		}
		else if(argv[1][0]=='-' && argv[1][1] == 'q'){
			//give quiet option, in case later want to change default verbose state, chitchat, 2/13/2000
			arg_num = 2;
			verbose = FALSE;
		}
	}
	
	if(argc > arg_num && argv[arg_num])
		tininit(argv[arg_num]);
	else
		tininit("");
	
	tintin_puts2("##################################################", ses);
	sprintf(temp,"#              T I N T I N + + %12s      #", "");
	tintin_puts2(temp, ses);
	tintin_puts2("#  (T)he k(I)cki(N) (T)ickin d(I)kumud clie(N)t  #", ses);
	tintin_puts2("#                a DIKU-mud client               #", ses);
	tintin_puts2("#     new code by Bill Reiss, David A. Wagner    #", ses);
	tintin_puts2("#        Rob Ellsworth, Jeremy C. Jack 1994,1999 #", ses);
	tintin_puts2("# thanks to Peter Unold for original TINTIN code #", ses);
	tintin_puts2("##################################################", ses);
	
	mainloop();
	return 1;
}

#endif

int read_command_file(char *name)
{
	int fd;
	char buf[BUFFER_SIZE], *cptr;
	
	if ((fd = open(name, O_RDONLY)) > 0)
	{    /* Check if it exists */
		close(fd);
		
		sprintf(buf, "Reading command file '%s'.\n\n", name);
		tintin_puts(buf, NULL);
#ifdef _WINDOWS
		while( (cptr=strchr(name, '\\')) != NULL)
			*cptr = '/'; //read_command stripes single '\\', avoid this! chitchat, 5/29/2000
#endif
		activesession = read_command(name, NULL);
		return 1;
	}
	
	return 0;
}

void tininit(char *cmdline)
{
	struct session *ses;
	char *strptr, temp[BUFFER_SIZE];
	char homepath[BUFFER_SIZE];
	int err;
	
	sessionlist = NULL;
	ses = NULL;
	text_came = FALSE;
	is_split = FALSE;
	
	strcpy(PROMPT_FOR_PW_TEXT, "assword");
	strcpy(TICK_PROMPT, "#TICK!!!");
	strcpy(DELIMITER, " "); //default delimiter string is " "
	strcpy(nlTail, "\r\n"); //default newline for write_line_mud()
	/* new with readline */
	rltab_read("", ses);	/*	Modified to allow variables in tab file names - Vasantha Crabb	*/

	*k_input = '\0';
	hist_num = -1;
#ifdef DEBUG_INIT
	tintin_puts("calling time\n", NULL);
#endif
	gettimeofday(&tv_started,0);
	time_started = time(NULL);

	srand((unsigned)time_started);
	
	
#ifdef SOCKS
	SOCKSinit(""); /* was (argv[0]); */
#elif defined(_WINDOWS) /*SOCKS none, do winsock*/
	/* initialize winsocket */
	if ((err = WSAStartup((WORD) 0x0101, &WSAData)) != 0)  /* register task with */
	{    /* winsock tcp/ip API */
			syserr("Winsock Initialization failure");
		return;
	}
	started = TRUE;
#endif
	
	common_aliases = init_list();
	common_actions = init_list();
	common_subs = init_list();
	common_myvars = init_list();
	common_highs = init_list();
	common_antisubs = init_list();
	common_pathdirs = init_list();
	common_functions = init_list();
	common_macros = init_list();
	
	mesvar[0] = DEFAULT_ALIAS_MESS;
	mesvar[1] = DEFAULT_ACTION_MESS;
	mesvar[2] = DEFAULT_SUB_MESS;
	mesvar[3] = DEFAULT_ANTISUB_MESS;
	mesvar[4] = DEFAULT_HIGHLIGHT_MESS;
	mesvar[5] = DEFAULT_VARIABLE_MESS;
	mesvar[6] = DEFAULT_PATHDIR_MESS;
	mesvar[7] = DEFAULT_READFILE_MESS;
  mesvar[8] = DEFAULT_DELIMITER_MESS;
	
	*homepath = '\0';
	if ( (strptr = getenv("HOME"))){
		strcpy(homepath, strptr);
		strcpy(home, strptr);
	}
	else {
		strcpy(homepath, DEFAULT_FILE_DIR);
#ifdef _WINDOWS
		strcpy(home, "c:");
#else
		strcpy(home, "/usr/local/share");	/*	Added so that you can't get a stupid $HOME in UN*X - Vasantha Crabb	*/
#endif
	}
	sprintf(temp, "#system variable $HOME is now set to %s !", home);
	tintin_puts(temp, NULL);
	
	if(cmdline[0] && cmdline[0] != '-')
	{
		/* Read in the file specified on the command line */
		
		if(!read_command_file(cmdline))
		{
			sprintf(temp, "Can't find command file '%s'.\n", cmdline);
			tintin_puts(temp, NULL);
		}
	}
	else
	{
		/* Try four different command files, in order. Only read
		a maximum of one */
#ifdef _WINDOWS
		strcpy(temp, "tintinrc");
#else
		strcpy(temp, ".tintinrc");
#endif
		if(!read_command_file(temp))
		{
			strcat(temp, ".txt");
			if(!read_command_file(temp) && homepath[0])
			{
				strcpy(temp, homepath);
#ifdef _WINDOWS
				strcat(temp, "\\tintinrc");
#else
				strcat(temp, "/.tintinrc");
#endif
				if(!read_command_file(temp))
				{
					strcat(temp, ".txt");
					if(!read_command_file(temp)){
						strcpy(temp, DEFAULT_FILE_DIR);
#ifdef _WINDOWS
						strcat(temp, "\\tintinrc");
#else
						strcat(temp, "/.tintinrc");
#endif
						if(!read_command_file(temp)){
							strcat(temp, ".txt");
							read_command_file(temp);
						}
					}
				}
			}
		}
	}
	version_command(NULL);
}


