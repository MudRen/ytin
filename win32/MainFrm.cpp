// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"

#include "ytin.h"
#include "U2AConv.h"

#include "MainFrm.h"
//#include "imm.h" //need link imm32.lib

#include "macro.proto"
#include "main.proto"
#include "misc.proto"
#include "net.proto"
#include "parse.proto"
#include "rl.proto"
#include "rltab.proto"
#include "rltick.proto"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

int sent_quit_msg;
extern int am_purist;
/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_BN_CLICKED(IDOK, OnOk)
	ON_WM_CLOSE()
	ON_WM_TIMER()
	ON_COMMAND(ID_TAB_COMPLETE, OnTabComplete)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_USER+1, OnAsyncSelect)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_PROMPT,
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
	b_combo = FALSE;
	NotifyCount = 1;
	m_bKeepLastline = AfxGetApp()->GetProfileInt(_T("Options"), _T("KeepLastline"), 0);
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	UINT nID;
	UINT style;
	int width;

	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	// create a view to occupy the client area of the frame
	if (!m_wndView.Create(NULL, NULL, AFX_WS_DEFAULT_VIEW | WS_VSCROLL,
		CRect(0, 0, 0, 0), this, AFX_IDW_PANE_FIRST, NULL))
	{
		TRACE0("Failed to create view window\n");
		return -1;
	}
	
/****************************************************************************/
	m_bToolTips = (AfxGetApp()->GetProfileInt(_T("General"),_T("ToolTips"),1)!=0);
	
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT,
			WS_CHILD | WS_VISIBLE | CBRS_SIZE_DYNAMIC |
			CBRS_TOP | ((m_bToolTips)?(CBRS_TOOLTIPS | CBRS_FLYBY):0) )
		|| !m_wndToolBar.LoadToolBar(IDR_MAINFRAME)
	)
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}
/****************************************************************************/
	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	//make Pane 1 to use rest of the space
	m_wndStatusBar.GetPaneInfo(0, nID, style, width);
	m_wndStatusBar.SetPaneInfo(0, nID , style ^ SBPS_STRETCH, width/3);
	m_wndStatusBar.GetPaneInfo(1, nID, style, width);
	m_wndStatusBar.SetPaneInfo(1, nID , style | SBPS_STRETCH, width);
/****************************************************************************/	
	//here are the code for the inputBar
	const int nDropHeight = 200;
	if (!m_wndInputBar.Create(this, IDD_INPUTBAR,
		CBRS_BOTTOM|CBRS_TOOLTIPS|CBRS_FLYBY, IDD_INPUTBAR) )
	{
		TRACE0("Failed to create inputbar\n");
		return FALSE;       // fail to create
	}
	// Create the combo box
	CRect rect;
	GetClientRect(&rect);
	
	rect.top = 3;
	rect.bottom = rect.top + nDropHeight;
	if (!m_comboInput.Create(
		CBS_DROPDOWN | CBS_AUTOHSCROLL
		| WS_VSCROLL | WS_VISIBLE |WS_CHILD,
		rect, &m_wndInputBar, IDW_COMBOINPUT))
	{
		TRACE0("Failed to create combo-box\n");
		return FALSE;
	}
	else {
		b_combo=1;
		m_comboInput.SetFocus();
#ifdef CHINESE
		m_comboInput.SetLocale(0x0804);
		SetThreadLocale(0x0804);
//		UINT acp = GetACP();
#endif
	}
	
	SetTimer(TIMER_QUEUE, 10, NULL); //10 millisecond
	
	// TODO: Delete these three lines if you don't want the toolbar to
	//  be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
//	m_wndInputBar.EnableDocking(CBRS_ALIGN_ANY);

	EnableDocking(CBRS_ALIGN_ANY);

	DockControlBar(&m_wndToolBar, AFX_IDW_DOCKBAR_TOP);
//	DockControlBar(&m_wndInputBar, AFX_IDW_DOCKBAR_BOTTOM);

	LoadBarState(_T("General"));
	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
    	
	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	
	cs.x = AfxGetApp()->GetProfileInt(_T("Window"), _T("left"), 0);
	cs.y = AfxGetApp()->GetProfileInt(_T("Window"), _T("top"), 0);
	cs.cx = AfxGetApp()->GetProfileInt(_T("Window"), _T("right"), ::GetSystemMetrics(SM_CXSCREEN));
	cs.cy = AfxGetApp()->GetProfileInt(_T("Window"), _T("bottom"), ::GetSystemMetrics(SM_CYMAXIMIZED));
	
	cs.cx -= cs.x;
	cs.cy -= cs.y;
	
	cs.lpszName = _T("yTin");
	cs.lpszClass = _T("yTin");

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG
/////////////////////////////////////////////////////////////////////////////
// A C function to return the main window handle
HWND getmainwnd()
{
	return AfxGetMainWnd()->m_hWnd;
}

// A C function to quit the application
void closemainwindow()
{
	// SendMessage(..) will cause crash for release dll version (fixed from v1.82c6?)
	// while PostQuitMessage() will not clean memory allocated in m_wndView
	if(!sent_quit_msg){
		sent_quit_msg = 1;
		((CMainFrame *)AfxGetMainWnd())->SendMessage(WM_CLOSE); 
	}
//	AfxGetMainWnd()->DestroyWindow();
//	::PostQuitMessage(0);
//This is sth from a FAQ, but it causes exception too
//	ASSERT(AfxGetMainWnd() != NULL); 
//	AfxGetMainWnd()->PostMessage(WM_CLOSE); 
}

void setprompt(char* str)
{
//more works ...
	CString cstr = str;
	((CMainFrame *)AfxGetMainWnd())->m_wndStatusBar.SetPaneText(1, cstr);
}

// A C function to return a text explanation given a Winsock error number
void winsockerrmsg(int nError, char* pszError)
{
	sprintf(pszError, "WinSock error %d: ", nError);
	
	switch (nError)
	{
		case WSAEINTR:
			strcat(pszError, "Interrupted system call");
			break;
		case WSAEBADF:
			strcat(pszError, "Bad file number");
			break;
		case WSAEACCES:
			strcat(pszError, "Permission denied");
			break;
		case WSAEFAULT:
			strcat(pszError, "Bad address");
			break;
		case WSAEINVAL:
			strcat(pszError, "Invalid argument");
			break;
		case WSAEMFILE:
			strcat(pszError, "Too many open files");
			break;
		case WSAEWOULDBLOCK:
			strcat(pszError, "Operation would block");
			break;
		case WSAEINPROGRESS:
			strcat(pszError, "Operation now in progress");
			break;
		case WSAEALREADY:
			strcat(pszError, "Operation already in progress");
			break;
		case WSAENOTSOCK:
			strcat(pszError, "Socket operation on non-socket");
			break;
		case WSAEDESTADDRREQ:
			strcat(pszError, "Destination address required");
			break;
		case WSAEMSGSIZE:
			strcat(pszError, "Message too long");
			break;
		case WSAEPROTOTYPE:
			strcat(pszError, "Protocol wrong type for socket");
			break;
		case WSAENOPROTOOPT:
			strcat(pszError, "Protocol not available");
			break;
		case WSAEPROTONOSUPPORT:
			strcat(pszError, "Protocol not supported");
			break;
		case WSAESOCKTNOSUPPORT:
			strcat(pszError, "Socket type not supported");
			break;
		case WSAEOPNOTSUPP:
			strcat(pszError, "Operation not supported on socket");
			break;
		case WSAEPFNOSUPPORT:
			strcat(pszError, "Protocol family not supported");
			break;
		case WSAEAFNOSUPPORT:
			strcat(pszError, "Address family not supported by protocol family");
			break;
		case WSAEADDRINUSE:
			strcat(pszError, "Address already in use");
			break;
		case WSAEADDRNOTAVAIL:
			strcat(pszError, "Can't assign requested address");
			break;
		case WSAENETDOWN:
			strcat(pszError, "Network is down");
			break;
		case WSAENETUNREACH:
			strcat(pszError, "Network is unreachable");
			break;
		case WSAENETRESET:
			strcat(pszError, "Network dropped connection on reset");
			break;
		case WSAECONNABORTED:
			strcat(pszError, "Software caused connection abort");
			break;
		case WSAECONNRESET:
			strcat(pszError, "Connection reset by peer");
			break;
		case WSAENOBUFS:
			strcat(pszError, "No buffer space available");
			break;
		case WSAEISCONN:
			strcat(pszError, "Socket is already connected");
			break;
		case WSAENOTCONN:
			strcat(pszError, "Socket is not connected");
			break;
		case WSAESHUTDOWN:
			strcat(pszError, "Can't send after socket shutdown");
			break;
		case WSAETOOMANYREFS:
			strcat(pszError, "Too many references: can't splice");
			break;
		case WSAETIMEDOUT:
			strcat(pszError, "Connection timed out");
			break;
		case WSAECONNREFUSED:
			strcat(pszError, "Connection refused");
			break;
		case WSAELOOP:
			strcat(pszError, "Too many levels of symbolic links");
			break;
		case WSAENAMETOOLONG:
			strcat(pszError, "File name too long");
			break;
		case WSAEHOSTDOWN:
			strcat(pszError, "Host is down");
			break;
		case WSAEHOSTUNREACH:
			strcat(pszError, "No route to host");
			break;
		case WSAENOTEMPTY:
			strcat(pszError, "Directory not empty");
			break;
		case WSAEPROCLIM:
			strcat(pszError, "Too many processes");
			break;
		case WSAEUSERS:
			strcat(pszError, "Too many users");
			break;
		case WSAEDQUOT:
			strcat(pszError, "Disc quota exceeded");
			break;
		case WSAESTALE:
			strcat(pszError, "Stale NFS file handle");
			break;
		case WSAEREMOTE:
			strcat(pszError, "Too many levels of remote in path");
			break;
			#ifdef _WIN32
		case WSAEDISCON:
			strcat(pszError, "Disconnect");
			break;
			#endif
		case WSASYSNOTREADY:
			strcat(pszError, "Network sub-system is unusable");
			break;
		case WSAVERNOTSUPPORTED:
			strcat(pszError, "WinSock DLL cannot support this application");
			break;
		case WSANOTINITIALISED:
			strcat(pszError, "WinSock not initialized");
			break;
		case WSAHOST_NOT_FOUND:
			strcat(pszError, "Host not found");
			break;
		case WSATRY_AGAIN:
			strcat(pszError, "Non-authoritative host not found");
			break;
		case WSANO_RECOVERY:
			strcat(pszError, "Non-recoverable error");
			break;
		case WSANO_DATA:
			strcat(pszError, "Valid name, no data record of requested type");
			break;
		default:
			strcat(pszError, "Not a WinSock error");
			break;
	}
	
	strcat(pszError, "\n");
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers
void CMainFrame::OnSetFocus(CWnd* pOldWnd)
{
	// forward focus to the view window
	m_wndView.SetFocus();
}

BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	// let the view have first crack at the command
	if (m_wndView.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;
	
	// otherwise, do default handling
	return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

//winsock related message handler
LONG CMainFrame::OnAsyncSelect(WPARAM wParam, LPARAM lParam)
{
	// wParam holds socket id, LOWORD(lParam) is event, HIWORD(lParam) is any error
	
	/* Check for error
	
	if(WSAGETSELECTERROR(lParam) != 0)
		return 0L;
	*/
	
	switch(WSAGETSELECTEVENT(lParam))
	{
		case FD_READ:
		case FD_CLOSE:
			/* cancel event notification until message queue is empty */
			//  TRACE("FD_READ\n");
			
			if(--NotifyCount <= 0)
			{
				WSAAsyncSelect((int)wParam, m_hWnd, 0, 0);
				NotifySockList.AddTail((void *) wParam);
				//   TRACE("ASYNCNOTIFY off\n");
			}
			
			tinread(wParam);
			break;
			/*
		case FD_CLOSE:
			tinclose(wParam);
			break;
			*/  
		default:
			break;
	}
	
	return 0L;   
}

void CMainFrame::EnableWSNotify()
{
	while(!NotifySockList.IsEmpty())
	{
		//  TRACE("ASYNCNOTIFY on\n");
		int sock = (int)NotifySockList.RemoveHead();
		
		// FD_CLOSE added per Feng Chen
		
		WSAAsyncSelect(sock, m_hWnd, WM_USER+1, FD_READ|FD_CLOSE);
	}
	
	NotifyCount = 1;
}

void rlhist_show(void)
{
  int i, N;
  CString cstr1, cstr2;

	CComboBox *combo = & (((CMainFrame *)AfxGetMainWnd())->m_comboInput);

	N = combo->GetCount();
  if(N<=0) {
    showstr(_T("#No history.\n"));
    return;
  }
  for(i = 0; i<N-1+((CMainFrame *)AfxGetMainWnd())->m_bKeepLastline; i++) {
    cstr1.Format(_T("%2d "), i);
		combo->GetLBText(i, cstr2);
		cstr1 += cstr2;
		cstr1 += _T("\n");
    showstr(cstr1.GetBuffer(256));
  }
}

void CMainFrame::OnOk() 
{
	// TODO: Add your control notification handler code here
	CString cstr, cstr0;
	TCHAR str[BUFFER_SIZE], *pt, *ptCRLF, *ptCstr;
	int i, length, found=0;
	
	m_comboInput.GetWindowText(cstr);
	ptCstr = cstr.GetBuffer(80);
	if(cstr.GetLength()>0){
		if(cstr[0]==_T('!')){
			if(!_stscanf(ptCstr, _T("!%d"), &i)){
				for(i=m_comboInput.GetCount()-2+m_bKeepLastline; i>=0; i--) {
					if(m_comboInput.GetLBText(i, str)==CB_ERR)
						break;
					if(_tcsstr(str, ptCstr+1) == str){
						found=1;
						break;
					}
				}
				if(!found){
					showstr(_T("#History not found!\n"));
					m_comboInput.SetCurSel(m_comboInput.GetCount()-1);
					return;
				}
			}
			else {
				if(!ptCstr[1]) i = m_comboInput.GetCount()-2+m_bKeepLastline; //handle single ! case
				if(i>=m_comboInput.GetCount()-1+m_bKeepLastline || m_comboInput.GetLBText(i, str)==CB_ERR){
					showstr(_T("#History out of range!\n"));
					m_comboInput.SetCurSel(m_comboInput.GetCount()-1);
					return;
				}
			}
			cstr.ReleaseBuffer();
			cstr = str;
			ptCstr = cstr.GetBuffer(80);
		}
		if((i=m_comboInput.FindStringExact(0,cstr))>=0)
			m_comboInput.DeleteString(i);
		i=m_comboInput.GetCount();
		if(i > HISTORY_SIZE){
			m_comboInput.DeleteString(0);
			i--;
		}
		else if(i==0 && !m_bKeepLastline){
			m_comboInput.InsertString(0, _T(""));
			i++;
		}
		if(!activesession || !activesession->remote_echo)
			m_comboInput.InsertString(i-1+m_bKeepLastline, cstr); //always keep "" as last one
		m_comboInput.SetCurSel(i);
	}
	
	if(!m_wndView.b_scroll){
		m_wndView.ScrollToBottom();
		m_wndView.Invalidate();
		m_wndView.UpdateWindow();
	}
/* the code below search CRLF and replace it by the user specified string */
	CString szCRLF = AfxGetApp()->GetProfileString(_T("Options"), _T("CR_LF"), _T("\\t"));
	if(szCRLF != _T("\\n")){
		ptCRLF = szCRLF.GetBuffer(10);
		while((pt=_tcsstr(ptCRLF, _T("\\n")))){
			*pt++ = '\n';
			memmove(pt, pt+sizeof(TCHAR), _tcslen(pt)*sizeof(TCHAR));
		}
		while((pt=_tcsstr(ptCRLF, _T("\\r")))){
			*pt++ = '\r';
			memmove(pt, pt+sizeof(TCHAR), _tcslen(pt)*sizeof(TCHAR));
		}
		while((pt=_tcsstr(ptCRLF, _T("\\t")))){
			*pt++ = '\t';
			memmove(pt, pt+sizeof(TCHAR), _tcslen(pt)*sizeof(TCHAR));
		}
		while( (pt=_tcschr(ptCstr, '\r')) )
			*pt = '\n';
		while( (pt=_tcsstr(ptCstr, _T("\n\n"))) )
			memmove(pt, pt+sizeof(TCHAR), _tcslen(pt)*sizeof(TCHAR));
		length = 0;
		while( (pt=_tcschr(ptCstr+length, '\n')) ){
			*pt++ = 0;
			cstr0.Format(_T("%s%s%s"), ptCstr, ptCRLF, pt);
			length = _tcslen(ptCstr) + _tcslen(ptCRLF);
			cstr.ReleaseBuffer();
			cstr = cstr0;
			ptCstr = cstr.GetBuffer(80);
		}
		szCRLF.ReleaseBuffer();
	}
/*end of replacement code */

	if((!activesession || !activesession->remote_echo) && !am_purist) {
		showstr(ptCstr);
		showstr(_T("\n"));
	}
	USES_CONVERSION;
	activesession = parse_input(T2A(ptCstr), activesession);
	cstr.ReleaseBuffer();
}

void CMainFrame::OnClose() 
{
	// TODO: Add your message handler code here and/or call default
	WINDOWPLACEMENT wndpl;
	GetWindowPlacement(&wndpl);
	AfxGetApp()->WriteProfileInt(_T("Window"), _T("left"), wndpl.rcNormalPosition.left);
	AfxGetApp()->WriteProfileInt(_T("Window"), _T("right"), wndpl.rcNormalPosition.right);
	AfxGetApp()->WriteProfileInt(_T("Window"), _T("top"), wndpl.rcNormalPosition.top);
	AfxGetApp()->WriteProfileInt(_T("Window"), _T("bottom"), wndpl.rcNormalPosition.bottom);
	SaveBarState(_T("General"));
	
	tinquit();
	CFrameWnd::OnClose();
}


void CMainFrame::OnTimer(UINT nIDEvent) 
{
	// TODO: Add your message handler code here and/or call default
	struct timeval to;
	checktick(&to);
	CFrameWnd::OnTimer(nIDEvent);
}

void CMainFrame::OnTabComplete() 
{
	// TODO: Add your command handler code here
	CString cstr, cstr0;
	char *pt, *pt2, *ptCstr;
	char buf[2];
	int l;
	
	if(toggle_raw && activesession) {
		buf[0] = '\t';
		write_raw_mud(buf, 1, activesession);
		return;
	}
	m_comboInput.GetWindowText(cstr);
	USES_CONVERSION;
	ptCstr = T2A(cstr.GetBuffer(80));
	if( (pt=strrchr(ptCstr, ' ')) ) pt++;
	else pt = ptCstr;
	if( !(pt2=rltab_generate(pt, 0)) ){
		showstr(_T("#Tab completion not found!\n"));
		return;
	}
	*pt = 0;
	cstr0 = ptCstr;
	cstr0 += pt2;
	cstr0 += " ";
	m_comboInput.SetWindowText(cstr0);
	l=cstr0.GetLength();
	m_comboInput.SetEditSel(l, l);
	free(pt2);
	cstr.ReleaseBuffer();
}

void hidePasswd(int hide)
{
	((CMainFrame *)AfxGetMainWnd())->Passwd(hide);
}
void CMainFrame::Passwd(int passwd)
{ //does not work, fix it later
/*
	if(passwd)
		((CEdit *)(&m_comboInput))->SetPasswordChar('*');
	else
		((CEdit *)(&m_comboInput))->SetPasswordChar(0);
*/
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg) 
{
	// TODO: Add your specialized code here and/or call the base class
	char buf[BUFFER_SIZE];
	CString cstr;
	struct keytype key;

/*
	if(pMsg->message == WM_IME_CHAR){
		CString cstr;
		int l;
    // add this string into text buffer of application
		m_comboInput.GetWindowText(cstr);
		cstr += (LPTSTR)(pMsg->wParam);
		m_comboInput.SetWindowText("| " + cstr + " |");
		l=cstr.GetLength();
		m_comboInput.SetEditSel(l, l);
	}
*/
/*
	if(pMsg->message == WM_IME_SETCONTEXT){
		return CFrameWnd::PreTranslateMessage(pMsg);
	}
*/
/*
	if(pMsg->message == WM_IME_COMPOSITION
			&& (pMsg->lParam & GCS_RESULTSTR)){
		CString cstr;
		HIMC hIMC;
		DWORD dwSize;
		HGLOBAL hstr;
		LPWSTR lpszW;
		int l;

		hIMC = ImmGetContext(m_hWnd);
		if(!hIMC) return 0;
		dwSize = ImmGetCompositionString(hIMC, GCS_RESULTSTR, NULL, 0);
		dwSize += sizeof(WCHAR);
		hstr = GlobalAlloc(GHND,dwSize);
		if (hstr == NULL) return 0;
		lpszW = (LPWSTR) GlobalLock(hstr);
		if(lpszW == NULL) return 0;
		ImmGetCompositionString(hIMC, GCS_RESULTSTR, lpszW, dwSize);
		ImmReleaseContext(m_hWnd, hIMC);

    // add this string into text buffer of application
		m_comboInput.GetWindowText(cstr);
		cstr += lpszW;
		m_comboInput.SetWindowText("| " + cstr + " |");
		l=cstr.GetLength();
		m_comboInput.SetEditSel(l, l);

		GlobalUnlock(hstr);
		GlobalFree(hstr);

		return 1;
	}
*/
	if(toggle_raw && activesession &&
			(pMsg->message == WM_CHAR || pMsg->message == WM_SYSCHAR)){
		buf[0] = pMsg->wParam;
		write_raw_mud(buf, 1, activesession);
		return 1;
	}
	//here we only take KEYDOWN messages, and only take bit 16-24 of lParam (OEM scan code plus ALT bit)
	if ( (pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN)
			&& !(pMsg->wParam>=0x10 && pMsg->wParam<=0x14) ){ //skip pure shift/alt/ctrl, caps lock, pause
		key.wParam = pMsg->wParam;
		key.lParam = HIWORD(pMsg->lParam) & 0x01FF;
		if(keycapture){
			if(key.wParam==27) {
				macro_command("0", NULL);
			}
			else {
				cstr.Format( _T("{%x, %x}\n"), key.wParam, key.lParam);
				showstr(cstr.GetBuffer(80));
			}
			return 1;
		}
		else if(check_macros(&key, activesession))
				return 1;
		else {
			if(toggle_raw && activesession &&
					((key.wParam==0x26 && key.lParam==0x148)
					|| (key.wParam==0x28 && key.lParam==0x150)
					|| (key.wParam==0x25 && key.lParam==0x14b)
					|| (key.wParam==0x27 && key.lParam==0x14d) )) {
				if(key.lParam==0x148) buf[2] = 0x41; // up arrow
				else if(key.lParam==0x150) buf[2] = 0x42; //down arrow
				else if(key.lParam==0x14b) buf[2] = 0x44; //left arrow
				else if(key.lParam==0x14d) buf[2] = 0x43; //right arrow
				buf[0] = 0x1b;
				buf[1] = 0x5b;
				write_raw_mud(buf, 3, activesession);
				return 1;
			}
		}
	}
	
	return CFrameWnd::PreTranslateMessage(pMsg);
}

void CMainFrame::OnSize(UINT nType, int cx, int cy)
// changes for resizing input window, by <tangzhi@163.net>
{
	CFrameWnd::OnSize(nType, cx, cy);
	
	// TODO: Add your message handler code here
	CRect rect;
	GetClientRect(&rect);	
	m_comboInput.MoveWindow(&rect,TRUE);
	m_wndView.b_newfont=1;
}
