/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: variables.c - functions related the the variables      */
/*               TINTIN ++               */
/*     (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t       */
/*           coded by peter unold 1992           */
/*********************************************************************/
#include <config.h>
#include <ctype.h>
#include "tintin.h"

#if defined(HAVE_STRING_H)
#include <string.h>
#endif
#if defined(HAVE_STRINGS_H)
#include <strings.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_TIME_H
#include <time.h>
#endif

#include "action.proto"
#include "alias.proto"
#include "ansi.proto"
#include "antisub.proto"
#include "files.proto"
#include "highlight.proto"
#include "help.proto"
#include "ivars.proto"
#include "llist.proto"
#include "main.proto"
#include "misc.proto"
#include "net.proto"
#include "parse.proto"
#include "path.proto"
#include "rl.proto"
#include "rlhist.proto"
#include "rltab.proto"
#include "session.proto"
#include "substitute.proto"
#include "text.proto"
#include "ticks.proto"
#include "utils.proto"
#include "variables.proto"
#include "function.proto"
#include "rltick.proto"

/*************************/
/* the #variable command */
/*************************/
void assign_var(const char* left, const char* right, struct session* ses)
{
	struct listnode *tempvars, *ln;
	tempvars=(!(*left=='_' && left[1]) && ses) ? ses->myvars : common_myvars;
	if((ln=searchnode_list(tempvars, left))!=NULL)
		deletenode_list(tempvars, ln);
	else varnum++;
	insertnode_list(tempvars, left, right, "0", ALPHA);
}

void var_command(const char *arg, struct session* ses)
{
	char left[BUFFER_SIZE], right[BUFFER_SIZE], arg2[BUFFER_SIZE];
	struct listnode *tempvars, *ln;
	int nullvar;
	/* char right2[BUFFER_SIZE]; */
	tempvars=(ses) ? ses->myvars : common_myvars;
	arg=get_arg_in_braces(arg, arg2,0);
	substitute_myvars(arg2, left, ses); /*added by chitchat*/
	nullvar = (strstr(arg, "{}")!=NULL);
	arg=get_arg_in_braces(arg, arg2,1);
	substitute_myvars(arg2, right, ses); /*added by chitchat*/
	if(!*left) {
		tintin_puts2("#THESE VARIABLES HAVE BEEN SET:", ses);
		show_list(tempvars);
		prompt(ses);
	}
	
	else if(*left && !nullvar && !*right) { //changed to handle #variable {x} {} to reset variable
		if(*left=='_' && left[1]) tempvars = common_myvars; /*treat $_ as local variable */
		if ((ln=search_node_with_wild(tempvars,left))!=NULL) {
			while((tempvars=search_node_with_wild(tempvars, left))!=NULL) {
				shownode_list(tempvars);
			}
			prompt(ses);
		}
		else
			if (mesvar[5])
			tintin_puts2("#THAT VARIABLE IS NOT DEFINED.", ses);
	}
	
	else {
		if(*left=='_' && left[1]) tempvars = common_myvars; /*$_ is local var */
		if((ln=searchnode_list(tempvars, left))!=NULL)
			deletenode_list(tempvars, ln);
		else varnum++;
		insertnode_list(tempvars, left, right, "0", ALPHA);
		if (mesvar[5]) {
			sprintf(arg2, "#Ok. $%s is now set to {%s}.",left, right);
			tintin_puts2(arg2, ses);
		}
	}
}
/************************/
/* the #unvar  command */
/************************/
void unvar_command(const char *arg, struct session* ses)
{
	char left[BUFFER_SIZE] ,result[BUFFER_SIZE];
	struct listnode *tempvars, *ln, *temp;
	int flag;
	flag=FALSE;
	tempvars=(!(*arg=='_' && arg[1]) && ses) ? ses->myvars : common_myvars;
	temp=tempvars;
	arg=get_arg_in_braces(arg,left,1);
	while ((ln=search_node_with_wild(temp, left))!=NULL) {
		if (mesvar[5]) {
			sprintf(result, "#Ok. $%s is no longer a variable.", ln->left);
			tintin_puts2(result, ses);
		}
		deletenode_list(tempvars, ln);
		flag=TRUE;
		temp=tempvars;
	}
	if (!flag && mesvar[5])
		tintin_puts2("#THAT VARIABLE IS NOT DEFINED.", ses);
}


/*************************************************************************/
/* copy the arg text into the result-space, but substitute the variables */
/* $<string> with the values they stand for               */
/*                                    */
/* Notes by Sverre Normann (sverreno@stud.ntnu.no):           */
/* xxxxxxxxx 1996: added 'simulation' of secstotick variable       */
/* Tue Apr 8 1997: added ${<string>} format to allow use of non-alpha  */
/*         characters in variable name              */
/*************************************************************************/
/* Note: $secstotick will return number of seconds to tick (from tt's  */
/*    internal counter) provided no variable named 'secstotick'    */
/*    already exists                         */
/*************************************************************************/
void substitute_myvars(const char *arg, char* result, struct session* ses)
{
	char temp[BUFFER_SIZE];
	substitute_functions(arg, temp, ses);
	substitute_myvariables(temp, result, ses);
}

void substitute_myvariables(const char *arg, char *result, struct session* ses)
{
	char *bound, varname[BUFFER_SIZE], temp[BUFFER_SIZE], buf[BUFFER_SIZE];
	int nest=0,counter,varlen, l;
	int specvar;
	static int seed;
	int time_now;
	struct listnode *ln, *tempvars;
	
	if(!seed){ //only initialize seed once
		srand( (unsigned)time( NULL ) );
		seed = 1;
	}
	tempvars=(ses) ? ses->myvars : common_myvars; /*variable local or not is check from varname later */
	fflush(stdout);
	bound = result+BUFFER_SIZE-1; //keep track of the length
	while(*arg) {
		if(*arg=='$') { /* substitute variable */
			counter=0;
			while (*(arg+counter)=='$')
				counter++;
			varlen=0;
			
			/* ${name} code added by Sverre Normann */
			if(*(arg+counter)!=DEFAULT_OPEN) {
				specvar=FALSE;
				/* while(isalpha(*(arg+varlen+counter))) /*changed by chitchat*/
				while(Is_Alnum(*(arg+varlen+counter))) 
					varlen++; 
				if (varlen>0)
					strncpy(varname,arg+counter,varlen);
				*(varname+varlen)='\0';
			}
			else {
				specvar=TRUE;
				get_arg_in_braces(arg+counter, temp, 0);
				substitute_myvars(temp, varname, ses);   /* RECURSIVE CALL */
				varlen=strlen(temp);
			}
			
			if(specvar) varlen+=2;
			
			if (counter==nest+1 && !isdigit(arg[counter])) { //fixed a bug, was isdigit(arg[counter+1]), chitchat, 2/28/2000
				if(!strcmp(varname, "time") || !strcmp(varname,"_time")) //_time makes more sense, since it is global, i.e., does not depend on session, chit, 7/10/2000
				{
					/* wdl new 'time' variable set to secs since started */
					time_now = (int)(time(NULL) - time_started);
					sprintf(buf, "%d", time_now);
					strncpy(result, buf, bound-result);
					result += strlen(result);
				}
				else if(!strcmp(varname, "_msec"))
				{
					/*global variable $_msec of the time since yTin start, chitchat, 7/10/2000*/
					sprintf(buf, "%lu", millisec_started());
					strncpy(result, buf, bound-result);
					result += strlen(result);
				}
				else if (ses && !strcmp(varname,"secstotick")) {
					/* secstotick code added by Sverre Normann */
					sprintf(buf,"%d",timetilltick(ses));
					strncpy(result, buf, bound-result);
					result += strlen(result);
				}
				else if(!strcmp(varname, "_clock"))
				{
					/*global variable $_clock of the absolute time, chitchat*/
					sprintf(buf, "%lu", time(NULL));
					strncpy(result, buf, bound-result);
					result += strlen(result);
				}
				else if(!strcmp(varname, "_random"))
				{
					sprintf(buf, "%u", rand());
					strncpy(result, buf, bound-result);
					result += strlen(result);
				}
			/*	Virtual operating system variable - vastheman 2001-08-07	*/
				else if(!strcmp(varname, "_OS")) {
#ifndef _WINDOWS
					strncpy(result, "UNIX", bound-result);
#else
					strncpy(result, "WIN32", bound-result);
#endif
					result += strlen(result);
				}
				else if(!strcmp(varname, "HOME"))
				{
					/*variable $HOME*/
					strncpy(result, home, bound-result);
					result += strlen(result);
				}
				else if((ln=searchnode_list(((!(*varname=='_' && varname[1]) && ses) ? tempvars : common_myvars),
					varname))!= NULL)
					/*make $time, $_clock and $_random's priority higher than defined variables*/
				{
					strncpy(result, ln->right, bound-result);
					result += strlen(result);
				}
				else 
				{
					//strncpy(result, arg, counter+varlen);
					//result += varlen+counter;
					l = (bound-result > counter+varlen) ? counter+varlen : bound-result;
					strncpy(result, arg, l); //important, need force length
					result += l;
				}
				arg += varlen+counter;
			}
			else {
				l = (bound-result > counter+varlen) ? counter+varlen : bound-result;
				strncpy(result, arg, l);
				result += l;
				arg += varlen+counter;
			}
		}
		else if(result>=bound){//added by chitchat, 10/17/2002, shall check those strncpy w/o specifying bounds later too
			break;
		}
    else if (big5 && (*arg & 0x80))  {
      *result++ = *arg++ ;
      if ( *arg  && result<bound)
        *result++= *arg++ ;
    }
		else if (*arg==DEFAULT_OPEN) {
			nest++;
			*result++= *arg++;
		}
		else if (*arg==DEFAULT_CLOSE) {
			nest--;
			*result++= *arg++;
		}
		else if (*arg=='\\' && *(arg+1)=='$' && nest==0) {
			arg++;
			*result++= *arg++;
		}
		else
			*result++= *arg++;
	}
	*result='\0';
}


/********************************************************/
/* #delimiter, set the delimiter for seperating lists   */
/* chitchat, 2/20/2000                                  */
/********************************************************/
extern void delimiter_command(const char *arg, struct session* ses)
{
	int show=1;
	char temp[BUFFER_SIZE], arg0[BUFFER_SIZE];
	arg=get_arg_in_braces(arg, temp,0);
	substitute_myvars(temp, arg0, ses);
	if(*arg0){
		if(strcmp(DELIMITER, arg0)==0) show = 0;
		else strcpy(DELIMITER, arg0);
	}
	if(show && mesvar[8]){
		sprintf(temp, "#Delimiter is now {%s}!", DELIMITER);
		tintin_puts2(temp, ses);
	}
}

/********************************************************/
/* the #getlistlength command * By Sverre Normann    */
/********************************************************/
/* Syntax: #getlistlength {destination variable} {list} */
/*****************************************************************/
/* Note: This will return the number of items in the list.    */
/*    An item is either a word, or grouped words in brackets. */
/*    Ex: #getl {listlength} {smile {say Hi!} flip bounce}  */
/*      -> listlength = 4                 */
/*****************************************************************/
void getlistlength_command(const char *arg, struct session* ses)
{
	char left[BUFFER_SIZE], list[BUFFER_SIZE],
	temp[BUFFER_SIZE];
	int i;
	
	arg = get_arg_in_braces(arg, left,0);
	if (!*left) {
		tintin_puts2("#Error - Syntax: #getlistlength {dest var} {list}", ses);
	}
	else {
		substitute_vars(left,temp);
		substitute_myvars(temp,left,ses);
		get_arg_in_braces(arg, list, 1);
		substitute_vars(list,temp);
		substitute_myvars(temp,list,ses);
		arg = list;
		i=0;
		do {
			if(*arg) i++;
			arg = get_item_in_braces(arg, temp, 0);
		} while(*arg);
		sprintf(temp, "%d",i);
		assign_var(left, temp, ses);
	}
}

/***********************/
/* the #forall command */
/***********************/

void forall_command(const char *arg, struct session* ses)
{
	char left[BUFFER_SIZE], right[BUFFER_SIZE], temp[BUFFER_SIZE];
	
	arg = get_arg_in_braces(arg, left,0);
	if (!*left) {
		tintin_puts2("Syntax: #forall {list} {command} (see also: #delimiter)", ses);
	}
	else {
		substitute_vars(left,temp);
		substitute_myvars(temp,left,ses);
		get_arg_in_braces(arg, right, 1);
//		substitute_vars(right,temp); //leave it to the loop
//		substitute_myvars(temp,right,ses); //leave it to parse_input
		arg = left;
		do {
			arg = get_item_in_braces(arg, temp, 0);
			strcpy(vars[0], temp);
			substitute_vars(right, temp);
			parse_input(temp, ses);
		} while(*arg);
	}
}


/******************************************************************/
/* the #getitemnr command * By Sverre Normann           */ 
/******************************************************************/
/* Syntax: #getitemnr {destination variable} {item number} {list} */
/******************************************************************/
/* Note: This will return a specified item from a list.      */
/*    An item is either a word, or grouped words in brackets. */
/*    Ex: #geti {dothis} {2} {smile {say Hi!} flip bounce}  */
/*      -> dothis = say Hi!                 */
/******************************************************************/
void getitemnr_command(const char *arg, struct session* ses) 
{
	char destvar[BUFFER_SIZE], itemnrtxt[BUFFER_SIZE],
	list[BUFFER_SIZE], temp1[BUFFER_SIZE];
	int i, itemnr;
	
	arg = get_arg_in_braces(arg, destvar, 0);
	arg = get_arg_in_braces(arg, itemnrtxt, 0);
	
	if (!*destvar || !*itemnrtxt) {
		tintin_puts2("#Error - Syntax: #getitemnr {destination variable} {item number(0 is 1st item)} {list}", ses);
	}
	else {
		substitute_vars(destvar,temp1);
		substitute_myvars(temp1,destvar,ses);
		
		substitute_vars(itemnrtxt,temp1);
		substitute_myvars(temp1,itemnrtxt,ses);
		itemnr = eval_expression(itemnrtxt);
		
		get_arg_in_braces(arg, list, 1);
		substitute_vars(list,temp1);
		substitute_myvars(temp1,list,ses);
		arg = list;
		i=-1; //change this to C-style index, begin from 0, chitchat, 2/28/2000
		if (itemnr>=0) {
			do {
				arg = get_item_in_braces(arg, temp1, 0);
				i++;
			} while (i!=itemnr);
				
			if (*temp1) {
				assign_var(destvar, temp1, ses);
			}
			else {
				tintin_puts2("#Error in #getitemnr - item doesn't exist!",ses);
			}
		}
	}
}


/*************************/
/* the #tolower command */
/*************************/
void tolower_command(const char *arg, struct session* ses)
{
	char left[BUFFER_SIZE], right[BUFFER_SIZE], arg2[BUFFER_SIZE], *p;
	struct listnode *tempvars, *ln;
	
	tempvars = (!(*arg=='_' && arg[1]) && ses) ? ses->myvars : common_myvars;
	arg = get_arg_in_braces(arg, left,0);
	arg = get_arg_in_braces(arg, arg2,1);
	substitute_myvars(arg2, right, ses);
	if (!*left || !*right) {
		tintin_puts2("#Syntax: #tolower <var> <text>", ses);
	} else {
		if ((ln=searchnode_list(tempvars, left)) != NULL)
			deletenode_list(tempvars, ln);
		for (p=right; *p; p++)
			*p = tolower(*p);
		insertnode_list(tempvars, left, right, "0", ALPHA);
		varnum++;
		if (mesvar[5]) {
			sprintf(arg2, "#Ok. $%s is now set to {%s}.",left, right);
			tintin_puts2(arg2, ses);
		}
	}
}

/*************************/
/* the #toLower command */
/*************************/
void toLower_command(const char *arg, struct session* ses)
{
	char left[BUFFER_SIZE], right[BUFFER_SIZE], arg2[BUFFER_SIZE];
	struct listnode *tempvars, *ln;
	
	tempvars = (!(*arg=='_' && arg[1]) && ses) ? ses->myvars : common_myvars;
	arg = get_arg_in_braces(arg, left,0);
	arg = get_arg_in_braces(arg, arg2,1);
	substitute_myvars(arg2, right, ses);
	if (!*left || !*right) {
		tintin_puts2("#Syntax: #toLower <var> <text>", ses);
	} else {
		if ((ln=searchnode_list(tempvars, left)) != NULL)
			deletenode_list(tempvars, ln);
		*right = tolower(*right);
		insertnode_list(tempvars, left, right, "0", ALPHA);
		varnum++;
		if (mesvar[5]) {
			sprintf(arg2, "#Ok. $%s is now set to {%s}.",left, right);
			tintin_puts2(arg2, ses);
		}
	}
}
/*************************/
/* the #toupper command */
/*************************/
void toupper_command(const char *arg, struct session* ses)
{
	char left[BUFFER_SIZE], right[BUFFER_SIZE], arg2[BUFFER_SIZE], *p;
	struct listnode *tempvars, *ln;
	
	tempvars = (!(*arg=='_' && arg[1]) && ses) ? ses->myvars : common_myvars;
	arg = get_arg_in_braces(arg, left,0);
	arg = get_arg_in_braces(arg, arg2,1);
	substitute_myvars(arg2, right, ses);
	if (!*left || !*right) {
		tintin_puts2("#Syntax: #toupper <var> <text>", ses);
	} else {
		if ((ln=searchnode_list(tempvars, left)) != NULL)
			deletenode_list(tempvars, ln);
		for (p=right; *p; p++)
			*p = toupper(*p);
		insertnode_list(tempvars, left, right, "0", ALPHA);
		varnum++;
		if (mesvar[5]) {
			sprintf(arg2, "#Ok. $%s is now set to {%s}.",left, right);
			tintin_puts2(arg2, ses);
		}
	}
}

/*************************/
/* the #toUpper command */
/*************************/
/*only convert 1st char of string to upper case, chitchat*/
void toUpper_command(const char *arg, struct session* ses)
{
	char left[BUFFER_SIZE], right[BUFFER_SIZE], arg2[BUFFER_SIZE];
	struct listnode *tempvars, *ln;
	
	tempvars = (!(*arg=='_' && arg[1]) && ses) ? ses->myvars : common_myvars;
	arg = get_arg_in_braces(arg, left,0);
	arg = get_arg_in_braces(arg, arg2,1);
	substitute_myvars(arg2, right, ses);
	if (!*left || !*right) {
		tintin_puts2("#Syntax: #toUpper <var> <text>", ses);
	} else {
		if ((ln=searchnode_list(tempvars, left)) != NULL)
			deletenode_list(tempvars, ln);
		*right= toupper(*right);
		insertnode_list(tempvars, left, right, "0", ALPHA);
		varnum++;
		if (mesvar[5]) {
			sprintf(arg2, "#Ok. $%s is now set to {%s}.",left, right);
			tintin_puts2(arg2, ses);
		}
	}
}


#ifdef CHINESE
#include "chinese.cpp"
/*************************/
/* the #ctoi command */
/*************************/
void ctoi_command(const char* arg, struct session* ses)
{
	char left[BUFFER_SIZE], right[BUFFER_SIZE], arg2[BUFFER_SIZE];
	char temp[BUFFER_SIZE];
	struct listnode *tempvars, *ln;
	
	arg=get_arg_in_braces(arg, left,0);
	arg=get_arg_in_braces(arg, temp,1);
	substitute_myvars(temp, right, ses);
	
	if (!*left || !*right) {
		tintin_puts2("#Syntax: #ctoi <var> <text>", ses);
	}
	else {
		tempvars = (!(*left=='_' && left[1]) && ses) ? ses->myvars : common_myvars;
		if ((ln=searchnode_list(tempvars, left)) != NULL)
			deletenode_list(tempvars, ln);
		ctoi(right);
		insertnode_list(tempvars, left, right, "0", ALPHA);
		varnum++;
		if (mesvar[5]) {
			sprintf(arg2, "#Ok. $%s is now set to {%s}.",left, right);
			tintin_puts2(arg2, ses);
		}
	}
}

/*************************/
/* the #gbfilter command */
/*************************/
void GBfilter_command(const char* arg, struct session* ses)
{
	char temp[BUFFER_SIZE];
	if(!arg || *arg==0){
		tintin_puts2("#GBfilter [? | 0 | {char_to_be_filtered}]", ses);
		return;
	}
	if(*arg=='?') {
		if(GBfilter){
			sprintf(temp, "#GBfilter is {%c}.", GBfilter);
			tintin_puts2(temp, ses);
		}
		else
			tintin_puts2("GBfilter option is off.", ses);
		return;
	}
	if(*arg=='0') {
		GBfilter = 0;
		tintin_puts2("#OK, GBfilter option is turned off!", ses);
		return;
	}

	arg=get_arg_in_braces(arg, temp,1);
	if(*temp==0) {
		GBfilter = 0;
		tintin_puts2("#OK, GBfilter option is turned off!", ses);
	}
	else {
		GBfilter = *temp;
//		sprintf(temp, "#GBfilter is {%c}.", GBfilter);
//		tintin_puts2(temp, ses);
	}
}
#endif

/*************************/
/* the #strxxx command series*/
/* we do not need strcpy, strncpy, strcmp,
we need substr(), ord() and chr() like in Perl though
we need strlen, strstr, strrchr, strncmp, (strchr is not necessary)
NOTE: here we use same number of arguments as C functions do,
the return value will be stored to $_ as well
since we cannot return a pointer, the returned value is index of the pointer in strstr() ...
/*************************/
void str_command(const char* command, const char* arg, struct session* ses)
{
	char arg0[BUFFER_SIZE], arg1[BUFFER_SIZE], arg2[BUFFER_SIZE], temp[BUFFER_SIZE];
	int i;
	char *ptr;
	
	arg=get_arg_in_braces(arg, temp,0);
	substitute_myvars(temp, arg0, ses);
	arg=get_arg_in_braces(arg, temp,0);
	substitute_myvars(temp, arg1, ses);
	arg=get_arg_in_braces(arg, temp,1);
	substitute_myvars(temp, arg2, ses);
	
	if(!strcmp(command, "strlen")){
		i = strlen(arg0);
	}
	else if(!strcmp(command, "strstr")){
		ptr = strstr(arg0, arg1);
		if(ptr) i = ptr - arg0;
		else i=-1;
	}
	else if(!strcmp(command, "strrchr")){
		ptr = strrchr(arg0, *arg1);
		if(ptr) i = ptr - arg0;
		else i=-1;
	}
	else if(!strcmp(command, "strncmp")){
		i = eval_expression(arg2); //3/15/2000: make it more flexible
		i = strncmp(arg0, arg1, i);
	}
	else {
		tintin_puts2("#AVAILABLE string COMMANDS: strstr, strrchr, strncmp, OTHER CHOICES NOT NEEDED!", ses);
		tintin_puts2("#SEE ALSO: substr, ord, chr, THESE ARE Perl LIKE COMMANDS", ses);
		return;
	}
	sprintf(temp, "%d", i);
	assign_var("_", temp, ses);
}

/********************/
/* substr, similar to Perl's substr()
not so smart yet, only take explicitly begin/end position
return value assigned to $_
Format: #substr {string} {offset} {length}
if length<0, truncate to left that long from end
/********************/
void substr_command(const char* arg, struct session* ses)
{
	char arg0[BUFFER_SIZE], arg1[BUFFER_SIZE], arg2[BUFFER_SIZE], temp[BUFFER_SIZE];
	int l, i, j;
	
	arg=get_arg_in_braces(arg, temp,0);
	substitute_myvars(temp, arg0, ses);
	arg=get_arg_in_braces(arg, temp,0);
	substitute_myvars(temp, arg1, ses);
	arg=get_arg_in_braces(arg, temp,1);
	substitute_myvars(temp, arg2, ses);
	
	l = strlen(arg0);
	i = eval_expression(arg1); //3/15/2000: make it more flexible
	j = eval_expression(arg2);
	if(i<=0) i=0;
	if(i>l) i=l;
	if(j<0) {
		j += l;
		if(j<i) j=i;
	}
	else {
		j += i;
		if(j>l) j=l;
	}
	arg0[j]=0;
	assign_var("_", arg0+i, ses);
}

/********************/
/* chr(), similar to Perl's chr()
/********************/
void chr_command(const char* arg, struct session* ses)
{
	char arg0[BUFFER_SIZE], arg1[BUFFER_SIZE], temp[BUFFER_SIZE];
	int i;
	
	arg=get_arg_in_braces(arg, temp,0);
	substitute_myvars(temp, arg0, ses);
	arg=get_arg_in_braces(arg, temp,1);
	substitute_myvars(temp, arg1, ses);
	
	i = eval_expression(arg1); //3/15/2000: make it more flexible
	sprintf(temp, "%c", i);
	assign_var(arg0, temp, ses);
}

/********************/
/* ord(), similar to Perl's ord()
/********************/
void ord_command(const char* arg, struct session* ses)
{
	char arg0[BUFFER_SIZE], arg1[BUFFER_SIZE], temp[BUFFER_SIZE];
	int i;
	
	arg=get_arg_in_braces(arg, temp,0);
	substitute_myvars(temp, arg0, ses);
	arg=get_arg_in_braces(arg, temp,1);
	substitute_myvars(temp, arg1, ses);
	
	i = (unsigned char)arg1[0];
	sprintf(temp, "%d", i);
	assign_var(arg0, temp, ses);
}
/********************/
/* stripansi
/********************/
void stripansi_command(const char* arg, struct session* ses)
{
	char left[BUFFER_SIZE], right[BUFFER_SIZE], temp[BUFFER_SIZE];
	
	arg=get_arg_in_braces(arg, temp,0);
	substitute_myvars(temp, left, ses);
	arg=get_arg_in_braces(arg, temp,1);
	substitute_myvars(temp, right, ses);
	
	assign_var(left, strip_ansi(right,temp), ses);
}

