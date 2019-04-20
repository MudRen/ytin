/*********************************************************************/
/* file: tintin.h - the include file for tintin++                    */
/*                             TINTIN ++                             */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                    modified by Bill Reiss 1993                    */
/*                     coded by peter unold 1992                     */
/*********************************************************************/
/* NOTE: get rid of the DEVELOPMENT warning in main() when you update this! */
#include <stdio.h>

#if (defined(_DEBUG) && !defined(_UNICODE))
	#define _CRTDBG_MAP_ALLOC // include Microsoft memory leak detection procedures
	#define _INC_MALLOC       // exclude standard memory alloc procedures
	#include <stdlib.h>
	#include <crtdbg.h>
#endif

#ifdef _WINDOWS
#include <afxwin.h>
#include <afxsock.h>
void showstr1(char *str);
void showstr(TCHAR *str);
void flush_view();
void setprompt(char* str);
void winsockerrmsg(int nError, char* strErr);
void closemainwindow();
void tinchar(unsigned);
int tinspecchar(char *);
void hidePasswd(int hide);
HWND getmainwnd();

extern int dim[2];
extern int COLOR[8][3];
extern int checkInternet;
extern TCHAR szPerl[MAX_PATH];

#else //UNIX
#include <sys/time.h>
#endif

#ifdef HAVE_TIME_H
#include <time.h>
#endif

/************************/
/* The meaning of life: */
/************************/
#define TRUE 1
#define FALSE 0

/******* Is_Alnum() means isalnum || '_', but isalnum() itself is language dependent *****/
/******* so we redefine it as one other macro, chitchat, 1/19/2000 ***********************/
extern char GBfilter;
#define Is_Alnum(c) (((c)>='a' && (c)<='z') || ((c)>='A' && (c)<='Z') || (c)=='_' || ((c)>='0' && (c)<='9'))

/***********************************************/
/* Some default values you might wanna change: */
/***********************************************/
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 24
#define ALPHA 1
#define PRIORITY 0
#define CLEAN 0
#define END 1
#define OLD_LOG 0 /* set to one to use old-style logging */
#define DEFAULT_PURIST_MODE 0 /* purity and chastity are overrated */
#define DEFAULT_OPEN '{' /*character that starts an argument */
#define DEFAULT_CLOSE '}' /*character that ends an argument */
#define SYSTEM_COMMAND_DEFAULT "system"   /* name of the system command */
#define SYSTEM2_COMMAND_DEFAULT "system2"   /* name of the system command */
#define HISTORY_SIZE 30                   /* history size */
#define MAX_PATH_LENGTH 200               /* max path lenght */
#define DEFAULT_TINTIN_CHAR '#'           /* tintin char */
#define DEFAULT_VERBATIM_CHAR '\\'        /* if an input starts with this
                                             char, it will be sent 'as is'
                                             to the MUD */
#ifndef DEFAULT_FILE_DIR
#ifdef _WINDOWS
#define DEFAULT_FILE_DIR "\\program files\\ytin"
#else
#define DEFAULT_FILE_DIR "/usr/local/lib/tintin" /* Path to Tintin files, or HOME */
#endif
#endif

#ifndef DEFAULT_RESOURCE_FILE
#define DEFAULT_RESOURCE_FILE ".tintinrc" /* Default name for the resource file */
#endif

#ifndef DEFAULT_HELP_FILE
#ifndef _WINDOWS
#define DEFAULT_HELP_FILE ".tt_help.txt" /* Default name for the help file */
#else
#define DEFAULT_HELP_FILE "tt_help.txt"
#endif
#endif

#define DEFAULT_CAT_STR "cat "                 /* for unix:     cat */
#define DEFAULT_COMPRESS_EXT ".Z"              /* for compress: .Z */
#define DEFAULT_UNCOMPRESS_STR "uncompress -c "/* for compress: uncompress -c */
#define DEFAULT_GZIP_EXT ".gz"                 /* for gzip:     .gz */
#define DEFAULT_GUNZIP_STR "gunzip -c "        /* for gzip:     gunzip -c */

#define DEFAULT_ECHO FALSE                /* echo */         
#define DEFAULT_IGNORE FALSE              /* ignore */
#define DEFAULT_SPEEDWALK TRUE           /* speedwalk */
#define DEFAULT_PRESUB TRUE               /* presub before actions */
#define DEFAULT_TOGGLESUBS FALSE          /* turn subs on and off FALSE=ON*/

#define DEFAULT_ALIAS_MESS TRUE           /* messages for responses */
#define DEFAULT_ACTION_MESS TRUE          /* when setting/deleting aliases, */
#define DEFAULT_SUB_MESS TRUE             /* actions, etc. may be set to */
#define DEFAULT_ANTISUB_MESS TRUE         /* default either on or off */
#define DEFAULT_HIGHLIGHT_MESS TRUE       /* TRUE=ON FALSE=OFF */
#define DEFAULT_VARIABLE_MESS TRUE        /* might want to turn off these */
#define DEFAULT_DELIMITER_MESS TRUE       /* new from 1.83c3 */
#define DEFAULT_PATHDIR_MESS TRUE
#define DEFAULT_READFILE_MESS 1 /*2 is the regular old style, 1 is brief, 0 is no message*/
#define DEFAULT_REDRAW FALSE

/**************************************************************************/
/* Whenever TINTIN has written something to the screen, the program sends */
/* a CR/LF to the diku to force a new prompt to appear. You can have      */
/* TINTIN print it's own pseudo prompt instead.                           */
/**************************************************************************/
/*
 * new for readline support -- this #define is ignored.
 * there's no more pseudo-prompting, it drives me haywire
 * (trying to code it).  -- daw
 */
#define PSEUDO_PROMPT TRUE

/**************************************************************************/
/* the codes below are used for highlighting text, and is set for the     */
/* codes for VT-100 terminal emulation. If you are using a different      */
/* teminal type, replace the codes below with the correct codes and       */
/* change the codes set up in highlight.c                                 */
/**************************************************************************/
#define DEFAULT_BEGIN_COLOR "\x1B["     /* Modified to work with more text    */
#define DEFAULT_END_COLOR "\x1B[m"      /* editors - vastheman 2001-07-23     */

/*************************************************************************/
/* The text below is checked for. If it trickers then echo is turned off */
/* echo is turned back on the next time the user types a return          */
/*************************************************************************/
//moved to main.c as char*, chitchat, 11/09/1999
extern char PROMPT_FOR_PW_TEXT[];
extern char TICK_PROMPT[];

extern char DELIMITER[];
extern char nlTail[];

extern int newline_tag;
extern int ignore;

/**************************************************************************/ 
/* The stuff below here shouldn't be modified unless you know what you're */
/* doing........                                                          */
/**************************************************************************/ 

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 8192
#endif

/************************ structures *********************/

struct listnode {
  struct listnode *next;
  char *left, *right, *pr;
};

struct log_list { /* now log files can be as many as you want */
  struct log_list *next;
  char filename[256];
  FILE* hd;
};

struct s_list { /*simple list, renamed from completelist, for more general use */
  struct s_list *next;
  char *strng;
};

struct int_list { /*list for long int, for general use */
  struct int_list *next;
  unsigned long l;
};

struct event;			/* Add by yansong chen(YChen@uh.edu)	*/
#define EVENT_WRITE    0x00
#define EVENT_SLEEP    0x01 //default purge mode is 1
#define EVENT_SLEEP2   0x02
#define EVENT_TICK 	   0x10	/*					*/
#define EVENT_TICKWARN 0x20	/*					*/

struct keytype {
	unsigned int wParam;
	unsigned int lParam;
};

struct session {
  struct session *next;
  char *name;
  char *address;
  int tickstatus;
	int decrypt;
	unsigned long ping2;
  
	struct timeval ticksize;	/* Add by yansong chen(YChen@uh.edu)	*/
  struct timeval nexttick;	/* To reimplement per-session ticker	*/
  struct timeval writepause;
  struct timeval lastwrite;

  struct event *tick;		/* and input delay.			*/
  struct event *tickwarn;	/*					*/
  int pausestatus;
  int snoopstatus;
	int slowstatus;
	int slowsent;
	int remote_echo; /* new: if true, will disable local_echo */
	int lograw; /*decide if will log raw telnet escape as well */
  int ignore;
  struct log_list *logfiles;
  struct listnode *aliases, *actions, *subs, *myvars, *highs, *antisubs, *myfuncs;
#ifndef _WINDOWS            /* Changed by vastheman - 2001-08-07    */
  struct listnode *macros;  /* Experimental meta key mapping        */
#endif
  struct s_list *slow;
  char *history[HISTORY_SIZE];
  struct listnode *path, *pathdirs;
  int path_length, path_list_size;
  unsigned int socket, socketbit;
  int old_more_coming,more_coming;
  char last_line[BUFFER_SIZE+1];
	/* The following line add by yansong chen to fix miss action bug */
	char act_buf[BUFFER_SIZE+1];
	/* End of modification by yansong chen */
};

/* Add by yansong chen (YChen@uh.edu)	*/
struct event {
  struct session *ses;
  int type;
  struct timeval tm;
	char* line;
  struct event *next, *prev;
};

