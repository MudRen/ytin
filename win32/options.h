#if !defined(AFX_OPTIONS_H__1CF41FC2_AF03_11D3_8359_00105A9F5B94__INCLUDED_)
#define AFX_OPTIONS_H__1CF41FC2_AF03_11D3_8359_00105A9F5B94__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// options.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// COptionDlg dialog

class COptionDlg : public CDialog
{
// Construction
public:
	COptionDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(COptionDlg)
	enum { IDD = IDD_VIEW_OPTIONS };
	int		m_n_buff_size;
	int		m_n_W80;
	int		m_n_extra_x;
	int		m_n_extra_y;
	int		m_n_font_weight;
	BOOL	m_bAnsi;
	int		m_n_TABSIZE;
	CString	m_sz_wintitle_boss;
	CString	m_sz_CRLF;
	int		m_bBoldAsHigh;
	CString	m_szPerl;
	BOOL	m_bKeepLastline;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COptionDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(COptionDlg)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OPTIONS_H__1CF41FC2_AF03_11D3_8359_00105A9F5B94__INCLUDED_)
