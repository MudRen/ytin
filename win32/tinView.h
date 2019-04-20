// tinView.h : interface of the CTinView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_TINVIEW_H__F524618A_A5A7_11D3_8359_00105A9F5B94__INCLUDED_)
#define AFX_TINVIEW_H__F524618A_A5A7_11D3_8359_00105A9F5B94__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CTinView window

class CTinView : public CWnd
{
// Construction
public:
	CTinView();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTinView)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	void FormatAbsoluteColor(CString &cs, int colorset);
	void SetFontAttrib(CDC &dc, int colorset);
	int PreTextOut(int x, TCHAR * str);
	int AnsiTextOut(int x, int y, TCHAR* str, int real=1);
	virtual ~CTinView();

public:
	void InvalidateHighlight(CRgn* oldRgn=NULL);
	void CalcHighlightRgn(CRgn &rgn);
	CSize GetVScrollInfo();
	void ScrollToBottom();
	void SetInputFocus(void);

public:
	void InitColor();
	int xPos, yPos;
	CString jumpstr; //we buffer the output, do jump scroll
	int b_scroll; //vertical autoscroll?
	BOOL b_newfont;	//indicate if font is changed or not
	BOOL b_ansi; //toggle if enable ansi
private:
	int TOTAL_BUFF_SIZE;
	int* lpDxWidth;
	HFONT hFont, hFontB;    // Font handle to use when writing to screen, hFontB is bold
	int xChar, yChar;		// Width and height of one character
	int YCHAR;					//logical height of a char, SV_MAX/TOTAL_BUFF_SIZE
	CRect crClientRect;		// Client rectangle dimensions, kept up to date         
	BOOL bHighlight;		// nonzero if text currently highlighted 
	BOOL bMouseTracking;    // nonzero if currently tracking mouse
	CPoint cpH1, cpH2;		// Starting and ending points of highlighted text (logical co-ords)
	CPoint cpH;		// Last recorded mouse position
	int nTimer;				// Timer number (nonzero if active)
	int m_n_extra_x, m_n_extra_y; //extra spacing for chars
	int m_line;			//current line
	int m_line0;		//the relative location of last line to m_line0
	int v_lines;		//size of current virtual_buffer
	int m_done;			//if the line is finished, if not, will not overwrite m_buff[m_line]
	int b_beep;
	TCHAR** m_buff;
	int bCheck2Byte;			//toggle if check double byte char(>160 <255), e.g., chinese
	int TABSIZE;
	int IRow;
	int w80, W80;				//width/max_width of the screen, default to 80
	int x80;            //w80 * xChar, wrap at this width of pixels indeed
	int m_bBoldAsHigh;  // treat ^[[1m as bold or high color?
	int fgColor[2], bgColor[2], bBold[2], bHighcolor[2], bHighcolor2[2];
	int m_bProp0, m_bProp1;
	int m_dim0, m_dim1, m_neg0, m_neg1;
	int rgb[16][3];
	SIZE sizeTotal;

	// Generated message map functions
protected:
	void EditCopy(BOOL forceclear);
	//{{AFX_MSG(CTinView)
	afx_msg void OnPaint();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnViewFont();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnEditCopy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnViewOptions();
	afx_msg void OnViewPageup();
	afx_msg void OnViewPagedown();
	afx_msg void OnViewBgColor();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TINVIEW_H__F524618A_A5A7_11D3_8359_00105A9F5B94__INCLUDED_)
