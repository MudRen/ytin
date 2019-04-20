/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: antisub.c - functions related to the substitute command     */
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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "action.proto"
#include "antisub.proto"
#include "llist.proto"
#include "main.proto"
#include "parse.proto"
#include "rl.proto"

/*******************************/
/* the #antisubstitute command */
/*******************************/

void parse_antisub(const char *arg, struct session* ses)
{
	char left[BUFFER_SIZE], result[BUFFER_SIZE];
	struct listnode *myantisubs, *ln;
	
	myantisubs = (ses ? ses->antisubs : common_antisubs);
	arg = get_arg_in_braces(arg, left, 1);
	
	if(!*left) {
		tintin_puts("#THESE ANTISUBSTITUTES HAS BEEN DEFINED:", ses);
		show_list(myantisubs);
		prompt(ses);
	}
	else {
		if((ln = searchnode_list(myantisubs, left)))
			deletenode_list(myantisubs, ln);
		insertnode_list(myantisubs, left, left, "0", ALPHA);
		antisubnum++;
		if(mesvar[3]) {
			sprintf(result, "Ok. Any line with {%s} will not be subbed.", left);
			tintin_puts2(result, ses);
		}
	}
}

/*********************************/
/* the #unantisubstitute command */
/*********************************/

void unantisubstitute_command(const char *arg, struct session* ses)
{
	char left[BUFFER_SIZE], result[BUFFER_SIZE];
	struct listnode *myantisubs, *ln;
	int flag = FALSE;
	
	myantisubs = (ses ? ses->antisubs : common_antisubs);
	arg = get_arg_in_braces(arg, left, 1);
	while((ln = search_node_with_wild(myantisubs, left))) {
		if(mesvar[3]) {
			sprintf(result, "#Ok. Lines with {%s} will now be subbed.", ln->left);
			tintin_puts2(result, ses);
		}
		deletenode_list(myantisubs, ln);
		flag = TRUE;
	}
	
	if(!flag && mesvar[3])
		tintin_puts2("#THAT ANTISUBSTITUTE IS NOT DEFINED.", ses);
}


int do_one_antisub(const char *line, struct session* ses)
{
	struct listnode *ln = ses->antisubs;
	
	while((ln = ln->next)) 
		if(check_one_action(line, ln->left, ses))
		return(TRUE);
	
	return(FALSE);
}
