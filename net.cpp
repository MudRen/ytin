/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: net.c - do all the net stuff                */
/*               TINTIN III              */
/*     (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t       */
/*           coded by peter unold 1992           */
/*********************************************************************/

#include <config.h>
#include "tintin.h"

#if defined(HAVE_STRING_H)
#include <string.h>
#elif defined(HAVE_STRINGS_H)
#include <strings.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <sys/types.h>

#ifndef _WINDOWS
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

#include <signal.h>

#ifdef HAVE_NET_ERRNO_H
#include <net/errno.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#ifndef _WINDOWS
#include <arpa/inet.h>
#endif
#endif

#ifdef SOCKS
#include <socks.h>
#endif

#include "log.proto"
#include "main.proto"
#include "misc.proto"
#include "net.proto"
#include "parse.proto"
#include "rl.proto"
#include "session.proto"
#include "utils.proto"
#include "queue.proto"

#ifndef BADSIG
#define BADSIG (RETSIGTYPE (*)(int))-1
#endif

#ifdef _WINDOWS
#define SOCK_BUFSIZ 1500
#else
//wintin use 1500 instead of 512, speed vs. response?
#define SOCK_BUFSIZ 1500
static RETSIGTYPE alarm_handler(int no_care)
{
	/* do nothing; the connect will fail, returning -1, with errno = EINTR */
}
#endif

/**************************************************/
/* try connect to the mud specified by the args  */
/* return fd on success / 0 on failure      */
/**************************************************/

int connect_mud(const char *host, const char *port, struct session* ses)
{
	int sock, connectresult;
	struct sockaddr_in sockaddr;
  int bOptVal = tcp_nodelay;

#ifdef _WINDOWS
	char errbuf[BUFFER_SIZE];
#endif
	
	if(isdigit(*host))              /* interprete host part */
		sockaddr.sin_addr.s_addr = inet_addr(host);
	else {
		struct hostent *hp;
		
		if(!(hp = gethostbyname(host))) {
			tintin_puts("#ERROR - UNKNOWN HOST.", ses);
			prompt(NULL); 
			return(0);
		}
		memcpy((char *)&sockaddr.sin_addr, hp->h_addr, sizeof(sockaddr.sin_addr));
	}
	
	if(isdigit(*port)) 
		sockaddr.sin_port = htons((unsigned short)atoi(port));   /* inteprete port part */
	else {
		tintin_puts("#THE PORT SHOULD BE A NUMBER.", ses);
		prompt(NULL);
		return(0);
	}
	
	sockaddr.sin_family = AF_INET;
	
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		syserr("socket");

	tintin_puts("#Trying to connect..", ses);
	
#ifndef _WINDOWS
	/* ignore the alarm: it'll cause the connect to return -1 with errno=EINTR */
	if(signal(SIGALRM, alarm_handler) == BADSIG)
		syserr("signal SIGALRM");
	alarm(30);     /* We'll allow connect to hang in 15seconds! NO MORE! */
#endif
	
	connectresult = connect(sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
	
#ifndef _WINDOWS
	alarm(0);
	
	if(connectresult) {
		close(sock);
		switch(errno) {
			case EINTR:
				tintin_puts("#CONNECTION TIMED OUT.", ses);
				break;
			case ECONNREFUSED:
				tintin_puts("#ERROR - CONNECTION REFUSED.", ses);
				break;
			case ENETUNREACH:
				tintin_puts("#ERROR - THE NETWORK IS NOT REACHABLE FROM THIS HOST.", ses);
				break;
			default:
				tintin_puts("#Couldn't connect.", ses);
		} 
		prompt(NULL);
		return(0);
	}

#else
	if (connectresult)
	{
		winsockerrmsg(WSAGetLastError(), errbuf);
		tintin_puts2(errbuf, NULL);
		prompt(NULL);
		return 0;
	}
	
	//Get async notification of incoming data
	// FD_CLOSE added per Feng Chen
	if (WSAAsyncSelect(sock, getmainwnd(), WM_USER + 1, FD_READ |FD_CLOSE ) == SOCKET_ERROR)
	{
		winsockerrmsg(WSAGetLastError(), errbuf);
		tintin_puts2(errbuf, ses); prompt(NULL); //messagebox(errbuf);
		syserr("async select");
	}
#endif

  if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&bOptVal, sizeof(bOptVal)) == SOCKET_ERROR) {
        tintin_puts("SOCKET: error setting TCP_NODELAY", ses);
#ifdef _WINDOWS
    winsockerrmsg(WSAGetLastError(), errbuf);
		tintin_puts2(errbuf, ses); prompt(NULL);
#endif
  }

  return(sock);
}

/************************************************************/
/* write raw data to the mud ses is connected to            */
/************************************************************/
void write_raw_mud(const char *buff, unsigned l, struct session* ses)
/* the reason we want length is because TEL_IS == 0 */
{
#ifdef _WINDOWS
	char errbuf[BUFFER_SIZE];
	int sent, sockerr;
#endif

	if(ses->lograw==1) logit(ses, "\n==>");
	if(ses->lograw<2) lograw(ses, buff, l);
	if(ses->lograw==1) logit(ses, "\n");
	if(ses->socket){ /*check if it is dummy session*/
		
#ifdef _WINDOWS
/* fixed the winsock block when #textin, chitchat, 4/19/2000 */
		do {
			sent=send(ses->socket, buff, l, 0);
		} while(sent == SOCKET_ERROR && (sockerr=WSAGetLastError())==WSAEWOULDBLOCK);
		if(sent == SOCKET_ERROR) { //errors other than block
			winsockerrmsg(sockerr, errbuf);
			syserr(errbuf);
		}
#else
		if(write(ses->socket, buff, l) == -1)
			syserr("write in write_to_mud");
#endif
	}
}
/************************************************************/
/* write line to the mud ses is connected to - add \n first */
/************************************************************/
void write_line_mud1(const char *line, struct session* ses)
{
	char outtext[BUFFER_SIZE+2];
#ifdef _WINDOWS
	int sent, sockerr;
	char errbuf[BUFFER_SIZE];
#endif
	
  // Nagle algorithm enabled or disabled here
	strcpy(outtext, line);
	strcat(outtext, nlTail);
	if(ses->lograw==1) logit(ses, "\n==>");
	if(ses->lograw<2) logit(ses, outtext);
	if(ses->socket){ /*check if it is dummy session*/
		
#ifdef _WINDOWS
/* fixed the winsock block when #textin, chitchat, 4/19/2000 */
		do {
			sent=send(ses->socket, outtext, strlen(outtext), 0);
		} while(sent == SOCKET_ERROR && (sockerr=WSAGetLastError())==WSAEWOULDBLOCK);
		if(sent == SOCKET_ERROR) { //errors other than block
			winsockerrmsg(sockerr, errbuf);
			syserr(errbuf);
		}
#else
		if(write(ses->socket, outtext, strlen(outtext)) == -1)
			syserr("write in write_to_mud");
#endif
	}
}

void write_line_mud2(const char *line, struct session* ses)
{
	gettimeofday(&(ses->lastwrite), 0);
	if(!ses->slowstatus)
		write_line_mud1(line, ses);
	else if((ses->slowstatus > ses->slowsent) && !ses->slow){ //only write when no slow in queue, chitchat, 1/21/2000
		ses->slowsent ++;
		write_line_mud1(line, ses);
	}
	else add_queue_slow(line, ses);
}


void write_line_mud(const char* line, struct session* ses)
{
	if (!ses->pausestatus) write_line_mud2(line, ses);
	else {
		struct timeval now;
		long tsec, tusec;
		gettimeofday(&now, 0);
		tusec = ses->lastwrite.tv_usec + ses->writepause.tv_usec;
		tsec = ses->lastwrite.tv_sec + ses->writepause.tv_sec +
		tusec / 1000000;
		tusec %= 1000000;
		if (now.tv_sec > tsec) write_line_mud2(line, ses);
		else if (now.tv_sec == tsec && now.tv_usec >=tusec)
			write_line_mud2(line, ses);
		else {
			struct timeval nextwrite;
			nextwrite.tv_sec = tsec; nextwrite.tv_usec = tusec;
			add_queue(ses, EVENT_WRITE, &nextwrite, line);
			ses->lastwrite = nextwrite;
		}
	}
}

/*******************************************************************/
/* read at most BUFFER_SIZE chars from mud - parse protocol stuff */
/*******************************************************************/

int read_buffer_mud(char *buffer, struct session* ses)
{
	int i, didget;
	unsigned char tmpbuf[BUFFER_SIZE], *cpsource, *cpdest, *ptr;
	
	/* if(!ses->socket) return 0; /*dummy session, moved to rl.c */
#ifdef _WINDOWS
	didget = recv(ses->socket, (char *)tmpbuf, SOCK_BUFSIZ, 0);
#else
	didget = read(ses->socket, (char *)tmpbuf, SOCK_BUFSIZ);
#endif
	ses->old_more_coming = ses->more_coming;
	ses->more_coming = (didget == SOCK_BUFSIZ ? 1 : 0);
	
	if(didget < 0) 
		return(0); /*syserr("read from socket"); we do this here instead - dunno quite
	why, but i got some mysterious connection read by peer on some hps */
	
	if(!ses->more_coming && ses->slowstatus) {
		ses->slowsent = 0;
		check_queue_slow(ses); /*new: reset slow counts, chitchat, 1/11/2000 */
	}

	if(didget==0)
		return(0);
	else {
		if(ses->lograw==1) {
			lograw(ses, (char *)tmpbuf, didget); //for debug :)
		}
		cpsource = tmpbuf;
		cpdest = (unsigned char *)buffer;
		i = didget;
		while(i > 0) {
			if(!*cpsource) { //some telnet will send byte 0, b4 we know how to handle this, ignore now, chitchat, 7/20/2000
				cpsource++;
				i--;
			}
			else if(*cpsource == IAC) {
				if(cpsource[1]<=GOAHEAD && cpsource[1]>=NOP) {
					i -= 2;
					cpsource += 2;
				}
				else if(cpsource[1]==SB){
					if( (ptr=(unsigned char*)strchr((char*)cpsource+1, IAC)) && ptr[1]==SE) {
						*ptr = 0;
						do_telnet_sbse(cpsource+2, ses);
						i -= (ptr+2-cpsource);
						cpsource = ptr+2;
					}
					else {
						i -= 2;
						cpsource += 2;
					}
				}
				else {
					do_telnet_protocol(cpsource[1], cpsource[2], ses);
					i -= 3;
					cpsource += 3;
				}
			}
			else {
				*cpdest++ = *cpsource++;
				i--;
			}
		}
	}
	*cpdest = '\0';
	return(didget);
}


/*****************************************************************/
/* respond according to the telnet protocol - weeeeelllllll..... */
/*****************************************************************/

void do_telnet_protocol(unsigned char dat1, unsigned char dat2, struct session* ses)
{
	char temp[BUFFER_SIZE];
	switch(dat1){
		case TEL_DO:
			if(skiptelnetseq & 0x02) break;
			if(dat2== TERMTYPE && toggle_raw)
// will send termtype right after password for mud, while negotiate termtype if toggle_raw
// in most case this is unix port 23
				send_telnet(TEL_WILL, dat2, ses);
			else
				send_telnet(TEL_WONT, dat2, ses); //a hack, chitchat 1/31/2000
			break;

		case TEL_DONT:
			switch(dat2){
			case ECHO:
			default:
				break;
			}
			break;
		// control passwd echoing: MudOS will NOT echo in any case, but for passwd
		// MudOS cheats client that it WILL echo, thus client won't echo, chitchat, 1/31/2000
		case TEL_WILL:
			switch(dat2){
				case ECHO:
				if(skiptelnetseq & 0x01) break;
					ses->remote_echo = 1;
#ifndef _WINDOWS
					readline_echoing_p = 0; // this tells readline not to echo
#else
					hidePasswd(1);
#endif
					break;
			case TIMING:
				if(ses && ses->ping2){
					sprintf(temp, "#TELNET PSEUDO PING RESPONSE: %lu ms", millisec_started()-ses->ping2);
					ses->ping2 = 0;
					tintin_puts(temp, ses);
				}
				break;
			default:
				send_telnet(TEL_DONT, dat2, ses);
				break;
			}
			break;
		case TEL_WONT:
			switch(dat2){
			case ECHO:
				if(skiptelnetseq & 0x01) break;
				ses->remote_echo = 0;
#ifndef _WINDOWS
				readline_echoing_p = 1; // this tells readline to echo again
#else
				hidePasswd(0);
#endif
				ses->decrypt++; //forbidden use of #decrypt command after login
				if(skiptelnetseq & 0x02) break;
				send_termtype(ses); //this is the right place to send termtype to MudOS, chitchat, 2/1/2000
				break;
			default:
				break;
			}
			break;
		default:
			break;
	}
}

/**********************************************/
/* handle bytes between IAC SB and IAC SE  ****/
/**********************************************/
void do_telnet_sbse(unsigned char* sbse, struct session* ses)
{
	switch(*sbse){
		case TERMTYPE:
			if(sbse[1]==TEL_SEND)
				send_termtype(ses);
			break;
		default:
			break;
	}
}


/*******************************************************************/
/* send some telnet response, simplest to make regular telnet work */
/* this mode will not get any response from unix host when using ls, echo ...
 * why? chitchat, 1/31/2000  */
/*******************************************************************/
void send_telnet(unsigned char dat1, unsigned char dat2, struct session* ses)
{
	unsigned char buf[4];
	buf[0] = IAC;
	buf[1] = dat1;
	buf[2] = dat2;
	buf[3] = 0;
	write_raw_mud((char *)buf, 3, ses); //for dat2==0, also send 3 bytes (mud specific or general rule?)!
}

void send_termtype(struct session* ses)
{
	unsigned char buff[BUFFER_SIZE];
	int l=0;
	buff[l++] = IAC;
	buff[l++] = SB;
	buff[l++] = (unsigned char)TERMTYPE;
	buff[l++] = TEL_IS;
	version_command((char *)buff+l);
	buff[(l=4+strlen((char *)buff+4))] = IAC;
//	strcpy(buff+l, "vt100");
//	buff[l=9] = IAC;
	buff[++l] = SE;
	buff[++l] = 0;
	write_raw_mud((char*)buff, l, ses);
}

