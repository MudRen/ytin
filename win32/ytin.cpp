// ytin.cpp : Defines the class behaviors for the application.
//
#include "stdafx.h"

//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>

#include "ytin.h"

#include <fcntl.h>
#include <io.h>

#include "MainFrm.h"
#include "PasswdDlg.h"
#include "AnsiDlg.h"
#include "U2AConv.h"
#include "GotoURL.h"

#include "macro.proto"
#include "main.proto"
#include "misc.proto"
#include "net.proto"
#include "rl.proto"
#include "session.proto"
#include "uuencode.proto"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CYtinApp

BEGIN_MESSAGE_MAP(CYtinApp, CWinApp)
	//{{AFX_MSG_MAP(CYtinApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_FILE_BOSS, OnFileBoss)
	ON_COMMAND(ID_FILE_BOSS2, OnFileBoss2)
	ON_UPDATE_COMMAND_UI(ID_FILE_BOSS2, OnUpdateFileBoss2)
	ON_COMMAND(ID_APP_NEW_WINDOW, OnAppNewWindow)
	ON_COMMAND(ID_FILE_CONNECT, OnFileConnect)
	ON_COMMAND(ID_VIEW_ANSICOLOR, OnViewAnsicolor)
	ON_COMMAND(ID_FILE_TOGGLERAW, OnFileToggleraw)
	ON_UPDATE_COMMAND_UI(ID_FILE_TOGGLERAW, OnUpdateFileToggleraw)
	ON_COMMAND(ID_HELP_ONLINE, OnHelpOnline)
	ON_COMMAND(ID_HELP_HOMEPAGE, OnHelpHomepage)
	ON_COMMAND(ID_HELP_VERSIONUPDATE, OnHelpVersionupdate)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CYtinApp construction
BOOL showall, hide;
CMainFrame* pFrame;
TCHAR szPerl[MAX_PATH];

CYtinApp::CYtinApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CYtinApp object

CYtinApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CYtinApp initialization

BOOL CYtinApp::InitInstance()
{
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);


	CString cstr;
	char *passwd;

#ifdef _AFXDLL
	Enable3dControls();   // Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic(); // Call this when linking to MFC statically
#endif
	
	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("fchen0000"));
	
	showall = 0;
	cstr = AfxGetApp()->GetProfileString(_T("windows"), _T("passwd"), _T(""));
	if(cstr.GetLength()){
		USES_CONVERSION;
		passwd = decrypt(T2A(cstr.GetBuffer(20)));
		cstr.ReleaseBuffer();
		::EnumWindows(MyEnumWindowProc, 2); //detect if there is hidden yTin
		if(showall) { //yes, yTin is hidden
			CPasswdDlg cpd;
			USES_CONVERSION;
			if(cpd.DoModal() != IDOK || strcmp(passwd, T2A(m_szPasswd.GetBuffer(20)))){
				free(passwd);
				return 1;
			}
		}
		AfxGetApp()->WriteProfileString(_T("windows"), _T("passwd"), _T(""));
		free(passwd);
	}

	showall = 0;
	hide = GetProfileInt(_T("windows"), _T("hide"), 0);
	::EnumWindows(MyEnumWindowProc, 1);
	WriteProfileInt(_T("windows"), _T("hide"), 0);
	if(showall) return TRUE;
	
	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

	WNDCLASS wndcls;
	memset(&wndcls, 0, sizeof(WNDCLASS));   // start with NULL defaults

	wndcls.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
	wndcls.lpfnWndProc = ::DefWindowProc;
	wndcls.hInstance = AfxGetInstanceHandle();
	wndcls.hIcon = LoadIcon(IDR_MAINFRAME); // or load a different
                                                  // icon
	wndcls.hCursor = LoadCursor( IDC_ARROW );
	wndcls.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
	wndcls.lpszMenuName = NULL;

	// Specify your own class name for using FindWindow later
	wndcls.lpszClassName = _T("yTin");

	// Register the new class and exit if it fails
	if(!AfxRegisterClass(&wndcls))
	{
		TRACE(_T("Class Registration Failed\n"));
		return FALSE;
	}


	InitParameter();
	
	// To create the main window, this code creates a new frame window
	// object and then sets it as the application's main window object.
	
	pFrame = new CMainFrame;
	m_pMainWnd = pFrame;
	
	// create and load the frame with its resources
	
	((CMainFrame *)m_pMainWnd)->LoadFrame(IDR_MAINFRAME,
		WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE,
		NULL,
		NULL);
	
	// The one and only window has been initialized, so show and update it.
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();

	::GetCurrentDirectory(256, m_szStartDir);

#ifdef PERLDLL
	tintin_puts("#Happy Perl'ing with yTin! This version implicitly loads perl56.dll.", NULL);
#else
	int fd;
	cstr = GetProfileString(_T("Options"), _T("PerlDll"), _T("c:\\perl\\bin\\perl56.dll"));
	_tcscpy(szPerl, cstr);
	USES_CONVERSION;
	if ((fd = _open(T2A(szPerl), O_RDONLY)) > 0){ // Check if it exists
		_close(fd);
		cstr.Format(_T("#Found %s! Happy Perl'ing with yTin!\n"), szPerl);
	}
	else {
		cstr.Format(_T("#%s does not exit, power user shall get one!\n"), szPerl);
		*szPerl = 0;
	}
	showstr(cstr.GetBuffer(80));
#endif

	tininit(T2A(m_lpCmdLine));

	return TRUE;
}


///////////////////////////////////////////////////////////////
void setTitle(char* title)
{
	USES_CONVERSION;
	AfxGetMainWnd()->SetWindowText(title ? A2T(title) : _T("yTin"));
}

void boss_command(const char* arg, struct session* ses)
{
	char* str;
	AfxGetApp()->WriteProfileInt(_T("windows"), _T("hide"), 1);
	if(arg[0]=='0' && arg[1]==0){
		pFrame->ShowWindow(SW_HIDE);
		return;
	}
	::EnumWindows(MyEnumWindowProc, 0);
	if(*arg){
		str = encrypt((char *)arg);
		USES_CONVERSION;
		AfxGetApp()->WriteProfileString(_T("windows"), _T("passwd"), A2T(str));
		free(str);
	}
	else AfxGetApp()->WriteProfileString(_T("windows"), _T("passwd"), _T(""));
}
void CYtinApp::OnFileBoss() 
{
	// TODO: Add your command handler code here
	keycapture = 0;
	if(bBoss) {
		boss_command("", NULL);
	}
}

BOOL CALLBACK MyEnumWindowProc(HWND hwnd, LPARAM lParam)
{
	TCHAR buff[120], buff2[120];
	CString cs = AfxGetApp()->GetProfileString(_T("Options"), _T("wintitles_boss"), _T("ytin/Telnet*"));
	TCHAR *pt, *pt0;
	int change=0;
	int l;
	int title = 0, cls = 0;
	WINDOWPLACEMENT wndpl;
	wndpl.length = sizeof(WINDOWPLACEMENT);
	
	pt0 = cs.GetBuffer(40);
	if((title=::GetWindowText(hwnd, buff, 119))){
		cls=::GetClassName(hwnd, buff2, 119);
		if(cls && _tcscmp(buff2, _T("yTin"))==0){
			if(lParam==1 && hide){
				showall = 1;
				::ShowWindow(hwnd, SW_SHOW);
				return 1;
			}
			if(lParam==2) {showall=1; return 1;} //detect if there is hidden yTin
			change = 1;
		}
		else if(lParam<2 && title){
			do {
				pt = _tcsrchr(pt0, '/');
				if(pt==NULL){
					pt = pt0; l = _tcslen(pt)-1;
					if(l>=0 && pt[l]=='*'){
						pt[l]=0;
						if(_tcsstr(buff, pt)-buff==0){
							change = 1;
						}
					}
					else if( _tcscmp(buff, pt)==0 ) {
						change = 1;
					}
					break;
				}
				else {
					*pt=0; pt++; l=_tcslen(pt)-1;
					if(l>=0 && pt[l]=='*'){
						pt[l]=0;
						if(_tcsstr(buff, pt)-buff==0){
							change = 1;
							break;
						}
					}
					else if( _tcscmp(buff, pt)==0 ) {
						change = 1;
						break;
					}
				}
			} while(1);
		}
	}
	cs.ReleaseBuffer();
	if(change) {
		::ShowWindow(hwnd, lParam ? SW_SHOWNA : SW_HIDE);
	}
	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();
	
	// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA
	
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
	
	// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CYtinApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CYtinApp message handlers
void CYtinApp::InitParameter()
{
	CString cs;
	
	checkInternet = GetProfileInt(_T("Options"), _T("checkInternet"), 0);
	backcolor = RGB(0, 0, 0);
	cs.Format(_T("%ld"), backcolor);
	
	if(_stscanf(GetProfileString(_T("windows"), _T("backcolor"), cs), _T("%ld"), &backcolor) != 1)
		backcolor = RGB(0,0,0);
	
	textcolor = RGB(192, 192, 192);
	cs.Format(_T("%ld"), textcolor);
	
	if(_stscanf(GetProfileString(_T("windows"), _T("textcolor"), cs), _T("%ld"), &textcolor) != 1)
		textcolor = RGB(192, 192, 192);
	
	bBoss = GetProfileInt(_T("Options"), _T("bosskey"), 1);
	int f_weight;
	if( (f_weight=GetProfileInt(_T("Options"), _T("font_weight"), 400)) <=100)
		f_weight = 400;
	if(GetProfileString(_T("font"), _T("facename")) == _T(""))
		hfont = (HFONT)GetStockObject(ANSI_FIXED_FONT);
	else
	{
		hfont = CreateFont(GetProfileInt(_T("font"), _T("size"), 10), 0, 0, 0,
			GetProfileInt(_T("font"), _T("weight"), f_weight),
			GetProfileInt(_T("font"), _T("italic"), 0),
			GetProfileInt(_T("font"), _T("underline"), 0),
			GetProfileInt(_T("font"), _T("strikeout"), 0),
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, 
			CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
			FF_DONTCARE|DEFAULT_PITCH,
			GetProfileString(_T("font"), _T("facename"), _T("Arial")) );
		
		if(hfont == NULL)
			hfont = (HFONT)GetStockObject(ANSI_FIXED_FONT);
	}
}

BOOL CYtinApp::OnIdle(LONG lCount) 
{
	if (lCount == 0)
	{
		((CMainFrame *)AfxGetMainWnd())->EnableWSNotify();
		return 1;
	}
	else
		return(CWinApp::OnIdle(lCount-1));
}


void CYtinApp::OnFileBoss2() 
{
	// TODO: Add your command handler code here
	bBoss = !bBoss;
	WriteProfileInt(_T("Options"), _T("bosskey"), bBoss);
}

void CYtinApp::OnUpdateFileBoss2(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	pCmdUI->SetCheck(bBoss);
}

void CYtinApp::OnAppNewWindow() 
{
	// TODO: Add your command handler code here
	STARTUPINFO stInfo;
	PROCESS_INFORMATION pInfo;
	GetStartupInfo(&stInfo);
	CreateProcess(NULL, GetCommandLine(), NULL, NULL, 0,
		CREATE_NO_WINDOW | CREATE_DEFAULT_ERROR_MODE | NORMAL_PRIORITY_CLASS,
		NULL,
		m_szStartDir,
		&stInfo,
		&pInfo
		);
}
/*
BOOL CYtinApp::PreTranslateMessage(MSG* pMsg) 
{
	// TODO: Add your specialized code here and/or call the base class
	char buf[BUFFER_SIZE];
	struct keytype key;

	if(toggle_raw && activesession && pMsg->message == WM_CHAR){
		buf[0] = pMsg->wParam;
		write_raw_mud(buf, 1, activesession);
		return 1;
	}
	//here we only take KEYDOWN messages, and only take bit 16-24 of lParam (OEM scan code plus ALT bit)
	if (macro
			&& (pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN)
			&& !(pMsg->wParam>=0x10 && pMsg->wParam<=0x14) ){ //skip pure shift/alt/ctrl, caps lock, pause
		key.wParam = pMsg->wParam;
		key.lParam = HIWORD(pMsg->lParam) & 0x01FF;
		if(keycapture){
			if(key.wParam==27) {
				macro_command("0", NULL);
			}
			else {
				sprintf(buf, "{%x, %x}\n", key.wParam, key.lParam);
				showstr(buf);
			}
			return 1;
		}
		else {
			if(check_macros(&key, activesession))
				return 1;
		}
	}
	
	return CWinApp::PreTranslateMessage(pMsg);
}
*/
/////////////////////////////////////////////////////////////////////////////
// CDlgConnect dialog


CDlgConnect::CDlgConnect(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgConnect::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgConnect)
	m_szIP = _T("");
	m_szSessionName = _T("");
	//}}AFX_DATA_INIT
}


void CDlgConnect::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgConnect)
	DDX_Text(pDX, IDC_EDIT_IP, m_szIP);
	DDX_Text(pDX, IDC_EDIT_SESSION, m_szSessionName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgConnect, CDialog)
	//{{AFX_MSG_MAP(CDlgConnect)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgConnect message handlers

void CDlgConnect::OnOK() 
{
	// TODO: Add extra validation here
	
	UpdateData(true);

	if(!m_szSessionName.GetLength()){
		MessageBox(_T("Please enter sth for session name!"));
		GetDlgItem(IDC_EDIT_SESSION)->SetFocus();
		return;
	}
	CDialog::OnOK();
	CString cstr = _T("{") + m_szSessionName + _T("} {") + m_szIP + _T("}");
	USES_CONVERSION;
	session_command(T2A(cstr.GetBuffer(80)), activesession);
}

void CYtinApp::OnFileConnect() 
{
	// TODO: Add your command handler code here
	CDlgConnect dlg1;
	dlg1.DoModal();
}

void CYtinApp::OnViewAnsicolor() 
{
	// TODO: Add your command handler code here
	CAnsiDlg dlg;
	dlg.DoModal();
}

void CYtinApp::OnFileToggleraw() 
{
	// TODO: Add your command handler code here
	toggle_raw = !toggle_raw;
}

void CYtinApp::OnUpdateFileToggleraw(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	pCmdUI->SetCheck(toggle_raw);
}


void CYtinApp::OnHelpOnline() 
{
	// TODO: Add your command handler code here
	GotoURL(_T("http://ytin.sourceforge.net/cgi-bin/help.pl/ytin"), SW_MAXIMIZE);
}

void CYtinApp::OnHelpHomepage() 
{
	// TODO: Add your command handler code here
	GotoURL(_T("http://ytin.sourceforge.net"), SW_MAXIMIZE);
}

void CYtinApp::OnHelpVersionupdate() 
{
	// TODO: Add your command handler code here
	char version[BUFFER_SIZE], *pt;
	version_command(version); //strlen("ytin \b") = 6
	if( (pt=strchr(version+6, ' ')) != NULL)
		*pt = 0; //e.g., ytinxx1.80f2 (Ansi), strip from space
	CString str = _T("http://ytin.sourceforge.net/cgi-bin/version.pl/ytin?");
  str += (version+6);
	GotoURL(str, SW_MAXIMIZE);
}

