/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: parse.c - some utility-functions							 */
/*							 TINTIN III							 */
/*		 (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t				*/
/*					 coded by peter unold 1992					 */
/*********************************************************************/

/* note: a bunch of changes were made here for readline support -- daw */

#include <config.h>
#include "tintin.h"

#if defined(HAVE_STRING_H)
#include <string.h>
#elif defined(HAVE_STRINGS_H)
#include <strings.h>
#endif

#include <ctype.h>
#include <signal.h>

#ifndef _WINDOWS
#include <readline/readline.h>
extern int readline_echoing_p;
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "action.proto"
#include "alias.proto"
#include "antisub.proto"
#include "files.proto"
#include "highlight.proto"
#include "help.proto"
#include "ivars.proto"
#include "llist.proto"
#include "log.proto"
#include "macro.proto"
#include "main.proto"
#include "misc.proto"
#include "net.proto"
#include "parse.proto"
#include "path.proto"
#include "pause.proto"
#include "queue.proto"
#include "rl.proto"
#include "rlhist.proto"
#include "rltab.proto"
#include "session.proto"
#include "stream.proto"
#include "substitute.proto"
#include "text.proto"
#include "ticks.proto"
#include "utils.proto"
#include "variables.proto"
#include "function.proto"

/**************************************************************************/
/* parse input, check for TINTIN commands and aliases and send to session */
/**************************************************************************/

struct session *parse_input(const char *input, struct session* ses)
{
	char command[BUFFER_SIZE], arg[BUFFER_SIZE];
	
	if(!*input) {
		if(ses)
			write_line_mud("", ses);
		else
			write_com_arg_mud("", "", ses);
		return(ses);
	}
	
	if(*input == tintin_char && (input[1]==0 || 
			(!is_abbrev(input+1, "version") && is_abbrev(input+1, "verbatim")) )) {
		/*for compatible of old verbatim shortcut, also #ver makes more sense for version */
		verbatim_command("");
		return(ses);
	}
	
	if(verbatim && ses) {
		write_line_mud(input, ses);
		return(ses);
	}
	
	if(*input == verbatim_char) {
		input++;
		write_line_mud(input, ses);
		return(ses);
	}
	
	while(*input) {
		if(*input==';')
			input++;
		input=get_arg_stop_spaces(input, command);
		input=get_arg_all(input, arg);
		if (*command==tintin_char && !strcmp(command+1, "sleep")) {
      if(input+1) // input would be sth like: ;dothis;that;... // 4/13/2006
        sleep_command(arg, ses, input+1, EVENT_SLEEP);
			return ses;
		}
		if (*command==tintin_char && !strcmp(command+1, "sleep2")) {
      if(input+1)
        sleep_command(arg, ses, input+1, EVENT_SLEEP2);
			return ses;
		}

		if(*command==DEFAULT_OPEN && *arg==0){ //10/2/2001, handle {a;b c}
			get_arg_in_braces(command, arg, 0);
			ses = parse_input(arg, ses);
		}
		else
			ses = parse_command(command, arg, ses);
	}
	return ses;
}

struct session *parse_command(const char* command0, char* arg, struct session* ses)
{
	struct listnode *ln;
	char command[BUFFER_SIZE];
	int i, simple, numands, caution;
	char *cpsource, *cpsource2, newcommand[BUFFER_SIZE];
	// substitute_vars(arg0, arg); //chitchat, 12/06/1999, do not add this!!
	substitute_myvars(command0, command, ses);
	if(*command==tintin_char) ses=parse_tintin_command(command+1, arg, ses);
	else if((ln=searchnode_list_begin((ses) ? ses->aliases : common_aliases, command, ALPHA))!=NULL) {
		
		strcpy(vars[0], arg);
		
		for(i=1, cpsource=arg; i<10; i++) {
			/* Next lines CHANGED to allow argument grouping with aliases */
			while (*cpsource == ' ' || *cpsource=='\t')
				cpsource++;
			simple = (*cpsource != DEFAULT_OPEN); //simple means no need worry about { } pairs, chitchat, 2/18/2000
			numands = !simple;
			cpsource += numands;
			caution = 0;
			for(cpsource2=cpsource; *cpsource2 &&
					((simple && *cpsource2!=' ' && *cpsource2!='\t') || (!simple && numands>0))
					;	cpsource2++){
				if(big5 && (*cpsource2 & 0x80)){
					if(*(++cpsource2) == 0) break;
				}
				else if(*cpsource2==DEFAULT_OPEN) numands++;
				else if(*cpsource2==DEFAULT_CLOSE) numands--;
				else if(*cpsource2 == ';' && (cpsource==cpsource2 || *(cpsource2-1)!='\\')) //check for un-escaped only, 5/17/2000
					caution = 1;
			}
			if(caution) //new: add/keep {} if met <<un-escaped>> ';', strip if not. but still has holes, chitchat, 2/21/2000, <<5/17/2000>>
				vars[i][0]=DEFAULT_OPEN;
			strncpy(vars[i]+caution, cpsource, (cpsource2-cpsource-1+simple>=0) ? (cpsource2-cpsource-1+simple) : 0);
			vars[i][cpsource2-cpsource+caution-1+simple]=DEFAULT_CLOSE;
			vars[i][cpsource2-cpsource+2*caution-1+simple]='\0';

			cpsource=(*cpsource2) ? cpsource2+1 : cpsource2;
		}
		prepare_actionalias(ln->right, newcommand, ses); 
		if(!strcmp(ln->right, newcommand) && *arg) {
			strcat(newcommand, " "); 
			strcat(newcommand, arg);
		}
		ses = parse_input(newcommand, ses);
	}
	else if(speedwalk && !*arg && is_speedwalk_dirs(command))
		do_speedwalk(command, ses);
	else {
		get_arg_with_spaces(arg,arg);
		write_com_arg_mud(command, arg, ses);
	}
	return ses;
}

/**********************************************************************/
/* return TRUE if commands only consists of capital letters N,S,E ... */
/**********************************************************************/

int is_speedwalk_dirs(const char *cp)
{
	int flag;
	char *d[12] = {"se", "sw", "ne", "nw",
		"su", "sd", "nu", "nd",
		"eu", "ed", "wu", "wd"};
	//when toggle speedwalk 2, we do not substitute single se, sw, ..., chitchat, 2/6/99
	if(speedwalk>1 && strlen(cp)==2){ //smarter speedwalk when toggle as 2
		for(flag=0;flag<12;flag++){
			if(d[flag][0] == cp[0] && d[flag][1] == cp[1])
				return FALSE;
		}
	}
	flag = FALSE;
	while (*cp)
	{
		if (*cp != 'n' && *cp != 'e' && *cp != 's' && *cp != 'w' && *cp != 'u' && *cp != 'd' &&
		    !isdigit(*cp))
			return FALSE;
		if (!isdigit(*cp))
			flag = TRUE;
		cp++;
	}
	return flag;
}

/**************************/
/* do the speedwalk thing */
/**************************/

void do_speedwalk(const char *cp, struct session* ses)
{
	char sc[2];
	const char *loc; 
	int multflag, loopcnt, i;
	
	strcpy(sc, "x");
	while(*cp) {
		loc = cp;
		multflag = FALSE;
		while(isdigit(*cp)) {
			cp++;
			multflag = TRUE;
		}
		if(multflag && *cp) {
			sscanf(loc, "%d%c", &loopcnt, sc);
			for(i = 0; i++ < loopcnt;)
				write_com_arg_mud(sc, "", ses);
		}
		else if(*cp) {
			sc[0] = *cp;
			write_com_arg_mud(sc, "", ses);
		}
		/* Added the if to make sure we didn't move the pointer outside the 
		bounds of the origional pointer. Corrects the bug with speedwalking
		where if you typed "u7" tintin would go apeshit. (JE)
		*/
		if(*cp)
			cp++;
	}
}

/*************************************/
/* parse most of the tintin-commands */
/*************************************/

struct session *parse_tintin_command(const char *command0, char *arg,	struct session* ses)
{
	struct session *sesptr;
	char command[BUFFER_SIZE], buf[BUFFER_SIZE];

	get_arg_in_braces(command0, command,1);
	/* moved this to 1st check, some1 like to use digits as session name, ack! chitchat, 1/25/2000 */
	if(isdigit(*command)) {
		int i = atoi(command);
		
		if(i > 0) {
			get_arg_in_braces(arg, arg, 1);
			while(i-- > 0)
				ses = parse_input(arg, ses);
		}
		else {
			tintin_puts("#YEAH RIGHT! GO REPEAT THAT YOURSELF DUDE.", ses);
			prompt(NULL);
		}
		return(ses);
	}
	
	/* new: double tintin char forces sessionname, chitchat, 1/25/2000 */
	/* e.g.: ##2 forces sessionname 2, while #2 means repeat twice */
	for(sesptr = sessionlist; sesptr; sesptr = sesptr->next) {
		if(!strcmp(sesptr->name, command) ||
				(*command==tintin_char && !strcmp(sesptr->name, command+1)) ) {
			if(*arg) {
				get_arg_with_spaces(arg, arg);
				parse_input(arg, sesptr); /* was: #sessioname commands */
				return(activesession);
			}
			else {
				if(activesession != sesptr) {//do not show activated msg if already active
					activesession = sesptr;
					sprintf(buf, "#SESSION '%s' ACTIVATED.", sesptr->name);
					tintin_puts(buf, sesptr);
					prompt(NULL);
				}
				return(sesptr);
			}
		}
	}
	
	if(is_abbrev(command, "action"))
		action_command(arg, ses);
	
	else if(is_abbrev(command, "alias"))
		alias_command(arg, ses);
	else if(is_abbrev(command, "al2variable"))
		al2variable_command(arg, ses);
	else if(is_abbrev(command, "all"))
		ses = all_command(arg, ses);
	
	else if(is_abbrev(command, "antisubstitute"))
		parse_antisub(arg,ses);
	
	else if(is_abbrev(command, "bell"))
		bell_command(arg, ses);
	
	else if(is_abbrev(command, "boss")) 
		boss_command(arg, ses);
	
	else if(is_abbrev(command, "cd"))
		cd_command(arg, ses);
	
	else if(is_abbrev(command, "char"))
		char_command(arg, ses);
	
	else if(is_abbrev(command, "chr"))
		chr_command(arg, ses);
	
	else if(is_abbrev(command, "cr"))
		cr_command(arg, ses); 
	
#ifdef CHINESE
	else if(is_abbrev(command, "ctoi"))
		ctoi_command(arg, ses);
#endif
	
	else if(is_abbrev(command, "decrypt"))
		decrypt_command(arg, ses);
	
	else if(is_abbrev(command, "delimiter"))
		delimiter_command(arg, ses);
	
	else if(is_abbrev(command, "echo"))
		echo_command(arg, ses);
	
	else if(is_abbrev(command, "encrypt"))
		encrypt_command(arg, ses);
	
	else if(is_abbrev(command, "end")) 
		end_command(command, ses);
	
	else if(is_abbrev(command, "fclose"))
		fclose_command(arg, ses);
	
	else if(is_abbrev(command, "feof"))
		feof_command(arg, ses);
	
	else if(is_abbrev(command, "fgets"))
		fgets_command(arg, ses);
	
	else if(is_abbrev(command, "fopen"))
		fopen_command(arg, ses);
	
	else if(is_abbrev(command, "fputs"))
		fputs_command(arg, ses);
	
	else if(is_abbrev(command, "fread"))
		fread_command(arg, ses);
	
	else if(is_abbrev(command, "fseek"))
		fseek_command(arg, ses);
	
	else if(is_abbrev(command, "fwrite"))
		fwrite_command(arg, ses);
	
#ifdef CHINESE
	else if(is_abbrev(command, "GBfilter"))
		GBfilter_command(arg, ses);
#endif
	
	else if(is_abbrev(command, "help")) 
		help_command(arg);
	
	else if(is_abbrev(command, "highlight"))
		parse_high(arg, ses);
	
	else if(is_abbrev(command, "history")) 
		rlhist_show();
	
	else if(is_abbrev(command, "if")) 
		if_command(arg, ses);
	
	else if(is_abbrev(command, "ifexist")) 
		ifexist_command(arg, ses);
	
	else if(is_abbrev(command, "ifmatch")) 
		ifmatch_command(arg, ses);
	
	else if(is_abbrev(command, "ignore"))
		ignore_command(arg, ses);
	
	else if(is_abbrev(command, "info"))
		display_info(ses);
	
	else if(is_abbrev(command, "killall"))
		kill_all(ses, CLEAN);
	
	else if(is_abbrev(command, "log")) 
		log_command(arg, ses);
	
	else if(is_abbrev(command, "loop"))
		loop_command(arg, ses);
	
	else if(is_abbrev(command, "while")) /* for #while - vastheman 2002-08-05 */
		while_command(arg, ses);
	
	else if(is_abbrev(command, "macro"))
		macro_command(arg, ses);

	
	else if(is_abbrev(command, "forall"))
		forall_command(arg, ses);
	
	else if(is_abbrev(command, "nop"))
		; 
	
	else if(is_abbrev(command, "map"))
		map_command(arg,ses);
	
	else if(is_abbrev(command, "math")) 
		math_command(arg, ses);
	
	else if(is_abbrev(command, "mark")) 
		mark_command(ses);
	
	else if(is_abbrev(command, "message"))
		message_command(arg, ses);
	
	else if(is_abbrev(command, "ord"))
		ord_command(arg, ses);
	
	else if(is_abbrev(command, "path")) 
		path_command(ses);
	
	else if(is_abbrev(command, "pathdir")) 
		pathdir_command(arg, ses); 
	
	else if(is_abbrev(command, "pause")) 
		pause_command(arg, ses); 

	else if(is_abbrev(command, "ping2")) 
		ping2_command(arg, ses); 
	
	else if(is_abbrev(command, "presub"))
		presub_command(arg, ses);
	
	else if(is_abbrev(command, "purge")) 
		purge_command(arg, ses); 
	
	else if(is_abbrev(command, "purist"))
		purist_command();
	
	else if(is_abbrev(command, "raw"))
		raw_command(arg, ses);
	
	else if(is_abbrev(command, "read"))
		ses = read_command(arg, ses);
	
	else if(is_abbrev(command, "redraw"))
		redraw_command(arg);
	
	else if(is_abbrev(command, "remove"))
		remove_command(arg, ses);
	
	else if(is_abbrev(command, "rename"))
		rename_command(arg, ses);
	
	else if(is_abbrev(command, "retab"))
		rltab_read(arg, ses);	/*	Modified to allow variables in tab file names - Vasantha Crabb	*/
	
	else if(is_abbrev(command, "return"))
		return_command(arg, ses);
	
	else if(is_abbrev(command, "revstring")) 
		revstring_command(arg, ses);
	
	else if(is_abbrev(command, "savepath"))
		savepath_command(arg, ses);
	
	else if(is_abbrev(command, "session"))
		ses = session_command(arg, ses);
	
	else if(is_abbrev(command, "showme"))
		showme_command(arg,ses);
	
	else if(is_abbrev(command, "slow")) 
		slow_command(arg, ses); 

	else if(is_abbrev(command, "snoop"))
		snoop_command(arg, ses);
	
	else if(is_abbrev(command, "speedwalk"))
		speedwalk_command(arg, ses);
	
	else if(is_abbrev(command, "split"))
		split_command(arg);

	else if(is_abbrev(command, "stripansi"))
		stripansi_command(arg, ses);

	else if(strncmp(command, "str", 3)==0)
		str_command(command, arg, ses);
	
	else if(is_abbrev(command, "substitute"))
		parse_sub(arg, ses);
	
	else if(is_abbrev(command, "substr"))
		substr_command(arg, ses);
	
#ifndef _WINDOWS
	/* CHANGED to allow suspending from within tintin. */
	/* I know, I know, this is a hack *yawn* */
	else if(is_abbrev(command, "suspend"))
		tstphandler(0);
#endif
	
	else if(is_abbrev(command, "tablist"))
		rltab_list();
	
	else if(is_abbrev(command, "tabadd"))
		do_tabadd(arg);
	
	else if(is_abbrev(command, "tabdelete"))
		do_tabdel(arg);
	
	else if(is_abbrev(command, "tabsave"))
		do_tabsave(arg, ses);	/*	Modified to allow different names for tab files - Vasantha Crabb	*/
	
	else if(is_abbrev(command, "telnet"))
		telnet_command(arg, ses);
	
	else if(is_abbrev(command, "textin"))
		read_file(arg, ses); 
	
	else if(is_abbrev(command, "unsplit"))
		unsplit_command(); 
	
	else if(is_abbrev(command, "gag"))
		gag_command(arg, ses);
	
	else if(system_com && is_abbrev(command, system_com))
		system_command(arg, ses);

#ifdef _WINDOWS	
	else if(system2_com && is_abbrev(command, system2_com))
		system2_command(arg, ses);
#endif
	
	else if(is_abbrev(command, "syscat"))
		syscat_command(arg, ses);
	
	else if(is_abbrev(command, "sysecho"))
		sysecho_command(arg, ses);
	
	else if(is_abbrev(command, "tick")) 
		tick_command(ses);
	
	else if(is_abbrev(command, "tickoff")) 
		tickoff_command(ses);
	
	else if(is_abbrev(command, "tickon")) 
		tickon_command(ses);
	
	else if(is_abbrev(command, "tickset"))
		tickset_command(ses);
	
	else if(is_abbrev(command, "ticksize")) 
		ticksize_command(arg, ses);
	
	else if(is_abbrev(command, "tolower"))
		tolower_command(arg, ses);
	
	else if(is_abbrev(command, "toLower"))
		toLower_command(arg, ses);
	
	else if(is_abbrev(command, "toggle"))
		toggle_command(arg, ses);
	
	else if(is_abbrev(command, "togglesubs"))
		togglesubs_command(arg, ses);
	
	else if(is_abbrev(command, "toupper"))
		toupper_command(arg, ses);
	
	else if(is_abbrev(command, "toUpper"))
		toUpper_command(arg, ses);
	
	else if(is_abbrev(command, "unaction"))
		unaction_command(arg, ses);
	
	else if(is_abbrev(command, "unalias"))
		unalias_command(arg, ses);
	
	else if(is_abbrev(command, "unantisubstitute"))
		unantisubstitute_command(arg, ses);
	
	else if(is_abbrev(command, "unhighlight"))
		unhighlight_command(arg, ses);
	
	else if(is_abbrev(command, "unsubstitute"))
		unsubstitute_command(arg, ses);
	
	else if(is_abbrev(command, "ungag"))
		unsubstitute_command(arg, ses);
	
	else if(is_abbrev(command, "unpath")) 
		unpath_command(ses);
	
	else if(is_abbrev(command, "unpurist"))
		unpurist_command();
	
	else if(is_abbrev(command, "variable"))
		var_command(arg, ses);
	
	else if(is_abbrev(command, "var2alias"))
		var2alias_command(arg, ses);
	
	else if(is_abbrev(command, "version"))
		version_command(NULL);
	
	else if(is_abbrev(command, "unvariable"))
		unvar_command(arg, ses);
	
	else if(is_abbrev(command, "wizlist")) 
		wizlist_command(ses);
	
	else if(is_abbrev(command, "write"))
		ses = write_command(arg, ses);
	
	else if(is_abbrev(command, "writesession"))
		ses = writesession_command(arg, ses);
	
	else if(is_abbrev(command, "zap"))
		ses = zap_command(ses);
	
	else if(is_abbrev(command, "setprompt"))
		setprompt_command(arg, ses);
	
	else if(is_abbrev(command, "function"))
		function_command(arg, ses);
	
	else if(is_abbrev(command, "unfunction"))
		unfunction_command(arg, ses);
	
	else if(is_abbrev(command, "unmacro"))
		unmacro_command(arg, ses);
	
	else if(is_abbrev(command, "result"))
		result_command(arg, ses);
	
	else if(is_abbrev(command, "getitemnr"))
		getitemnr_command(arg, ses);
	
	else if(is_abbrev(command, "getlistlength"))
		getlistlength_command(arg, ses);
	
#ifndef _WINDOWS
	else if(is_abbrev(command, "clearprompt")) {
		tintin_puts("#PROMPT LINE CLEARED.", ses);
		prompt_line[0] = '\0';
		sprintf(arg,"%s","-------------------------------------------------------------------------");
		status_in_splitline(arg, ses);
		prompt_on = 0;
	}
#endif
	
	else {
		sprintf(buf, "#UNKNOWN TINTIN-COMMAND(%s).", command);
		tintin_puts(buf, ses);
		prompt(NULL);
	}
	
	return(ses);
}

/**********************************************/
/* get all arguments - don't remove "s and \s */
/**********************************************/

const char *get_arg_all(const char *s, char *arg)
{
	int nest = 0;
	
	s = space_out(s);
	while(*s)
    if (big5 && (*s & 0x80) )  {
      *arg++= *s++ ;
      if ( *s )
        *arg++= *s++ ;
    }
		else if(*s == '\\') {
			*arg++ = *s++;
			if(*s)
				*arg++ = *s++;
		}
		else if(*s == ';' && nest < 1)
			break;
		else if(*s == DEFAULT_OPEN) {
			nest++;
			*arg++ = *s++;
		}
		else if(*s == DEFAULT_CLOSE) {
			nest--;
			*arg++ = *s++;
		}
		else
			*arg++ = *s++;
	
	*arg = '\0';
	return(s);
}

/**************************************/
/* get all arguments - remove "s etc. */
/* Example:							 */
/* In: "this is it" way way hmmm;		*/
/* Out: this is it way way hmmm		 */ 
/**************************************/

const char *get_arg_with_spaces(const char *s, char *arg)
{
	int nest = 0;
	/* int inside = FALSE; */
	
	s = space_out(s);
	while(*s) {
    if (big5 && (*s & 0x80) )  {
      *arg++= *s++ ;
      if ( *s )
        *arg++= *s++ ;
    }
		else if(*s == '\\') {
			if(*++s)
				*arg++ = *s++;
		}
		else if(*s == ';' && !nest)
			break;
		else if(*s == DEFAULT_OPEN) {
			nest++;
			*arg++ = *s++;
		}
		else if(*s == DEFAULT_CLOSE) {
			*arg++ = *s++;
			nest--;
		}
		else
			*arg++ = *s++;
	}
	
	*arg = '\0'; 
	return(s);
}

/********************/
/* my own routine  */
/********************/

const char *get_arg_in_braces(const char *s, char *arg, int flag)
{
	int nest = 0, extra = 0;
	//{{xxx}}, double quote: nest always >0 except before the last }
	//{{xx} {xx}}, nest will be 0 in the mid somewhere
	const char *ptr;
	char *arg0;
	
	s = space_out(s);
	ptr = s;
	if(*s != DEFAULT_OPEN) {
		if(!flag)
			s = get_arg_stop_spaces(ptr, arg);
		else
			s = get_arg_with_spaces(ptr, arg);
		return(s);
	}
	
	s++;
	if(*s == DEFAULT_OPEN) {
		extra = 1;
		arg0 = arg;
	}
  while(*s && !(*s == DEFAULT_CLOSE && !nest)) {
		if(big5 && (*s & 0x80)) {
			*arg++ = *s++;
			if(!*s) break;
		}
		else if(*s == DEFAULT_OPEN)
			nest++;
		else if(*s == DEFAULT_CLOSE){
			nest--;
			if(nest == 0 && s[1] != DEFAULT_CLOSE)
				extra = 0;
		}
		*arg++ = *s++;
	}
	
	if(!*s)
		tintin_puts2("#Unmatched braces error!", NULL);
	else
		s++;
	
	*arg = '\0';
	if(extra) {
		*(--arg) = '\0';
		memmove(arg0, arg0+1, sizeof(char)*(arg-arg0));
	}
	return(s);
}

const char *get_item_in_braces(const char *s, char *arg, int flag)
{
//currently flag only can be 0
	int nest = 0, extra = 0;
	const char *ptr;
	
	s = delimiter_out(s);
	ptr = s;
	if(*s != DEFAULT_OPEN) {
		s = get_item_stop_delimiter(ptr, arg);
		return(s);
	}
	
	s++;
	if(*s==DEFAULT_OPEN) { //now let's be smart, strip 2 outmost {}, 2/21/2000, chitchat
		s++; 
		extra = 1;
		nest = 1;
	}
	while(*s && !(*s == DEFAULT_CLOSE && !nest)) {
		if(big5 && (*s & 0x80)) {
			*arg++ = *s++;
			if(!*s) break;
		}
		else if(*s == DEFAULT_OPEN)
			nest++;
		else if(*s == DEFAULT_CLOSE)
			nest--;
		if(nest || !extra) *arg++ = *s++;
		else s++;
	}
	
	if(!*s)
		tintin_puts2("#Unmatched braces error!", NULL);
	else
		s++;
	
	*arg = '\0';
	return(s);
}

/**********************************************/
/* get one arg, stop at spaces				*/
/* remove quotes							 */
/**********************************************/

const char *get_arg_stop_spaces(const char *s, char *arg)
{
	int inside = FALSE, nest=0;
	
	s = space_out(s);
	
	while(*s) {
    if (big5 && (*s & 0x80) )  {
      *arg++= *s++ ;
      if ( *s )
        *arg++= *s++ ;
    }
		else if(*s == '\\') {
			if(*++s)
				*arg++ = *s++;
		}
		else if(*s == '"') {
			s++;
			inside = !inside;
		}
		else if(*s == ';' && !inside && nest<1) {
			break;
		}
/* This breaks tabs in com-files - fixing - ovema@sol.no, 2001-02-14 */
   else if(!inside && nest<1 && (*s == ' ' || *s == '\t'))
		 break;
	 else if(*s == DEFAULT_OPEN) {
			nest++;
			*arg++ = *s++;
		}
		else if(*s == DEFAULT_CLOSE) {
			nest--;
			*arg++ = *s++;
		}
		else
			*arg++ = *s++;
	}
	
	*arg = '\0';
	return(s);
}

const char *get_item_stop_delimiter(const char *s, char *arg)
{
	int inside = FALSE, nest=0;
	
	s = delimiter_out(s);
	
	while(*s) {
		if(!inside && nest<1 && !strncmp(s, DELIMITER, strlen(DELIMITER)) )
			break;
    else if (big5 && (*s & 0x80) )  {
      *arg++= *s++ ;
      if ( *s )
        *arg++= *s++ ;
    }
		else if(*s == '\\') {
			if(*++s)
				*arg++ = *s++;
		}
		else if(*s == '"') {
			s++;
			inside = !inside;
		}
		else if(*s == DEFAULT_OPEN) {
			nest++;
			*arg++ = *s++;
		}
		else if(*s == DEFAULT_CLOSE) {
			nest--;
			*arg++ = *s++;
		}
		else *arg++ = *s++;
	}
	
	*arg = '\0';
	return(s);
}

/*********************************************/
/* spaceout - advance ptr to next none-space */
/* return: ptr to the first none-space		*/
/*********************************************/ 

const char *space_out(const char *s)
{
	/*changed isspace(*s) here,it causes various unexpected behavious for some platforms, chitchat*/
	while(*s==' ' || *s=='\t')
		s++;
	return(s);
}
const char *delimiter_out(const char *s)
{
	while(*s && strstr(s, DELIMITER)==s)
		s+= strlen(DELIMITER);
	return(s);
}

/************************************/
/* send command+argument to the mud */
/************************************/

void write_com_arg_mud(const char *command, const char *arg, struct session* ses)
{
	char outtext[BUFFER_SIZE];
	char argument[BUFFER_SIZE];
	
	if(!ses) {
		char buf[BUFFER_SIZE];
		
		sprintf(buf, "#NO SESSION ACTIVE. USE THE %csession COMMAND TO START ONE.\n(See also: %chelp, %chelp session)",
			tintin_char, tintin_char, tintin_char);
		tintin_puts(buf, ses);
		prompt(NULL);
	}
	else {
		check_insert_path(command, ses);
		strncpy(outtext, command, BUFFER_SIZE);
		substitute_myvars(arg, argument, ses); /*added here to handle recursive #math #var in 1 line, see also: parse_command(), chitchat*/
		if(*argument) {
			strncat(outtext, " ", BUFFER_SIZE-strlen(command)-1); 
			strncat(outtext, argument, BUFFER_SIZE-strlen(command)-2);
		}
		write_line_mud(outtext, ses);
/* move this to the actual time that write to mud, chitchat, 2/1/2000
		if(ses->logfile) {
			outtext[i = strlen(outtext)] = '\n';
			fwrite(outtext, i+1, 1, ses->logfile);
		}
*/
	}
}


/***************************************************************/
/* show a prompt - mud prompt if we're connected/else just a > */
/***************************************************************/

void prompt(struct session* ses)
{
	return; /* for readline -- please god, no pseudo prompting... <beg> */
	
	/*
	char strng[BUFFER_SIZE];
	
	if(ses && !PSEUDO_PROMPT)
	write_line_mud("", ses);
	else if(!is_split)
	write(1, "> ", 2);
	else {
		sprintf(strng, "8> 7[%d;%df", input_row, input_col);
		write(1, strng, strlen(strng));
		display_col += 2;
	} 
	*/
}

/**********************************************************/
/* do all of the functions to one line of buffer		 */
/**********************************************************/
//TO AVOID CRASH: always check valid_session() after more than
//one command related to session, and stick on 1 same session
//in ONE function, return the changed one if it was changed.
struct session* do_one_line(char *line, struct session* ses)
{
	struct session *s = ses;
	if(!presub && !ignore && !ses->ignore){
		s = check_all_actions(line, ses);
		if(!valid_session(ses)) return s;
	}
	if(!togglesubs && !do_one_antisub(line, ses))
		do_one_sub(line, ses);
	if(presub && !ignore && !ses->ignore){
		s = check_all_actions(line, ses);
		if(!valid_session(ses)) return s;
	}

	do_one_high(line, ses);
	return s;
}
