// ytin.h : main header file for the YTIN application
//

#if !defined(AFX_YTIN_H__F5246184_A5A7_11D3_8359_00105A9F5B94__INCLUDED_)
#define AFX_YTIN_H__F5246184_A5A7_11D3_8359_00105A9F5B94__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resrc1.h"         // app studio generated
#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CYtinApp:
// See ytin.cpp for the implementation of this class
//
BOOL CALLBACK MyEnumWindowProc(HWND hwnd, LPARAM lParam);

class CYtinApp : public CWinApp
{
public:
	CYtinApp();
	COLORREF backcolor, textcolor;
	HFONT hfont;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CYtinApp)
	public:
	virtual BOOL InitInstance();
	virtual BOOL OnIdle(LONG lCount);
	//}}AFX_VIRTUAL

// Implementation

public:
	BOOL bBoss;
	TCHAR m_szStartDir[256];
	CString m_szClass;
	CString m_szPasswd;
	void InitParameter();
	//{{AFX_MSG(CYtinApp)
	afx_msg void OnAppAbout();
	afx_msg void OnFileBoss();
	afx_msg void OnFileBoss2();
	afx_msg void OnUpdateFileBoss2(CCmdUI* pCmdUI);
	afx_msg void OnAppNewWindow();
	afx_msg void OnFileConnect();
	afx_msg void OnViewAnsicolor();
	afx_msg void OnFileToggleraw();
	afx_msg void OnUpdateFileToggleraw(CCmdUI* pCmdUI);
	afx_msg void OnHelpOnline();
	afx_msg void OnHelpHomepage();
	afx_msg void OnHelpVersionupdate();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CDlgConnect dialog

class CDlgConnect : public CDialog
{
// Construction
public:
	CDlgConnect(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgConnect)
	enum { IDD = IDD_CONNECT };
	CString	m_szIP;
	CString	m_szSessionName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgConnect)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgConnect)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_YTIN_H__F5246184_A5A7_11D3_8359_00105A9F5B94__INCLUDED_)
