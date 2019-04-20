/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */

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

#include "ansi.proto"
#include "action.proto"
#include "ivars.proto"
#include "llist.proto"
#include "main.proto"
#include "parse.proto"
#include "rl.proto"
#include "variables.proto"
#include "glob.proto"

int stacks[100][4]; /*added for check + - * /, chitchat*/

#define P_LBRA 2 /* ( */
#define P_RBRA 3 /* ) */
#define P_NOT 4 /* ! */
#define P_MOD 5 /*priority is % (we use //) */
#define P_DIV 6 /* priority is / */
/*making P_DIV higher than P_MULT still will have bug becoz tt++'s math is int type, chitchat*/
#define P_MULT 6 /* priority is * */
#define P_MINUS 7 /* priority is - */
#define P_PLUS 8 /* priority is + */
#define P_GT 10 /* priority is > */
#define P_GE 11 /* priority is >= */
#define P_LT 12 /* priority is < */
#define P_LE 13 /* priority is <= */
#define P_EQ 14 /* priority is == */
#define P_NE 15 /* priority is != */
#define P_AND 16 /* priority is && */
#define P_OR 16 /* priority is || */ /*make it same level as AND*/
#define P_LOWEST 17 /*{ }'s*/

/*********************/
/* the #math command */
/*********************/
void math_command(const char* line, struct session* ses)
{
	/* char left[BUFFER_SIZE], right[BUFFER_SIZE], arg2[BUFFER_SIZE], */ 
	char left[BUFFER_SIZE], right[BUFFER_SIZE], 
	temp[BUFFER_SIZE], result[BUFFER_SIZE];
	struct listnode *my_vars, *ln;
	int i; 
	line=get_arg_in_braces(line, left, 0);
	line=get_arg_in_braces(line, right, 1);
	my_vars=(!(*left=='_' && left[1]) && ses) ? ses->myvars : common_myvars;
	substitute_vars(left, result); /*added by chitchat*/
	substitute_myvars(result, left, ses); /*added by chitchat*/
	substitute_vars(right, result);
	substitute_myvars(result, right, ses);
	i=eval_expression(right);
	sprintf(temp,"%d",i);
	if ((ln=searchnode_list(my_vars,left))!=NULL) 
		deletenode_list(my_vars, ln);
	insertnode_list(my_vars, left, temp, "0", ALPHA); 
}

/*********************/
/* the #if command */
/*********************/
void if_command(const char* line, struct session* ses)
{
	char left[BUFFER_SIZE], right[BUFFER_SIZE], 
	temp[BUFFER_SIZE], elsebuf[BUFFER_SIZE];
	
	line=get_arg_in_braces(line, left, 0);
	line=get_arg_in_braces(line, right, 1);
	
	substitute_vars(left,temp);
	while(eval_strcmp(temp, ses));
	substitute_myvars(temp,left,ses);  /*do it after string comparison*/
	
	if (eval_expression(left)) {
		substitute_vars(right,temp);
		/* substitute_myvars(temp,right,ses); /*moved to write_line_mud, chitchat*/
		parse_input(temp, ses); 
	}
	else {
		/* ADDITION for if-else addition */
		line= (char *) get_arg_stop_spaces(line, elsebuf);
		if (*elsebuf && strcmp(elsebuf, "else") == 0) {
			line = get_arg_in_braces(line, elsebuf, 1);
			substitute_vars(elsebuf, temp);
			/*  substitute_myvars(temp,elsebuf,ses); /* moved to write_line_mud, chitchat*/
			parse_input(temp, ses);
		}
	}
}

int eval_strcmp(char* src, struct session* ses)
/*does not support [x=y] in loops, not necessary either i guess, chitchat*/
{
	char *ptl, *ptl2, *ptr, *ptm;
	int reverse = 0, result=0, extra=0;
	char buff[BUFFER_SIZE], left[BUFFER_SIZE], right[BUFFER_SIZE];
	
	if((ptl=strchr(src, '['))==NULL)
		return 0;
	for(ptr=ptl+1; *ptr; ptr++){ //fixed to handle [$str=[xx]]
		if(*ptr=='[') extra++;
		else if(*ptr==']'){
			if(extra==0) break;
			extra--;
		}
	}
	*ptr = 0;
	ptl2 = ptl+1;
	if((ptm=strrchr(ptl2, '='))==NULL){/* [$x]  is shortcut for [$x!=]
		[!$x] is shortcut for [$x==] */
		if(*ptl2 == '!') {
			ptl2++;
		}
		else reverse = 1;
		*right = 0;
	}
	else {
		*ptm = 0;
		ptm--;
		if(*ptm=='!'){ /* != case */
			reverse = 1;
			*ptm= 0;
		}
		else if(*ptm=='='){ /* == case */
			*ptm = 0;
		}
		substitute_myvars(ptm+2, right, ses);
	}
	substitute_myvars(ptl2, left, ses);
	if(*left=='$') *left=0; /* set un-initialized variables to "", chitchat */
	if(*right=='$') *right=0; /* set un-initialized variables to "" */
	if ((!reverse && (strcmp(right,left)==0))
		|| (reverse && !(strcmp(right,left)==0) ) ){
		result=1;
	}
	*ptl = 0;
	sprintf(buff, "%s%d%s", src, result, ptr+1);
	strncpy(src, buff, BUFFER_SIZE);
	return 1;
}

int eval_expression(const char* arg)
{
	/* int i, begin, end, flag, prev, ptr; */
	int i, begin, end, flag, prev;
	char temp[BUFFER_SIZE];
	if(*arg==0) return 0;
	i=conv_to_ints(arg);
	if (i==1) { /* changed for if strings */
		while(1) {
			i=0;
			flag=1;
			begin= -1;
			end= -1;
			prev= -1;
			while(stacks[i][0] && flag) {
				if (stacks[i][1]==P_LBRA) {
					begin=i;
				}
				else if(stacks[i][1]==P_RBRA) {
					end=i;
					flag=0;
				}
				prev=i;
				i=stacks[i][0];
			}
			if ((flag && (begin!= -1)) || (!flag && (begin== -1))) {
				sprintf(temp, "#Unmatched parentheses error: %s", arg);
				tintin_puts2(temp, NULL);
				return 0;
			}
			if (flag) {
				if (prev== -1)
					return(stacks[0][2]);
				begin= -1;
				end=i;
			}
			i=do_one_inside(begin,end);
			if (!i) {
				sprintf(temp, "#Invalid expression to evaluate in {%s}", arg);
				tintin_puts2(temp, NULL);
				return 0;
			}
		}  
	}
	else if (i==2) return 1; /* changed for #if strings */
	else return 0; 
}

int conv_to_ints(const char* arg)
{
	int i, flag;
	char *ptr, *tptr, buff[BUFFER_SIZE];
	char left[256];
	
	i=0;
	strip_ansi(arg, buff); /*strip colors when convert to int, chitchat*/
	if(*(ptr=buff+strlen(buff)-1)=='\n' || *ptr=='\r')
		*ptr = '\0'; /*strip ending \n or \r, only once shall be enuf, chitchat */
	ptr=buff;
	while(*ptr==' ' || *ptr=='\t') //chitchat, 5/15/2000
		memmove(ptr, ptr+1, strlen(ptr));
	strcpy(ptr, buff);
	while (*ptr) {
		if(*ptr==' ' || *ptr=='\t') ;
		else if (*ptr=='(') {
				stacks[i][1]=P_LBRA;
		}
		else if(*ptr==')') {
			stacks[i][1]=P_RBRA;
		}
		else if(*ptr=='!') {
			if (*(ptr+1)=='=') { 
				stacks[i][1]=P_NE;
				ptr++;
			}
			else
				stacks[i][1]=P_NOT;
		}
		else if(*ptr=='*') {
			stacks[i][1]=P_MULT;
			stacks[i][3]=1;
		}
		else if(*ptr=='/') {
			if (*(ptr+1)=='/'){/*added // for module, should be % but // is clearer for coding */
					/*let's make // 1 higher priority than *, */
					/*tintin1.64's math sequence was not right anyway */
					ptr++;
				stacks[i][1]=P_MOD;
			}
			else{
				stacks[i][1]=P_DIV;
				stacks[i][3]=0;
			}
		}
		else if(*ptr=='+') {
			stacks[i][1]=P_PLUS;
		}
		else if(*ptr=='-') {
			flag= -1;
			if (i>0)
				flag=stacks[i-1][1];
			if (flag==P_LOWEST){
				stacks[i][1]=P_MINUS;
			}
			else {
				tptr=ptr;
				ptr++;
				while(isdigit(*ptr))
					ptr++;
				sscanf(tptr,"%d",&stacks[i][2]);
				stacks[i][1]=P_LOWEST;
				ptr--;
			}
		}
		else if(*ptr=='>') {
			if (*(ptr+1)=='=') {
				stacks[i][1]=P_GE;
				ptr++;
			}
			else
				stacks[i][1]=P_GT;
		}
		else if(*ptr=='<') {
			if (*(ptr+1)=='=') {
				ptr++;
				stacks[i][1]=P_LE;
			}
			else
				stacks[i][1]=P_LT;
		}
		else if(*ptr=='=') {
			stacks[i][1]=P_EQ;
			if (*(ptr+1)=='=')
				ptr++;
		}
		else if(*ptr=='&') {
			stacks[i][1]=P_AND;
			stacks[i][3]=1;
			if (*(ptr+1)=='&')
				ptr++;
		}
		else if(*ptr=='|') {
			stacks[i][1]=P_OR;
			stacks[i][3]=0;
			if (*(ptr+1)=='|')
				ptr++;
		}
		else if (isdigit(*ptr)) {
			stacks[i][1]=P_LOWEST;
			tptr=ptr;
			while (isdigit(*ptr))
				ptr++;
			sscanf(tptr,"%d",&stacks[i][2]);
			ptr--;
		}
		else if (*ptr=='T') {
			stacks[i][1]=P_LOWEST;
			stacks[i][2]=1;
		}
		else if(*ptr=='F') {
			stacks[i][1]=P_LOWEST;
			stacks[i][2]=0;
		}
		
		
		else {
			if(*buff != '$') {/*do not give warning for un-initialized variables */
				sprintf(left, "#Warning(#if, #while or #math): converted %s to 0", buff); /* added #while - vastheman 2002-08-05 */
				tintin_puts2(left, NULL);
				return 0;
			}
			else {/*also handle un-initialized $x+1, $x=0 case */
				for(ptr=buff+1;Is_Alnum(*ptr);ptr++) ;
				*(--ptr) = '0'; /*so $x+1 is now 0+1 */
				return conv_to_ints(ptr);
			}
		}
		if (*ptr!=' ') {
			stacks[i][0]=i+1;
			i++;
		}
		ptr++;
	}
	if (i>0)
		stacks[i][0]=0;
	return 1;
}

int do_one_inside(int begin, int end)
{
	/* int prev, ptr, highest, loc, ploc, next, nval, flag; */
	int prev, ptr, highest, loc, ploc, next;
	while(1) {
		ptr=0;
		if (begin>-1)
			ptr=stacks[begin][0];
		highest=P_LOWEST+1;
		loc= -1;
		ploc= -1;
		prev= -1;
		while (ptr<end) {
			if (stacks[ptr][1]<highest) {
				highest=stacks[ptr][1];
				loc=ptr;
				ploc=prev;
			}
			prev=ptr;
			ptr=stacks[ptr][0];
		}
		if (highest==P_LOWEST) {
			if (begin>-1) {
				stacks[begin][1]=P_LOWEST;
				stacks[begin][2]=stacks[loc][2];
				stacks[begin][0]=stacks[end][0];
				return 1;
			}
			else {
				stacks[0][0]=stacks[end][0];
				stacks[0][1]=P_LOWEST;
				stacks[0][2]=stacks[loc][2];
				return 1;
			}
		}
		else if (highest==P_NOT) {
			next=stacks[loc][0];
			if (stacks[next][1]!=P_LOWEST || stacks[next][0]==0) {
				return 0;
			} 
			stacks[loc][0]=stacks[next][0];
			stacks[loc][1]=P_LOWEST;
			stacks[loc][2]=!stacks[next][2];
		} 
		else {
			next=stacks[loc][0];
			if (ploc== -1 || stacks[next][0]==0 || stacks[next][1]!=P_LOWEST) 
				return 0;
			if (stacks[ploc][1]!=P_LOWEST)
				return 0;  
			switch (highest) {
				case P_MOD: /*highest priority is % (we use //) */
					stacks[ploc][0]=stacks[next][0];
					if (stacks[next][2]) 
						stacks[ploc][2]%=stacks[next][2];
						else
						tintin_puts2("#ERROR - module by 0.", NULL);
						break;
					case P_MULT: /* highest priority is * */
						if(stacks[loc][3]){ /* multiple */
							stacks[ploc][0]=stacks[next][0];
							stacks[ploc][2]*=stacks[next][2];
						}
						/*   break;
					case P_DIV: /* highest priority is / */
						else { /* div */
							stacks[ploc][0]=stacks[next][0];
							if (stacks[next][2]) 
							stacks[ploc][2]/=stacks[next][2];
							else
							tintin_puts2("#ERROR - dividing by zero.. You know something I don't know yet? :-)", NULL);
						}
						break;
					case P_MINUS: /* highest priority is - */
						stacks[ploc][0]=stacks[next][0];
						stacks[ploc][2]-=stacks[next][2];
						break;
					case P_PLUS: /* highest priority is + */
						stacks[ploc][0]=stacks[next][0];
						stacks[ploc][2]+=stacks[next][2];
						break;
					case P_GT: /* highest priority is > */
						stacks[ploc][0]=stacks[next][0];
						stacks[ploc][2]=(stacks[ploc][2]>stacks[next][2]);
						break;
					case P_GE: /* highest priority is >= */
						stacks[ploc][0]=stacks[next][0];
						stacks[ploc][2]=(stacks[ploc][2]>=stacks[next][2]);
						break;
					case P_LT: /* highest priority is < */
						stacks[ploc][0]=stacks[next][0];
						stacks[ploc][2]=(stacks[ploc][2]<stacks[next][2]);
						break;
					case P_LE: /* highest priority is <= */
						stacks[ploc][0]=stacks[next][0];
						stacks[ploc][2]=(stacks[ploc][2]<=stacks[next][2]);
						break;
					case P_EQ: /* highest priority is == */
						stacks[ploc][0]=stacks[next][0];
						stacks[ploc][2]=(stacks[ploc][2]==stacks[next][2]);
						break;
					case P_NE: /* highest priority is != */
						stacks[ploc][0]=stacks[next][0];
						stacks[ploc][2]=(stacks[ploc][2]!=stacks[next][2]);
						break;
					case P_AND: /* highest priority is && */
						if(stacks[loc][3]){ /* && */
							stacks[ploc][0]=stacks[next][0];
							stacks[ploc][2]=(stacks[ploc][2]&&stacks[next][2]);
						}
						/*     break;
					case P_OR: /* highest priority is || */
						else { /* || */
							stacks[ploc][0]=stacks[next][0];
							stacks[ploc][2]=(stacks[ploc][2]||stacks[next][2]);
						}
						break;
					default:
						tintin_puts2("#Programming error *slap Bill*", NULL);
						return 0;   
				}
			}
		}
	}
/***************************/
/* the #ifstrequal command */
/* this is not necessary since it is included in #if {[left=right]},
/*finally get rid of it in this version, chitchat, 6/1/2000
/***************************/

void ifexist_command(const char *line, struct session* ses)
{
	char varname[BUFFER_SIZE], dothis[BUFFER_SIZE];
	char temp[BUFFER_SIZE], elsebuf[BUFFER_SIZE];
	struct listnode *my_vars, *ln;
	
	line = get_arg_in_braces(line, varname, 0);
	line = get_arg_in_braces(line, dothis, 1);
	
	substitute_vars(varname,temp);
	substitute_myvars(temp, varname, ses);
		
	my_vars=(!(*varname=='_' && varname[1]) && ses) ? ses->myvars : common_myvars;
		
	if ((*varname) && (*dothis)) {
		if((ln = searchnode_list(my_vars, varname))) {
			//substitute_vars(dothis, temp);
			parse_input(dothis, ses); //6/1/2000, chit, thanks petter for point this out
		}
		else {
			line= (char *) get_arg_stop_spaces(line, elsebuf);
			if(*elsebuf && strcmp(elsebuf, "else") == 0) {
				line = get_arg_in_braces(line, elsebuf, 1);
				substitute_vars(elsebuf, temp);
				/*  substitute_myvars(temp, elsebuf, ses); /*moved to write_line_mud, chitchat*/
				parse_input(temp, ses);
			}
		}
	}
	else tintin_puts2("#NOT ENOUGH ARGUMENTS!!!", ses);
} 

void revstring_command(const char *line, struct session* ses)
{
		char left[BUFFER_SIZE], result[BUFFER_SIZE];
		char string[BUFFER_SIZE], temp[BUFFER_SIZE];
		struct listnode *my_vars, *ln;
		int i;
		
		line = get_arg_in_braces(line, left, 0);
		line = get_arg_in_braces(line, string, 1);
		
		my_vars=(!(*left=='_' && left[1]) && ses) ? ses->myvars : common_myvars;
		
		if ((*left) && (*string)) {
			for(i=(int)strlen(string)-1; i>=0; i--)
			temp[strlen(string)-i-1] = string[i];
			temp[strlen(string)] = '\0';
			if((ln = searchnode_list(my_vars,left)))
			deletenode_list(my_vars, ln);
			insertnode_list(my_vars, left, temp, "0", ALPHA); 
			varnum++;
			if(mesvar[5]) {
				sprintf(result, "#Ok. $%s is now set to {%s}.", left, temp);
				tintin_puts2(result, ses);
			}
		}
		else
			tintin_puts2("#NOT ENOUGH ARGUMENTS!!!", ses);
} 

/*********************************************************************/
/* the #ifmatch command * By Sverre Normann             */
/*********************************************************************/
/* Syntax: #ifmatch {mask} {string} {do this} else {do this instead} */
/*********************************************************************/
/* Uses "int match" from glob.c */
/********************************/
void ifmatch_command(const char *arg, struct session* ses)
{
		char mask[BUFFER_SIZE], string[BUFFER_SIZE],
		right[BUFFER_SIZE], elsebuf[BUFFER_SIZE],
		temp[BUFFER_SIZE];
		arg=get_arg_in_braces(arg, mask, 0);
		arg=get_arg_in_braces(arg, string, 0);
		arg=get_arg_in_braces(arg, right, 1);
		if (!*mask || !*string || !*right) {
			tintin_puts2("#Syntax: #ifmatch {mask} {string} {do this} else {do this instead}", ses);
		}
		else {
			substitute_vars(mask,temp);
			substitute_myvars(temp,mask,ses);
			substitute_vars(string,temp);
			substitute_myvars(temp,string,ses);
			if (match(mask,string)) {
				/* Match */
				substitute_vars(right,temp);
				/*  substitute_myvars(temp, right, ses); /*moved to write_line_mud, chitchat*/
				parse_input(temp, ses);
			}
			else {
				/* No match; check for else */
				arg = (char *) get_arg_stop_spaces(arg, elsebuf);
				if (*elsebuf && strcmp(elsebuf, "else") == 0) {
					arg = get_arg_in_braces(arg, elsebuf, 1);
					substitute_vars(elsebuf, temp);
					/*   substitute_myvars(temp, elsebuf, ses); /*moved to write_line_mud, chitchat*/
					parse_input(temp, ses);
				}
			}
		}
	}

