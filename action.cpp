/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: action.c - funtions related to the action command           */
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

#ifndef _WINDOWS
#include <readline/readline.h>
extern int readline_echoing_p;
#endif

#include "action.proto"
#include "ansi.proto"
#include "llist.proto"
#include "main.proto"
#include "parse.proto"
#include "rl.proto"
#include "variables.proto"

static int var_len[10];
static const char *var_ptr[10];

/***********************/
/* the #action command */
/***********************/

/*  Priority code added by Robert Ellsworth 2/2/94 */

void action_command(const char *arg, struct session* ses)
{
	char left[BUFFER_SIZE], right[BUFFER_SIZE], result[BUFFER_SIZE];
	char pr[BUFFER_SIZE];
	struct listnode *myactions, *ln;
	
	myactions = (ses ? ses->actions : common_actions);
	arg = get_arg_in_braces(arg, left, 0);
	arg = get_arg_in_braces(arg, right, 1);
	arg = get_arg_in_braces(arg, pr, 1);
	if(!*pr)
		strcpy(pr, "5"); /* defaults priority to 5 if no value given */
	if(!*left) {
		tintin_puts2("#Defined actions:", ses);
		show_list_action(myactions);
		prompt(ses);
	}
	else if(*left && !*right) {
		if((ln = search_node_with_wild(myactions,left))) {
			while((myactions = search_node_with_wild(myactions, left))) {
				shownode_list_action(myactions);
			}
			prompt(ses);
		}
		else
			if(mesvar[1])
			tintin_puts("#That action is not defined.", ses);
	}
	else {
		if((ln = searchnode_list(myactions, left)))
			deletenode_list(myactions, ln);
		insertnode_list(myactions, left, right, pr, PRIORITY);
		if(mesvar[1]) {
			sprintf(result, "#Ok. {%s} now triggers {%s} @ {%s}", left, right, pr);
			tintin_puts2(result, ses);
		}
		acnum++;
	}
}

/*************************/
/* the #unaction command */
/*************************/

void unaction_command(const char *arg, struct session* ses)
{
	char left[BUFFER_SIZE], result[BUFFER_SIZE];
	struct listnode *myactions, *ln;
	int flag = FALSE;
	
	myactions = (ses ? ses->actions : common_actions);
	arg = get_arg_in_braces(arg, left, 1);
	while((ln = search_node_with_wild(myactions, left))) {
		if(mesvar[1]) {
			sprintf(result, "#Ok. {%s} is no longer a trigger.", ln->left);
			tintin_puts2(result, ses);
		}
		deletenode_list(myactions, ln);
		flag = TRUE;
	}
	
	if(!flag && mesvar[1]) {
		sprintf(result, "#No match(es) found for {%s}", left);
		tintin_puts2(result, ses);
	}
}

/**************************************************************************/
/* run throught each of the commands on the right side of an alias/action */
/* expression, call substitute_text() for all commands but #alias/#action */
/**************************************************************************/

void prepare_actionalias(const char *string, char *result, struct session* ses)
{
	/*  char arg[BUFFER_SIZE];
	*/
	*result = '\0';
	/*********marked by chitchat, for fix of recursive variables**********
	substitute_vars(string, arg);
	substitute_myvars(arg, result, ses);
	********************************************************************/
	substitute_vars(string, result);
}

/*************************************************************************/
/* copy the arg text into the result-space, but substitute the variables */
/* %0..%9 with the real variables                                        */
/*************************************************************************/

void substitute_vars(const char *arg, char *result)
{
	int nest = 0, numands, n;
	char *ptr;
	
	while(*arg) {
		if(*arg == '%') { /* substitute variable */
			numands = 0;
			while(arg[numands] == '%')
				numands++;
			if(isdigit(arg[numands]) && numands == nest+1) {
				n = arg[numands]-'0';
				strcpy(result, vars[n]);
				result += strlen(vars[n]);
				arg += numands+1;
			}
			else {
				strncpy(result, arg, numands+1);
				result += numands+1;
				arg += numands+1;
			}
		}
		else if(*arg == '$') { /* substitute variable */
			numands = 0;
			while(arg[numands] == '$') 
				numands++;
			if(isdigit(arg[numands]) && numands == nest+1) {
				n = arg[numands]-'0';
				ptr = vars[n];
				while(*ptr)
					if(*ptr == ';')
						ptr++;
					else if(!(*ptr=='\\' && ptr[1]==0)) //avoid ending \ causing security problem too, chitchat, 2/18/2000
						*result++ = *ptr++;
				arg += numands+1;
			}
			else {
				strncpy(result, arg, numands);
				arg += numands;
				result += numands;
			}
		}
		else if(*arg == DEFAULT_OPEN) {
			nest++;
			*result++ = *arg++;
		}
		else if(*arg == DEFAULT_CLOSE) {
			nest--;
			*result++ = *arg++;
		}
		else if(*arg == '\\' && !nest) {
			while(*arg == '\\')
				*result++ = *arg++;
			if(*arg == '%' || *arg == '$') {
				result--;
				*result++ = *arg++;
				*result++ = *arg++;
			}
		}
		else if(*arg)
			*result++ = *arg++;
	}
	
	*result = '\0';
}

/**********************************************/
/* check actions from a sessions against line */
/**********************************************/

struct session *check_all_actions(const char *line, struct session* ses)
/* This Function is modified by yansong chen to fix miss action bug */
{
	struct listnode *ln;
	char linebuf[BUFFER_SIZE];
	char* line_act;
	
	/*user has control whether strip then action or action 1st, chitchat*/
	if(stripcolor) strip_ansi(line, linebuf);
	else {
		strncpy(linebuf, line, BUFFER_SIZE);
		if(linebuf[strlen(linebuf)-1] == '\r')
			linebuf[strlen(linebuf)-1]=0;
	}
	
	if (ses) {
		line_act = ses->act_buf;
		strcat(line_act, linebuf);
	} else line_act = linebuf;
	
	ln = (ses ? ses->actions : common_actions);
	while((ln = ln->next)) {
		if(check_one_action(line_act, ln->left, ses)) {
			char buffer[BUFFER_SIZE], strng[BUFFER_SIZE];
			
			prepare_actionalias(ln->right, buffer, ses);
			if(echo && activesession == ses) { 
				sprintf(strng, "[ACTION: {%s}{%s} @ {%s}]", ln->left, (echo>1) ? ln->right : buffer, ln->pr);
				tintin_puts2(strng, ses);
			}
			if (ses) ses->act_buf[0] = '\0';
			return parse_input(buffer, ses);
		}
	}
	return ses;
}

int match_a_string(const char *line, const char *mask)
{
	const char *lptr = line;
	
	while(*lptr && *mask && (*mask != '%' || !isdigit(mask[1])))
		if(*lptr++ != *mask++)
		return(-1);
	
	if(!*mask || (*mask == '%' && isdigit(mask[1])))
		return((int)(lptr-line));
	
	return(-1);
}

int check_one_action(const char *line, const char *action, struct session* ses)
{
	int i,j, k;
	
	if(check_a_action(line, action, ses)) {
		for(i = 0; i < 10; i++){
/* remark the codes below, now escape all ; in the action to \; , chitchat 1/28/2000
			if(var_len[i] != -1) {
			strncpy(vars[i], var_ptr[i], var_len[i]);
			vars[i][var_len[i]] = '\0'; 
			}
*/
			for(j=0, k=0; j<var_len[i] && k<BUFFER_SIZE; ){ //here we check BUFFER_SIZE, let's correct these later for all
				                                            //such cases, hate the crash simply caused by the buffer overflow
				if(var_ptr[i][j] == ';') vars[i][k++] = '\\';
				vars[i][k++] = var_ptr[i][j++];
			}
			vars[i][k] = 0;
		}
		return(TRUE);
	}
	else
		return(FALSE);
}

/******************************************************************/
/* check if a text triggers an action and fill into the variables */
/* return TRUE if triggered                                       */
/******************************************************************/

int check_a_action(const char *line, const char *action, struct session* ses)
{   
	char result[BUFFER_SIZE];
	char *temp2, *tptr = result;
	const char *line2;
	int  i, len, flag;
	
	for(i = 0; i < 10; i++)
		var_len[i] = -1;
	substitute_myvars(action, result, ses);
	
	if(*tptr == '^') {
		if((len = match_a_string(line, ++tptr)) == -1)
			return(FALSE);
		line += len;
		tptr += len;
	}
	else {
		flag = TRUE;
		len = -1;
		while(*line && flag)
			if((len = match_a_string(line, tptr)) != -1)
			flag = FALSE;
		else
			line++;
		
		if(len != -1) {
			line += len;
			tptr += len;
		}
		else
			return(FALSE);
	}
	
	while(*line && *tptr) {
		temp2 = tptr+2;
		if(!*temp2) {
			var_len[tptr[1]-'0'] = strlen(line);
			var_ptr[tptr[1]-'0'] = line;
			return(TRUE);
		}
		
		line2 = line;
		flag = TRUE;
		len = -1;
		
		while(*line2 && flag)
			if((len = match_a_string(line2, temp2)) != -1)
			flag = FALSE;
		else 
			line2++;
		
		if(len != -1) {
			var_len[tptr[1]-'0'] = line2-line;
			var_ptr[tptr[1]-'0'] = line;
			line = line2+len;
			tptr = temp2+len;
		}
		else
			return(FALSE);
	}
	
	return(*tptr ? FALSE : TRUE);
}       
