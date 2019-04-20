/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: function.c - functions related to the functions             */
/* Modified the substitute.c file to create this,   SN              */
/*********************************************************************/
#include <config.h>
#include "tintin.h"

#include <ctype.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#elif defined(HAVE_STRINGS_H)
#include <strings.h>
#endif

char last_result[BUFFER_SIZE];

#include "action.proto"
#include "llist.proto"
#include "main.proto"
#include "parse.proto"
#include "rl.proto"
#include "help.proto"
#include "ivars.proto"
#include "misc.proto"
#include "net.proto"
#include "path.proto"
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


/************************************************************************/
/* Usage of functions:                                                  */
/************************************************************************/
/* ex1:                                                                 */
/* #function {rnd} {#random {temp} {%1,%2};#result {$temp};#unvar temp} */
/* #showme A random number between 0 and 100: @rnd{0 100}               */
/************************************************************************/
/* ex2:                                                                 */
/* #function {ifs} {#ifstringequal {%1} {%2} {#result 1}                */
/*                  else {#result 0}}                                   */
/* #if {!@ifs{{$name} {$myname}} && $spam} {say Hi $name!}              */
/************************************************************************/
/* In short: functions are treated much like variables. Their value is  */
/* a command-line which is executed, and the functions are substituted  */
/* by the parameter last sent to the #result command                    */
/************************************************************************/
/* last ex:                                                             */
/* #function {lastfuncresult} {#nop}                                    */
/* #showme Last use of a function gave @lastfuncresult as result.       */
/************************************************************************/

/*************************/
/* the #function command */
/*************************/
void function_command(const char *arg, struct session* ses)
{
	char left[BUFFER_SIZE], right[BUFFER_SIZE], result[BUFFER_SIZE];
	struct listnode *myfunctions, *ln;
	myfunctions=(ses) ? ses->myfuncs : common_functions;
	arg=get_arg_in_braces(arg, left,0);
	arg=get_arg_in_braces(arg, right,1);

	if(!*left) {
		tintin_puts2("#THESE FUNCTIONS HAVE BEEN DEFINED:", ses);
		show_list(myfunctions);
		prompt(ses);
	}

	else if(*left && !*right) {
		if ((ln=search_node_with_wild(myfunctions,left))!=NULL) {
			while((myfunctions=search_node_with_wild(myfunctions, left))!=NULL) {
				shownode_list(myfunctions);
			}
			prompt(ses);
		}
		else
			/* same mesvar toggle as aliases */
			if (mesvar[0])
			tintin_puts2("#THAT FUNCTION IS NOT DEFINED.", ses);
	}

	else {
		if((ln=searchnode_list(myfunctions, left))!=NULL)
			deletenode_list(myfunctions, ln);
		insertnode_list(myfunctions, left, right, "0", ALPHA);
		funcnum++;
		if (mesvar[0]) {
			sprintf(result, "#Ok. {%s} now gives {%s}.",left,right);
			tintin_puts2(result, ses);
		}
	}
}

/*****************************/
/* the #unfunction command */
/*****************************/
void unfunction_command(const char* arg, struct session* ses)
{
	char left[BUFFER_SIZE] ,result[BUFFER_SIZE];
	struct listnode *myfunctions, *ln, *temp;
	int flag;
	flag=FALSE;
	myfunctions=(ses) ? ses->myfuncs : common_functions;
	temp=myfunctions;
	arg=get_arg_in_braces(arg,left,1);
	while ((ln=search_node_with_wild(temp, left))!=NULL) {
		if (mesvar[0]) {
			sprintf(result, "#Ok. {%s} is no longer a function.", ln->left);
			tintin_puts2(result, ses);
		}
		deletenode_list(myfunctions, ln);
		flag=TRUE;
		temp=ln;
	}
	if (!flag && mesvar[2])
		tintin_puts2("#THAT FUNCTION IS NOT DEFINED.", ses);
}


/***********************/
/* the #result command */
/***********************/
void result_command(const char *arg, struct session* ses)
{
	char left[BUFFER_SIZE], temp[BUFFER_SIZE];
	arg=get_arg_in_braces(arg, left,1);
	substitute_vars(left,temp);
	substitute_myvars(temp,left,ses);
	strcpy(last_result,left);
}

/*************************************************************************/
/* copy the arg text into the result-space, but substitute the functions */
/* @<function>{params} with the values they stand for                    */
/*************************************************************************/
/* this is just a modification of substitute_myvars in variables.c       */
/*************************************************************************/
/* functions also allow the 'new' format used for variables. that is     */
/* @{name}{params} to allow complex names containing non-alpha chars.    */
/*************************************************************************/
void substitute_functions(const char *arg, char *result, struct session* ses)
{
	char *bound, funcname[BUFFER_SIZE], temp[BUFFER_SIZE];
	int nest=0,counter,funclen;
	int specfunc;
	char params[BUFFER_SIZE];
	int i, simple, numands, caution;
	char *cpsource, *cpsource2;
	struct listnode *ln, *tempfuncs;
	
	tempfuncs=(ses) ? ses->myfuncs : common_functions;
	//	fflush(stdout);
	
	bound = result + BUFFER_SIZE-1; //prevent overflow, chitchat 10/17/2002
	while(*arg) {
		if(*arg=='@') { 			 /* substitute function */
			counter=0;
			while (*(arg+counter)=='@')
				counter++;
			funclen=0;
			
			if(*(arg+counter)!=DEFAULT_OPEN) {			/* using the @{name} or @name format? */
				specfunc=FALSE;
				while(Is_Alnum(*(arg+funclen+counter)))
					funclen++;
				if (funclen>0)
					strncpy(funcname,arg+counter,funclen);
				*(funcname+funclen)='\0';
			}
			else {
				specfunc=TRUE;
				get_arg_in_braces(arg+counter, temp, 0);
				substitute_myvars(temp, funcname, ses);	/* RECURSIVE CALL */
				funclen=strlen(temp);
			}
			
			if(specfunc) funclen+=2;
			
			if (counter==nest+1 && !isdigit(*(arg+counter+1))) {
				if((ln=searchnode_list(tempfuncs, funcname))!= NULL) {
					arg+=counter+funclen;
					
					if (*arg==DEFAULT_OPEN) {
						/* get parameters and change the %0-%9 vars */
						arg=get_arg_in_braces(arg, params, 1);
						
						/* The following chunk of code is copied from parse_input in parse.c */
						strcpy(vars[0], params);
						for(i=1, cpsource=params; i<10; i++) {
							/* Next lines CHANGED to allow argument grouping with aliases */
							while (*cpsource == ' ' || *cpsource=='\t')
								cpsource++;
							simple = (*cpsource != DEFAULT_OPEN); //simple means no need worry about { } pairs, chitchat, 2/18/2000
							numands = !simple;
							cpsource += numands;
							caution = 0;
							for(cpsource2=cpsource; *cpsource2 &&
								((simple && *cpsource2!=' ' && *cpsource2!='\t') || (!simple && numands>0))
								; cpsource2++){
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
					}
					substitute_vars(ln->right, temp);
					parse_input(temp, ses);
					strncpy(result, last_result, bound-result);
					result+=strlen(result);
				}
				else {
					strncpy(result,arg,counter+funclen);
					result+=funclen+counter;
					arg+=counter+funclen;
				}
			}
			else	{
				strncpy(result,arg,counter+funclen);
				result+=funclen+counter;
				arg+=funclen+counter;
			}
		}
		else if(result>=bound){//added by chitchat, 10/17/2002, shall check those strncpy w/o specifying bounds later too
			break;
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
		else {
			*result++= *arg++;
		}
	}
	*result='\0';
}
