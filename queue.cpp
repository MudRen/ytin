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
#include <unistd.h>
#endif


#if defined(HAVE_SYS_TERMIO_H) || !defined(TIOCGWINSZ)
/*
* For the Unix PC (ATT 7300 & 3B1):
* Since WIOCGETD is defined in sys/window.h, we can't use that to decide
* whether to include sys/window.h. Use SIGPHONE from signal.h instead.
*/
#include <signal.h>
#ifdef SIGPHONE
#include <sys/window.h>
#endif
#endif

#include "parse.proto"
#include "queue.proto"
#include "net.proto"
#include "rl.proto"
#include "utils.proto"
#include "variables.proto"

struct event *first = NULL;

void check_queue(struct timeval* tv)
{
	struct timeval now;
	long dsec, dusec;
	if (!first) 
		return; /*timeval unchanged */
	gettimeofday(&now, 0);
	dsec = first->tm.tv_sec - now.tv_sec;
	dusec = first->tm.tv_usec - now.tv_usec;
	if(dsec < 0 || (dsec==0 && dusec<=0)){
		process_event(tv); /*process the next event, then call check_queue again */
	}
	else {
		if(dusec < 0) {
			dusec += 1000000;
			dsec--;
		}
		if(dsec < tv->tv_sec ||
			(dsec==tv->tv_sec && dusec<tv->tv_usec)){
			/*assign the shorter one to tv */
			tv->tv_sec = dsec;
			tv->tv_usec = dusec;
		}
	}
}

void process_event(struct timeval* tv)
{
	struct timeval now;
	struct event* tmp;
	struct session* ses;
	char* buf;
	int do_parse = 0;

	if (!first) return;
	/* printf("process event %d\n", first);*/
	ses = first->ses;
	if (ses && first->type==EVENT_TICK) {
		if (ses->tickstatus) {
			gettimeofday(&now, 0); //make the tick a little bit "soft"
			if (ses->ticksize.tv_sec > 10) {
				struct timeval tickwarn;
				tickwarn.tv_sec = now.tv_sec + ses->ticksize.tv_sec - 10;
				tickwarn.tv_usec = now.tv_usec;
				add_queue(ses, EVENT_TICKWARN, &tickwarn, 0);
			}
			ses->nexttick.tv_sec = now.tv_sec + ses->ticksize.tv_sec;
			ses->nexttick.tv_usec = now.tv_usec;
			add_queue(ses, EVENT_TICK, &(ses->nexttick), 0);
			tintin_puts("#TICK!!!", ses);
		}
		else ses->tick = 0;
	}
	else if (ses && first->type==EVENT_TICKWARN) {
		if (ses->tickstatus) tintin_puts("#10 SECONDS TO TICK!!!", ses);
		ses->tickwarn = 0;
	}
	else if (ses && first->type==EVENT_WRITE) {
		write_line_mud1(first->line, ses);
	}
	else if (first->line) { //need free things 1st, then parse_input(), chitchat 1/18/2000
		buf = mystrdup(first->line);
		do_parse = 1;
	}
	tmp = first->next;
	if (first->line)
		free(first->line);
	free(first);
	if( (first=tmp) ) first->prev = 0;

	if(do_parse) {
		parse_input(buf, ses);
		free(buf);
	}

	if(first){
		check_queue(tv);
	}
}

void add_queue(struct session* &ses, int type, struct timeval *tp, const char* line)
{
		struct event* new_event;
		int length;
		struct event* tmp_event;
		new_event = (struct event*) malloc(sizeof(struct event));
		if (!new_event) syserr("add_queue:malloc");
		/* printf("add queue %d, %d (%d, %d)\n", new_event, type, tp->tv_sec, tp->tv_usec); */
		new_event->ses = ses;
		new_event->type = type;
		new_event->tm.tv_sec = tp->tv_sec;
		new_event->tm.tv_usec = tp->tv_usec;
		new_event->next = (struct event *)0;
		new_event->prev = (struct event *)0;
		if (ses && type==EVENT_TICK) ses->tick = new_event;
		if (ses && type==EVENT_TICKWARN) ses->tickwarn = new_event;
		if (line) {
			length = strlen(line);
			new_event->line = (char *) malloc(length+1);
			if (!new_event->line) syserr("add_queue:malloc");
			strcpy(new_event->line, line);
		}
		else
			new_event->line = 0;
		if (!first) {
			first = new_event;
		}
		else if ((first->tm.tv_sec>tp->tv_sec)||
			(first->tm.tv_sec==tp->tv_sec && first->tm.tv_usec>tp->tv_usec)) {
			new_event->next = first;
			first->prev = new_event;
			first = new_event;
		}
		else {
			struct event *next_event;
			tmp_event = first;
			next_event = first->next;
			while((next_event)&&(next_event->tm.tv_sec<tp->tv_sec||
				(next_event->tm.tv_sec==tp->tv_sec
				&& next_event->tm.tv_usec<=tp->tv_usec))) {
				tmp_event = next_event;
				next_event = tmp_event->next;
			}
			new_event->next = next_event;
			new_event->prev = tmp_event;
			tmp_event->next = new_event;
			if (next_event) next_event->prev = new_event;
		}
} 

void remove_queue(struct event* this_event)
{
/*note: have to make sure this_even is in queue, otherwise crash, chitchat, 1/19/2000 */
		if (!this_event) return;
		/* printf("remove queue: %d\n", this_event);
		printf("remove:%d,%d,%d\n", this_event->next, this_event->line, this_event->next); */
		if (this_event->next) this_event->next->prev = this_event->prev;
		if (this_event->line) free(this_event->line);
		if (this_event->prev) this_event->prev->next = this_event->next;
		else {
			first = this_event->next;
		}
		/*printf("remove queue:done\n");*/
		free(this_event);
}

void kill_all_event(struct session* ses)
{
		struct event* this_event = first;
		/*printf("ok, kill all event\n");*/
		while (this_event) {
			struct event* next_event;
			/*	printf("<%10d,%10d,%10d>\n", this_event->prev, this_event,
			this_event->next);*/
			next_event = this_event->next;
			if (this_event->ses == ses) remove_queue(this_event);
			this_event = next_event;
		}
		purge_slow(ses);
}

void show_all_event(struct session* ses)
{
	char buf[BUFFER_SIZE];
	struct event* this_event = first;
	while (this_event) {
		if(this_event->ses == ses) {
			sprintf(buf, "sec=%d, usec=%d, type=%d, %s",
				this_event->tm.tv_sec, this_event->tm.tv_usec, this_event->type,
				(this_event->type >= EVENT_WRITE) ? this_event->line : "");
			tintin_puts2(buf, ses);
		}
		this_event = this_event->next;
	}
}

void kill_all_pending_command(int purgemode, struct session* ses)
{
	struct event* this_event = first;
	while (this_event) {
				struct event* next_event;
				next_event = this_event->next;
				if (this_event->ses==ses
					&& (this_event->type==EVENT_WRITE 
            || (purgemode & this_event->type) )) //changed to bit comparison, 4/13/2006
					remove_queue(this_event);
				this_event = next_event;
	}
	gettimeofday(&(ses->lastwrite), 0);
}

void sleep_command(const char* arg, struct session* &ses, const char* line, int event_type)
{
	float tlen;
	long sec, usec;
	struct timeval now, tm;
	char result[BUFFER_SIZE];
	if ((!line)||!(*line)) return;
	substitute_myvars(arg, result, ses);
	/* tlen = atof(result); /*ack, this atof() is buggy when math lib forgot to link*/
	sscanf(result, "%f", &tlen);
	gettimeofday(&now, 0);
	sec = (int) tlen; usec = (int)((tlen - (float)sec)*1e6);
	usec += now.tv_usec; sec += now.tv_sec + usec / 1000000;
	usec %= 1000000;
	tm.tv_sec = sec; tm.tv_usec = usec;
	add_queue(ses, event_type, &tm, line); //event_type is EVENT_SLEEP or EVENT_SLEEP2
}

void add_queue_slow(const char* line, struct session* ses)
{
	struct s_list *new_slow, *slist;
	new_slow = (struct s_list*) malloc(sizeof(struct s_list));
	if (!new_slow) syserr("add_queue_slow: malloc");
	new_slow->next = NULL;
	new_slow->strng = mystrdup(line);
	if(!ses->slow) {
		ses->slow = new_slow;
		return;
	}
	for(slist=ses->slow; slist->next; slist = slist->next) ;
	slist->next = new_slow;
}

void check_queue_slow(struct session* ses)
{
	struct s_list *slist;
	for(; ses->slow; ){
		slist = ses->slow;
		if(!ses->slowstatus) ;
		else if(ses->slowstatus > ses->slowsent){
			ses->slowsent ++;
		}
		else break;
		write_line_mud1(slist->strng, ses);
		free(slist->strng);
		ses->slow = slist->next;
		free(slist);
	}
}

void purge_slow(struct session* ses)
{
	struct s_list *slist;
	if(!ses) return;
	for(; ses->slow; ){
		slist = ses->slow;
		free(slist->strng);
		ses->slow = slist->next;
		free(slist);
	}
}

