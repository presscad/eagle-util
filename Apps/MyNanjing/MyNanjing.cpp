// MyNanjing.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "MyNanjing.h"
#include "MyNanjingDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CIEProxyApp

BEGIN_MESSAGE_MAP(CIEProxyApp, CWinApp)
END_MESSAGE_MAP()


// CIEProxyApp construction

CIEProxyApp::CIEProxyApp()
{
    // TODO: add construction code here,
    // Place all significant initialization in InitInstance
}


// The one and only CIEProxyApp object

CIEProxyApp theApp;


// CIEProxyApp initialization

BOOL CIEProxyApp::InitInstance()
{
    // InitCommonControls() is required on Windows XP if an application
    // manifest specifies use of ComCtl32.dll version 6 or later to enable
    // visual styles.  Otherwise, any window creation will fail.
    InitCommonControls();

    CWinApp::InitInstance();

    AfxEnableControlContainer();

    CMyNanjingDlg dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();

    // Since the dialog has been closed, return FALSE so that we exit the
    //  application, rather than start the application's message pump.
    return FALSE;
}
