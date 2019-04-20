/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: files.c - funtions for logfile and reading/writing comfiles */
/*               TINTIN + +              */
/*     (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t       */
/*           coded by peter unold 1992           */
/*          New code by Bill Reiss 1993          */
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
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "files.proto"
#include "llist.proto"
#include "main.proto"
#include "misc.proto"
#include "parse.proto"
#include "rl.proto"
#include "utils.proto"
#include "variables.proto"

struct s_list *complete_head;

/**********************************/
/* load a completion file     */
/**********************************/
void read_complete(char* arg)
{
	FILE *myfile;
	char filename[BUFFER_SIZE], buffer[BUFFER_SIZE], *cptr;
	int flag;
	struct s_list *tcomplete, *tcomp2;
	flag=TRUE;
	
	free_complete();
	if ((complete_head=(struct s_list *)(malloc(sizeof(struct s_list))))==NULL) 
	{
		syserr("couldn't alloc completehead");
	}
	tcomplete=complete_head;
	
	
	if(!arg || !*arg) strcpy(filename, "tab.txt");
	else {
		strcpy(filename, arg);
#ifdef _WINDOWS
		/*make file can be unix format too */
		while( (cptr=strchr(filename, '/')) != NULL)
			*cptr = '\\';
#endif
	}
	if((myfile=fopen(filename, "r"))==NULL) 
	{
		if((cptr=(char *) getenv("HOME"))) 
		{
			strcpy(buffer,cptr);
			strcat(buffer,"\\");
			strcat(buffer, filename);
			myfile=fopen(buffer, "r");
		}
	}
	
	if(myfile==NULL) 
	{
		sprintf(buffer, "No tab_completion file %s!", filename);
		tintin_puts2(buffer, NULL);
		tcomplete->next=NULL;
		
		return;
	}
	
	while(fgets(buffer, sizeof(buffer), myfile)) 
	{
		for(cptr=buffer; *cptr && *cptr!='\n'; cptr++);
		*cptr='\0';
		
		if ((tcomp2=(struct s_list *)(malloc(sizeof(struct s_list))))==NULL) 
		{
			syserr("couldn't alloc s_list");
		}
		
		if ((cptr=(char *)(malloc(strlen(buffer)+1)))==NULL) 
		{
			syserr("couldn't alloc memory for string in complete");
		}
		
		strcpy(cptr, buffer);
		tcomp2->strng=cptr;
		tcomplete->next=tcomp2;   
		tcomplete=tcomp2;
	}
	
	tcomplete->next=NULL;
	fclose(myfile);
	sprintf(buffer, "tabfile %s loaded.", filename);
	tintin_puts2(buffer, NULL);
	prompt(NULL);
	/* tintin_puts2("\n", (struct session *)NULL); */
}

/********************/
/* free complete_head, by chitchat */
/********************/
void free_complete()
{
	struct s_list *tcomp;
	while(complete_head){
		tcomp = complete_head;
		if( (complete_head = complete_head->next) != NULL)
			free(complete_head->strng);
		free(tcomp);
	}
}


/***********************************/
/* read and execute a command file */
/***********************************/
struct session *read_command(char *filename0, struct session* ses)
{
	FILE *myfile;
	char filename[BUFFER_SIZE], buffer[BUFFER_SIZE], old_buffer[BUFFER_SIZE], *cptr;
	char message[BUFFER_SIZE];
	int flag;
	
	flag=TRUE;
	get_arg_in_braces(filename0,filename0, 1);
	substitute_myvars(filename0, filename, ses); /*added by chitchat*/
#ifdef _WINDOWS
	while( (cptr=strchr(filename, '/')) != NULL)
		*cptr = '\\'; /*make file can be unix format too */
#endif
	
	if((myfile=fopen(filename, "rt"))==NULL) /*text mode */
	{
		sprintf(message, "#ERROR(read) - COULDN'T OPEN %s!", filename);
		tintin_puts(message, ses);
		prompt(NULL);
		return ses;
	}
	
	if (!verbose) 
		puts_echoing=FALSE;
	
	alnum=0;
	acnum=0;
	subnum=0;
	varnum=0;
	hinum=0;
	antisubnum=0;
	
	old_buffer[0]='\0';
	while(fgets(buffer, sizeof(buffer), myfile)) 
	{
		/*get rid of empty lines */
		if(buffer[0]!='\n'){
			if (flag) /*1st line */
			{
				puts_echoing=TRUE;
				message[0]=buffer[0];
				message[1]=0;
				char_command(message, ses); /*fix here, chitchat, char_command will change buffer */
				
				if (!verbose) 
					puts_echoing=FALSE;
				
				flag=FALSE;
			}
			
			/*enable multiline alias et al, join lines ends with \  */
			if(buffer[strlen(buffer)-2] == '\\'){
				buffer[strlen(buffer)-2] = 0;
				strcat(old_buffer, buffer);
			}
			else {/*end of \ block */
				strcat(old_buffer, buffer);
				for(cptr=old_buffer; *cptr && *cptr!='\n'; cptr++)
					; /*empty command here */
				
				*cptr='\0';
				
				ses=parse_input(old_buffer, ses); 
				old_buffer[0] = 0;
			}
		}
	}
	
	if (!verbose) 
	{
		puts_echoing=TRUE;
		if(mesvar[7]>1){
			sprintf(message,"#OK. %d ALIASES LOADED.",alnum);
			tintin_puts2(message,ses);
			sprintf(message,"#OK. %d ACTIONS LOADED.",acnum);
			tintin_puts2(message,ses);
			sprintf(message,"#OK. %d ANTISUBS LOADED.",antisubnum);
			tintin_puts2(message,ses);
			sprintf(message,"#OK. %d SUBSTITUTES LOADED.",subnum);
			tintin_puts2(message,ses);
			sprintf(message,"#OK. %d VARIABLES LOADED.",varnum);
			tintin_puts2(message,ses);
			sprintf(message,"#OK. %d HIGHLIGHTS LOADED.",hinum);
		}
		if(mesvar[7]>0){
			sprintf(message, "#OK. Done reading file %s!", filename);
			tintin_puts2(message,ses);
			prompt(NULL);
		}
	}
	
	fclose(myfile);
	
	return ses;
}

/************************/
/* write a command file */
/************************/
struct session *write_command(char *filename0, struct session* ses)
{
	FILE *myfile;
	char filename[BUFFER_SIZE], buffer[BUFFER_SIZE], *cptr;
	struct listnode *nodeptr;
	get_arg_in_braces(filename0,filename0, 1);
	substitute_myvars(filename0, filename, ses); /*added by chitchat*/
	
	if (*filename=='\0') 
	{
		tintin_puts("#ERROR - COULDN'T OPEN THAT FILE.", ses);
		prompt(NULL);
		return(0); /* added zero return */
	}  
	
#ifdef _WINDOWS
	while( (cptr=strchr(filename, '/')) != NULL)
		*cptr = '\\'; /*make file can be unix format too */
#endif
	if((myfile=fopen(filename, "w"))==NULL) 
	{
		sprintf(buffer, "#ERROR - COULDN'T OPEN FILE %s", filename);
		tintin_puts(buffer, ses);
		prompt(NULL);
		return(0); /* added zero return */
	}
	
	nodeptr=(ses) ? ses->myfuncs : common_functions;
	
	while((nodeptr=nodeptr->next)) 
	{
		prepare_for_write("function", nodeptr->left, nodeptr->right, "\0", buffer);
		fputs(buffer, myfile);
	}
	
	nodeptr=(ses) ? ses->aliases : common_aliases;
	
	while((nodeptr=nodeptr->next)) 
	{
		prepare_for_write("alias", nodeptr->left, nodeptr->right, "\0", buffer);
		fputs(buffer, myfile);
	}
	
	nodeptr=(ses) ? ses->actions : common_actions;
	
	while((nodeptr=nodeptr->next)) 
	{
		prepare_for_write("action", nodeptr->left, nodeptr->right, nodeptr->pr, buffer);
		fputs(buffer, myfile);
	}
	
	nodeptr=(ses) ? ses->antisubs : common_antisubs;
	
	while((nodeptr=nodeptr->next)) 
	{    
		prepare_for_write("antisubstitute", nodeptr->left,
			nodeptr->right, "\0", buffer);
		fputs(buffer, myfile);
	} 
	
	nodeptr=(ses) ? ses->subs : common_subs;
	
	while((nodeptr=nodeptr->next)) 
	{
		prepare_for_write("substitute", nodeptr->left, nodeptr->right, "\0", buffer);
		fputs(buffer, myfile);
	}
	
	nodeptr=(ses) ? ses->myvars : common_myvars;
	
	while((nodeptr=nodeptr->next)) 
	{
		prepare_for_write("variable", nodeptr->left, nodeptr->right, "\0", buffer);
		fputs(buffer, myfile);
	}
	
	nodeptr=(ses) ? ses->highs : common_highs;
	
	while((nodeptr=nodeptr->next)) 
	{  
		prepare_for_write("highlight", nodeptr->right, nodeptr->left, "\0", buffer);
		fputs(buffer, myfile);
	}

#ifndef _WINDOWS    /* Experimental meta-key binding - vastheman 2001-08-07 */
	nodeptr=(ses) ? ses->macros : common_macros;
#else
	nodeptr=common_macros;
#endif

	while((nodeptr=nodeptr->next)) 
	{  
		prepare_for_write("macro", nodeptr->left, nodeptr->right, "\0", buffer);
		fputs(buffer, myfile);
	}
	
	nodeptr=(ses) ? ses->pathdirs : common_pathdirs;
	
	while((nodeptr=nodeptr->next)) 
	{
		prepare_for_write("pathdir", nodeptr->right, nodeptr->left, "\0", buffer);
		fputs(buffer, myfile);
	}
	
	fclose(myfile);
	tintin_puts("#COMMAND-FILE WRITTEN.", ses);
	
	return ses;
}

/************************/
/* write a command file */
/************************/
struct session *writesession_command(char *filename0, struct session* ses)
{
	FILE *myfile;
	char filename[BUFFER_SIZE], buffer[BUFFER_SIZE], *cptr;
	struct listnode *nodeptr;
	
	get_arg_in_braces(filename0,filename0,1);
	substitute_myvars(filename0, filename, ses); /*added by chitchat*/
	
	if(*filename=='\0') 
	{
		tintin_puts("#ERROR - COULDN'T OPEN THAT FILE.", ses);
		prompt(NULL);
		return(0); /* added zero return */
	}
#ifdef _WINDOWS
	while( (cptr=strchr(filename, '/')) != NULL)
		*cptr = '\\'; /*make file can be unix format too */
#endif
	if((myfile=fopen(filename, "w"))==NULL) 
	{
		sprintf(buffer, "#ERROR - COULDN'T OPEN FILE %s", filename);
		tintin_puts(buffer, ses);
		prompt(NULL);
		return(0); /* added zero return */
	}
	
	nodeptr=(ses) ? ses->myfuncs : common_functions;
	
	while((nodeptr=nodeptr->next)) 
	{
		if(ses && searchnode_list(common_functions, nodeptr->left))
			continue;
		
		prepare_for_write("function", nodeptr->left, nodeptr->right, "\0", buffer);
		fputs(buffer, myfile);
	}
	
	nodeptr=(ses) ? ses->aliases : common_aliases;
	
	while((nodeptr=nodeptr->next)) 
	{
		if(ses && searchnode_list(common_aliases, nodeptr->left))
			continue;
		
		prepare_for_write("alias", nodeptr->left, nodeptr->right, "\0", buffer);
		fputs(buffer, myfile);
	}
	
	nodeptr=(ses) ? ses->actions : common_actions;
	
	while((nodeptr=nodeptr->next)) 
	{
		if(ses && searchnode_list(common_actions, nodeptr->left))
			continue;
		
		prepare_for_write("action", nodeptr->left, nodeptr->right, nodeptr->pr, buffer);
		fputs(buffer, myfile);
	}
	
	nodeptr=(ses) ? ses->antisubs : common_antisubs;
	
	while((nodeptr=nodeptr->next)) 
	{
		if(ses && searchnode_list(common_antisubs, nodeptr->left))
			continue;
		
		prepare_for_write("antisubstitute", nodeptr->left, "", "\0", buffer);
		fputs(buffer,myfile);
	}
	
	nodeptr=(ses) ? ses->subs : common_subs;
	
	while((nodeptr=nodeptr->next)) 
	{
		if(ses && searchnode_list(common_subs, nodeptr->left))
			continue;
		
		prepare_for_write("substitute", nodeptr->left, nodeptr->right, "\0", buffer);
		fputs(buffer, myfile);
	}
	
	nodeptr=(ses) ? ses->myvars : common_myvars;
	
	while((nodeptr=nodeptr->next)) 
	{
		if(ses && searchnode_list(common_myvars, nodeptr->left))
			continue;
		
		prepare_for_write("variable", nodeptr->left, nodeptr->right, "\0", buffer);
		fputs(buffer, myfile);
	}
	
	nodeptr=(ses) ? ses->highs : common_highs;
	
	while((nodeptr=nodeptr->next)) 
	{
		if(ses && searchnode_list(common_highs, nodeptr->left))
			continue;
		
		prepare_for_write("highlight", nodeptr->right, nodeptr->left, "\0", buffer);
		fputs(buffer, myfile);
	}
	
#ifndef _WINDOWS    /* Experimental meta-key binding - vastheman 2001-08-07 */
	nodeptr=(ses) ? ses->macros : common_macros;
#else
	nodeptr=common_macros;
#endif

	while((nodeptr=nodeptr->next)) 
	{  
		if(ses && searchnode_list(common_macros, nodeptr->left))
			continue;

		prepare_for_write("macro", nodeptr->left, nodeptr->right, "\0", buffer);
		fputs(buffer, myfile);
	}
	
	fclose(myfile);
	tintin_puts("#COMMANDO-FILE WRITTEN.", ses);
	return ses;
}


void prepare_for_write(const char *command, const char *left, const char *right, const char *pr, char *result)
{
	*result=tintin_char;
	*(result+1)='\0';
	strcat(result, command);
	strcat(result, " {");
		strcat(result, left);
		strcat(result, "}");
	
	if (strlen(right)!=0) 
	{
		strcat(result, " {");
			strcat(result, right);
			strcat(result, "}");
	}
	
	if (strlen(pr)!=0) 
	{
		strcat(result, " {");
			strcat(result, pr);
			strcat(result, "}");
	}
	
	strcat(result,"\n");
}

void prepare_quotes(char *string)
{
	char s[BUFFER_SIZE], *cpsource, *cpdest;
	int nest=FALSE;
	strcpy(s, string);
	
	cpsource=s;
	cpdest=string;
	
	while(*cpsource) 
	{
		if(*cpsource=='\\') 
		{
			*cpdest++=*cpsource++;
			
			if(*cpsource)
				*cpdest++=*cpsource++;
		}
		else if(*cpsource=='\"' && nest==FALSE) 
		{
				*cpdest++='\\';
				*cpdest++=*cpsource++;
			}
			else if(*cpsource=='{') 
			{
					nest=TRUE;
					*cpdest++=*cpsource++;
				}
				else if(*cpsource=='}') 
		{
				nest=FALSE;
				*cpdest++=*cpsource++;
			}
			else
			*cpdest++=*cpsource++; 
		}
		
		*cpdest='\0';
	}


