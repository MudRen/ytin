/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: utils.c - some utility-functions               */
/*               TINTIN III               */
/*     (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t       */
/*           coded by peter unold 1992           */
/*********************************************************************/

/* note: changed a little bit for readline support -- daw */

#include <config.h>
#include "tintin.h"

#if defined(HAVE_STRING_H)
#include <string.h>
#elif defined(HAVE_STRINGS_H)
#include <strings.h>
#endif

#include <ctype.h>

#ifndef _WINDOWS
#include <sys/param.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "main.proto"
#include "rl.proto"
#include "utils.proto"

/**********************************************/
/* return: TRUE if s1 is an abbrevation of s2 */
/**********************************************/

int is_abbrev(const char *s1, const char *s2)
{
	if(!s1 || !s2 || !*s1 || !*s2)
		return(FALSE);
	
	if(strlen(s2) < strlen(s1))
		return(FALSE); /* not really abbrev if s2 < s1 */
	
	return(!strncmp(s2, s1, strlen(s1))); /*change strncasecmp to strncmp */
}

/********************************/
/* strdup - duplicates a string */
/* return: address of duplicate */
/********************************/

char *mystrdup(const char *str)
{
	char *buf;
	
	if(!(buf = (char *)malloc(strlen(str)+1)))
		syserr("Not enought memory for mystrdup().");
	strcpy(buf, str);
	return(buf);
}

/*************************************************/
/* print system call error message and terminate */
/*************************************************/

void syserr(const char *msg)
{ //changed according to YoonChan Jhi, 5/16/2000
	extern int errno;
	char s[128], *syserrmsg;

	syserrmsg = strerror(errno) ;
	
	if(syserrmsg)
		sprintf(s, "ERROR: %s (%d: %s)", msg, errno, syserrmsg);
	else
		sprintf(s, "ERROR: %s (%d)", msg, errno);
	quitmsg(s);
}

/* Whoops, strcasecmp wasn't found. */
#if !defined(HAVE_STRCASECMP)
#define UPPER(c) (islower(c) ? toupper(c) : c)

int strcasecmp(const char *string1, const char *string2)
{
	for(; UPPER(*string1) == UPPER(*string2); string1++, string2++)
		if(!*string1)
		return(0);
	return(UPPER(*string1) - UPPER(*string2));
}

int strncasecmp(const char *string1, const char *string2, size_t count)
{
	if(count)
		do {
		if(UPPER(*string1) != UPPER(*string2))
			return(UPPER(*string1) - UPPER(*string2));
		if(!*string1++)
			break;
		string2++;
	} while(--count);
	return(0);
}
#endif

unsigned long millisec_started()
{
	static int SPD = 60*60*24; //second per day
	time_t time_now;
	struct timeval now;

	gettimeofday(&now,0);
	time_now = (time(NULL) - time_started);
	return ((now.tv_sec-tv_started.tv_sec+SPD)%SPD+time_now/SPD*SPD)*1000 + now.tv_usec/1000;
}

/*
static FILE *fpdbg1, *fpdbg2;
void* dbgMalloc(size_t size)
{
	void *v;
	if(!fpdbg1) fpdbg1 = fopen("c:\\temp\\debug1.log", "w");
	v = malloc(size);
//	fprintf(fpdbg1, "Malloc: address=%x, size=%ul\n", v, size);
	fprintf(fpdbg1, "%x\n", v);
	return v;
}

void dbgFree(void* mem){
	if(!fpdbg2)
//		fpdbg2 = fpdbg1;
		fpdbg2 = fopen("c:\\temp\\debug2.log", "w");
//	fprintf(fpdbg2, "Free  : address=%x\n", mem);
	fprintf(fpdbg2, "%x\n", mem);
	free(mem);
}
*/
