/* New for v2.0: readline support -- daw */

/* i put the logging crap in it's own file, in case we wanna change it later */

#include <config.h>
#include "tintin.h"

#ifdef HAVE_UNISTD_H
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "log.proto"
#include "parse.proto"
#include "rl.proto"
#include "variables.proto"

struct log_list *new_loglist(struct log_list *left){
	struct log_list *ptr = (struct log_list *)malloc(sizeof(struct log_list));
	if(left) left->next = ptr;
	ptr->next = NULL;
  return ptr;
}

void logit(struct session* &ses, const char *b)
{
	static char bb[BUFFER_SIZE+1];
	int i, j;
	struct log_list *pt;
  char *str;
	
	if(!ses->logfiles)
		return;
	
#if OLD_LOG
	str = b;
#else
	/* new logging behavior: ignore control-m's */
	for(i = j = 0; b[i]; i++)
		if(b[i] != '\r')
		bb[j++] = b[i];
	bb[j] = '\0';
	str = bb;
#endif

  for(pt = ses->logfiles; pt; pt = pt->next)
    fputs(str, pt->hd);
}

void lograw(struct session* &ses, const char *b, int l)
{
	struct log_list *pt;
  for(pt = ses->logfiles; pt; pt = pt->next)
    fwrite(b, sizeof(char), l, pt->hd);
}

/********************/
/* the #log command, moved from file.cpp to log.cpp, 4/13/2006 */
/********************/
void log_command(const char *arg0, struct session* ses)
{
	struct log_list  *pt, *ptnext;
  char *cpt;
	char    temp[BUFFER_SIZE];
	char    arg[BUFFER_SIZE], flags[BUFFER_SIZE], dir[BUFFER_SIZE], mode[3] = {'w', '\0', '\0'};
	int     append = 0, i, logoff = 0;
	FILE *fd;

	if(ses) 
	{
		if(!*arg0) //log off for all files
		{
		/*	Changed around to allow a single command to switch log files - vastheman 2001-07-28	*/
			if (ses->logfiles) {
				pt = ses->logfiles;
				do {
					fclose(pt->hd);
					ptnext = pt->next;
					free(pt);
					pt = ptnext;
				} while(pt);
        ses->logfiles=NULL;
				tintin_puts("#OK. ALL LOGGINGS TURNED OFF.", ses);
			} else
				tintin_puts("#NO LOGS TO BE CLOSED.", ses);
		}
		else if(arg0[0]=='?'){// inqury for current logging files, 4/13/2006
			if(ses->logfiles){
				strcpy(temp, "#FILES BEING LOGGED: ");
				for(pt = ses->logfiles; pt; pt = pt->next){
					strcat(temp, pt->filename);
					if(pt->next)  strcat(temp, ", ");
				}
				strcat(temp, ".");
				tintin_puts(temp, ses);
			}
			else {
				tintin_puts("#NO FILES ARE BEING LOGGED SO FAR.", ses);
			}
		}
		else   // Allow user to switch log files with a single command - vastheman 2001-07-28
		// changed, allowing multiple log files instead -- chitchat 4/13/2006
		{
			substitute_myvars(arg0, flags, ses); /*added by chitchat*/
			arg0 = get_arg_in_braces(flags, arg, 0);
			arg0 = get_arg_in_braces(arg0, flags, 1);

/* The code below make the last directory, to avoid some errors */
			strcpy(dir, arg);
			if( (cpt=strrchr(dir, '/')) != NULL
#ifdef _WINDOWS
				|| (cpt = strrchr(dir, '\\')) != NULL
#endif
				){
				*cpt = 0;
#ifdef _WINDOWS
				while( (cpt=strchr(dir, '/')) != NULL)
					*cpt = '\\'; /*make path can be unix format too */
#endif
				mkdir(dir, 0755); //unix relies on umask for the right of the new dir
			}                         //remove potential security hazard - vastheman 2001-07-28
/* end of the code to make directory */

#ifdef _WINDOWS
			while( (cpt=strchr(arg, '/')) != NULL)
				*cpt = '\\'; /*make file can be unix format too */
#endif

		/*	Section allows appending to log files - vastheman 2001-07-28	*/
			ses->lograw = 0;
			for(i = 0; !logoff && flags[i]; i++) {
				switch (flags[i]) {
				case '-':  // - overwrites everything, turn log off for the specific file
					logoff = 1;
					break;
					
				case '1':   // 1 activates raw mode
					ses->lograw = 1;
					mode[1] = 'b';  // Use binary mode when logging telnet sequences
					break;

				case '+':   // + activates append mode
					mode[0] = 'a';
					break;
					
				default:    // Ignore anything else
					break;
				}
			}
			if(logoff){
				log_off(arg, ses);
				return;
			}
		/*	End modified section						*/

			if((fd=fopen(arg, mode))){
				if(!ses->logfiles) {
					ses->logfiles = new_loglist(NULL);
					ptnext = ses->logfiles;
				}
				else { // note it is hard to check whether a file already being logged, due to path/different names ..., so it is up to the users to make sure
					for(pt = ses->logfiles; pt->next; pt = pt->next); //now pt is the last one
					ptnext = new_loglist(pt);
				}
				strcpy(ptnext->filename, arg);
				ptnext->hd = fd;
				sprintf(flags, "#OK. LOGGING..... (RAWMODE=%d%s)", ses->lograw, (mode[0] == 'a' ? " APPEND" : ""));
				tintin_puts(flags, ses);
			}
			else
				tintin_puts("#COULDN'T OPEN FILE.", ses);
		}
	}
	else
		tintin_puts("#THERE'S NO SESSION TO LOG.", ses);
	
	prompt(NULL);
}

void log_off(const char *filename, struct session* ses){
// note that the filename need to be EXACT as the one used for log
// ses is non-zero (guaranteed from prev. calls)
	struct log_list *pt, *ptprev, *ptnext;
	char temp[BUFFER_SIZE];

	ptprev = NULL;
	for(pt = ses->logfiles; pt; ptprev = pt, pt = pt->next){
		if(!stricmp(pt->filename, filename)){
			fclose(pt->hd);
			ptnext = pt->next;
			free(pt);
			if(!ptprev) ses->logfiles = ptnext; //ptnext could be NULL, no more log files
			else ptprev->next = ptnext;
			sprintf(temp, "#LOG FILE \"%s\" IS NOW OFF.", filename);
			tintin_puts(temp, ses);
			return;
		}
	}
	sprintf(temp, "#NO FILE \"%s\" IS BEING LOGGED!", filename);
	tintin_puts(temp, ses);
}