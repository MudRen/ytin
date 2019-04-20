/* moved version_num to here, since it is frequently changed, chitchat, 1/18/2000 */
#define VERSION_NUM "1.83d1"

#ifdef _UNICODE
#define UNIANSI "Unicode"
#else
#define UNIANSI "Ansi"
#endif

#include <config.h>
#include "tintin.h"

#ifdef TIME_WITH_SYS_TIME
#include "sys/time.h"
#endif

#ifdef HAVE_UNISTD_H
#include "unistd.h"
#endif

#include <ctype.h>
#if defined(HAVE_STRING_H)
#include <string.h>
#elif defined(HAVE_STRINGS_H)
#include <strings.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "action.proto"
#include "ansi.proto"
#include "ivars.proto"
#include "llist.proto"
#include "main.proto"
#include "misc.proto"
#include "net.proto"
#include "parse.proto"
#include "queue.proto"
#include "rl.proto"
#include "session.proto"
#include "utils.proto"
#include "uuencode.proto"
#include "variables.proto"

/****************************/
/* the version command      */
/****************************/

void version_command(char* buff)
{ 
  char temp[BUFFER_SIZE];

	if(!buff){
		if(activesession) {
			activesession->remote_echo = 0;
#ifndef _WINDOWS
			readline_echoing_p = 1; // this tells readline to echo again
#else
			hidePasswd(0);
#endif
			activesession->decrypt++; //forbidden use of #decrypt command after login
			send_termtype(activesession);
		}
		sprintf(temp, "#You are using yTin \b%s (%s)!\n", VERSION_NUM, UNIANSI);
	  tintin_puts2(temp, NULL);
	  prompt(NULL);
	}
	else
		sprintf(buff, "yTin \b%s (%s)", VERSION_NUM, UNIANSI);
} 

/****************************/
/* the cr command           */
/****************************/
void cr_command(const char* arg, struct session* ses)
{
	char *newline0[] = {"\n", "\r", "\r\n", "\n\r"};
	char *newline1[] = {"\\n", "\\r", "\\r\\n", "\\n\\r"};
	char temp[BUFFER_SIZE], temp2[BUFFER_SIZE];
	int i;
	if(arg && *arg) {
		if(arg[0]>='0' && arg[0]<='3' && arg[1]==0){
			i = atoi(arg);
			strcpy(nlTail, newline0[i]);
			sprintf(temp, "#TINTIN newline is now \"%s\" !", newline1[i]);
			tintin_puts(temp, ses);
		}
		else {
			strcpy(temp, "#Available options are: ");
			for(i=0; i<4; i++) {
				sprintf(temp2, "%d(\"%s\") ", i, newline1[i]);
				strcat(temp, temp2);
				if(strcmp(newline0[i], nlTail)==0){
					sprintf(temp2, "#TINTIN newline is now \"%s\" !", newline1[i]);
					tintin_puts(temp2, ses);
				}
			}
			strcat(temp, ".");
			tintin_puts(temp, ses);
		}
	}
  else if(ses)
    write_line_mud("", ses);
}

/**************************************************************/
/* the ping2 command                                          */
/* pseudo ping by sending TEL_DO TIMING telnet sequence       */
/**************************************************************/
void ping2_command(const char* arg, struct session* ses)
{
	if(!ses || !ses->socket){
		tintin_puts("#ERROR: ping2 only works for real session!", ses);
		return;
	}
	if(arg[0]=='0' && arg[1]==0){
		tintin_puts("#PING2 reset by user, timing may be incorrect!", ses);
		ses->ping2 = 0;
		return;
	}
	if(ses->ping2){
		tintin_puts("#ERROR: still waiting for the last response! use #ping2 0 to reset.", ses);
		return;
	}
	ses->ping2 = millisec_started();
	send_telnet(TEL_DO, TIMING, ses);
//	send_telnet(TEL_DO, ECHO, ses);
	tintin_puts("#SENDING TELNET SEQUENCE TO HOST, WAITING ...", ses);
}
/**************************************************************/
/* the telnet command, send 1 byte from 241-249               */
/* NOP, DM, BREAK, IP, AO, AYT, EC, EL, GOAHEAD               */
/**************************************************************/
void telnet_command(const char* arg, struct session* ses)
{
	char byte;
	char temp[BUFFER_SIZE], buff[BUFFER_SIZE], *pt;
	if(!ses || !ses->socket){
		tintin_puts("#ERROR: ping2 only works for real session!", ses);
		return;
	}
	get_arg_in_braces(arg, temp, 1);
	substitute_myvars(temp, buff, ses);
	byte = atoi(buff);
	if(!*buff || *buff=='?'){
		tintin_puts2("#TELNET: send these telnet sequence to host (241-249): NOP, DM, BREAK, IP, AO, AYT, EC, EL, GOAHEAD", ses);
		return;
	}
	if(byte==0){
		for(pt=buff; *pt; pt++) *pt = toupper(*pt);
		if(!strcmp(buff, "NOP")) byte = NOP;
		else if(!strcmp(buff, "DM")) byte = DM;
		else if(!strcmp(buff, "BREAK")) byte = BREAK;
		else if(!strcmp(buff, "IP")) byte = IP;
		else if(!strcmp(buff, "AO")) byte = AO;
		else if(!strcmp(buff, "AYT")) byte = AYT;
		else if(!strcmp(buff, "EC")) byte = EC;
		else if(!strcmp(buff, "EL")) byte = EL;
		else if(!strcmp(buff, "GOAHEAD")) byte = GOAHEAD;
		else {
			tintin_puts2("#TELNET: send these telnet sequence to host (241-249): NOP, DM, BREAK, IP, AO, AYT, EC, EL, GOAHEAD", ses);
			return;
		}
	}

	send_telnet(byte, 0, ses);
	sprintf(temp, "#SENDING TELNET SEQUENCE %d TO HOST, WAITING ...", (unsigned char)byte);
	tintin_puts(temp, ses);
}
/**************************************************************/
/* the raw command, send bytes without newline                */
/**************************************************************/
void raw_command(const char* arg, struct session* ses)
{
	char temp[BUFFER_SIZE], buff[BUFFER_SIZE];

	get_arg_in_braces(arg, temp, 1);
	substitute_myvars(temp, buff, ses);
	write_raw_mud(buff, strlen(buff), ses);
}
/****************************/
/* the verbatim command,    */
/* used as a toggle         */
/****************************/

void verbatim_command(char* arg)
{
	if(*arg != '\0')
		verbatim = atoi(arg);
	else verbatim = !verbatim;

  if(verbatim) 
    tintin_puts2("#All text is now sent 'as is'.", NULL);
  else 
    tintin_puts2("#Text is no longer sent 'as is'.", NULL); 
  prompt(NULL);
}

/********************/
/* the #all command */
/********************/

struct session *all_command(const char *arg, struct session* ses)
{
  struct session *sesptr, *next_ses;

  if(sessionlist) {
    char left[BUFFER_SIZE];

    get_arg_in_braces(arg, left, 1);
    for(sesptr = sessionlist; sesptr; sesptr = next_ses) {
      next_ses = sesptr->next;
      parse_input(left, sesptr);
    }
  }
  else
    tintin_puts("BUT THERE ISN'T ANY SESSION AT ALL!", ses);

  return(sessionlist ? ses : NULL);
}

/***********************/
/* the #redraw command */
/***********************/

void redraw_command(char* arg)
{
	if(*arg != '\0')
		redraw = atoi(arg);
	else redraw = !redraw;

  if(redraw) 
    tintin_puts2("#Ok. I now redraw input line when text arrives.", NULL);
  else
    tintin_puts2("#Ok. I no longer redraw the input line.", NULL);
  prompt(NULL);
}

/*********************/
/* the #bell command */
/*********************/

void bell_command(const char *arg, struct session* ses)
{
	char temp[BUFFER_SIZE], left[BUFFER_SIZE], right[BUFFER_SIZE], right2[BUFFER_SIZE], right3[BUFFER_SIZE];
#define N_BELL_ARGS 2
  static const char * bellargs[N_BELL_ARGS] =
  {
		"option", "threshold"
  };
	static int option = 0; //0: sound, 1: speaker, 2: flashwindow
	static int threshold = 5; //for #bell 5 and above, make sound
	static int freq = 700, duration = 20; //700Hz, 20 ms
	int i = 0, l;

  arg = get_arg_in_braces(arg, temp, 0);
  substitute_myvars(temp, left, ses);  
  arg = get_arg_in_braces(arg, temp, 0);
  substitute_myvars(temp, right, ses);
  arg = get_arg_in_braces(arg, temp, 0);
  substitute_myvars(temp, right2, ses);
  arg = get_arg_in_braces(arg, temp, 0);
  substitute_myvars(temp, right3, ses);

	while (i < N_BELL_ARGS && !is_abbrev(left, bellargs[i]))
		i++;
	if (i == N_BELL_ARGS){ //do real bell stuff
		if(*left == 0) l = 5; //default #bell 5
		else l = atoi(left);
		if(l < threshold) {
			sprintf(temp, "#BELL %s is silent (threshold = %d)", left, threshold);
			tintin_puts(temp, ses);
			return;
		}
#ifdef _WINDOWS
		if(option == 1)
			Beep(freq, duration);
		else
			MessageBeep(MB_OK);
#else
		temp[0] = 7;
		temp[1] = 0;
		write(1, temp, 1);
#endif
	}
	else if(is_abbrev(left, bellargs[0])){//options
		if(*right)
			option = atoi(right);
		if(option==1 && *right2) freq = atoi(right2);
		if(option==1 && *right3) duration = atoi(right3);
		if(freq<0x35 || freq>0x7fff) freq = 700;
		if(duration<=0) duration = 20;
		sprintf(temp, "#BELL option is now %d, freq=%dHz, duration=%dms. (0: soundcard, 1: speaker)", option, freq, duration);
		tintin_puts(temp, ses);
	}
	else if(is_abbrev(left, bellargs[1])){
		if(*right)
			threshold = atoi(right);
		sprintf(temp, "#BELL threshold is now %d.", threshold);
		tintin_puts(temp, ses);
	}
}

/*********************/
/* the #boss command */
/*********************/
#ifndef _WINDOWS
void boss_command(const char* arg, struct session* ses)
{
  char temp[BUFFER_SIZE];
  int i;

  for(i = 0; i < 50; i++) {
    sprintf(temp, "in-order traverse of tree starting from node %d"
	    "resulted in %d red nodes\n", i, 50-i);
    tintin_puts2(temp, NULL);
  }
  getchar(); /* stop screen from scrolling stuff */
}
#endif

/*********************/
/* the #decrypt command */
/*********************/
void decrypt_command(const char* arg, struct session* ses)
{
	char buff[BUFFER_SIZE], temp[BUFFER_SIZE], *bin;
	struct log_list *logs;

	if(!ses || !ses->socket){
		tintin_puts2("#For security, you have to use decrypt in a real session!", ses);
		return;
	}
	if(ses->decrypt)
		tintin_puts2("#For security, decrypt already disabled!", ses);

	get_arg_in_braces(arg, temp, 1);
	substitute_myvars(temp, buff, ses);
	if(*buff==0) return;
	bin = decrypt(buff);
	ses->decrypt++; //does not allow use of decrypt more than once!
	logs = ses->logfiles; ses->logfiles = NULL; //well, this does not fix all, e.g., slow, pause mode
	write_line_mud(bin, ses);
	ses->logfiles = logs;
	free(bin);
}
/*********************/
/* the #encrypt command */
/*********************/
void encrypt_command(const char* arg, struct session* ses)
{
	char buff[BUFFER_SIZE], temp[BUFFER_SIZE], *str;
	static time_t t0;
	time_t t1;

	get_arg_in_braces(arg, temp, 1);
	substitute_myvars(temp, buff, ses);

	if(*buff==0) return;
	if( (t1=time(NULL)) <= t0+10){
		tintin_puts2("#For security, you can only use encrypt every 10 seconds!", ses);
		return;
	}
	t0 = t1;
	str = encrypt(buff);
	tintin_puts2(str, ses);
	free(str);
}

/*********************/
/* the #cd command */
/*********************/
void cd_command(const char *arg, struct session* ses)
{
	char str[BUFFER_SIZE], buff[BUFFER_SIZE], temp[BUFFER_SIZE], *pt;
	get_arg_in_braces(arg, temp, 1);
	substitute_myvars(temp, buff, ses); /*added by chitchat*/

	if(*buff == '\0'){
		getcwd(str, 256);
		sprintf(temp, "#Current dir is \"%s\".", str);
		tintin_puts2(temp, ses);
	}
	else {
		if(strncmp(buff, "~", 1)==0){
			strcpy(temp, buff+1);
			sprintf(buff, "%s%s", getenv("HOME"), temp);
		}
#ifdef _WINDOWS
		while( (pt=strchr(buff, '/')) != NULL)
			*pt='\\'; /*makes unix file format compatible */
#endif
		if(chdir(buff)==0)
			cd_command("", ses);
		else {
			sprintf(temp, "#Cannot change dir to \"%s\".", buff);
			tintin_puts2(temp, ses);
		}
	}
}

/*********************/
/* the #char command */
/*********************/

void char_command(const char *arg, struct session* ses)
{
  char left[BUFFER_SIZE], strng[BUFFER_SIZE];

  get_arg_in_braces(arg, left, 1);
  if(ispunct(*left)) {
		if(tintin_char != *arg){
			tintin_char = *arg;
			sprintf(strng, "#OK. TINTIN-CHAR is now {%c}", tintin_char);
			tintin_puts2(strng, NULL);
		}
  }
  else
    tintin_puts2("#SPECIFY A PROPER TINTIN-CHAR! SOMETHING LIKE # OR /!", NULL);
}

/*********************/
/* the #echo command */
/*********************/

void echo_command(char* arg, struct session* ses)
{
	if(*arg != '\0')
		echo = atoi(arg);
	else echo = !echo;

  if(echo)
    tintin_puts("#ECHO IS NOW ON.", ses);
  else
    tintin_puts("#ECHO IS NOW OFF.", ses);
}

/*********************/
/* the #end command */
/*********************/

void end_command(const char *command, struct session* ses)
{
  if(strcmp(command, "end"))
    tintin_puts("#YOU HAVE TO WRITE #end - NO LESS, TO END!", ses);
  else
    quitmsg(NULL);
}

/***********************/
/* the #ignore command */
/***********************/

void ignore_command(char* arg, struct session* ses)
{
  if(ses){
	if(*arg != '\0')
		ses->ignore = atoi(arg);
	else ses->ignore = !ses->ignore;

    if(ses->ignore)
      tintin_puts("#ACTIONS ARE IGNORED FROM NOW ON.", ses);
    else
      tintin_puts("#ACTIONS ARE NO LONGER IGNORED.", ses);
  }
  else
    tintin_puts("#No session active => Nothing to ignore!", ses);     
}

/***********************/
/* the #presub command */
/***********************/

void presub_command(char *arg, struct session* ses)
{
	if(*arg != '\0')
		presub = atoi(arg);
	else presub = !presub;

  if(presub)
    tintin_puts("#ACTIONS ARE NOW PROCESSED ON SUBSTITUTED BUFFER.", ses);
  else
    tintin_puts("#ACTIONS ARE NO LONGER DONE ON SUBSTITUTED BUFFER.", ses);
}

/***************************/
/* the #togglesubs command */
/***************************/

void togglesubs_command(char* arg, struct session* ses)
{
	if(*arg != '\0')
		togglesubs = atoi(arg);
	else togglesubs = !togglesubs;

  if(togglesubs)
    tintin_puts("#SUBSTITUTES ARE NOW IGNORED.", ses);
  else 
    tintin_puts("#SUBSTITUTES ARE NO LONGER IGNORED.", ses);
}

/***********************/
/* the #showme command */
/***********************/

void showme_command(const char *arg, struct session* ses)
{
  char left[BUFFER_SIZE], result[BUFFER_SIZE];
	int old_echoing;

  get_arg_in_braces(arg, left, 1);
	substitute_myvars(left, result, ses); /*added by chitchat*/
  /* prepare_actionalias(left, result, ses); /*remarked by chitchat*/
	old_echoing = puts_echoing; //backup old status, force it show, chitchat, 2/13/2000
	puts_echoing = TRUE;
  tintin_puts(result, ses);
	puts_echoing = old_echoing;
}

/*********************/
/* the #loop command */
/*********************/

void loop_command(const char *arg, struct session* ses)
{
  char left[BUFFER_SIZE], right[BUFFER_SIZE];
  char result[BUFFER_SIZE], temp[BUFFER_SIZE];
	char *pt;

  int bound1, bound2, counter, step;

  arg = get_arg_in_braces(arg, left, 0);
  arg = get_arg_in_braces(arg, right, 1);
  substitute_vars(left, temp);
  substitute_myvars(temp, left, ses);  
//  if(sscanf(left, "%d,%d", &bound1, &bound2) != 2)
	if((pt=strchr(left, ','))==NULL)
    tintin_puts2("#Wrong number of arguments in #loop", ses);
  else {
		*pt = 0;
		bound1 = eval_expression(left); //chitchat, 5/15/2000
		bound2 = eval_expression(++pt);
    if(bound1 < bound2) {
      step = 1;
      bound2++;
    }
    else {
      step = -1;
      bound2--;
    }

    for(counter = bound1; counter != bound2; counter += step) {
      sprintf(vars[0], "%d", counter);
      substitute_vars(right, result);
      parse_input(result, ses);
    }
  }  
}

/* Function while_command added to support #while loop command - vastheman 2002-08-05 */
/*********************/
/* the #while command */
/*********************/

void while_command(const char *arg, struct session* ses)
{
  char left[BUFFER_SIZE], right[BUFFER_SIZE];
  char temp1[BUFFER_SIZE], temp2[BUFFER_SIZE];

// Extract the left and right arguments
  arg = get_arg_in_braces(arg, left, 0);
  arg = get_arg_in_braces(arg, right, 1);

// Substitute variables in expression for the first time
  substitute_vars(left, temp1);
  while(eval_strcmp(temp1, ses));
  substitute_myvars(temp1, temp2, ses);  

// Keep executing second argument until expression becomes false
  while(eval_expression(temp2)) {
    substitute_vars(right, temp1);
    parse_input(temp1, ses);

  // Substitute variables every time around
    substitute_vars(left, temp1);
    while(eval_strcmp(temp1, ses));
    substitute_myvars(temp1, temp2, ses);  
  }
}

/************************/
/* the #message command */
/************************/
void message_command(const char *arg, struct session* ses)
/*added pathdir, read, also can toggle absolutely, chitchat*/
{
	char offon[2][5];
	int mestype = 0;
	char left[BUFFER_SIZE], right[BUFFER_SIZE], tpstr[80];

  static const char * ms[N_MESS_TYPES] =
  {
    "aliases", "actions", "substitutes",
    "antisubstitutes", "highlights", "variables",
	"pathdir", "read", "delimiter"
  };
	strcpy(offon[0], "OFF.");
	strcpy(offon[1], "ON.");
	arg=get_arg_in_braces(arg, left, 0);
	arg=get_arg_in_braces(arg, right, 1);

	while (mestype < N_MESS_TYPES && !is_abbrev(left, ms[mestype]))
		mestype++;
	if (mestype == N_MESS_TYPES){
		strcpy(right, "#Available message toggle types are:");
		for(mestype=0; mestype<N_MESS_TYPES; mestype++){
			strcat(right, " ");
			strcat(right, ms[mestype]);
			strcat(right, (mestype == N_MESS_TYPES-1) ? "." : ",");
		}
		tintin_puts2(right, ses);
	}
	else
	{
		if(right[0] != '\0')
			mesvar[mestype] = atoi(right);
		else
			mesvar[mestype] = !mesvar[mestype];
		sprintf(tpstr, "#Ok. messages concerning %s are now %s(%d)",
			ms[mestype], offon[mesvar[mestype]!=0], mesvar[mestype]);
		tintin_puts2(tpstr, ses);
	}
}

/**********************/
/* the #snoop command */
/**********************/

void snoop_command(const char *arg, struct session* ses)
{
  struct session *sesptr = ses;
  char left[BUFFER_SIZE], temp[BUFFER_SIZE];

  if(ses) {
    get_arg_in_braces(arg, temp, 1);
		substitute_myvars(temp, left, ses); /*added by chitchat*/
    if(*left) {
      for(sesptr = sessionlist;
	  sesptr && strcmp(sesptr->name, left);
	  sesptr = sesptr->next)
	;
      if(!sesptr) {
        tintin_puts("#NO SESSION WITH THAT NAME!", ses);
        return;
      }
    }

    if(sesptr->snoopstatus) {
      sesptr->snoopstatus = FALSE;
      sprintf(left, "#UNSNOOPING SESSION '%s'", sesptr->name);
      tintin_puts(left, ses);
    }
    else {
      sesptr->snoopstatus = TRUE;
      sprintf(left, "#SNOOPING SESSION '%s'", sesptr->name);
      tintin_puts(left, ses);
    }
  }
  else
    tintin_puts("#NO SESSION ACTIVE => NO SNOOPING", ses);
}

/**************************/
/* the #speedwalk command */
/**************************/

void speedwalk_command(char* arg, struct session* ses)
{
	if(*arg != '\0')
		speedwalk = atoi(arg);
	else speedwalk = !speedwalk;

  if(speedwalk)
    tintin_puts("#SPEEDWALK IS NOW ON.", ses);
  else
    tintin_puts("#SPEEDWALK IS NOW OFF.", ses);
}

/***********************/
/* the #system command */
/***********************/
#ifdef PERLDLL
//this is for use together with perl56.lib
extern "C" int RunPerl(int argc, char **argv, char **env);
#else
//this is for calling perl56.dll dynamically
//cannot use AfxBlahLibrary with static linked version?
#	ifndef _AFXDLL
#		define AfxLoadLibrary LoadLibrary
#		define AfxFreeLibrary FreeLibrary
#	endif
typedef int (* LPFNRUNPERL)(int argc, char **argv, char **env);
#endif

/*massively re-written for windows by chitchat, enhanced for Unix too */
void system_command(const char *arg, struct session *ses)
{
	int l, option = 0, perldll = 0;//default tintin++ option
  char temp0[BUFFER_SIZE], temp[BUFFER_SIZE];
#ifdef _WINDOWS
	char *pt, *pt0;
	int argc = 1;
	char **argv, **argv0;
#endif

//option 1: add > tintin.out, tintin_puts the contents of tintin.out
//option 0: do not apprend " > tintin.out" to cmdline, default TinTin++ style
	
	strncpy(temp, arg, BUFFER_SIZE);
	l=strlen(temp);
	while(l>0 && temp[l-1]==' ') { //later shall use get_arg_in_braces() ? shall not
		                             // e.g.: #system update.pl 
		temp[l-1]=0;
		l--;
	}
	if(strlen(temp) > 4 && temp[l-3]== '{' && isdigit(temp[l-2]) && temp[l-1]=='}'){
		option = temp[l-2] - '0'; //get the raw option
		temp[l-3] = 0;
	}

  get_arg_in_braces(temp, temp0, 1);
	substitute_myvars(temp0, temp, ses);
	
	if(option==1){
		unlink("tintin.out");
		strcat(temp, " > tintin.out");
	}

#ifdef _WINDOWS
	if(
#ifndef PERLDLL
			*szPerl &&
#endif
			tolower(temp[0])=='p'	&& tolower(temp[1])=='e'
			&& tolower(temp[2])=='r'	&& tolower(temp[3])=='l'){
		perldll = 1;
		while( (pt=strstr(temp, "  ")) ) //get rid of double spaces
			memmove(pt, pt+1, strlen(pt));
		pt = temp;
		while( (pt0=strchr(pt, ' ')) ){
			argc++;
			pt = pt0 + 1;
		}
		if(argc<=1)
			*temp = 0;
		else {
			argv = (char **)malloc((argc+1)*sizeof(char *));
			argv0 = (char **)malloc((argc)*sizeof(char *)); //contents of argv[] might be changed, backup it for free()
			pt = temp;
			for(int i=0; i<argc; i++){
				pt0 = strchr(pt, ' ');
				if(pt0) *pt0 = 0;
				argv[i] = argv0[i] = (char *)malloc(strlen(pt)+1);
				strcpy(argv[i], pt);
				pt = pt0 +1;
			}
			argv[argc] = NULL;
		}
	}
		/*make file can be unix format too, but only replace /'s before space */
	pt0=strchr(temp, ' ');
	while( (pt=strchr(temp, '/')) != NULL && (pt0==NULL || pt0-pt>0))
		*pt = '\\';
#endif
	
	if (*temp)
	{
		tintin_puts2("#OK EXECUTING SHELL COMMAND.", ses);
		if(!perldll)
			system(temp);
#ifdef _WINDOWS
		else {
#ifdef PERLDLL
			RunPerl(argc, argv, _environ);
#else
			LPFNRUNPERL lpRunPerl;
			HMODULE hdPerl;
			if((hdPerl = AfxLoadLibrary(szPerl))==NULL){
				tintin_puts("#ERROR loading dll version of perl!", ses);
				return;
			}
			if((lpRunPerl = (LPFNRUNPERL)GetProcAddress(hdPerl, "RunPerl"))==NULL){
				tintin_puts("#ERROR loading function \"RunPerl\"!", ses);
				AfxFreeLibrary(hdPerl);
				hdPerl = NULL;
				return;
			}
			lpRunPerl(argc, argv, _environ);
			AfxFreeLibrary(hdPerl);
#endif
			for(int i=0; i<argc; i++)
				free(argv0[i]);
			free(argv0);
			free(argv);
		}
#endif
		
		if(option==1){
			syscat_command("tintin.out", ses);
			unlink("tintin.out");
		}
		
		sprintf(temp, "#OK COMMAND EXECUTED (OPTION %d).", option);
		tintin_puts2(temp, ses);
	}
	else
		tintin_puts2("#EXECUTE WHAT COMMAND?", ses);
	prompt(NULL);
}

/* #system2 command, will invoke another new process, current ytin
   will not hang, and if the invoked program is windows program, won't
	 bring up a DOS prompt */
#ifdef _WINDOWS

void system2_command(const char *arg, struct session *ses)
{
  char temp0[BUFFER_SIZE], temp[BUFFER_SIZE], *pt, *pt0;

	strncpy(temp, arg, BUFFER_SIZE);

  get_arg_in_braces(temp, temp0, 1);
	substitute_myvars(temp0, temp, ses);
	
/* for windows only, and only convert before space, coz arguments may
   still need be '/', limitation: c:/program files/ytin may need be written
	as c:/progra~1/ytin, or use c:\\program files\\ytin */
	pt0=strchr(temp, ' ');
	while( (pt=strchr(temp, '/')) != NULL && (pt0==NULL || pt0-pt>0))
		*pt = '\\';
	
	if (*temp)
	{
		STARTUPINFO stInfo;
		PROCESS_INFORMATION pInfo;
		GetStartupInfo(&stInfo);

    CString str = temp;

		tintin_puts2("#OK EXECUTING CreateProcess COMMAND.", ses);
		CreateProcess(NULL, str.GetBuffer(80), NULL, NULL, 0,
			CREATE_NO_WINDOW | CREATE_DEFAULT_ERROR_MODE | NORMAL_PRIORITY_CLASS,
			NULL,
			NULL, //m_szStartDir,
			&stInfo,
			&pInfo
			);
	}
	else
		tintin_puts2("#EXECUTE WHAT COMMAND?", ses);
	prompt(NULL);

}
#endif

/************************/
/* the #syscat command */
/************************/
void syscat_command(const char *filename0, struct session* ses)
{
	char *pt, filename[BUFFER_SIZE], temp[BUFFER_SIZE];
	int l;
	FILE *fp;

	get_arg_in_braces(filename0, temp, 1);
	substitute_myvars(temp, filename, ses);
#ifdef _WINDOWS
	while( (pt=strchr(filename, '/')) != NULL)
		*pt = '\\'; /*make file can be unix format too */
#endif

	unvar_command("_", ses); //remove $_ variable
	if((fp=fopen(filename, "rt"))==NULL) /*text mode */
	{
		sprintf(temp, "#ERROR(syscat) - COULDN'T OPEN %s!", filename);
		tintin_puts(temp, ses);
		prompt(NULL);
		return;
	}

	l=1;
#ifdef _WINDOWS
	buffer_view = 1;
#endif
	while(fgets(temp, BUFFER_SIZE-1-4, fp)){//-4 leave space for adding _ {%s}
		if(temp[strlen(temp)-1]=='\n')
			temp[strlen(temp)-1]=0;
		if(*temp) tintin_puts(temp, ses); //it can be actioned!
		if(l && *temp){//put the 1st non_trivil line to $_ variable
			l=strlen(temp);
			temp[l+4]=0; temp[l+3]='}';
			for(--l; l>=0; l--)
				temp[l+3] = temp[l];
				temp[0]= '_'; temp[1]=' '; temp[2]='{';
				var_command(temp, ses);
				l=0;
			}
		}
		fclose(fp);
#ifdef _WINDOWS
		flush_view();
#endif
}

/************************/
/* the #sysecho command */
/************************/
void sysecho_command(const char *arg0, struct session* ses)
{
	char *pt, buff[BUFFER_SIZE], file[BUFFER_SIZE], arg[BUFFER_SIZE];
	char mode[3];
	int l;
	FILE *fp;

	substitute_vars(arg0, buff);
	substitute_myvars(buff, arg, ses);
	if(pt=strstr(arg, "{>>}")) {
		strcpy(mode, "a+");
		strcpy(file, pt+4);
		*pt=0;
		strcpy(buff, arg);
	}
	else if(pt=strstr(arg, "{>}")){
		strcpy(mode, "w");
		strcpy(file, pt+3);
		*pt=0;
		strcpy(buff, arg);
	}
	else if(pt=strstr(arg, ">>")){
		strcpy(mode, "a+");
		strcpy(file, pt+2);
		*pt=0;
		strcpy(buff, arg);
	}
	else if(pt=strrchr(arg, '>')){
		strcpy(mode, "w");
		strcpy(file, pt+1);
		*pt=0;
		strcpy(buff, arg);
	}
	else {
		tintin_puts2("#ERROR: Syntax: sysecho {STRING} {>} {file} or sysecho {STRING} {>>} {file}", ses);
		return;
	}

	if(*buff)
		while(buff[(l=strlen(buff)-1)] == ' ')
			buff[l] = 0;
	while(*file == ' ')
		memmove(file, file+1, strlen(file));
	get_arg_in_braces(file, file, 1);
	strcat(buff, "\n");

/* The code below make the last directory, to avoid some errors */
				strcpy(arg, file);
				if( (pt=strrchr(arg, '/')) != NULL
#ifdef _WINDOWS
					|| (pt = strrchr(arg, '\\')) != NULL
#endif
					){
					*pt = 0;
#ifdef _WINDOWS
					while( (pt=strchr(arg, '/')) != NULL)
						*pt = '\\'; /*make path can be unix format too */
#endif
					mkdir(arg, 0777); //unix relies on umask for the right of the new dir
				}
/* end of the code to make directory */

#ifdef _WINDOWS
				while( (pt=strchr(file, '/')) != NULL)
					*pt = '\\'; /*make file can be unix format too */
#endif

	if ((fp=fopen(file, mode))==NULL)
	{
		sprintf(buff, "#ERROR: cannot open file %s for \"%s\"!", file, mode);
		tintin_puts2(buff, ses);
		prompt(NULL);
		return;
	}
	fputs(buff, fp);
	fclose(fp);
//	tintin_puts2("#SYSECHO done!", ses);
//	prompt(NULL);
}

/********************/
/* the #zap command */
/********************/

struct session *zap_command(struct session* ses)
{
  tintin_puts("#ZZZZZZZAAAAAAAAPPPP!!!!!!!!! LET'S GET OUTTA HERE!!!!!!!!", ses);
  if(!ses){
    end_command("end", NULL); /* Does never return */
		return NULL;
	}

	else {
		cleanup_session(ses);
		return(activesession); //8/07/2000
	}
}

/************************/
/* the #wizlist command */
/************************/
/*
void wizlist_command(struct session* ses)
{
  tintin_puts2("==========================================================================", ses);
  tintin_puts2("                           Implementor:", ses);
  tintin_puts2("                              Valgar ", ses);
  tintin_puts2("", ses);
  tintin_puts2("          Special thanks to Grimmy for all her help :)", ses);
  tintin_puts2("\n\r                         TINTIN++ testers:", ses);
  tintin_puts2(" Nemesis, Urquan, Elvworn, Kare, Merjon, Grumm, Tolebas, Winterblade ", ses); 
  tintin_puts2("\n\r A very special hello to Zoe, Oura, GODDESS, Reyna, Randela, Kell, ", ses);
  tintin_puts2("                  and everyone else at GrimneDIKU\n\r", ses);
  tintin_puts2("==========================================================================", ses);
  prompt(ses);
} */

void wizlist_command(struct session* ses)
{

  tintin_puts2("==========================================================================", ses);
  tintin_puts2("  There are too many people to thank for making tintin++ into one of the", ses);
  tintin_puts2("finest clients available.  Those deserving mention though would be ", ses);
  tintin_puts2("Peter Unold, Bill Reiss, Robert Ellsworth, Jeremy Jack, and the many people", ses);
  tintin_puts2("who send us bug reports and suggestions.", ses);
  tintin_puts2("            Enjoy!!!  And keep those suggestions coming in!!\n\r", ses);
  tintin_puts2("                       The Management...", ses);
  tintin_puts2("==========================================================================", ses);
}

/*********************/
/* the #info command */
/*********************/

void display_info(struct session* ses)
{
  char buf[BUFFER_SIZE];
  int actions = 0, aliases = 0, variables = 0, 
      subs = 0, antisubs = 0, highs = 0, funcs = 0, pause = 0, slow = 0;
	float psize = 0;
  int ignore;

  actions = count_list(ses ? ses->actions : common_actions);
  aliases = count_list(ses ? ses->aliases : common_aliases);
  subs = count_list(ses ? ses->subs : common_subs);
  antisubs = count_list(ses ? ses->antisubs : common_antisubs);
  variables = count_list(ses ? ses->myvars : common_myvars); /*need fix for _ et al, chitchat*/
  highs = count_list(ses ? ses->highs : common_highs);
  funcs = count_list(ses ? ses->myfuncs  : common_functions);

  ignore = (ses ? ses->ignore : 0);
	pause = (ses ? ses->pausestatus : 0);
	if(pause) psize = (float)((double)ses->writepause.tv_sec + (double)ses->writepause.tv_usec/1e6);

  tintin_puts2("You have defined the following:",ses);
  sprintf(buf, "Actions: %d", actions);
  tintin_puts2(buf, ses);
  sprintf(buf, "Aliases: %d", aliases);
  tintin_puts2(buf, ses);
  sprintf(buf, "Substitutes: %d", subs);
  tintin_puts2(buf, ses);
  sprintf(buf, "Antisubstitutes: %d",antisubs);
  tintin_puts2(buf, ses);
  sprintf(buf, "Variables: %d", variables);
  tintin_puts2(buf, ses);
  sprintf(buf, "Highlights: %d", highs);
  tintin_puts2(buf, ses);
  sprintf(buf, "Functions: %d", funcs);
  tintin_puts2(buf, ses);
  sprintf(buf, "Echo: %d (1 - on, 0 - off)    Speedwalking: %d   Redraw: %d",
	  echo, speedwalk, redraw);
  tintin_puts2(buf, ses);
  sprintf(buf, "Toggle Subs: %d   Ignore Actions: %d   PreSub-ing: %d remote_echo: %d",
		togglesubs, ignore, presub, (ses ? ses->remote_echo : 0));
  tintin_puts2(buf, ses);
	sprintf(buf, "Pause: %d(%g), TCP_NODELAY: %d, Slow: %d, Stripcolor: %d",
		pause, psize, tcp_nodelay, slow, stripcolor);
  tintin_puts2(buf, ses);
	show_all_event(ses);

  prompt(ses);
}

/****************************/
/* the #setprompt command   */
/****************************/

void setprompt_command(const char *arg, struct session* ses)
{
  char left[BUFFER_SIZE], result[BUFFER_SIZE];

  arg = get_arg_in_braces(arg, result, 1);
	substitute_myvars(result, left, ses);
  if(!*left) {
   if (prompt_on == 1) {
    tintin_puts2("#Defined prompt:", ses);
    tintin_puts2(prompt_line, ses);
    prompt(ses);
   }
  }
  else {
#ifdef _WINDOWS
		setprompt(strip_ansi(left, result));
#else
    strcpy(prompt_line, left);
    prompt_on = 1;
#endif
    if(mesvar[1]) {
      sprintf(result, "#Ok. {%s} will now be displayed on the status line ", left);
      tintin_puts2(result, ses);
    }
  }
}


/************************/
/* the #toggle command */
/************************/
/*it does both toggle and set, chitchat */
/*later can get rid of most simple commands, like speedwalk, verbtimine ... */
void toggle_command(const char *arg, struct session* ses)
{
	char offon[2][5];
	char tpstr[BUFFER_SIZE];
	char left[BUFFER_SIZE], right[BUFFER_SIZE];
	int i=0, j=0, bOptVal;
	
	strcpy(offon[0], "OFF");
	strcpy(offon[1], "ON");
	arg=get_arg_in_braces(arg, left, 0);
	arg=get_arg_in_braces(arg, right, 1);
	if(right[0] != '\0'){
		j = 1;
		i = atoi(right);
	}
	if(is_abbrev(left, "big5")){
		if(j) big5 = i;
		else big5 = !big5;
		sprintf(tpstr, "#BIG5 support IS NOW %s(%d).", offon[big5!=0], big5);
		tintin_puts2(tpstr, ses);
	}
	else if(is_abbrev(left, "echo")){
		if(j) echo = i;
		else echo = !echo;
		sprintf(tpstr, "#ECHO IS NOW %s(%d).", offon[echo!=0], echo);
		tintin_puts2(tpstr, ses);
	}
	else if(is_abbrev(left, "ignore")){
		if(ses){
			if(j) ses->ignore = i;
			else ses->ignore = !ses->ignore;
			sprintf(tpstr, "#IGNORE on action IS NOW %s(%d).", offon[(ses->ignore)!=0], ses->ignore);
			tintin_puts2(tpstr, ses);
		}
		else
			tintin_puts("#No session active => Nothing to ignore!", ses);     
	}
	else if(is_abbrev(left, "presub")){
		if(j) presub = i;
		else presub = !presub;
		sprintf(tpstr, "#PRESUB IS NOW %s(%d).", offon[presub!=0], presub);
		tintin_puts2(tpstr, ses);
	}
	else if(is_abbrev(left, "prompt_on")){
		if(j) prompt_on = i;
		else prompt_on = !prompt_on;
		sprintf(tpstr, "#PROMPT IS NOW %s(%d).", offon[prompt_on!=0], prompt_on);
		tintin_puts2(tpstr, ses);
	}
	else if(is_abbrev(left, "raw")){
		if(j) toggle_raw = i;
		else toggle_raw = !toggle_raw;
		sprintf(tpstr, "#RAW IS NOW %s(%d).", offon[toggle_raw!=0], toggle_raw);
		tintin_puts2(tpstr, ses);
	}
	else if(is_abbrev(left, "redraw")){
		if(j) redraw = i;
		else redraw = !redraw;
		sprintf(tpstr, "#REDRAW IS NOW %s(%d).", offon[redraw!=0], redraw);
		tintin_puts2(tpstr, ses);
	}
	else if(is_abbrev(left, "skiptelnetseq")){
		if(j) skiptelnetseq = i;
		else skiptelnetseq = !skiptelnetseq;
		sprintf(tpstr, "#SKIPTELNETSEQ IS NOW %s(%d).", offon[skiptelnetseq!=0], skiptelnetseq);
		tintin_puts2(tpstr, ses);
	}
	else if(is_abbrev(left, "speedwalk")){
		if(j) speedwalk = i;
		else speedwalk = !speedwalk;
		sprintf(tpstr, "#SPEEDWALK IS NOW %s(%d).", offon[speedwalk!=0], speedwalk);
		tintin_puts2(tpstr, ses);
	}
	else if(is_abbrev(left, "stripcolor")){
		if(j) stripcolor = i;
		else stripcolor = !stripcolor;
		sprintf(tpstr, "#STRIPCOLOR before action IS NOW %s(%d).", offon[stripcolor!=0], stripcolor);
		tintin_puts2(tpstr, ses);
	}
	else if(is_abbrev(left, "ticker_in_split")){
		if(j) ticker_in_split = i;
		else ticker_in_split = !ticker_in_split;
		sprintf(tpstr, "#TICKCOUNTER IS NOW %s(%d) in split mode.", offon[ticker_in_split!=0], ticker_in_split);
		tintin_puts2(tpstr, ses);
	}
  else if(is_abbrev(left, "tcp_nodelay")){
		if(j) tcp_nodelay = i;
		else tcp_nodelay = !tcp_nodelay;
		sprintf(tpstr, "#TCP_NODELAY IS NOW %s(%d), NAGLE ALGORITHM IS NOW %s.", offon[tcp_nodelay!=0], tcp_nodelay, offon[tcp_nodelay==0]);
		tintin_puts2(tpstr, ses);
    if(ses && ses->socket){
      bOptVal = tcp_nodelay;
      if (setsockopt(ses->socket, IPPROTO_TCP, TCP_NODELAY, (char*)&bOptVal, sizeof(bOptVal)) == SOCKET_ERROR)
        tintin_puts("SOCKET: error setting TCP_NODELAY", ses);
    }
  }
	else if(is_abbrev(left, "togglesubs")){
		if(j) togglesubs = i;
		else togglesubs = !togglesubs;
		sprintf(tpstr, "#TOGGLESUBS IS NOW %s(%d).", offon[togglesubs!=0], togglesubs);
		tintin_puts2(tpstr, ses);
	}
	else if(is_abbrev(left, "verbatim")){
		if(j) verbatim = i;
		else verbatim = !verbatim;
		sprintf(tpstr, "#VERBATIM IS NOW %s(%d).", offon[verbatim!=0], verbatim);
		tintin_puts2(tpstr, ses);
	}
	else
		tintin_puts2("#Invalid type to toggle, #help toggle for details.", ses);
}
