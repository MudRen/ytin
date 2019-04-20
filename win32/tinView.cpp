// tinView.cpp : implementation of the CTinView class
//

#include "stdafx.h"

#include <mmsystem.h>

#include "ytin.h"
#include "tinView.h"
#include "mainfrm.h"
#include "options.h"
#include "U2AConv.h"

#include "ansi.proto"
#include "log.proto"
#include "main.proto"
#include "misc.proto"
#include "parse.proto"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern int sent_quit_msg;
/////////////////////////////////////////////////////////////////////////////
// CTinView
#define SV_MAX 16000
CTinView* pThis;

CTinView::CTinView()
{
	// TODO: add construction code here
	pThis = this;
	int i;
	CWinApp* pApp=AfxGetApp();

	W80 = pApp->GetProfileInt(_T("Options"), _T("max_width"), 80);
	w80 = W80/2*2;
//	if(!(lpDxWidth = (int *)calloc(w80+10, sizeof(int))) )
//		MessageBox(_T("CTinView: cannot alloc lpDxWidth!"));

	TOTAL_BUFF_SIZE = pApp->GetProfileInt(_T("Options"), _T("buff_size"), 1000);
	YCHAR = SV_MAX/TOTAL_BUFF_SIZE;

	if(!(m_buff = (TCHAR **)calloc(TOTAL_BUFF_SIZE, sizeof(TCHAR *))) )
		MessageBox(_T("CTinView: cannot alloc *m_buff!"));
	for(i=0; i<TOTAL_BUFF_SIZE; i++){
		if( !(m_buff[i]=(TCHAR*)calloc(1, sizeof(TCHAR))) ){
			exit(0);
			MessageBox(_T("CTinView: cannot alloc m_buff!"));
			break;
		}
	}
	m_line0=m_line = 0; m_done=1;
	b_scroll=1;
	b_beep=2;
	xPos = yPos = 0;
	fgColor[0] = fgColor[1] = 37;
	bgColor[0] = bgColor[1] = 0;
	bBold[0] = bBold[1] = 0;
	bHighcolor[0] = bHighcolor[1] = 0;
	bHighcolor2[0] = bHighcolor2[1] = 0;
#ifdef _UNICODE
	bCheck2Byte = 0;
#else
	bCheck2Byte = 1;
#endif
	b_newfont=1;
	TABSIZE = pApp->GetProfileInt(_T("Options"), _T("TABSIZE"), 2);;
	jumpstr = "";

	LOGFONT lf;

	bHighlight = FALSE;
	bMouseTracking = FALSE;

	hFont = ((CYtinApp *)pApp)->hfont;
	GetObject(hFont, sizeof(LOGFONT), &lf);
	hFontB = CreateFont(lf.lfHeight, 0, 0, 0,
		//  FW_BOLD,
		pApp->GetProfileInt(_T("Options"), _T("font_weight"), 600),
		lf.lfItalic,
		lf.lfUnderline,
		lf.lfStrikeOut,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		FF_DONTCARE|DEFAULT_PITCH,
		lf.lfFaceName);
	if(hFontB == NULL)
		hFontB = (HFONT)GetStockObject(ANSI_FIXED_FONT);

	m_n_extra_x = pApp->GetProfileInt(_T("Options"), _T("h_spacing"), 0);
	m_n_extra_y = pApp->GetProfileInt(_T("Options"), _T("v_spacing"), 2);

	InitColor();
}

CTinView::~CTinView()
{
	int i;
	for(i=0; i<TOTAL_BUFF_SIZE; i++)
		if(m_buff[i]) free(m_buff[i]);
	free(m_buff);
//	free(lpDxWidth);
}


BEGIN_MESSAGE_MAP(CTinView,CWnd )
	//{{AFX_MSG_MAP(CTinView)
	ON_WM_PAINT()
	ON_WM_VSCROLL()
	ON_COMMAND(ID_VIEW_FONT, OnViewFont)
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_WM_SIZE()
	ON_COMMAND(ID_VIEW_OPTIONS, OnViewOptions)
	ON_COMMAND(ID_VIEW_PAGEUP, OnViewPageup)
	ON_COMMAND(ID_VIEW_PAGEDOWN, OnViewPagedown)
	ON_COMMAND(ID_VIEW_BG_COLOR, OnViewBgColor)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONUP()
	ON_WM_SETFOCUS()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CTinView message handlers

BOOL CTinView::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CWnd::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS,
		::LoadCursor(NULL, IDC_ARROW),
		HBRUSH(COLOR_WINDOWTEXT+1),
		NULL);

	return TRUE;
}

void CTinView::OnPaint()
{
	int i, n, dy, m;
	CSize cs;

	CPaintDC dc(this); // device context for painting
	CRect rect0;
	dc.SetMapMode(MM_TEXT);
	dc.GetClipBox(&rect0);

	if(b_newfont){
		b_newfont=FALSE;
		b_ansi = AfxGetApp()->GetProfileInt(_T("Options"), _T("enable_ansi"), 1);
		m_bBoldAsHigh = AfxGetApp()->GetProfileInt(_T("Options"), _T("BoldAsHigh"), 1);
		TEXTMETRIC  tm;
		dc.SelectObject(hFont);
		dc.GetTextMetrics(&tm);
		xChar = tm.tmMaxCharWidth;
		yChar = tm.tmHeight + tm.tmExternalLeading + m_n_extra_y;
		if(xChar>yChar*3/4)
			xChar /= 2;
		xChar += m_n_extra_x;

		if(crClientRect.right/xChar<W80/2*2) w80=crClientRect.right/xChar/2*2;
		else w80 = W80/2*2;
		x80 = w80 * xChar; //if font width not fix, e.g., bold, actually we need to wrap at fixed width not # of chars
//		lpDxWidth = (int *)realloc(lpDxWidth, (w80+10)*sizeof(int));
//		for(i=0; i<w80+10; i++)
//			lpDxWidth[i] = xChar;

		ScrollToBottom();
	}

	// clear it out to get rid of scroll crap

	CBrush cbw;
	cbw.CreateSolidBrush(((CYtinApp *)AfxGetApp())->backcolor);
	dc.FillRect(&rect0, &cbw);

	// Find offset on screen of first and last row to draw

	cs = GetVScrollInfo(); //cx and cy stores nPos and nPage of SB_VERT
	m=TOTAL_BUFF_SIZE-(cs.cx+cs.cy-1)/YCHAR; //m==0 means scroll to bottom
	if(m==0) m_line0 = m_line;

	dy = -cs.cx%YCHAR*yChar/YCHAR;

	int ROW = min(IRow, (rect0.bottom-1)/yChar);
	for (i=rect0.top/yChar; i<=IRow; i++)
	{
		n = (m_line0 -IRow + i -m + 2*TOTAL_BUFF_SIZE)%TOTAL_BUFF_SIZE;
		AnsiTextOut(0, i, m_buff[n]); //when bCheck80 == 0, it does not write to m_buff and ses->buff, so no need ses parameter
	}
	// Check for highlighting
	if(bHighlight)
	{
		CRgn rgn, rgn0;
		CalcHighlightRgn(rgn);
		rgn0.CreateRectRgnIndirect(&rect0);
		rgn.CombineRgn(&rgn, &rgn0, RGN_AND);
		dc.InvertRgn(&rgn);
		rgn0.DeleteObject();
		rgn.DeleteObject();
	}

	// Do not call CWnd::OnPaint() for painting messages
}

void showstr1(char *str)
{
	USES_CONVERSION;
	showstr(A2T(str));
}

void showstr(TCHAR *str)
{
	if(sent_quit_msg) return; //avoid crash due to #zap, #end while there is still output pending

/* debug only *
	if(_tcsstr(str, _T("using "))){
		str = str;
	}
/****/
	if(buffer_view){
		pThis->jumpstr += str;
	}
	else
		pThis->xPos = pThis->PreTextOut(pThis->xPos, str);
}

void flush_view()
{
	pThis->xPos = pThis->PreTextOut(pThis->xPos, pThis->jumpstr.GetBuffer(200));
	pThis->jumpstr.ReleaseBuffer();
	buffer_view = 0;
	pThis->jumpstr = "";
}

int CTinView::PreTextOut(int x, TCHAR *buff)
{
	int in_escape = 0, bDByte=0, l, pos=x, dy, nScroll=0;
	TCHAR *pt, *pt0;
	CString cs; //the absolute color before each m_buff and ses->buff
	CSize size;
	CRect rect = crClientRect;
	static int In_escape;

	CClientDC dc(this);

	SCROLLINFO si;
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_ALL;
	size = GetVScrollInfo();
	b_scroll = ((TOTAL_BUFF_SIZE-(size.cx+size.cy-1)/YCHAR) <= 0);
	dy = -size.cx%YCHAR*yChar/YCHAR;

	while(pt=_tcsstr(buff, _T("\r\n"))){
		memmove(pt, pt+sizeof(TCHAR), _tcslen(pt)*sizeof(TCHAR));
	}
/* for debug break only *
		char buf1[256];
		USES_CONVERSION;
		strcpy(buf1, T2A(buff));
		char *cpt;
	if((cpt=strstr(buf1, "Ãë"))){
		USES_CONVERSION;
		UINT ui = A2T(cpt)[0];
	}
/****/
	for(pt=pt0=buff; *pt; pt++){
		if(in_escape && *pt && *pt!=';' && !isdigit(*pt)) {
			in_escape=0;
			continue; //7/21/2000, bug fix
		}
		if(*pt==0x1b && (pt[1]==0 || pt[1]=='[')) { //handle broken packet
			in_escape=1;
			if(pt[1]) pt++;
		}
		if(!in_escape) {
			if(*pt == '\t') pos += (TABSIZE-(pos%TABSIZE))*xChar;
			else if(*pt=='\b') pos -= (pos>0)*xChar; //backspace
			else if(*pt==7) { //b_beep=0: disable beep, b_beep=2: force beep on scroll-hold
				if(b_beep+b_scroll>=2) bell_command("", NULL);
			}
			else if((*pt & 0xFF) >= ' '){
					pos += xChar;
			}
			if(bCheck2Byte){
				if((*pt & 0xFF) >= 128) bDByte = !bDByte;
				else bDByte = 0; //reset double byte position
			}
			if(*pt == 0x3002){
				UINT ui = _T('\n');
			}
			if((pos > x80-xChar && pos <= x80 && bDByte) || pos > x80 || *pt=='\n'){ //note real \n keeps \n while col80 appends \r, isn't it special to send \n\r ? :D
				if(pos > x80) pt--; //only wrap when pos>w80, not >=w80, 7/22/2000
				if(pos > x80-xChar && pos <= x80 && bDByte) pt--; //smart double byte display
				bDByte=0;
				FormatAbsoluteColor(cs, 0);
				m_buff[m_line] = (TCHAR*)realloc(m_buff[m_line],
					(l=(1-m_done)*_tcslen(m_buff[m_line]) + _tcslen(pt0)+3 + m_done*cs.GetLength())*sizeof(TCHAR));
				if(!m_buff[m_line]) {
					MessageBox(_T("Error: cannot alloc mem for m_buff!"));
					exit(0);
				}
				if(m_done) _tcscpy(m_buff[m_line], cs);
				else {
					cs = pt0;
					cs += " "; //increase size of cs by 1
					cs.SetAt(pt+1-pt0, 0);
				}
				_tcsncat(m_buff[m_line], pt0, pt+1-pt0);
				if((pos > x80-xChar && pos <= x80 && bDByte) || pos > x80) {
					if(!m_done) cs += '\r';
					m_buff[m_line][l-2]='\r';
					m_buff[m_line][l-1]=0;
				}
				else m_buff[m_line][l-2]=0;
				pt0 = pt+1;
				//TODO:
				//copy m_buff to ses->buff[ses->line]
				//if ses==NULL, copy to all sessions

				if(b_scroll){
					nScroll ++;
					AnsiTextOut(0, IRow, m_buff[m_line], 0); //does not draw, only get the correct color
				}
				m_line=(m_line+1)%TOTAL_BUFF_SIZE;
				m_buff[m_line][0]=0;
				x=pos=0;
				m_done=1;
			}
		}
	}
	if(b_scroll) m_line0 = m_line;
	if(nScroll) {
		if(bHighlight) {
			cpH1.y -= nScroll*YCHAR; cpH.y -= nScroll*YCHAR; cpH2.y -= nScroll*YCHAR;
		}
		ScrollWindowEx(0, -nScroll*yChar, 0,0,0,0, SW_INVALIDATE);
		rect.bottom = dy + IRow*yChar -nScroll*yChar+yChar+ yChar/3;
		rect.top = dy + IRow*yChar - nScroll*yChar - yChar/3;
		InvalidateRect(&rect);
	}
	if(*pt0){
		FormatAbsoluteColor(cs, 0);
		m_buff[m_line] = (TCHAR*)realloc(m_buff[m_line],
			((1-m_done)*_tcslen(m_buff[m_line]) + _tcslen(pt0)+1 + m_done*cs.GetLength())*sizeof(TCHAR));
		if(!m_buff[m_line]) {
			MessageBox(_T("Error: cannot alloc mem for m_buff!"));
			exit(0);
		}
		if(m_done) _tcscpy(m_buff[m_line], cs);
		_tcscat(m_buff[m_line], pt0);
		if(b_scroll /*&& !nScroll*/) {
			if(In_escape)
				In_escape = AnsiTextOut(0, IRow, m_buff[m_line], 2); //handle broken packet here
			else //save some drawing effort
				In_escape = AnsiTextOut(x, IRow, m_done ? m_buff[m_line] : pt0, 2);
			In_escape = In_escape | bDByte;
		}
		m_done=0;
	}
	return pos;
}

int CTinView::AnsiTextOut(int x, int y, TCHAR *buff, int m)
//real=0: do not draw, only update color set 0,
//real=1: draw and update colorset 1
//real=2: draw and update colorset 0
{
	int in_escape[2] = {0,0}, bDByte=0, ansiCount, l, pos, dy, bHigh;
	TCHAR ch0, *pt, *pt1;
	CString cs; //the absolute color before each m_buff and ses->buff
	int TabStopPos = TABSIZE*xChar;

	int format, highIsBg;

	CRect rect;
	CSize size;
	CClientDC dc(this);
	dy = -GetVScrollInfo().cx%YCHAR*yChar/YCHAR;

	//note shall OutText as a whole piece when we can (speed/MBCS et al)
	cs = "";
	pos = x;
	for(pt=buff; *pt; pt++){
		if( (*pt=='' && pt[1]=='[') || *pt=='\b') {
			if (cs.GetLength() && m){
				SetFontAttrib(dc, m);
//				size = dc.GetTextExtent(cs);
				size = dc.GetTabbedTextExtent(cs, 1, &TabStopPos);
				rect.left = x; rect.right = x + size.cx;
				rect.top = yChar*y+dy; rect.bottom =rect.top + yChar;
//				dc.ExtTextOut(x, y*yChar+dy, ETO_OPAQUE | ETO_CLIPPED, &rect, cs, NULL);//lpDxWidth);
				dc.TabbedTextOut(x, y*yChar+dy, cs, 1, &TabStopPos, 0);
				x = rect.right;
				pos = x;
			}
			cs = "";
			if(*pt=='\b'){
				pt++;
				x -= xChar;
				pos = x;
			}
			else {
				pt += 2; //jump to in_escape[m%2] == true
				in_escape[m%2] = 1;
				ansiCount = 0;
				highIsBg = 0;
			}
		}
		if(!in_escape[m%2]) {
/*			if(*pt == '\n' || *pt == '\r') {
				pos = 0;
			}
			else
*/
			if(*pt == '\t') {
/*				l = TABSIZE-(pos/xChar%TABSIZE);
				pos += l*xChar;
				for(;l>0;l--) cs += " ";
*/				cs += "\t";
			}
			else if((*pt & 0xFF) >= 0x20) { //treat all char b4 ' ' as non-printable, 7/20/2000
				cs += *pt;
				pos += xChar;
			}
		}
		else{ //in_escape[m%2] == true
			//get the digits after ^[[
			pt1 = pt;
			while(isdigit(*pt) || *pt==';') pt++;
			if(*pt != 'm') { //sequence other than ^[[x;x;xm
				if(*pt) in_escape[m%2] = 0; //not due to broken packets
			}
			else {//color sequence ends with m
				pt = pt1;
				while(isdigit(*pt)) pt++;
				ch0 = *pt; *pt = 0;
				if(*pt1==0) format = 0;
				else _stscanf(pt1, _T("%d"), &format);
				*pt = ch0;
				if(*pt=='m') {
					in_escape[m%2] = 0;
					if(bHigh){
						if(highIsBg) bHighcolor2[m%2] = 1;
						else bHighcolor[m%2] = 1;
					}
				}
				else ansiCount++; //*pt==';' in this case

				if(format == 0){
					bHigh=bgColor[m%2]=bBold[m%2]=bHighcolor[m%2]=bHighcolor2[m%2]=0;
					fgColor[m%2]=37;
				}
				else if(format==1){
					if(ansiCount==0 && !m_bBoldAsHigh) //^[[1m - bold, and not treat as high ...
						bBold[m%2] = 1;
					else bHigh = 1;
				}
				else if(format>=30 && format<=37){ //forground color: 30-37
					highIsBg = 0;
					fgColor[m%2] = format;
				}
				else if(format>=40 && format<=47){ //bg color: 40-47
					highIsBg = 1;
					bgColor[m%2] = format;
				}
			}
		}
	}
	if(l=cs.GetLength()){
		if(cs[l-1] == '\n' || cs[l-1] == '\r'){
			cs.SetAt(l-1, 0);
			l=1;//tag for set pos back to 0
		}
		else l=0;
		if(m) {
			SetFontAttrib(dc, m);
				if(cs.Find('\t')>=0)
					pos = pos;
//			size = dc.GetTextExtent(cs);
			size = dc.GetTabbedTextExtent(cs, 1, &TabStopPos);
			rect.left = x;  rect.right = x + size.cx;
			rect.top = yChar*y+dy; rect.bottom =rect.top + yChar;
//			dc.ExtTextOut(x, yChar*y+dy, ETO_OPAQUE | ETO_CLIPPED, &rect, cs, NULL);//lpDxWidth);
			dc.TabbedTextOut(x, y*yChar+dy, cs, 1, &TabStopPos, 0);
			x = rect.right;
			pos = x;
		}
		if(l) pos = x = 0;
	}
	return in_escape[m%2];
}

void CTinView::FormatAbsoluteColor(CString &cs, int m)
{
	m = m%2;
	if(!bgColor[m] && (!fgColor[m] || fgColor[m]==37) && !bBold[m] && !bHighcolor[m] && !bHighcolor2[m])
		cs = "[0m"; //here we adopt xterm style reset color, coz it is shorter
	else {
		cs.Format(_T("[%d%sm[%s%dm"), bgColor[m], bHighcolor2[m] ? ";1" : "", bHighcolor[m] ? "1;" : "", fgColor[m]);
		if(bBold[m]) cs += "[1m";
	}
}

void CTinView::SetFontAttrib(CDC &dc, int m)
{
	COLORREF cr;
	int i, set;
	// ^[[1;32m is highcolor, high background color, ^[[42;1m
	m = m%2;
	if(bBold[m]) dc.SelectObject(hFontB);
	else dc.SelectObject(hFont);


	set = 0;
	if(b_ansi && bHighcolor[m])
		i = fgColor[m]-30+8;
	else if(!b_ansi || fgColor[m]==0 || fgColor[m]==37){
		set = 1;
		dc.SetTextColor(((CYtinApp *)AfxGetApp())->textcolor);
	}
	else
		i = fgColor[m]-30;
	if(!set) {
		if(m_bProp0 && i<8)
			cr = RGB(m_dim0*COLOR[i][0]+m_neg0*(1-COLOR[i][0]), m_dim0*COLOR[i][1]+m_neg0*(1-COLOR[i][1]), m_dim0*COLOR[i][2]+m_neg0*(1-COLOR[i][2]));
		else if(m_bProp1 && i>=8)
			cr = RGB(m_dim1*COLOR[i-8][0]+m_neg1*(1-COLOR[i-8][0]), m_dim1*COLOR[i-8][1]+m_neg1*(1-COLOR[i-8][1]), m_dim1*COLOR[i-8][2]+m_neg1*(1-COLOR[i-8][2]));
		else cr = RGB(rgb[i][0], rgb[i][1], rgb[i][2]);
		dc.SetTextColor(cr);
	}

	set = 0;
	if(b_ansi && bHighcolor2[m])
		i = bgColor[m]-40+8;
	else if(!b_ansi || bgColor[m]==0 || bgColor[m]==40){
		set = 1;
		dc.SetBkColor(((CYtinApp *)AfxGetApp())->backcolor);
	}
	else
		i = bgColor[m]-40;
	if(!set) {
		if(m_bProp0 && i<8)
			cr = RGB(m_dim0*COLOR[i][0]+m_neg0*(1-COLOR[i][0]), m_dim0*COLOR[i][1]+m_neg0*(1-COLOR[i][1]), m_dim0*COLOR[i][2]+m_neg0*(1-COLOR[i][2]));
		else if(m_bProp1 && i>=8)
			cr = RGB(m_dim1*COLOR[i-8][0]+m_neg1*(1-COLOR[i-8][0]), m_dim1*COLOR[i-8][1]+m_neg1*(1-COLOR[i-8][1]), m_dim1*COLOR[i-8][2]+m_neg1*(1-COLOR[i-8][2]));
		else cr = RGB(rgb[i][0], rgb[i][1], rgb[i][2]);
		dc.SetBkColor(cr);
	}
}


void CTinView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: Add your message handler code here and/or call default
	// CWnd ::OnVScroll(nSBCode, nPos, pScrollBar);
	SCROLLINFO si;
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_ALL;
	GetScrollInfo(SB_VERT, &si);
	int oldPos = si.nPos;
	switch(nSBCode) {
		case SB_BOTTOM:
			si.nPos = si.nMax;
			break;
		case SB_LINEDOWN:
			si.nPos += 3*YCHAR;
			break;
		case SB_LINEUP:
			si.nPos -= 3*YCHAR;
			break;
		case SB_PAGEDOWN:
			si.nPos += si.nPage;
			break;
		case SB_PAGEUP:
			si.nPos -= si.nPage;
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			si.nPos = si.nTrackPos;
			break;
		default:
			return;
	}
	SetScrollInfo(SB_VERT, &si, TRUE);
	GetScrollInfo(SB_VERT, &si, SIF_ALL);

	Invalidate();
	/* oldPos -= si.nPos;
	if(oldPos>=(signed)si.nPage || -oldPos>=(signed)si.nPage)
		Invalidate();
	else {
		CRect rect = crClientRect;
		if(oldPos<0) //scroll up
			rect.top = rect.bottom + oldPos;
		else
			rect.bottom = rect.top + oldPos;
		if(oldPos) {
			ScrollWindowEx(0, oldPos*yChar/YCHAR,0,0,0,0,SW_INVALIDATE);
		}
	}
	*/
}
void CTinView::OnViewPageup()
{
	// TODO: Add your command handler code here
	SCROLLINFO si;
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_ALL;
	GetScrollInfo(SB_VERT, &si);
	si.nPos -= si.nPage;
	SetScrollInfo(SB_VERT, &si, TRUE);
	Invalidate();
	UpdateWindow();
}

void CTinView::OnViewPagedown()
{
	// TODO: Add your command handler code here
	SCROLLINFO si;
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_ALL;
	GetScrollInfo(SB_VERT, &si);
	si.nPos += si.nPage;
	SetScrollInfo(SB_VERT, &si, TRUE);
	Invalidate();
	UpdateWindow();
}

void CTinView::OnViewFont()
{
	CYtinApp *wApp = (CYtinApp *)AfxGetApp();
	LOGFONT lf;

	CClientDC dc(this);
	dc.SetMapMode(MM_TEXT);

	POINT p;

	GetObject(hFont, sizeof(LOGFONT), &lf);

	p.y = lf.lfHeight;
	dc.LPtoDP(&p);
	lf.lfHeight = p.y;

	CFontDialog cfd(&lf, CF_INITTOLOGFONTSTRUCT|CF_EFFECTS | CF_SCREENFONTS);
	cfd.m_cf.rgbColors = wApp->textcolor;

	if(cfd.DoModal() == IDOK)
	{
		p.y = cfd.m_cf.lpLogFont->lfHeight;
		dc.DPtoLP(&p);
		cfd.m_cf.lpLogFont->lfHeight = p.y;

		if((hFont = CreateFont(cfd.m_cf.lpLogFont->lfHeight, 0, 0, 0, cfd.GetWeight(),
			cfd.IsItalic(), cfd.IsUnderline(), cfd.IsStrikeOut(),
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE|DEFAULT_PITCH,
			cfd.GetFaceName())) != NULL)
		{
			wApp->WriteProfileString(_T("font"), _T("facename"), cfd.GetFaceName());
			wApp->WriteProfileInt(_T("font"), _T("strikeout"), cfd.IsStrikeOut());
			wApp->WriteProfileInt(_T("font"), _T("underline"), cfd.IsUnderline());
			wApp->WriteProfileInt(_T("font"), _T("italic"), cfd.IsItalic());
			wApp->WriteProfileInt(_T("font"), _T("weight"), cfd.GetWeight());
			wApp->WriteProfileInt(_T("font"), _T("size"), cfd.m_cf.lpLogFont->lfHeight);

			wApp->textcolor = cfd.GetColor();
			CString cs;
			cs.Format(_T("%ld"), wApp->textcolor);
			wApp->WriteProfileString(_T("windows"), _T("textcolor"), cs);

			hFontB = CreateFont(lf.lfHeight, 0, 0, 0,
				FW_BOLD,
				lf.lfItalic,
				lf.lfUnderline,
				lf.lfStrikeOut,
				DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
				CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
				FF_DONTCARE|DEFAULT_PITCH,
				lf.lfFaceName);
			if(hFontB == NULL)
				hFontB = (HFONT)GetStockObject(ANSI_FIXED_FONT);

//		((CMainFrame *)AfxGetMainWnd())->SendMessage(WM_SETFONT, (WPARAM)hFont, true);

			b_newfont=1;
			Invalidate();
		}
	}
}

void CTinView::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	if(nFlags & MK_SHIFT)
	{
		// extend highlighted region, if any, by modifying cpH2
		if(!bHighlight)
			return;

		// Cancel any outstanding highlight
		if(bHighlight)
		{
			bHighlight = FALSE;
			InvalidateHighlight();
		}
		//  MarkHighlightRegion(point);
	}
	else
	{
		// Cancel any outstanding highlight
		if(bHighlight)
		{
			bHighlight = FALSE;
			InvalidateHighlight();
		}

		cpH1.x = (point.x)/xChar*xChar;
		cpH1.y = (point.y*YCHAR/yChar + GetVScrollInfo().cx)/YCHAR*YCHAR;

		cpH = cpH1;

		// Capture mouse and note we are tracking

		bMouseTracking = TRUE;
		SetCapture();
	}

	CWnd ::OnLButtonDown(nFlags, point);
}

void CTinView::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CWnd ::OnMouseMove(nFlags, point);

	if(!bMouseTracking)
		return;
	CPoint cpH0;
	CRgn oldRgn;
	int oldhigh = 0;
	cpH0.x = (point.x)/xChar*xChar;
	cpH0.y = (point.y*YCHAR/yChar + GetVScrollInfo().cx)/YCHAR*YCHAR;
	if(bHighlight) {
		oldhigh = 1;
		CalcHighlightRgn(oldRgn);
	}
	bHighlight = (cpH0 != cpH1);
	if(cpH0!=cpH){
		CRect rect;
		rect.left = min(cpH.x, cpH0.x);
		rect.right = max(cpH.x, cpH0.x);
		rect.top = min(cpH.y, cpH0.y)-GetVScrollInfo().cx;
		rect.bottom = max(cpH.y, cpH0.y)-GetVScrollInfo().cx;
		cpH = cpH0;
		InvalidateHighlight(oldhigh ? &oldRgn : NULL);
		if(oldhigh) oldRgn.DeleteObject();
		UpdateWindow();
	}
}

void CTinView::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CWnd ::OnLButtonUp(nFlags, point);

	// ignore if we're not tracking
	if(!bMouseTracking)
		return;
	/*
	// Cancel timer if running
	if(nTimer) {
		KillTimer(nTimer);
		nTimer = 0;
	}

	if(!crClientRect.PtInRect(point))
	{
		// Cancel any outstanding highlight
		if(bHighlight)
		{
			bHighlight = FALSE;
			InvalidateHighlight();
		}
	}
	else
		MarkHighlightRegion(point);
	*/
	cpH2.x = (point.x)/xChar*xChar;
	cpH2.y = (point.y*YCHAR/yChar + GetVScrollInfo().cx)/YCHAR*YCHAR;

	cpH = cpH2;

	ReleaseCapture();
	bMouseTracking = FALSE;

//	SetInputFocus();
}


void CTinView::SetInputFocus()
{
	int b_combo;
	CComboBox* combo;
	combo = &((CMainFrame *)AfxGetMainWnd())->m_comboInput;
	b_combo = ((CMainFrame *)AfxGetMainWnd())->b_combo;
	if(b_combo) {
		combo->SetFocus();
		combo->SetEditSel(-1, 0); //un-select edit text
	}
}


void CTinView::OnSetFocus(CWnd* pOldWnd)
{
	CWnd ::OnSetFocus(pOldWnd);

	// TODO: Add your message handler code here
	SetInputFocus();
}


void CTinView::ScrollToBottom()
{
	SCROLLINFO si;
	CRect rect;
	GetClientRect(&rect);
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_ALL;

	si.nMax = YCHAR*TOTAL_BUFF_SIZE;
	si.nMin = 0;
	IRow = (rect.bottom-rect.top)/yChar-1;
	si.nPage = IRow * YCHAR;
	si.nPos = si.nMax;
	SetScrollInfo(SB_VERT, &si, TRUE);
	b_scroll = TRUE;
}

CSize CTinView::GetVScrollInfo()
//a hack
{
	CSize cs;
	SCROLLINFO si;
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_ALL;
	GetScrollInfo(SB_VERT, &si);
	cs.cx = si.nPos;
	cs.cy = si.nPage;
	return cs;
}

void CTinView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	int l;
	TCHAR *pt;

	cpH1.x = 0;
	cpH1.y = (point.y*YCHAR/yChar + GetVScrollInfo().cx)/YCHAR*YCHAR;

	cpH.x = 0;
	cpH.y = cpH1.y;
	do {
		pt = m_buff[(m_line0 + cpH.y/YCHAR)%TOTAL_BUFF_SIZE];
		l = _tcslen(pt);
		cpH.y += YCHAR;
	} while(l>0 && pt[l-1]!='\n' && pt[l-1]!='\r');

	bHighlight=1;
	bMouseTracking=0;

	CRgn rgn;
	CalcHighlightRgn(rgn);
	InvalidateRgn(&rgn);
	UpdateWindow();

	// CWnd ::OnLButtonDblClk(nFlags, point);
}

void CTinView::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	OnLButtonDblClk(nFlags, point);
	// CWnd ::OnRButtonDblClk(nFlags, point);
}

void CTinView::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	EditCopy(1);

	CComboBox* combo;
	combo = &((CMainFrame *)AfxGetMainWnd())->m_comboInput;
	combo->Paste();

	// CWnd ::OnRButtonUp(nFlags, point);
}

void CTinView::OnEditCopy()
{
	EditCopy(0);
}

void CTinView::EditCopy(BOOL forceclear)
{
	// TODO: Add your command handler code here
	if(!bHighlight && !forceclear) return;
	if(!OpenClipboard()) return;
	if(!EmptyClipboard())
	{
		CloseClipboard();
		return;
	}
	if(forceclear && !bHighlight) {
		CloseClipboard();
		return;
	}

	CPoint cp1, cp2;
	unsigned int n, x1, x2;
	CString cstr=_T("");
	CSize cs = GetVScrollInfo();
	char buffer[512];

	cp1 = (cpH1.y < cpH.y) ? cpH1 : cpH;
	cp2 = (cpH1.y < cpH.y) ? cpH : cpH1;
	cp1.y = (cp1.y)/YCHAR;
	cp2.y = (cp2.y)/YCHAR;
	if(cs.cx%YCHAR==0) {cp1.y--; cp2.y--;}
	cp1.x /= xChar; cp2.x /= xChar;
	x1 = min(cp1.x, cp2.x); x2 = max(cp1.x, cp2.x);
	if(cp1.y == cp2.y){
		n = (m_line0 + cp1.y)%TOTAL_BUFF_SIZE;
		USES_CONVERSION;
		strip_ansi(T2A(m_buff[n]), buffer);
		if(x2 <= strlen(buffer))
			buffer[x2]=0;
		cstr += (buffer+x1);
	}
	else {
		n = (m_line0 + cp1.y)%TOTAL_BUFF_SIZE;
		USES_CONVERSION;
		strip_ansi(T2A(m_buff[n]), buffer);
		cstr += (buffer+x1);
		while(cp1.y + 1 < cp2.y){
			n = (n+1)%TOTAL_BUFF_SIZE;
			USES_CONVERSION;
			strip_ansi(T2A(m_buff[n]), buffer);
			cstr += buffer;
			cp1.y++;
		}
		n = (m_line0 + cp2.y)%TOTAL_BUFF_SIZE;
		strip_ansi(T2A(m_buff[n]), buffer);
		if((unsigned)cp2.x <= strlen(buffer))
			buffer[cp2.x]=0;
		cstr += buffer;
	}

	HGLOBAL hg;
	TCHAR FAR *gb, *pt;

	hg = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, cstr.GetLength()+1);  // +1 for trailing zero

	if(hg == NULL)
	{
		CloseClipboard();
		return;
	}

	gb = (TCHAR *) GlobalLock(hg);   // we are in large model...

	if(gb == NULL)
	{
		CloseClipboard();
		return;
	}

	_tcscpy(gb, cstr);
	while((pt=_tcsstr(gb, _T("\r\n"))) && pt[2]!='\r'){
		memmove(pt, pt+sizeof(TCHAR), sizeof(TCHAR)*(_tcslen(pt)));
	}
	while(pt=_tcsstr(gb, _T("\n\r"))){ //"\n\r" means it is single line, broken due to w80
		memmove(pt, pt+2, sizeof(TCHAR)*(_tcslen(pt)-1));
	}
	if(gb[_tcslen(gb)-1]=='\n' || gb[_tcslen(gb)-1]=='\r') gb[_tcslen(gb)-1] = 0;
	// Set the data
	GlobalUnlock(hg);

	SetClipboardData(CF_TEXT, hg);
	CloseClipboard();

	bHighlight = FALSE;

	InvalidateHighlight();
	UpdateWindow();
}

void CTinView::CalcHighlightRgn(CRgn &rgn)
{
	CPoint cp1, cp2;
	CSize cs = GetVScrollInfo();
	CRect rect;
	CRgn rgn2;
	//  int dy = -cs.cx%YCHAR*yChar/YCHAR;

	cp1 = (cpH1.y < cpH.y) ? cpH1 : cpH;
	cp2 = (cpH1.y < cpH.y) ? cpH : cpH1;
	cp1.y = (cp1.y-cs.cx)*yChar/YCHAR;
	cp2.y = (cp2.y-cs.cx)*yChar/YCHAR;
	if(cp1.y == cp2.y){
		rect.top = cp1.y; rect.bottom = rect.top + yChar;
		rect.left = cp1.x; rect.right = cp2.x;
		rgn.CreateRectRgnIndirect(&rect);
	}
	else {
		rect.top = cp1.y; rect.bottom = rect.top + yChar;
		rect.left = cp1.x; rect.right = w80*xChar;
		rgn.CreateRectRgnIndirect(&rect);
		if(cp1.y + yChar < cp2.y){
			rect.left = 0; rect.right = w80*xChar;
			rect.top = cp1.y + yChar; rect.bottom = cp2.y;
			rgn2.CreateRectRgnIndirect(&rect);
			rgn.CombineRgn(&rgn, &rgn2, RGN_OR);
			rgn2.DeleteObject();
		}
		rect.top = cp2.y; rect.bottom = rect.top + yChar;
		rect.left = 0; rect.right = cp2.x;
		rgn2.CreateRectRgnIndirect(&rect);
		rgn.CombineRgn(&rgn, &rgn2, RGN_OR);
		rgn2.DeleteObject();
	}
}

void CTinView::InvalidateHighlight(CRgn* oldRgn)
{
	CRgn rgn;
	CalcHighlightRgn(rgn);
	if(oldRgn) InvalidateRgn(oldRgn);
	InvalidateRgn(&rgn);
	rgn.DeleteObject();
}

void CTinView::OnSize(UINT nType, int cx, int cy)
{
	CWnd ::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	GetClientRect(&crClientRect);
	b_newfont=1;
}

void CTinView::OnViewOptions()
{
	// TODO: Add your command handler code here
	COptionDlg dlg;
	dlg.DoModal();
}


void CTinView::OnViewBgColor()
{
	CYtinApp* pApp = (CYtinApp *)AfxGetApp();
	CColorDialog ccd(pApp->backcolor);

	if(ccd.DoModal() == IDOK)
	{
		pApp->backcolor = ccd.GetColor();

		CString cs;
		cs.Format(_T("%ld"), pApp->backcolor);
		pApp->WriteProfileString(_T("windows"), _T("backcolor"), cs);

		Invalidate();
	}
}

void CTinView::InitColor()
{
	int i, j, k;
	CString cstr;
	CWinApp *pApp = AfxGetApp();
	m_bProp0 = pApp->GetProfileInt(_T("AnsiColors"), _T("prop0"), 1);
	m_bProp1 = pApp->GetProfileInt(_T("AnsiColors"), _T("prop1"), 1);
	m_dim0 = pApp->GetProfileInt(_T("AnsiColors"), _T("dim0"), dim[0]);
	m_dim1 = pApp->GetProfileInt(_T("AnsiColors"), _T("dim1"), dim[1]);
	m_neg0 = pApp->GetProfileInt(_T("AnsiColors"), _T("neg0"), 0);
	m_neg1 = pApp->GetProfileInt(_T("AnsiColors"), _T("neg1"), 0);
	for(i=0; i<8; i++)
		for(j=0; j<2; j++)
			for(k=0; k<3; k++){
				cstr.Format(_T("%d%d%d"), j, i, k);
				rgb[j*8+i][k] = pApp->GetProfileInt(_T("AnsiColors"), cstr, COLOR[i][k]*dim[j]);
			}
}
