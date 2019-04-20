/*********************************************************************/
/* file: macro.c - funtions related to the macro command             */
/* yTin, yet another TinTin++, compiles under both Windows and Unix  */
/*                     coded by chitchat, 2/8/2000                   */
/*********************************************************************/

/* This file has been radically modified for UNIX macro handling.    */
/* The macro command has very different syntax under Windows and     */
/* UNIX.  Please read the notes on CVS - vastheman 2001-08-07        */

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

#include "llist.proto"
#include "macro.proto"
#include "main.proto"
#include "parse.proto"
#include "rl.proto"
#include "variables.proto"

int keycapture = 0;     /* moved by vastheman - 2001-08-08      */

/***************************************/
/* dispatch a PFn-key triggered action */
/***************************************/

#ifndef _WINDOWS
void keypad_dispatch (int count, int key)
{
	struct listnode *ln;
	char pattern[6] = "{PFx";

	pattern[3] = toupper(key) + '1' - 'P';
	if (keycapture) {       /* live key capture support - vastheman 2001-08-08      */
		pattern[4] = '}';
		pattern[5] = 0;
		tintin_puts2(pattern, activesession);
	} else if ((ln = searchnode_list((activesession ? activesession->macros : common_macros), pattern + 1)))
		parse_input(ln->right, activesession);
	else
		fputc('\a', stdout);
}
#endif

/****************************************/
/* dispatch a meta-key triggered action */
/****************************************/

#ifndef _WINDOWS
void meta_dispatch (int count, int key)
{
	struct listnode *ln;
	char pattern[6] = "{M-x";

	pattern[3] = toupper(key & 0x7F);
	if (keycapture) {       /* live key capture support - vastheman 2001-08-08      */
		pattern[4] = '}';
		pattern[5] = 0;
		tintin_puts2(pattern, activesession);
	} else if ((ln = searchnode_list((activesession ? activesession->macros : common_macros), pattern + 1)))
		parse_input(ln->right, activesession);
	else
		rl_insert(count, key);
}
#endif

/********************************/
/* set up mappings for PFn keys */
/********************************/

#ifndef _WINDOWS
void macro_init (void)
{
	char sequence[4] = "\x1Bxx";

	for (sequence[1] = 'O', sequence[2] = 'P'; sequence[2] <= 'S'; sequence[2]++) {
		rl_generic_bind(ISFUNC, sequence, (char *)keypad_dispatch, rl_get_keymap());
		rl_generic_bind(ISFUNC, sequence, (char *)keypad_dispatch, vi_insertion_keymap);
	}
	for (sequence[1] = 'O', sequence[2] = 'p'; sequence[2] <= 's'; sequence[2]++) {
		rl_generic_bind(ISFUNC, sequence, (char *)keypad_dispatch, rl_get_keymap());
		rl_generic_bind(ISFUNC, sequence, (char *)keypad_dispatch, vi_insertion_keymap);
	}
	for (sequence[0] = '\0'; sequence[0] >= 0; sequence[0]++) {
		rl_bind_key(sequence[0] | 0x0080, (Function *)meta_dispatch);
		rl_bind_key_in_map(sequence[0] | 0x0080, (Function *)meta_dispatch, vi_insertion_keymap);
	}
}
#endif

/**********************/
/* the #macro command */
/**********************/

void macro_command(const char *arg, struct session* ses)
{
	char left[BUFFER_SIZE], right[BUFFER_SIZE], result[BUFFER_SIZE];
	struct listnode *ln;

#ifndef _WINDOWS
	struct listnode *mymacros = (ses ? ses->macros : common_macros);
#else
	struct listnode *&mymacros = common_macros;
#endif
	
	arg = get_arg_in_braces(arg, left, 0);
	arg = get_arg_in_braces(arg, right, 1);

/*	#macro with a single number is the real-time capture	*/
	if(left[0]=='0' && left[1]==0){
		tintin_puts2("#Macro key capture is now off!", ses);
		keycapture = 0;

	} else if(left[0]=='1' && left[1]==0){
#ifdef _WINDOWS         /* UNIX live key capture support - vastheman 2001-08-08 */
		tintin_puts2("#Macro key testing mode, write down the keycode for future use, press ESC to end...", ses);
#else
		tintin_puts2("#Macro key testing mode, write down the keycode for future use, type \"#macro 0\" to end...", ses);
#endif
		keycapture = 1;

/*	#macro with less than two arguments lists defined macros	*/
	} else if(!*left) {
		tintin_puts2("#Defined macros:", ses);
		show_list(mymacros);
		prompt(ses);

	} else if(*left && !*right) {
		if((ln = search_node_with_wild(mymacros,left))) {
			while((mymacros = search_node_with_wild(mymacros, left))) {
				shownode_list(mymacros);
			}
			prompt(ses);
		}
		else
			if(mesvar[1])
			tintin_puts("#That macro is not defined.", ses);
	}

/*	Anything else defines a macro	*/
	else {
#ifdef _WINDOWS
		if((ln = searchnode_list(mymacros, left)))
			deletenode_list(mymacros, ln);
		insertnode_list(mymacros, left, right, "0", ALPHA);
		if(mesvar[1]) {
			sprintf(result, "#Ok. key {%s} now mapped to macro {%s}", left, right);
			tintin_puts2(result, ses);
		}
		acnum++;
#else
	/*	Check the syntax of the macro - must be PF1-PF4 or M-k	*/
		left[0] = toupper(left[0]);
		left[1] = toupper(left[1]);
		if ((left[0] == 'P') && (left[1] == 'F') && (left[2] >= '1') && (left[2] <= '4') && !left[3]) {
			if((ln = searchnode_list(mymacros, left)))
				deletenode_list(mymacros, ln);
			insertnode_list(mymacros, left, right, "0", ALPHA);
			if(mesvar[1]) {
				sprintf(result, "#Ok. key %s now mapped to macro {%s}", left, right);
				tintin_puts2(result, ses);
			}
			acnum++;
		} else if ((left[0] == 'M') && (left[1] == '-') && left[2] && !left[3]) {
			left[2] = toupper(left[2]);
			if((ln = searchnode_list(mymacros, left)))
				deletenode_list(mymacros, ln);
			insertnode_list(mymacros, left, right, "0", ALPHA);
			if(mesvar[1]) {
				sprintf(result, "#Ok. meta-%c now mapped to macro {%s}", left[2], right);
				tintin_puts2(result, ses);
			}
			acnum++;
		} else {
			if(mesvar[1]) {
				sprintf(result, "#BAD KEYSTROKE SYNTAX.", left);
				tintin_puts2(result, ses);
			}
		}
#endif
	}
}

/*************************/
/* the #unmacro command */
/*************************/

void unmacro_command(const char *arg, struct session* ses)
{
	char left[BUFFER_SIZE], result[BUFFER_SIZE];
	struct listnode *ln;
	int flag = FALSE;
	
#ifndef _WINDOWS
	struct listnode *mymacros = (ses ? ses->macros : common_macros);
#else
	struct listnode *&mymacros = common_macros;
#endif
	
	arg = get_arg_in_braces(arg, left, 1);
#ifndef _WINDOWS
	if ((left[0] == 'p') || (left[0] == 'P')) {
		left[0] = toupper(left[0]);
		left[1] = toupper(left[1]);
	} else if ((left[0] == 'm') || (left[0] == 'M')) {
		left[0] = toupper(left[0]);
		left[2] = toupper(left[2]);
	}
#endif
	while((ln = search_node_with_wild(mymacros, left))) {
		if(mesvar[1]) {
#ifdef _WINDOWS
			sprintf(result, "#Ok. key {%s} is no longer a macro.", ln->left);
#else
			if (ln->left[0] == 'P')
				sprintf(result, "#Ok. key %s is no longer a macro.", ln->left);
			else if (ln->left[0] == 'M')
				sprintf(result, "#Ok. meta-%c is no longer a macro.", ln->left[2]);
			else
				sprintf(result, "#Ok. key {%s} is no longer a macro.", ln->left);
#endif
			tintin_puts2(result, ses);
		}
		deletenode_list(mymacros, ln);
		flag = TRUE;
	}
	
	if(!flag && mesvar[1]) {
		sprintf(result, "#No match(es) found for {%s}", left);
		tintin_puts2(result, ses);
	}
}

/************************************************/
/* check macros, return 1 if found, otherwise 0 */
/************************************************/

int check_macros(struct keytype* key, struct session* ses)
/* This Function is modified by yansong chen to fix miss macro bug */
{
	struct listnode *ln;
		
	ln = common_macros;
	while((ln = ln->next)) {
		if(check_one_macro(key, ln->left, ses)) {
			char buffer[BUFFER_SIZE], strng[BUFFER_SIZE];
			
			substitute_myvars(ln->right, buffer, ses);
			if(echo && activesession == ses) { 
				sprintf(strng, "[MACRO: {%s}={%s}]", ln->left, (echo>1) ? ln->right : buffer);
				tintin_puts2(strng, ses);
			}
			parse_input(buffer, ses);
			return 1;
		}
	}
	return 0;
}

int check_one_macro(struct keytype* key, const char* left, struct session* ses){
	struct keytype key0;
	char result[BUFFER_SIZE];

	substitute_myvars(left, result, ses);
	sscanf(result, "%x,%x", &(key0.wParam), &(key0.lParam));
	return (key0.wParam == key->wParam && key0.lParam == key->lParam);
}
