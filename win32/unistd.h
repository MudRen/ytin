#ifndef __UNISTD_H__
#define __UNISTD_H__

#define popen(a,b) _popen((a), (b))
#define pclose(a) _pclose(a)
#define flushall() _flushall()

#include <io.h>
#define read(a,b,c) _read((a), (b), (c))
#define write(a,b,c) _write((a), (b), (c))
#define unlink(a) _unlink(a)

#include <direct.h>
#define getcwd(a,b) _getcwd((a), (b))
#define chdir(a) _chdir(a)
#define mkdir(a, b) _mkdir(a)

#include <mmsystem.h>
extern DWORD _my_tv_msec;
#define gettimeofday(a, b)\
	_my_tv_msec = timeGetTime();\
	((struct timeval *)(a))->tv_sec = (long)(_my_tv_msec/1000);\
	((struct timeval *)(a))->tv_usec = (long)((_my_tv_msec%1000)*1000)
#endif
