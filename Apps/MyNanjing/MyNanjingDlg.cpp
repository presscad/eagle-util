// MyNanjingDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MyNanjing.h"
#include "MyNanjingDlg.h"
#include "Ini/SimpleIni.h"
#include "basic/FileUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#define WM_GET_TRAVEL_INFO  WM_USER

#define DEFAULT_INI_TEXT    \
    "[Main]\n" \
    "proxy =\n" \
    "userId =\n" \
    "[TravelInfo]\n" \
    "url = http://58.213.141.220:10001/greentravel-api/getUserTravelInfo \n" \
    "method = POST" \
    "httpHeaders = content-type: application/x-www-form-urlencoded" \
    "postFields = userId=<userId>" \
    "[MetroCredit]\n" \
    "url = http://58.213.141.220:10001/greentravel-api/applyPointsByType \n" \
    "method = POST" \
    "httpHeaders = content-type: application/x-www-form-urlencoded" \
    "postFields = userId=<userId>&applyType=2" \
    "threadsNum = 4"


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
    ON_BN_CLICKED(IDC_BTN_TASK1, OnBnClickedBtnTask1)
    ON_BN_CLICKED(IDC_BTN_TASK2, OnBnClickedBtnTask2)
    ON_BN_CLICKED(IDC_BTN_TASK3, OnBnClickedBtnTask3)
    ON_BN_CLICKED(IDC_BTN_TASK4, OnBnClickedBtnTask4)
    ON_MESSAGE(WM_GET_TRAVEL_INFO, OnGetTravelInfo) 
    ON_WM_TIMER()
END_MESSAGE_MAP()


// CIEProxyDlg message handlers

BOOL CIEProxyDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE);			// Set big icon
    SetIcon(m_hIcon, FALSE);		// Set small icon

    // TODO: Add extra initialization here
    m_stcStatus.SetWindowText(_T("Retrieving IE proxy settings..."));
    this->PostMessage(WM_GET_TRAVEL_INFO);

    ReadSettings();
    UpdateUiBySettings();

    return TRUE;  // return TRUE  unless you set the focus to a control
}

BOOL CIEProxyDlg::ReadSettings()
{
    m_tasks.SetSize(MAX_NUM_TASKS);

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
    for (int i=0; i<MAX_NUM_TASKS; i++)
    {
        Task &proxy = m_tasks.GetAt(i);
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
    for (int i=0; i<MAX_NUM_TASKS; i++)
    {
        UINT buttonid = 0;
        switch (i+1)
        {
        case 1: buttonid = IDC_BTN_TASK1; break;
        case 2: buttonid = IDC_BTN_TASK2; break;
        case 3: buttonid = IDC_BTN_TASK3; break;
        case 4: buttonid = IDC_BTN_TASK4; break;
        default:
            ASSERT(FALSE);
            return FALSE;
        }
        this->GetDlgItem(buttonid)->SetWindowText(m_tasks[i].displayName);
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



BOOL CIEProxyDlg::OnBnClickedBtnTask_X(int num)
{
    BOOL res = TRUE;
    CWaitCursor wait;

    m_stcStatus.SetWindowText(_T("Changing IE proxy settings..."));
    return res;
}

void CIEProxyDlg::OnBnClickedBtnTask1()
{
    OnBnClickedBtnTask_X(1);
}

void CIEProxyDlg::OnBnClickedBtnTask2()
{
    OnBnClickedBtnTask_X(2);
}

void CIEProxyDlg::OnBnClickedBtnTask3()
{
    OnBnClickedBtnTask_X(3);
}

void CIEProxyDlg::OnBnClickedBtnTask4()
{
    OnBnClickedBtnTask_X(4);
}

LRESULT CIEProxyDlg::OnGetTravelInfo(WPARAM wParam, LPARAM lParam)
{
    BOOL res = TRUE;
    return res;
}

void CIEProxyDlg::OnTimer(UINT_PTR nIDEvent)
{
    CDialog::OnTimer(nIDEvent);
    CIEProxyDlg::OnOK();
}
