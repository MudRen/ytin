#if !defined(AFX_ANSIDLG_H__EECCC3E0_283D_11D4_8359_00105A9F5B94__INCLUDED_)
#define AFX_ANSIDLG_H__EECCC3E0_283D_11D4_8359_00105A9F5B94__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AnsiDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAnsiDlg dialog

class CAnsiDlg : public CDialog
{
// Construction
public:
	CAnsiDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAnsiDlg)
	enum { IDD = IDD_ANSICOLOR };
	int		m_dim0;
	int		m_dim1;
	BOOL	m_bProp0;
	BOOL	m_bProp1;
	int		m_neg0;
	int		m_neg1;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAnsiDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	int rgb[16][3];

	// Generated message map functions
	//{{AFX_MSG(CAnsiDlg)
	afx_msg void OnPaint();
	afx_msg void OnTest();
	virtual void OnOK();
	//}}AFX_MSG
	void OnColor(UINT nID);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ANSIDLG_H__EECCC3E0_283D_11D4_8359_00105A9F5B94__INCLUDED_)
