/* New for v2.0: readline support -- daw */

/* all the tab completion support is here in this one file */

#include <config.h>
#include "tintin.h"

#if defined(HAVE_STRING_H)
#include <string.h>
#elif defined(HAVE_STRINGS_H)
#include <strings.h>
#endif

#include <ctype.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "parse.proto"
#include "rl.proto"
#include "rltab.proto"
#include "utils.proto"
#include "variables.proto"	/*	Added to allow variables in tab file names - Vasantha Crabb	*/
#include "main.proto"           /* Added to allow honouring of message flags - vastheman 2001-07-24     */

typedef struct list_s {
	char *word;
	struct list_s *next;
} list_t;

static list_t *complist = 0; /* tab completion list */

/*
* the completion generation function for readline.
* this function will be called repeatedly with the
* same s -- state=0 the first time, and state!=0 the
* rest of the time.
* 
* it returns a pointer to a match each time, and a
* (char *) 0 pointer when it's done.
*/
char *rltab_generate(const char *s, int state)
{
	static list_t *p;
	list_t *p_old;
	char *match;
	
	if(!state) {
		if(!complist)
			return(0);
		p = complist->next;
	}
	
	for(p_old=p; p; p = p->next)
		if(!strncmp(s, p->word, strlen(s))) { //changed strncasecmp to strncmp, chitchat, 3/9/2000
			match = mystrdup(p->word);
			p = 0;                            /* Only want the first matching word */
			return(match);
		}
	for(p=p_old; p; p = p->next)
		if(!strncasecmp(s, p->word, strlen(s))) { //when case sensitive failed, check old method
			match = mystrdup(p->word);
			p = 0;                            /* Only want the first matching word */
			return(match);
		}
	return(0);
}

static void rltab_add(const char *word)
{
	list_t *p, *last;
	
	if(!complist) {
		/* add dummy header node */
		if(!(complist = (list_t *)malloc(sizeof(list_t))))
			syserr("rltab_add: malloc");
		complist->word = 0;
		if(!(p = complist->next = (list_t *)malloc(sizeof(list_t))))
			syserr("rltab_add: malloc");
		p->word = mystrdup(word);
		p->next = 0;
		return;
	}
	
	/* avoid duplicates */
	for(last = complist, p = complist->next; p; last = p, p = p->next)
		if(!strcasecmp(p->word, word))
		return;
	
	/* add to end of list */
	if(!(p = (list_t *)malloc(sizeof(list_t))))
		syserr("rltab_add: malloc");
	p->word = mystrdup(word);
	p->next = 0;
	last->next = p;
}

void rltab_delete(const char *word)
{
	list_t *p, *q;
	
	if(!complist) {
		tintin_puts("#Delete failed: empty completion list.", NULL);
		return;
	}
	
	for(p = complist; p->next; p = p->next)
		if(!strcasecmp(p->next->word, word)) {
		q = p->next;
		p->next = p->next->next;
		free(q->word);
		free(q);
		tintin_puts("#Ok, deleted.", NULL);
		return;
	}
	tintin_puts("#Delete failed: word not in completion list.", NULL);
}

/************************/
/* the #tablist command */
/************************/

void rltab_list(void)
{
	list_t *p;
	int col = 0, ncols = 5;
	char line[BUFFER_SIZE];
	
	if(!complist || !complist->next) {
		tintin_puts("#Empty completion list.", NULL);
		return;
	}
	
	*line = '\0';
	for(p = complist->next; p; p = p->next) {
		sprintf(line+strlen(line), "%-15.15s ", p->word);
		if(++col == ncols) {
			tintin_puts(line, NULL);
			col = 0;
			*line = '\0';
		}
	}
	if(*line)
		tintin_puts(line, NULL);
}

void rltab_purge(void)
{
	list_t *p, *q;
	
	if(!complist) //fixed for mem-leak, chitchat, 7/5/2001
		return;
	
	for(p = complist->next; p; p = q) {
		q = p->next;
		free(p->word);
		free(p);
	}
	free(complist);
	complist = 0;
}

/**********************/
/* the #retab command */
/**********************/

void rltab_read(char *arg, struct session* ses)	/*	Modified to allow variables in tab file names - Vasantha Crabb	*/
{
	FILE *f;
	char *s, *t, line[BUFFER_SIZE], filename[BUFFER_SIZE], *cptr;	/*	Modified to allow variables in tab file names - Vasantha Crabb	*/
	
	/*chitchat: rltab_read now takes arguments */
	if(!arg || !*arg) strcpy(filename, "tab.txt");
	else {
		substitute_myvars(arg, line, ses);	/*	Added to allow variables in tab file names - Vasantha Crabb	*/
		get_arg_in_braces(line, filename, 0);	/*	Modified to allow semicolons in tab file names - Vasantha Crabb	*/
#ifdef _WINDOWS
		/*make file can be unix format too */
		while( (cptr=strchr(filename, '/')) != NULL)
			*cptr = '\\';
#endif
	}
	
	if(!(f = fopen(filename, "r"))) {
		sprintf(line, "#Couldn't open tabfile %s.", filename);	/*	Modified to give more meaningful messages - Vasantha Crabb	*/
		tintin_puts(line, NULL);
		return;
	}
	
	rltab_purge();
	
	while(fgets(line, sizeof(line), f)) {
		/* delete leading and trailing whitespace */
		for(s = line; isspace(*s); s++)
			;
		for(t = s; *t && !isspace(*t); t++)
			;
		*t = '\0';
		rltab_add(s);
	}
	fclose(f);
	if (mesvar[7]) { /* Modified so message will only appear if message read is on - vastheman 2001-07-24  */
		sprintf(line, "#Read %s, completion list initialized.", filename);	/*	Modified to give more meaningful messages - Vasantha Crabb	*/
		tintin_puts(line, NULL);
	}
}

/***********************/
/* the #tabadd command */
/***********************/

void do_tabadd(const char *arg)
{
	char buf[BUFFER_SIZE];
	
	if(!arg || !*arg) {
		tintin_puts("#Add failed: no word specified.", NULL);
		return;
	}
	
	get_arg_in_braces(arg, buf, 1);
	rltab_add(buf);
	tintin_puts("#Added.", NULL);
}

/***********************/
/* the #tabdel command */
/***********************/

void do_tabdel(const char *arg)
{
	char buf[BUFFER_SIZE];
	
	if(!arg || !*arg) {
		tintin_puts("#Delete failed: no word specified.", NULL);
		return;
	}
	
	get_arg_in_braces(arg, buf, 1);
	rltab_delete(buf);
}

/************************/
/* the #tabsave command */
/************************/

void do_tabsave(char *arg, struct session* ses)	/*	Modified to allow different names for saved tab file - Vasantha Crabb	*/
{
	FILE *f;
	list_t *p;

	char line[BUFFER_SIZE], filename[BUFFER_SIZE], *cptr;	/*	Added to allow tab file name as parameter - Vasantha Crabb	*/
	
	if(!complist || !complist->next) {
		tintin_puts("#Empty completion list, nothing to save.", NULL);
		return;
	}
	
/*	Following section added to allow tab file name as parameter - Vasantha Crabb	*/
	if(!arg || !*arg) strcpy(filename, "tab.txt");
	else {
		substitute_myvars(arg, line, ses);
		get_arg_in_braces(line, filename, 0);

#ifdef _WINDOWS
		/*make file can be unix format too */
		while( (cptr=strchr(filename, '/')) != NULL)
			*cptr = '\\';
#endif
	}
/*	End added section	*/

	if(!(f = fopen(filename, "w"))) {	/*	Modified to allow different names for saved tab file - Vasantha Crabb	*/
		sprintf(line, "#Couldn't open tabfile %s.", filename);	/*	Modified to give more meaningful messages - Vasantha Crabb	*/
		tintin_puts(line, NULL);
		return;
	}
	
	for(p = complist->next; p; p = p->next)
		fprintf(f, "%s\n", p->word);
	fclose(f);
	sprintf(line, "#Saved completion list to %s.", filename);	/*	Modified to give more meaningful messages - Vasantha Crabb	*/
	tintin_puts(line, NULL);
}
