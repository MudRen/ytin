/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */

#include <config.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#elif defined(HAVE_STRINGS_H)
#include <strings.h>
#endif

#include "glob.proto"

/* match -- returns 1 if `string' satisfised `regex' and 0 otherwise      */
/* stolen from Spencer Sun: only recognizes * and \ as special characters */

int match(const char *regex, const char *string)
{
	const char *save;
	char ch;
	
	while(*regex) {
		switch(ch = *regex++) {
			case '*':
				if(!*string)  /* match empty string at end of `string' */
					return('\0' == *regex); /* but only if we're done with the pattern */
				/* greedy algorithm: save starting location, then find end of string */
				save = string;
				string += strlen(string);
				do {
					if(match(regex, string))     /* return success if we can match here */
						return(1);
				} while(--string >= save);  /* otherwise back up and try again */
				/* Backed up all the way to starting location (i.e. `*' matches
				* empty string) and we _still_ can't match here.  Give up. */
				return(0);
				
			case '\\':
				if((ch = *regex++)) {
					/* if not end of pattern, match next char explicitly */
					if(ch != *string++)
						return(0);
					break;
				}
				/* else FALL THROUGH to match a backslash */
			default: /* normal character */
				if(ch != *string++)
					return(0);
				break;
		}
	}
	/* OK, we successfully matched the pattern if we got here.  Now return
	* a match if we also reached end of string, otherwise failure */
	return('\0' == *string);
}
