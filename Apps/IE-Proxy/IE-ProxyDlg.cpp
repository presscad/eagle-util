// IE-ProxyDlg.cpp : implementation file
//

#include "stdafx.h"
#include "IE-Proxy.h"
#include "IE-ProxyDlg.h"
#include "Ini/SimpleIni.h"
#include "Net/InetUtil.h"
#include "Fs/FileUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#define WM_GET_PROXY_STATUS   WM_USER

#define DEFAULT_INI_TEXT    \
    "[proxy-1]\n" \
    "tag = wwwgate0\n" \
    "proxy = wwwgate0.mot.com\n" \
    "port = 1080\n" \
    "bypass = 127.0.0.1;*.mot.com;*.mot-mobility.com\n" \
    "\n" \
    "[proxy-2]\n" \
    "tag = wwwgate0-ch\n" \
    "proxy = wwwgate0-ch.mot.com\n" \
    "port = 1080\n" \
    "bypass = 127.0.0.1;*.mot.com;*.mot-mobility.com\n"


using namespace utils;


std::string GetIniPathName()
{
    std::string modulePathName = GetCurModulePathname();
    std::string pathname = GetFilePath(modulePathName.c_str()) + GetFileTitle(modulePathName.c_str()) + ".ini";
    return pathname;
}

// CIEProxyDlg dialog

CIEProxyDlg::CIEProxyDlg(CWnd* pParent /*=NULL*/)
: CDialog(CIEProxyDlg::IDD, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CIEProxyDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_STATUS, m_stcStatus);
}

BEGIN_MESSAGE_MAP(CIEProxyDlg, CDialog)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    //}}AFX_MSG_MAP
    ON_BN_CLICKED(IDC_BTN_NO_PROXY, OnBnClickedBtnNoProxy)
    ON_BN_CLICKED(IDC_BTN_PROXY1, OnBnClickedBtnProxy1)
    ON_BN_CLICKED(IDC_BTN_PROXY2, OnBnClickedBtnProxy2)
    ON_BN_CLICKED(IDC_BTN_PROXY3, OnBnClickedBtnProxy3)
    ON_BN_CLICKED(IDC_BTN_PROXY4, OnBnClickedBtnProxy4)
    ON_BN_CLICKED(IDC_BTN_SYS_PROXY_SETTING, OnBnClickedBtnSysProxySetting)
    ON_MESSAGE(WM_GET_PROXY_STATUS, OnGetProxyStatus) 
    ON_WM_TIMER()
END_MESSAGE_MAP()


// CIEProxyDlg message handlers
namespace utils {
extern void DoIniTest();
}

BOOL CIEProxyDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE);			// Set big icon
    SetIcon(m_hIcon, FALSE);		// Set small icon

    // TODO: Add extra initialization here
    m_stcStatus.SetWindowText(_T("Retrieving IE proxy settings..."));
    this->PostMessage(WM_GET_PROXY_STATUS);

    //DoIniTest();

    ReadSettings();
    UpdateUiBySettings();

    return TRUE;  // return TRUE  unless you set the focus to a control
}

BOOL CIEProxyDlg::ReadSettings()
{
    m_proxies.SetSize(MAX_NUM_PROXY);

    std::string iniPath = GetIniPathName();
    if (!FileExists(iniPath.c_str()))
    {
        FILE *fp = NULL;
        fopen_s(&fp, iniPath.c_str(), "wt");
        if (fp)
        {
            fwrite(DEFAULT_INI_TEXT, strlen(DEFAULT_INI_TEXT), 1, fp);
            fclose(fp);
        }
    }

    CSimpleIni ini;
    for (int i=0; i<MAX_NUM_PROXY; i++)
    {
        Proxy &proxy = m_proxies.GetAt(i);
        CStringA section;
        section.Format("proxy-%d", i+1);

        CStringA defaultTag = CStringA("Not set. Check ") + GetFileName(iniPath.c_str()).c_str();

        proxy.displayName = ini.GetString(section, "tag", defaultTag).c_str();
        proxy.proxy = ini.GetString(section, "proxy", "unset").c_str();
        proxy.port = ini.GetInt(section, "port", 0);
        proxy.bypass = ini.GetString(section, "bypass", "unset").c_str();
    }


    return TRUE;
}

BOOL CIEProxyDlg::UpdateUiBySettings()
{
    for (int i=0; i<MAX_NUM_PROXY; i++)
    {
        UINT buttonid = 0;
        switch (i+1)
        {
        case 1: buttonid = IDC_BTN_PROXY1; break;
        case 2: buttonid = IDC_BTN_PROXY2; break;
        case 3: buttonid = IDC_BTN_PROXY3; break;
        case 4: buttonid = IDC_BTN_PROXY4; break;
        default:
            ASSERT(FALSE);
            return FALSE;
        }
        this->GetDlgItem(buttonid)->SetWindowText(m_proxies[i].displayName);
    }
    return TRUE;
}

void CIEProxyDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        // CAboutDlg dlgAbout;
        // dlgAbout.DoModal();
    }
    else
    {
        CDialog::OnSysCommand(nID, lParam);
    }
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CIEProxyDlg::OnPaint() 
{
    if (IsIconic())
    {
        CPaintDC dc(this); // device context for painting

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialog::OnPaint();
    }
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CIEProxyDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}



BOOL CIEProxyDlg::OnBnClickedBtnProxy_X(int num)
{
    BOOL res;
    CWaitCursor wait;

    m_stcStatus.SetWindowText(_T("Changing IE proxy settings..."));

    CStringA strProxy;
    CStringA strBypass;
    unsigned int port;

    if (num == 0)
    {
        res = EnableIEProxy(FALSE);
    }
    else if (num > 0)
    {
        ReadSettings();

        const Proxy &proxy = m_proxies.GetAt(num-1);
        strProxy = proxy.proxy;
        if (strProxy != "unset")
        {
            strBypass = proxy.bypass;
            port = proxy.port;
            res = EnableIEProxy(TRUE, strProxy, port, strBypass);
        }
        else
        {
            CString inipath(GetIniPathName().c_str());
            ShellExecute(NULL, TEXT("open"), TEXT("notepad.exe"), inipath, NULL, SW_SHOWNORMAL);

            m_stcStatus.SetWindowText(inipath + _T(" is open. This program will be closed in few seconds."));
            this->SetTimer(1, 5000, NULL);
            res =  FALSE;
        }
    }

    if (res == TRUE)
    {
        m_stcStatus.SetWindowText(_T("New IE proxy is set successfully!\nThis program will be closed in few seconds."));
        this->SetTimer(1, 5000, NULL);
    }

    return res;
}

void CIEProxyDlg::OnBnClickedBtnNoProxy()
{
    OnBnClickedBtnProxy_X(0);
}

void CIEProxyDlg::OnBnClickedBtnProxy1()
{
    OnBnClickedBtnProxy_X(1);
}

void CIEProxyDlg::OnBnClickedBtnProxy2()
{
    OnBnClickedBtnProxy_X(2);
}

void CIEProxyDlg::OnBnClickedBtnProxy3()
{
    OnBnClickedBtnProxy_X(3);
}

void CIEProxyDlg::OnBnClickedBtnProxy4()
{
    OnBnClickedBtnProxy_X(4);
}

void CIEProxyDlg::OnBnClickedBtnSysProxySetting()
{
    ShowNetworkProxySettings();
}

LRESULT CIEProxyDlg::OnGetProxyStatus(WPARAM wParam, LPARAM lParam)
{
    bool bEnable;
    std::string proxy, byPass;
    BOOL res = GetIEProxy(bEnable, proxy, byPass);

    if (res)
    {
        CString status, proxyT(proxy.c_str()), byPassT(byPass.c_str());
        if (!bEnable)
            status = "IE proxy server is disabled.";
        else
            status.Format(_T("Current Proxy: %s"), proxyT); 
        m_stcStatus.SetWindowText(status);
    }

    return res;
}

void CIEProxyDlg::OnTimer(UINT nIDEvent)
{
    CDialog::OnTimer(nIDEvent);
    CIEProxyDlg::OnOK();
}
