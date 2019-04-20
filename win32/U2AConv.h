#ifndef __U2ACONV_H__
#define __U2ACONV_H__

#include <atlconv.h>

#undef USES_CONVERSION
#ifndef _DEBUG
	#define USES_CONVERSION int _convert; _convert; UINT _acp = 936; _acp; LPCWSTR _lpw; _lpw; LPCSTR _lpa; _lpa
#else
	#define USES_CONVERSION int _convert = 0; _convert; UINT _acp = 936; _acp; LPCWSTR _lpw = NULL; _lpw; LPCSTR _lpa = NULL; _lpa
#endif

#endif // _U2ACONV_IMPL
