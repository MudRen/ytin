// options.cpp : implementation file
//

#include "stdafx.h"

#include "ytin.h"
#include "mainfrm.h"
#include "options.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COptionDlg dialog


COptionDlg::COptionDlg(CWnd* pParent /*=NULL*/)
: CDialog(COptionDlg::IDD, pParent)
{
	CWinApp* pApp = AfxGetApp();
	//{{AFX_DATA_INIT(COptionDlg)
	m_n_buff_size = pApp->GetProfileInt(_T("Options"), _T("buff_size"), 1000);
	m_n_W80 = pApp->GetProfileInt(_T("Options"), _T("max_width"), 80);
	m_n_extra_x = pApp->GetProfileInt(_T("Options"), _T("h_spacing"), 0);
	m_n_extra_y = pApp->GetProfileInt(_T("Options"), _T("v_spacing"), 2);
	m_n_font_weight = pApp->GetProfileInt(_T("Options"), _T("font_weight"), 600);
	m_bAnsi = pApp->GetProfileInt(_T("Options"), _T("enable_ansi"), 1);
	m_bKeepLastline = pApp->GetProfileInt(_T("Options"), _T("KeepLastline"), 0);
	m_n_TABSIZE = pApp->GetProfileInt(_T("Options"), _T("TABSIZE"), 2);
	m_sz_wintitle_boss = pApp->GetProfileString(_T("Options"), _T("wintitles_boss"), _T("ytin/Telnet*"));
	m_sz_CRLF = pApp->GetProfileString(_T("Options"), _T("CR_LF"), _T("\\t"));
	m_bBoldAsHigh = pApp->GetProfileInt(_T("Options"), _T("BoldAsHigh"), 1);
	m_szPerl = pApp->GetProfileString(_T("Options"), _T("PerlDll"), _T("c:\\perl\\bin\\perl56.dll"));
	//}}AFX_DATA_INIT
}


void COptionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionDlg)
	DDX_Text(pDX, IDC_BUFF_SIZE, m_n_buff_size);
	DDV_MinMaxInt(pDX, m_n_buff_size, 100, 16000);
	DDX_Text(pDX, IDC_MAX_WIDTH, m_n_W80);
	DDV_MinMaxInt(pDX, m_n_W80, 10, 400);
	DDX_Text(pDX, IDC_EXTRA_X, m_n_extra_x);
	DDX_Text(pDX, IDC_EXTRA_Y, m_n_extra_y);
	DDX_Text(pDX, IDC_FONT_WEIGHT, m_n_font_weight);
	DDV_MinMaxInt(pDX, m_n_font_weight, 100, 900);
	DDX_Check(pDX, IDC_ENABLE_ANSI, m_bAnsi);
	DDX_Text(pDX, IDC_TABSIZE, m_n_TABSIZE);
	DDV_MinMaxInt(pDX, m_n_TABSIZE, 1, 80);
	DDX_Text(pDX, IDC_WINTITLE_BOSS, m_sz_wintitle_boss);
	DDX_Text(pDX, IDC_CR_LF, m_sz_CRLF);
	DDX_Radio(pDX, IDC_BOLD_AS_HIGH, m_bBoldAsHigh);
	DDX_Text(pDX, IDC_PERLDLL, m_szPerl);
	DDV_MaxChars(pDX, m_szPerl, 240);
	DDX_Check(pDX, IDC_KEEPLASTLINE, m_bKeepLastline);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COptionDlg, CDialog)
	//{{AFX_MSG_MAP(COptionDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COptionDlg message handlers

void COptionDlg::OnOK() 
{
	UpdateData(TRUE);
	
	CWinApp* pApp = AfxGetApp();
	
	pApp->WriteProfileInt(_T("Options"), _T("h_spacing"), m_n_extra_x);
	pApp->WriteProfileInt(_T("Options"), _T("v_spacing"), m_n_extra_y);
	pApp->WriteProfileInt(_T("Options"), _T("font_weight"), m_n_font_weight);
	pApp->WriteProfileInt(_T("Options"), _T("max_width"), m_n_W80);
	pApp->WriteProfileInt(_T("Options"), _T("buff_size"), m_n_buff_size);
	pApp->WriteProfileInt(_T("Options"), _T("enable_ansi"), m_bAnsi);
	pApp->WriteProfileInt(_T("Options"), _T("TABSIZE"), m_n_TABSIZE);
	pApp->WriteProfileInt(_T("Options"), _T("BoldAsHigh"), m_bBoldAsHigh);
	pApp->WriteProfileInt(_T("Options"), _T("KeepLastline"), m_bKeepLastline);
//	pApp->WriteProfileInt(_T("Options"), _T("checkInternet"), m_bCheckInternet);
	pApp->WriteProfileString(_T("Options"), _T("wintitles_boss"), m_sz_wintitle_boss);
	pApp->WriteProfileString(_T("Options"), _T("CR_LF"), m_sz_CRLF);
	pApp->WriteProfileString(_T("Options"), _T("PerlDll"), m_szPerl);
	_tcscpy(szPerl, m_szPerl);
	
//	checkInternet = m_bCheckInternet;

	((CMainFrame *)AfxGetMainWnd())->m_bKeepLastline = m_bKeepLastline;
	CComboBox *combo = & (((CMainFrame *)AfxGetMainWnd())->m_comboInput);
	int N = combo->GetCount();
	combo->InsertString(N, _T(""));
	combo->SetCurSel(N);

	((CMainFrame *)AfxGetMainWnd())->m_wndView.b_newfont = 1;
	((CMainFrame *)AfxGetMainWnd())->m_wndView.Invalidate();
	CDialog::OnOK();
}
