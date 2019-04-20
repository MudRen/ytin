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

#include "parse.proto"
#include "rl.proto"
#include "stream.proto"
#include "utils.proto"
#include "variables.proto"

struct int_list *fpList = (struct int_list*)NULL;

void add_file(FILE* fp)
{
	struct int_list *new_list, *slist;
	new_list = (struct int_list*) malloc(sizeof(struct int_list));
	if (!new_list) syserr("add_file: malloc");
	new_list->next = NULL;
	new_list->l = (unsigned long)fp;
	if(!fpList) {
		fpList = new_list;
		return;
	}
	for(slist=fpList; slist->next; slist = slist->next) ;
	slist->next = new_list;
}

int valid_file(unsigned long fp)
{
	char temp[BUFFER_SIZE];
	struct int_list *slist;
	for(slist = fpList; slist; slist=slist->next){
		if(slist->l == fp) return 1;
	}
	sprintf(temp, "#ERROR: *FILE(%lx) is invalid!", fp);
	tintin_puts2(temp, NULL);
	return 0;
}

void remove_file(unsigned long fp)
{
	struct int_list *slist, *olist;
	for(slist = fpList; slist; olist=slist, slist=slist->next){
		if(slist->l == fp) {
			if(slist == fpList)
				fpList = slist->next;
			else 
				olist->next = slist->next;
			free(slist);
			return;
		}
	}
}
void clean_all_file() //called when quitting yTin, in case
{
	struct int_list *slist;
	for(; fpList; ){
		slist = fpList;
		fpList = slist->next;
		fclose((FILE *)slist->l);
		free(slist);
	}
}

void fopen_command(const char* arg, struct session* ses){
	FILE* fp;
	char arg0[BUFFER_SIZE], arg1[BUFFER_SIZE], temp[BUFFER_SIZE];
	
	arg=get_arg_in_braces(arg, temp,0);
	substitute_myvars(temp, arg0, ses);
	arg=get_arg_in_braces(arg, temp,1);
	substitute_myvars(temp, arg1, ses);

	if((fp = fopen(arg0, arg1))==NULL){
		sprintf(temp, "#ERROR: fopen(\"%s\", \"%s\")!", arg0, arg1);
		tintin_puts2(temp, ses);
		return;
	}
	add_file(fp);
	sprintf(temp, "%lu", (unsigned long)fp);
	assign_var("_", temp, ses);
}

void fclose_command(const char* arg, struct session* ses){
	unsigned long fp;
	int result;
	char arg0[BUFFER_SIZE], temp[BUFFER_SIZE];
	
	arg=get_arg_in_braces(arg, temp,1);
	substitute_myvars(temp, arg0, ses);

	sscanf(arg0, "%lu", &fp);
	if(valid_file(fp)){
		result = fclose((FILE *)fp);
		remove_file(fp);
	}
	else 	result = EOF;
	sprintf(temp, "%d", result);
	assign_var("_", temp, ses);
}


void feof_command(const char* arg, struct session* ses){
	unsigned long fp;
	int result;
	char arg0[BUFFER_SIZE], temp[BUFFER_SIZE];
	
	arg=get_arg_in_braces(arg, temp,1);
	substitute_myvars(temp, arg0, ses);

	sscanf(arg0, "%lu", &fp);
	if(valid_file(fp))
		result = feof((FILE *)fp);
	else 	result = 1;
	sprintf(temp, "%d", result);
	assign_var("_", temp, ses);
}

void fseek_command(const char* arg, struct session* ses){
	char arg0[BUFFER_SIZE], arg1[BUFFER_SIZE], arg2[BUFFER_SIZE], temp[BUFFER_SIZE];
	long offset;
	int result, pos;
	unsigned long fp;
	
	arg=get_arg_in_braces(arg, temp,0);
	substitute_myvars(temp, arg0, ses);
	arg=get_arg_in_braces(arg, temp,0);
	substitute_myvars(temp, arg1, ses);
	arg=get_arg_in_braces(arg, temp,1);
	substitute_myvars(temp, arg2, ses);

	sscanf(arg0, "%ld", &fp);
	if(valid_file(fp)){
		sscanf(arg1, "%ld", &offset);

		if(strcmp(arg2, "SEEK_CUR")==0) pos = SEEK_CUR;
		else if(strcmp(arg2, "SEEK_END")==0) pos = SEEK_END;
		else if(strcmp(arg2, "SEEK_SET")==0) pos = SEEK_SET;
		else sscanf(arg2, "%d", &pos);

		result = fseek((FILE *)fp, offset, pos);
	}
	else result = 1;
	sprintf(temp, "%d", result);
	assign_var("_", temp, ses);
}

void fread_command(const char* arg, struct session* ses){
	char arg0[BUFFER_SIZE], arg1[BUFFER_SIZE], arg2[BUFFER_SIZE*2], arg3[BUFFER_SIZE*2], temp[BUFFER_SIZE];
	int result, size, n;
	unsigned long fp;
	
	arg=get_arg_in_braces(arg, temp,0);
	substitute_myvars(temp, arg0, ses);
	arg=get_arg_in_braces(arg, temp,0);
	substitute_myvars(temp, arg1, ses);
	arg=get_arg_in_braces(arg, temp,0);
	substitute_myvars(temp, arg2, ses);
	arg=get_arg_in_braces(arg, temp,1);
	substitute_myvars(temp, arg3, ses);

	sscanf(arg1, "%d", &size);
	if(size!=sizeof(char))
		tintin_puts2("#Warning: fread only accept size==sizeof(char) now!", ses);
	sscanf(arg2, "%d", &n);
	if(n>=BUFFER_SIZE) n=BUFFER_SIZE;
	sscanf(arg3, "%ld", &fp);
	if(valid_file(fp))
		result = fread(temp, sizeof(char), n, (FILE *)fp);
	else result = 0;
	assign_var(arg0, temp, ses);
	sprintf(temp, "%d", result);
	assign_var("_", temp, ses);
}

/*
void fgetc_command(const char* arg, struct session* ses){

}
*/

void fgets_command(const char* arg, struct session* ses){
	char arg0[BUFFER_SIZE], arg1[BUFFER_SIZE], arg2[BUFFER_SIZE*2], temp[BUFFER_SIZE];
	int result, n;
	unsigned long fp;
	
	arg=get_arg_in_braces(arg, temp,0);
	substitute_myvars(temp, arg0, ses);
	arg=get_arg_in_braces(arg, temp,0);
	substitute_myvars(temp, arg1, ses);
	arg=get_arg_in_braces(arg, temp,1);
	substitute_myvars(temp, arg2, ses);

	sscanf(arg1, "%d", &n);
	if(n>=BUFFER_SIZE) n=BUFFER_SIZE;
	sscanf(arg2, "%ld", &fp);
	if(valid_file(fp))
		result = (fgets(temp, n, (FILE *)fp) != NULL);
	else result = 0;
	assign_var(arg0, temp, ses);
	sprintf(temp, "%d", result);
	assign_var("_", temp, ses);
}

/** actually I do not know how to handle this :(, fortunately I can live w/o this :)
void fscanf_command(const char* arg, struct session* ses){
}
*/

void fwrite_command(const char* arg, struct session* ses){
	char arg0[BUFFER_SIZE], arg1[BUFFER_SIZE], arg2[BUFFER_SIZE*2], arg3[BUFFER_SIZE*2], temp[BUFFER_SIZE];
	int result, size, n;
	unsigned long fp;
	
	arg=get_arg_in_braces(arg, temp,0);
	substitute_myvars(temp, arg0, ses);
	arg=get_arg_in_braces(arg, temp,0);
	substitute_myvars(temp, arg1, ses);
	arg=get_arg_in_braces(arg, temp,0);
	substitute_myvars(temp, arg2, ses);
	arg=get_arg_in_braces(arg, temp,1);
	substitute_myvars(temp, arg3, ses);

	sscanf(arg1, "%d", &size);
	if(size!=sizeof(char))
		tintin_puts2("#Warning: fwrite only accept size==sizeof(char) now!", ses);
	sscanf(arg2, "%d", &n);
	if(n>=BUFFER_SIZE) n=BUFFER_SIZE;
	sscanf(arg3, "%ld", &fp);
	if(valid_file(fp))
		result = fwrite(arg0, sizeof(char), n, (FILE *)fp);
	else result = 0;
	sprintf(temp, "%d", result);
	assign_var("_", temp, ses);
}

void fputs_command(const char* arg, struct session* ses){
	char arg0[BUFFER_SIZE], arg1[BUFFER_SIZE], temp[BUFFER_SIZE];
	int result;
	unsigned long fp;
	
	arg=get_arg_in_braces(arg, temp,0);
	substitute_myvars(temp, arg0, ses);
	arg=get_arg_in_braces(arg, temp,1);
	substitute_myvars(temp, arg1, ses);

	sscanf(arg1, "%ld", &fp);
	if(valid_file(fp))
		result = fputs(arg0, (FILE *)fp);
	else result = -1;
	sprintf(temp, "%d", result);
	assign_var("_", temp, ses);
}

/* do these later too
void fputc_command(const char* arg, struct session* ses){

}

void fprintf_command(const char* arg, struct session* ses){

}
*/
void remove_command(const char* arg, struct session* ses){
	char arg0[BUFFER_SIZE], temp[BUFFER_SIZE];
	int result;
	
	arg=get_arg_in_braces(arg, temp,1);
	substitute_myvars(temp, arg0, ses);

	result = remove(arg0);
	sprintf(temp, "%d", result);
	assign_var("_", temp, ses);
}

void rename_command(const char* arg, struct session* ses){
	char arg0[BUFFER_SIZE], arg1[BUFFER_SIZE], temp[BUFFER_SIZE];
	int result;
	
	arg=get_arg_in_braces(arg, temp,0);
	substitute_myvars(temp, arg0, ses);
	arg=get_arg_in_braces(arg, temp,1);
	substitute_myvars(temp, arg1, ses);

	result = rename(arg0, arg1);
	sprintf(temp, "%d", result);
	assign_var("_", temp, ses);
}