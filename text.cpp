/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */

/*********************************************************************/
/* file: text.c  - funtions for logfile and reading/writing comfiles */
/*                             TINTIN + +                            */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by peter unold 1992                     */
/*                    New code by Joann Ellsworth                    */
/*********************************************************************/

#include <config.h>
#include "tintin.h"

#if defined(HAVE_STRING_H)
#include <string.h>
#elif defined(HAVE_STRINGS_H)
#include <strings.h>
#endif

#include "net.proto"
#include "parse.proto"
#include "rl.proto"
#include "text.proto"

/**********************************/
/* load a file for input to mud.  */
/**********************************/

void read_file(const char *arg, struct session* ses)
{
	FILE *myfile;
	int l;
	char buffer[BUFFER_SIZE];
	
	get_arg_in_braces(arg, buffer, 1);
	if(!ses) {
		tintin_puts("You can't read any text in without an active session.", NULL);
		prompt(NULL);
		return;
	}
	
	if(!(myfile = fopen(buffer, "rb"))) {
		tintin_puts("ERROR: No file exists under that name.\n", NULL);
		prompt(NULL);
		return;
	}
	
	while( (l=fread(buffer, sizeof(char), sizeof(buffer), myfile)) ) {
		write_raw_mud(buffer, (unsigned)l, ses);
	}
	
	fclose(myfile);
	tintin_puts("File read - Success.\n", NULL);
	prompt(NULL);
	tintin_puts("\n", NULL);
}
