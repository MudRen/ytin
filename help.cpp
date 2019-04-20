/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*****************************************************************/
/* functions for the #help command                               */
/* Some small patches by David Hedbor (neotron@lysator.liu.se)   */
/* to make it work better.                                       */
/*****************************************************************/

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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "main.proto"
#include "help.proto"
#include "parse.proto"
#include "rl.proto"
#include "utils.proto"

FILE *check_file(const char *filestring)
{
	FILE *myfile;
	char buf[BUFFER_SIZE];
	
	#ifdef _WINDOWS
	if((myfile = fopen(filestring, "r")))
		return myfile;
	return NULL;
	#endif
	if((myfile = fopen(filestring, "r"))) {
		fclose(myfile);
		sprintf(buf, "%s %s", DEFAULT_CAT_STR, filestring);
		return(popen(buf, "r"));
	}
	
	sprintf(buf, "%s%s", filestring, DEFAULT_GZIP_EXT);
	if((myfile = fopen(buf, "r"))) {
		fclose(myfile);
		sprintf(buf, "%s %s%s", DEFAULT_GUNZIP_STR, filestring, DEFAULT_GZIP_EXT);
		return(popen(buf, "r"));
	}
	
	sprintf(buf, "%s%s", filestring, DEFAULT_COMPRESS_EXT);
	if((myfile = fopen(buf, "r"))) {
		fclose(myfile);
		sprintf(buf, "%s %s%s",
			DEFAULT_UNCOMPRESS_STR, filestring, DEFAULT_COMPRESS_EXT);
		return(popen(buf, "r"));
	}
	
	return(NULL);
}

/*********************/
/* the #help command */
/*********************/

void help_command(const char *arg)
{
	FILE *myfile = NULL;
	char *cptr, text[81], line[81], filestring[500];
	int flag = 0, counter = 0, mixed=0;
	
#ifdef _WINDOWS
	sprintf(filestring, "%s/%s", getenv("windir"), DEFAULT_HELP_FILE);
	/*make file can be unix format too */
	while( (cptr=strchr(filestring, '/')) != NULL)
		*cptr = '\\';
#else
	sprintf(filestring, "%s/%s", getenv("HOME"), DEFAULT_HELP_FILE);
#endif
	myfile = check_file(filestring);
	
	if(!myfile && strcmp(DEFAULT_FILE_DIR, "HOME")) {
		sprintf(filestring, "%s/%s", DEFAULT_FILE_DIR, DEFAULT_HELP_FILE);
#ifdef _WINDOWS
		/*make file can be unix format too */
		while( (cptr=strchr(filestring, '/')) != NULL)
			*cptr = '\\';
#endif
		myfile = check_file(filestring);
	}
	
	if(!myfile) {
		myfile = check_file(DEFAULT_HELP_FILE);
	}
	
	if(!myfile) {
		tintin_puts2("#Help file '" DEFAULT_HELP_FILE "' not found - no help available.", NULL);
		prompt(NULL);
		return;
	}
	
#ifdef _WINDOWS
	buffer_view = 1;
#endif
	if(*arg) {
		sprintf(text, "~%s", arg);
		cptr = text;
		
		while(*++cptr) {
			if(*cptr>='a' && *cptr<='z')
				mixed = mixed | 1; //the lowest bit is for lowercase char
			if(*cptr>='A' && *cptr<='Z')
				mixed = mixed | 2; //the 2nd lowest bit is for uppercase char
			*cptr = toupper(*cptr);
		}
		if(mixed == 1 + 2) //mixed-case, then use original mixed-case
			sprintf(text, "~%s", arg);
		
		while(fgets(line, sizeof(line)-1, myfile)){
			if(line[0] == '~' && line[1] == '*') {
					tintin_puts2("#Sorry, no help on that word.", NULL);
					break;
			}
			if(flag && line[0] =='~') break;
			if(flag){
				if (line[strlen(line) - 1] == '\n') // Fix bug where last character on a wrapped line is killed - vastheman 2001-07-28
					line[strlen(line)-1] = '\0';
				tintin_puts2(line, NULL);
				if(counter++ > 21) {
					tintin_puts2("[ -- Press a key to continue -- ]", NULL);
					getchar();
					counter = 0;
				}
			}
			else if(is_abbrev(text, line)){
				flag = 1;
			}
		}
	}
	else {
		while(fgets(line, sizeof(line)-1, myfile)){
			if(*line == '~') 
				break;
			else {
				line[strlen(line)-1] = '\0';
				tintin_puts2(line, NULL);
			}
		}
	}
	
	prompt(NULL);
#ifdef _WINDOWS
	fclose(myfile);
	flush_view();
#else
	pclose(myfile);
#endif
}
