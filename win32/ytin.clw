; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CTinView
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "ytin.h"
LastPage=0

ClassCount=8
Class1=CYtinApp
Class3=CMainFrame
Class4=CAboutDlg

ResourceCount=14
Resource1=IDD_ABOUTBOX (English (U.S.))
Resource2=IDD_VIEW_OPTIONS (English (U.S.))
Class2=CTinView
Resource3=IDR_MAINFRAME
Resource4=IDD_ANSICOLOR (English (U.S.))
Resource5=IDD_INPUTBAR (English (U.S.))
Resource6=IDD_CONNECT (English (U.S.))
Resource7=IDD_VIEW_OPTIONS
Class5=COptionDlg
Resource8=IDD_PASSWD
Class6=CDlgConnect
Resource9=IDD_INPUTBAR
Class7=CPasswdDlg
Resource10=IDD_PASSWD (English (U.S.))
Class8=CAnsiDlg
Resource11=IDR_MAINFRAME (English (U.S.))
Resource12=IDD_ABOUTBOX
Resource13=IDD_CONNECT
Resource14=IDD_ANSICOLOR

[CLS:CYtinApp]
Type=0
HeaderFile=ytin.h
ImplementationFile=ytin.cpp
Filter=N
LastObject=CYtinApp
BaseClass=CWinApp
VirtualFilter=AC

[CLS:CTinView]
Type=0
HeaderFile=tinView.h
ImplementationFile=tinView.cpp
Filter=W
BaseClass=CWnd 
VirtualFilter=WC
LastObject=CTinView

[CLS:CMainFrame]
Type=0
HeaderFile=MainFrm.h
ImplementationFile=MainFrm.cpp
Filter=T
LastObject=ID_HELP_VERSIONUPDATE
BaseClass=CFrameWnd
VirtualFilter=fWC




[CLS:CAboutDlg]
Type=0
HeaderFile=ytin.cpp
ImplementationFile=ytin.cpp
Filter=D
LastObject=CAboutDlg

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=5
Control1=IDC_STATIC,static,1342177283
Control2=IDC_VERSION,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889
Control5=IDC_STATIC,static,1342308352

[MNU:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_FILE_BOSS2
Command2=ID_FILE_TOGGLERAW
Command3=ID_APP_NEW_WINDOW
Command4=ID_FILE_CONNECT
Command5=ID_APP_EXIT
Command6=ID_EDIT_COPY
Command7=ID_VIEW_TOOLBAR
Command8=ID_VIEW_STATUS_BAR
Command9=ID_VIEW_FONT
Command10=ID_VIEW_BG_COLOR
Command11=ID_VIEW_ANSICOLOR
Command12=ID_VIEW_OPTIONS
Command13=ID_HELP_ONLINE
Command14=ID_HELP_VERSIONUPDATE
Command15=ID_HELP_HOMEPAGE
Command16=ID_APP_ABOUT
CommandCount=16

[ACL:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_EDIT_COPY
Command2=ID_APP_NEW_WINDOW
Command3=ID_FILE_BOSS
Command4=ID_VIEW_PAGEDOWN
Command5=ID_VIEW_PAGEUP
Command6=ID_TAB_COMPLETE
CommandCount=6

[TB:IDR_MAINFRAME (English (U.S.))]
Type=1
Class=?
Command1=ID_EDIT_COPY
Command2=ID_APP_ABOUT
CommandCount=2

[MNU:IDR_MAINFRAME (English (U.S.))]
Type=1
Class=CMainFrame
Command1=ID_FILE_BOSS2
Command2=ID_FILE_TOGGLERAW
Command3=ID_APP_NEW_WINDOW
Command4=ID_FILE_CONNECT
Command5=ID_APP_EXIT
Command6=ID_EDIT_COPY
Command7=ID_VIEW_TOOLBAR
Command8=ID_VIEW_STATUS_BAR
Command9=ID_VIEW_FONT
Command10=ID_VIEW_BG_COLOR
Command11=ID_VIEW_ANSICOLOR
Command12=ID_VIEW_OPTIONS
Command13=ID_HELP_ONLINE
Command14=ID_HELP_VERSIONUPDATE
Command15=ID_HELP_HOMEPAGE
Command16=ID_APP_ABOUT
CommandCount=16

[ACL:IDR_MAINFRAME (English (U.S.))]
Type=1
Class=CMainFrame
Command1=ID_EDIT_COPY
Command2=ID_APP_NEW_WINDOW
Command3=ID_FILE_BOSS
Command4=ID_VIEW_PAGEDOWN
Command5=ID_VIEW_PAGEUP
Command6=ID_TAB_COMPLETE
CommandCount=6

[DLG:IDD_ABOUTBOX (English (U.S.))]
Type=1
Class=CAboutDlg
ControlCount=5
Control1=IDC_STATIC,static,1342177283
Control2=IDC_VERSION,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889
Control5=IDC_STATIC,static,1342308352

[TB:IDR_MAINFRAME]
Type=1
Class=?
Command1=ID_EDIT_COPY
Command2=ID_APP_ABOUT
CommandCount=2

[DLG:IDD_INPUTBAR]
Type=1
Class=?
ControlCount=1
Control1=IDOK,button,1073741825

[DLG:IDD_INPUTBAR (English (U.S.))]
Type=1
Class=?
ControlCount=1
Control1=IDOK,button,1073741825

[DLG:IDD_VIEW_OPTIONS (English (U.S.))]
Type=1
Class=COptionDlg
ControlCount=25
Control1=IDC_BUFF_SIZE,edit,1350631552
Control2=IDC_MAX_WIDTH,edit,1350631552
Control3=IDC_EXTRA_X,edit,1350631552
Control4=IDC_EXTRA_Y,edit,1350631552
Control5=IDC_FONT_WEIGHT,edit,1350631552
Control6=IDOK,button,1342242817
Control7=IDCANCEL,button,1342242816
Control8=IDC_STATIC,static,1342308352
Control9=IDC_STATIC,static,1342308352
Control10=IDC_STATIC,static,1342308352
Control11=IDC_STATIC,static,1342308352
Control12=IDC_STATIC,static,1342308352
Control13=IDC_ENABLE_ANSI,button,1342242819
Control14=IDC_TABSIZE,edit,1350631552
Control15=IDC_STATIC,static,1342308352
Control16=IDC_WINTITLE_BOSS,edit,1350631552
Control17=IDC_STATIC,static,1342308352
Control18=IDC_STATIC,static,1342308352
Control19=IDC_CR_LF,edit,1350631552
Control20=IDC_BOLD_AS_HIGH,button,1342308361
Control21=IDC_BOLD_AS_HIGH2,button,1342177289
Control22=IDC_STATIC,button,1342177287
Control23=IDC_STATIC,static,1342308352
Control24=IDC_PERLDLL,edit,1350631552
Control25=IDC_KEEPLASTLINE,button,1342242819

[DLG:IDD_VIEW_OPTIONS]
Type=1
Class=COptionDlg
ControlCount=25
Control1=IDC_BUFF_SIZE,edit,1350631552
Control2=IDC_MAX_WIDTH,edit,1350631552
Control3=IDC_EXTRA_X,edit,1350631552
Control4=IDC_EXTRA_Y,edit,1350631552
Control5=IDC_FONT_WEIGHT,edit,1350631552
Control6=IDOK,button,1342242817
Control7=IDCANCEL,button,1342242816
Control8=IDC_STATIC,static,1342308352
Control9=IDC_STATIC,static,1342308352
Control10=IDC_STATIC,static,1342308352
Control11=IDC_STATIC,static,1342308352
Control12=IDC_STATIC,static,1342308352
Control13=IDC_ENABLE_ANSI,button,1342242819
Control14=IDC_TABSIZE,edit,1350631552
Control15=IDC_STATIC,static,1342308352
Control16=IDC_WINTITLE_BOSS,edit,1350631552
Control17=IDC_STATIC,static,1342308352
Control18=IDC_STATIC,static,1342308352
Control19=IDC_CR_LF,edit,1350631552
Control20=IDC_BOLD_AS_HIGH,button,1342308361
Control21=IDC_BOLD_AS_HIGH2,button,1342177289
Control22=IDC_STATIC,button,1342177287
Control23=IDC_STATIC,static,1342308352
Control24=IDC_PERLDLL,edit,1350631552
Control25=IDC_KEEPLASTLINE,button,1342242819

[CLS:COptionDlg]
Type=0
HeaderFile=options.h
ImplementationFile=options.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=IDC_KEEPLASTLINE

[DLG:IDD_CONNECT (English (U.S.))]
Type=1
Class=CDlgConnect
ControlCount=6
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_STATIC,static,1342308352
Control4=IDC_STATIC,static,1342308352
Control5=IDC_EDIT_SESSION,edit,1350631552
Control6=IDC_EDIT_IP,edit,1350631552

[CLS:CDlgConnect]
Type=0
HeaderFile=ytin.h
ImplementationFile=ytin.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=CDlgConnect

[DLG:IDD_PASSWD (English (U.S.))]
Type=1
Class=CPasswdDlg
ControlCount=3
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_PASSWD,edit,1350631584

[CLS:CPasswdDlg]
Type=0
HeaderFile=PasswdDlg.h
ImplementationFile=PasswdDlg.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=CPasswdDlg

[DLG:IDD_ANSICOLOR (English (U.S.))]
Type=1
Class=CAnsiDlg
ControlCount=36
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_COLOR0,static,1342181650
Control4=IDC_STATIC,static,1342308352
Control5=IDC_STATIC,static,1342308352
Control6=IDC_STATIC,static,1342308352
Control7=IDC_STATIC,static,1342308352
Control8=IDC_STATIC,static,1342308352
Control9=IDC_STATIC,static,1342308352
Control10=IDC_STATIC,static,1342308352
Control11=IDC_STATIC,static,1342308352
Control12=IDC_COLOR1,static,1342181650
Control13=IDC_COLOR2,static,1342181650
Control14=IDC_COLOR3,static,1342181650
Control15=IDC_COLOR4,static,1342181650
Control16=IDC_COLOR5,static,1342181650
Control17=IDC_COLOR6,static,1342181650
Control18=IDC_COLOR7,static,1342181650
Control19=IDC_COLOR8,static,1342181650
Control20=IDC_COLOR9,static,1342181650
Control21=IDC_COLOR10,static,1342181650
Control22=IDC_COLOR11,static,1342181650
Control23=IDC_COLOR12,static,1342181650
Control24=IDC_COLOR13,static,1342181650
Control25=IDC_COLOR14,static,1342181650
Control26=IDC_COLOR15,static,1342181650
Control27=IDC_CHECK0,button,1342242819
Control28=IDC_CHECK1,button,1342242819
Control29=IDC_EDIT0,edit,1350631552
Control30=IDC_EDIT1,edit,1350631552
Control31=IDC_STATIC,button,1342177287
Control32=IDC_TEST,button,1342242816
Control33=IDC_EDIT2,edit,1350631552
Control34=IDC_EDIT3,edit,1350631552
Control35=IDC_STATIC,static,1342308352
Control36=IDC_STATIC,static,1342308352

[CLS:CAnsiDlg]
Type=0
HeaderFile=AnsiDlg.h
ImplementationFile=AnsiDlg.cpp
BaseClass=CDialog
Filter=W
VirtualFilter=dWC
LastObject=CAnsiDlg

[DLG:IDD_CONNECT]
Type=1
Class=?
ControlCount=6
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_STATIC,static,1342308352
Control4=IDC_STATIC,static,1342308352
Control5=IDC_EDIT_SESSION,edit,1350631552
Control6=IDC_EDIT_IP,edit,1350631552

[DLG:IDD_PASSWD]
Type=1
Class=?
ControlCount=3
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_PASSWD,edit,1350631584

[DLG:IDD_ANSICOLOR]
Type=1
Class=?
ControlCount=36
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_COLOR0,static,1342181650
Control4=IDC_STATIC,static,1342308352
Control5=IDC_STATIC,static,1342308352
Control6=IDC_STATIC,static,1342308352
Control7=IDC_STATIC,static,1342308352
Control8=IDC_STATIC,static,1342308352
Control9=IDC_STATIC,static,1342308352
Control10=IDC_STATIC,static,1342308352
Control11=IDC_STATIC,static,1342308352
Control12=IDC_COLOR1,static,1342181650
Control13=IDC_COLOR2,static,1342181650
Control14=IDC_COLOR3,static,1342181650
Control15=IDC_COLOR4,static,1342181650
Control16=IDC_COLOR5,static,1342181650
Control17=IDC_COLOR6,static,1342181650
Control18=IDC_COLOR7,static,1342181650
Control19=IDC_COLOR8,static,1342181650
Control20=IDC_COLOR9,static,1342181650
Control21=IDC_COLOR10,static,1342181650
Control22=IDC_COLOR11,static,1342181650
Control23=IDC_COLOR12,static,1342181650
Control24=IDC_COLOR13,static,1342181650
Control25=IDC_COLOR14,static,1342181650
Control26=IDC_COLOR15,static,1342181650
Control27=IDC_CHECK0,button,1342242819
Control28=IDC_CHECK1,button,1342242819
Control29=IDC_EDIT0,edit,1350631552
Control30=IDC_EDIT1,edit,1350631552
Control31=IDC_STATIC,button,1342177287
Control32=IDC_TEST,button,1342242816
Control33=IDC_EDIT2,edit,1350631552
Control34=IDC_EDIT3,edit,1350631552
Control35=IDC_STATIC,static,1342308352
Control36=IDC_STATIC,static,1342308352

