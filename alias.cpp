/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: alias.c - funtions related the the alias command            */
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

#include "alias.proto"
#include "llist.proto"
#include "main.proto"
#include "parse.proto"
#include "rl.proto"
#include "variables.proto"

/**********************/
/* the #alias command */
/**********************/

void alias_command(const char *arg, struct session* ses)
{
	char left[BUFFER_SIZE], right[BUFFER_SIZE], arg2[BUFFER_SIZE];
	struct listnode *myaliases, *ln;
	
	myaliases = (ses ? ses->aliases : common_aliases);
	arg = get_arg_in_braces(arg, arg2, 0);
	substitute_myvars(arg2, left, ses);
	arg = get_arg_in_braces(arg, right, 1);
	
	if(!*left) {
		tintin_puts2("#Defined aliases:", ses);
		show_list(myaliases);
		prompt(ses);
	}
	else if(*left && !*right) {
		if((ln = search_node_with_wild(myaliases, left))) {
			while((myaliases = search_node_with_wild(myaliases, left)))
				shownode_list(myaliases);
			prompt(ses);
		}
		else if(mesvar[0]) {
			sprintf(right, "#No match(es) found for {%s}", left);
			tintin_puts2(right, ses);
		}
	}
	else {
		if((ln = searchnode_list(myaliases, left)))
			deletenode_list(myaliases, ln);
		insertnode_list(myaliases, left, right, "0", ALPHA);
		if(mesvar[0]) {
			sprintf(arg2, "#Ok. {%s} aliases {%s}.", left, right);
			tintin_puts2(arg2, ses);
		}
		alnum++;
	}
}

/************************/
/* the #unalias command */
/************************/

void unalias_command(const char *arg, struct session* ses)
{
	char left[BUFFER_SIZE], result[BUFFER_SIZE];
	struct listnode *myaliases, *ln;
	int flag = FALSE;
	
	myaliases = (ses ? ses->aliases : common_aliases);
	arg = get_arg_in_braces(arg, left, 1);
	while((ln = search_node_with_wild(myaliases, left))) {
		if(mesvar[0]) {
			sprintf(result, "#Ok. {%s} is no longer an alias.", ln->left);
			tintin_puts2(result, ses);
		}
		deletenode_list(myaliases, ln);
		flag = TRUE;
	}
	
	if(!flag && mesvar[0]) {
		sprintf(result, "#No match(es) found for {%s}", left);
		tintin_puts2(result, ses);
	}    
}

/****************************/
/* the #al2variable command */
/****************************/

void al2variable_command(const char *arg, struct session* ses)
{
	char left[BUFFER_SIZE], right[BUFFER_SIZE], arg2[BUFFER_SIZE];
	struct listnode *myaliases, *ln, *tempvars;
	
	myaliases = (ses ? ses->aliases : common_aliases);
	arg = get_arg_in_braces(arg, arg2, 0);
	substitute_myvars(arg2, left, ses);
	arg = get_arg_in_braces(arg, arg2, 1);
	substitute_myvars(arg2, right, ses);
	
	if(!*left || !*right) {
		tintin_puts2("#Usage: #al2variable {var_to_be_assigned} {alias_to_be_converted}", ses);
		prompt(ses);
	}
	else {
		tempvars = (!(*left=='_' && left[1]) && ses) ? ses->myvars : common_myvars;
		if((ln=searchnode_list(tempvars, left))!=NULL)
			deletenode_list(tempvars, ln);
		insertnode_list(tempvars, left, 
			(ln = searchnode_list(myaliases, right)) ? ln->right : "",
			"0", ALPHA);
		varnum++;
		if (mesvar[5]) {
			sprintf(arg2, "#Ok. $%s is now set to {%s}.",left, right);
			tintin_puts2(arg2, ses);
		}
	}
}

/**********************/
/* the #var2alias command */
/**********************/

void var2alias_command(const char *arg, struct session* ses)
{
	char left[BUFFER_SIZE], right[BUFFER_SIZE], arg2[BUFFER_SIZE];
	struct listnode *myaliases, *ln;
	
	myaliases = (ses ? ses->aliases : common_aliases);
	arg = get_arg_in_braces(arg, arg2, 0);
	substitute_myvars(arg2, left, ses);
	arg = get_arg_in_braces(arg, arg2, 1);
	substitute_myvars(arg2, right, ses); //that's the only thing different that alias_command()
	
	if(!*left || !*right) {
		tintin_puts2("#Usage: #var2alias {alias_to_be_assigned} {$variable_to_be_converted}", ses);
		prompt(ses);
	}
	else {
		if((ln = searchnode_list(myaliases, left)))
			deletenode_list(myaliases, ln);
		insertnode_list(myaliases, left, right, "0", ALPHA);
		if(mesvar[0]) {
			sprintf(arg2, "#Ok. {%s} aliases {%s}.", left, right);
			tintin_puts2(arg2, ses);
		}
		alnum++;
	}
}

