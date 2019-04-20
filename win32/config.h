/* config.h.  this config.h sits in ytin source dir to override tintin\config.h  */

/* Define if you have the <dirent.h> header file.  */
#define HAVE_DIRENT_H 1

/* Define if you have the <stdlib.h> header file.  */
#define HAVE_STDLIB_H 1

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <time.h> header file.  */
#define HAVE_TIME_H 1

/* Define if you have <unistd.h> header file, we have a customer-written unistd.h for WIN32 here */
#define HAVE_UNISTD_H 1

/*define this to include customer-written socks.h, which is actually winsock2.h*/
//well, include afxsock.h in stdafx.h
//#define SOCKS