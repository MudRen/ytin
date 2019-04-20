/* New for v2.0: readline support -- daw */

/*
* Routines to figure out the screen size for split mode.
* Code is mostly stolen from less-v177!
*/

#include <config.h>
#include "tintin.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef Xenix
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_TERMIO_H
#include <sys/termio.h>
#elif !defined(_WINDOWS)
#include <sgtty.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#if defined(HAVE_SYS_TERMIO_H) || !defined(TIOCGWINSZ)
/*
* For the Unix PC (ATT 7300 & 3B1):
* Since WIOCGETD is defined in sys/window.h, we can't use that to decide
* whether to include sys/window.h.  Use SIGPHONE from signal.h instead.
*/
#include <signal.h>
#ifdef SIGPHONE
#include <sys/window.h>
#endif
#endif

/* for SCO.  god only knows if this'll work.  -- daw */
#if defined(HAVE_PTEM_H) && defined(TIOCGWINSZ)
/*
* All this just to get struct winsize.  Sigh.
*/
#include <sys/types.h>
#include <sys/stream.h>
#include <sys/ptem.h>
#endif

#ifdef HAVE_TERMCAP_H
#include <termcap.h>
#else
extern "C" int tgetnum(char *id);
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

/*
* These two variables are sometimes defined in,
* and needed by, the termcap library.
* It may be necessary on some systems to declare them extern here.
*/
/*extern short ospeed;*/ /* Terminal output baud rate */
/*extern char PC;*/  /* Pad character */

/*
* Get size of the output screen.
*/
void scrsize(unsigned int *p_height, unsigned int *p_width)
{
	register char *s;
	#if defined(TIOCGWINSZ)
	struct winsize w;
	#elif defined(WIOCGETD)
	struct uwdata w;
	#endif
	
#if defined(TIOCGWINSZ)
	if(!ioctl(2, TIOCGWINSZ, &w) && w.ws_row > 0)
		*p_height = w.ws_row;
	else
#elif defined(WIOCGETD)
		if(!ioctl(2, WIOCGETD, &w) && w.uw_height > 0)
		*p_height = w.uw_height/w.uw_vs;
	else
#endif
		if((s = getenv("LINES")))
		*p_height = atoi(s);
	else
		*p_height = tgetnum("li");
	
	if(*p_height <= 0)
		*p_height = SCREEN_HEIGHT;
	
#if defined(TIOCGWINSZ)
	if(!ioctl(2, TIOCGWINSZ, &w) && w.ws_col > 0)
		*p_width = w.ws_col;
	else
#elif defined(WIOCGETD)
		if(!ioctl(2, WIOCGETD, &w) && w.uw_width > 0)
		*p_width = w.uw_width/w.uw_hs;
	else
#endif
		if((s = getenv("COLUMNS")))
		*p_width = atoi(s);
	else
		*p_width = tgetnum("co");
	
	if(*p_width <= 0)
		*p_width = SCREEN_WIDTH;
}
