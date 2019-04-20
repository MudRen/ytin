// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__F5246188_A5A7_11D3_8359_00105A9F5B94__INCLUDED_)
#define AFX_MAINFRM_H__F5246188_A5A7_11D3_8359_00105A9F5B94__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "tinView.h"

class CMainFrame : public CFrameWnd
{
	
public:
	CMainFrame();
protected: 
	DECLARE_DYNAMIC(CMainFrame)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
	void CMainFrame::EnableWSNotify();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

public:
	void Passwd(int passwd);
	CComboBox		m_comboPrompt;
	CComboBox   m_comboInput;
	BOOL				b_combo;
	BOOL        m_bKeepLastline;
	CTinView    m_wndView;
	CStatusBar  m_wndStatusBar;
protected:  // control bar embedded members
	CToolBar    m_wndToolBar;
	CDialogBar	m_wndInputBar;
	BOOL        m_bToolTips;
//data
	int NotifyCount;		// number of notifies until we idle
	CPtrList NotifySockList;	// socks to re-enable on idle

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSetFocus(CWnd *pOldWnd);
	afx_msg void OnOk();
	afx_msg void OnClose();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnTabComplete();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LONG OnAsyncSelect(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__F5246188_A5A7_11D3_8359_00105A9F5B94__INCLUDED_)
