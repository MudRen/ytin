/* New for v2.0: readline support -- daw */

/*
* this is a reimplementation of the standard tickcounter, *without* alarm()!
* god i hate alarm() -- it screws everything up, and isn't portable,
* and tintin was calling alarm() every second! blech.
*/

#include <config.h>
#include "tintin.h"

#ifdef HAVE_TIME_H
#include <time.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "main.proto"
#include "queue.proto"
#include "rl.proto"
#include "rltick.proto"
#include "ticks.proto"

int timetilltick(struct session* ses)
{
	struct timeval now;
	gettimeofday(&now, 0);
	return ses->nexttick.tv_sec - now.tv_sec;
}

/*
* returns how long before the next event, in seconds.
* (i.e. if we're 13 seconds till tick in some session,
* we return 3, because in 3 seconds we'll need to warn
* the user that they are 10 seconds short of the tick.)
* 
* also prints the tick warnings, by the way. :-)
* 
* bug: if you suspend tintin++ for a few minutes, then
* bring it back, you get lots of tick warnings. is this
* the desired behavior?
*/
void checktick(struct timeval* tv)
{
	int found=0;
	
	tv->tv_sec = 100; /*big number*/
	tv->tv_usec = 0;
	
#ifndef _WINDOWS
	/* printf("debug: bait called!\n"); */
	if(is_split && ticker_in_split) {
		if(activesession && activesession->tickstatus)
			found = 1;
		
		if(tickcounter_in_splitline(found ? timetilltick(activesession) : -1))
			tv->tv_sec = 1; /* Needs to be updated each second... */
	}
#endif
	
	check_queue(tv);
}
