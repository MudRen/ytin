// PasswdDlg.cpp : implementation file
//

#include "stdafx.h"

#include "ytin.h"
#include "PasswdDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPasswdDlg dialog


CPasswdDlg::CPasswdDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPasswdDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPasswdDlg)
	m_szPasswd = _T("");
	//}}AFX_DATA_INIT
}


void CPasswdDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPasswdDlg)
	DDX_Text(pDX, IDC_PASSWD, m_szPasswd);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPasswdDlg, CDialog)
	//{{AFX_MSG_MAP(CPasswdDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPasswdDlg message handlers

void CPasswdDlg::OnOK() 
{
	// TODO: Add extra validation here	
	CDialog::OnOK();
	((CYtinApp *)AfxGetApp())->m_szPasswd = m_szPasswd;
}
