/***********************************************************************
*  based on codes from Stuart Patterson<spatters@smog.ess.harris.com> *
*  modified by chitchat, 7/5/2001 																		*
/***********************************************************************/

#include "stdAfx.h"
#include "gotourl.h"

TCHAR browserPath[MAX_PATH];

long GetRegKey( HKEY key, LPTSTR subkey, LPTSTR retdata )
{
	long retval;
	HKEY hkey;
	
#if WIN32
	retval = RegOpenKeyEx(key, subkey, 0, KEY_QUERY_VALUE, &hkey);
#else
	retval = RegOpenKey(key, subkey, &hkey);
#endif
	
	if(retval == ERROR_SUCCESS)
	{
		long datasize = MAX_PATH;
		TCHAR data[MAX_PATH];
		
		RegQueryValue(hkey,NULL,data,&datasize);
		
		lstrcpy(retdata,data);
		RegCloseKey(hkey);
	}
	
	return retval;
}

BOOL GotoURL(LPCTSTR url, int showcmd)
{
	TCHAR key[MAX_PATH + MAX_PATH];
	BOOL retflag = FALSE;
	
	/* if the ShellExecute() fails					*/
	if(*browserPath!=0 || (long)ShellExecute(NULL,_T(""),url,NULL,NULL,showcmd) <= 32)
	{
		/* get the .htm regkey and lookup the program */
		if(*browserPath==0){
			if(GetRegKey(HKEY_CLASSES_ROOT,_T(".htm"), key) == ERROR_SUCCESS){
				lstrcat(key,_T("\\shell\\open\\command"));
				if(GetRegKey(HKEY_CLASSES_ROOT,key,key) == ERROR_SUCCESS){
					TCHAR *pos;
					pos = _tcsstr(key, _T("\"%1\""));
					if(pos == NULL) 		/* if no quotes */
					{
						/* now check for %1, without the quotes */
						pos = _tcsstr(key, _T("%1"));
						if(pos == NULL) /* if no parameter */
							pos = key+lstrlen(key)-1;
						else
							*pos = '\0';		/* remove the parameter */
					}
					else
						*pos = '\0';				/* remove the parameter */
					
					_tcscpy(browserPath, key);
				}
			}
		}
		else
			_tcscpy(key, browserPath);
		
		if(*browserPath != 0){
			lstrcat(key, _T(" "));
			lstrcat(key, url);
			STARTUPINFO stInfo;
			PROCESS_INFORMATION pInfo;
			GetStartupInfo(&stInfo);
			CreateProcess(NULL, key, NULL, NULL, 0,
				CREATE_NO_WINDOW | CREATE_DEFAULT_ERROR_MODE | NORMAL_PRIORITY_CLASS,
				NULL,
				NULL,
				&stInfo,
				&pInfo
			);
//			if(WinExec(key, showcmd) > 31)
				retflag = TRUE;
		}
		
	}
	else
		retflag = TRUE;
	
	return retflag;
}

