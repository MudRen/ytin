/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: substitute.c - functions related to the substitute command  */
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

#include "action.proto"
#include "llist.proto"
#include "main.proto"
#include "parse.proto"
#include "rl.proto"
#include "variables.proto"

/***************************/
/* the #substitute command */
/***************************/

void parse_sub(const char *arg, struct session* ses)
{
	char left[BUFFER_SIZE], right[BUFFER_SIZE], result[BUFFER_SIZE];
	struct listnode *mysubs, *ln;
	int nullsub;
	
	mysubs = (ses ? ses->subs : common_subs);
	arg = get_arg_in_braces(arg, left, 0);
	nullsub = (strstr(arg, "{}")!=NULL);
	arg = get_arg_in_braces(arg, right, 1);
	
	if(!*left) {
		tintin_puts2("#THESE SUBSTITUTES HAVE BEEN DEFINED:", ses);
		show_list(mysubs);
		prompt(ses);
	}
	else if(*left && !nullsub && !*right) {
		if((ln = search_node_with_wild(mysubs, left))) {
			while((mysubs = search_node_with_wild(mysubs, left)))
				shownode_list(mysubs);
			prompt(ses);
		}
		else if(mesvar[2])
			tintin_puts2("#THAT SUBSTITUTE IS NOT DEFINED.", ses);
	}
	else {
		if((ln = searchnode_list(mysubs, left)))
			deletenode_list(mysubs, ln);
		insertnode_list(mysubs, left, right, "0", ALPHA);
		subnum++;
		if(strcmp(right, "."))
			sprintf(result, "#Ok. {%s} now replaces {%s}.", right, left);
		else
			sprintf(result, "#Ok. {%s} is now gagged.", left);
		if(mesvar[2])
			tintin_puts2(result, ses);
	}
}

/***************************/
/* the #gag command */
/***************************/

void gag_command(const char *arg, struct session* ses)
{
	char result[BUFFER_SIZE];
	
	result[0] = '{';
	arg = get_arg_in_braces(arg, result+1, 1);
	strcat(result, "} {}");
	parse_sub(result, ses);
}
/*****************************/
/* the #unsubstitute command */
/*****************************/

void unsubstitute_command(const char *arg, struct session* ses)
{
	char left[BUFFER_SIZE], result[BUFFER_SIZE];
	struct listnode *mysubs, *ln;
	int flag = FALSE;
	
	mysubs = (ses ? ses->subs : common_subs);
	arg = get_arg_in_braces(arg, left, 1);
	while((ln = search_node_with_wild(mysubs, left))) {
		if(mesvar[2]) {
			if(*ln->right == '.' && !ln->right[1])
				sprintf(result, "#Ok. {%s} is no longer gagged.", ln->left);
			else
				sprintf(result, "#Ok. {%s} is no longer substituted.", ln->left);
			tintin_puts2(result, ses);
		}
		deletenode_list(mysubs, ln);
		flag = TRUE;
	}
	
	if(!flag && mesvar[2])
		tintin_puts2("#THAT SUBSTITUTE IS NOT DEFINED.", ses);
}


void do_one_sub(char *line, struct session* ses)
{
	char temp[BUFFER_SIZE];
	struct listnode *ln = (ses ? ses->subs : common_subs);

	while((ln = ln->next)) 
		if(check_one_action(line, ln->left, ses)) {
			if(!*(ln->right)) newline_tag = 0; //gag case
			prepare_actionalias(ln->right, temp, ses);
			substitute_myvars(temp, line, ses);

		/*  Fix the problem with ANSI colours after substitutions - vastheman 2001-09-19    */
			for (int i = 0; line[i]; i++)
				if ((line[i] == '\\') && (line[i + 1] == ';'))
					strcpy(&line[i], &line[i + 1]);
		}
}
