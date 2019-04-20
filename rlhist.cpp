/* New for v2.0: readline support -- daw */

/* random code to save history, expand !'s, etc */

#include <config.h>
#include "tintin.h"

#ifndef _WINDOWS
#include <readline/readline.h>
#include <readline/history.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "rl.proto"
#include "rlhist.proto"
#include "utils.proto"
#include "main.proto"

void rlhist_show(void)
{
	HIST_ENTRY **a;
	int i;
	char msg[BUFFER_SIZE];
	
	a = history_list();
	if(!a || !*a) {
		tintin_puts("#No history.", NULL);
		return;
	}
	for(i = 0; *a; i++) {
		sprintf(msg, "%2d %s", i, (*a++)->line);
		tintin_puts(msg, NULL);
	}
}

void my_add_history(char *s) 
{
	static char lasthist[BUFFER_SIZE];
	
	if (readline_echoing_p) {
		if (strcmp(s,lasthist)) {
			add_history(s);
			strcpy(lasthist,s);
		} 
	}
}

char *rlhist_expand(char *line)
{
	char error_msg[BUFFER_SIZE];
	char *expansion;
	
	if(!*line)
		return(line);
	
	/* don't try to do history expansion on "say hi there!; bow" */
	if(*line != '!') {
		my_add_history(line);
		return(line);
	}
	
	/* hack to make "!" work as it used to */
	if(!strcmp(line, "!")) {
		free(line);
		line = mystrdup("!!");
	}
	
	if(history_expand(line, &expansion) < 0) {
		strcpy(error_msg, "Expansion error. ");
		if(expansion)
			strcat(error_msg, expansion);
		tintin_puts(error_msg, NULL);
		free(line);
		free(expansion);
		return(0);
	}
	
	free(line);
	my_add_history(expansion);
	return(expansion);
}
