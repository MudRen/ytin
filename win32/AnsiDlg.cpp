// AnsiDlg.cpp : implementation file
//

#include "stdafx.h"

#include "ytin.h"
#include "MainFrm.h"
#include "tinView.h"
#include "AnsiDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAnsiDlg dialog

CAnsiDlg::CAnsiDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAnsiDlg::IDD, pParent)
{
	int i, j, k;
	CString cstr;
	CWinApp *pApp = AfxGetApp();
	//{{AFX_DATA_INIT(CAnsiDlg)
	//primary color based on ansi
	m_dim0 = pApp->GetProfileInt(_T("AnsiColors"), _T("dim0"), dim[0]);
	m_dim1 = pApp->GetProfileInt(_T("AnsiColors"), _T("dim1"), dim[1]);
	//complimentary colors
	m_neg0 = pApp->GetProfileInt(_T("AnsiColors"), _T("neg0"), 0);
	m_neg1 = pApp->GetProfileInt(_T("AnsiColors"), _T("neg1"), 0);
	m_bProp0 = pApp->GetProfileInt(_T("AnsiColors"), _T("prop0"), 1);
	m_bProp1 = pApp->GetProfileInt(_T("AnsiColors"), _T("prop1"), 1);
	//}}AFX_DATA_INIT
	for(i=0; i<8; i++)
		for(j=0; j<2; j++)
			for(k=0; k<3; k++){
				cstr.Format(_T("%d%d%d"), j, i, k);
				rgb[j*8+i][k] = pApp->GetProfileInt(_T("AnsiColors"), cstr, COLOR[i][k]*dim[j]);
			}
}


void CAnsiDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAnsiDlg)
	DDX_Text(pDX, IDC_EDIT0, m_dim0);
	DDV_MinMaxInt(pDX, m_dim0, 0, 255);
	DDX_Text(pDX, IDC_EDIT1, m_dim1);
	DDV_MinMaxInt(pDX, m_dim1, 0, 255);
	DDX_Check(pDX, IDC_CHECK0, m_bProp0);
	DDX_Check(pDX, IDC_CHECK1, m_bProp1);
	DDX_Text(pDX, IDC_EDIT2, m_neg0);
	DDV_MinMaxInt(pDX, m_neg0, 0, 255);
	DDX_Text(pDX, IDC_EDIT3, m_neg1);
	DDV_MinMaxInt(pDX, m_neg1, 0, 255);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAnsiDlg, CDialog)
	//{{AFX_MSG_MAP(CAnsiDlg)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_TEST, OnTest)
	//}}AFX_MSG_MAP
	ON_CONTROL_RANGE(BN_CLICKED, IDC_COLOR0, IDC_COLOR15, OnColor)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAnsiDlg message handlers

void CAnsiDlg::OnColor(UINT nID) 
{
	// TODO: Add your control notification handler code here

	int i = nID - IDC_COLOR0;
	ASSERT( i >= 0 && i < 16 );
  if(i<8) m_bProp0 = 0;
  else m_bProp1 = 0;
	UpdateData(false);
	Invalidate();

	CColorDialog ccd(RGB(rgb[i][0], rgb[i][1], rgb[i][2]));
	COLORREF cr;
	if(ccd.DoModal() == IDOK)
	{
		cr = ccd.GetColor();
		rgb[i][0] = GetRValue(cr);
		rgb[i][1] = GetGValue(cr);
		rgb[i][2] = GetBValue(cr);
		Invalidate();
	}
}

void CAnsiDlg::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	CRect rect;
	CBrush brush[16];
	COLORREF cr;
	
	// TODO: Add your message handler code here
	for(int i=0;i<16; i++){
		GetDlgItem(IDC_COLOR0+i)->GetWindowRect(&rect);
		ScreenToClient(&rect);
		if(m_bProp0 && i<8)
			cr = RGB(m_dim0*COLOR[i][0]+m_neg0*(1-COLOR[i][0]), m_dim0*COLOR[i][1]+m_neg0*(1-COLOR[i][1]), m_dim0*COLOR[i][2]+m_neg0*(1-COLOR[i][2]));
		else if(m_bProp1 && i>=8)
			cr = RGB(m_dim1*COLOR[i-8][0]+m_neg1*(1-COLOR[i-8][0]), m_dim1*COLOR[i-8][1]+m_neg1*(1-COLOR[i-8][1]), m_dim1*COLOR[i-8][2]+m_neg1*(1-COLOR[i-8][2]));
		else cr = RGB(rgb[i][0], rgb[i][1], rgb[i][2]);
		brush[i].CreateSolidBrush(cr);
		dc.FillRect(&rect, &brush[i]);
	}
	// Do not call CDialog::OnPaint() for painting messages
}

void CAnsiDlg::OnTest() 
{
	// TODO: Add your control notification handler code here
	if(!UpdateData()) return;
	Invalidate();
}

void CAnsiDlg::OnOK() 
{
	// TODO: Add extra validation here
	CDialog::OnOK();

	int i, j, k;
	CString cstr;
	CWinApp *pApp = AfxGetApp();
	pApp->WriteProfileInt(_T("AnsiColors"), _T("prop0"), m_bProp0);
	pApp->WriteProfileInt(_T("AnsiColors"), _T("prop1"), m_bProp1);
	pApp->WriteProfileInt(_T("AnsiColors"), _T("dim0"), m_dim0);
	pApp->WriteProfileInt(_T("AnsiColors"), _T("dim1"), m_dim1);
	pApp->WriteProfileInt(_T("AnsiColors"), _T("neg0"), m_neg0);
	pApp->WriteProfileInt(_T("AnsiColors"), _T("neg1"), m_neg1);
	for(i=0; i<8; i++)
		for(j=0; j<2; j++)
			for(k=0; k<3; k++){
				cstr.Format(_T("%d%d%d"), j, i, k);
				pApp->WriteProfileInt(_T("AnsiColors"), cstr, rgb[j*8+i][k]);
			}
	CTinView *pView = &(((CMainFrame *)AfxGetMainWnd())->m_wndView);
	pView->InitColor();
	pView->Invalidate();
}
