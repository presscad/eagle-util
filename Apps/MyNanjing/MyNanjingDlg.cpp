// MyNanjingDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MyNanjing.h"
#include "MyNanjingDlg.h"
#include <boost/process.hpp>
#include "Ini/SimpleIni.h"
#include "basic/FileUtil.h"
#include "common/common_utils.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


using namespace utils;


std::string GetIniPathName()
{
    std::string modulePathName = GetCurModulePathname();
    std::string curDir = GetCurDirectory();
    if (curDir.back() != '\\') {
        curDir += '\\';
    }
    std::string pathname = curDir + GetFileTitle(modulePathName.c_str()) + ".ini";
    return pathname;
}

// CMyNanjingDlg dialog

CMyNanjingDlg::CMyNanjingDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CMyNanjingDlg::IDD, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMyNanjingDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_STATUS, m_stcStatus);
    DDX_Control(pDX, IDC_EDIT1, m_edResult);
    DDX_Control(pDX, IDC_BTN_TASK1, m_btnTask1);
    DDX_Control(pDX, IDC_BTN_TASK2, m_btnTask2);
    DDX_Control(pDX, IDC_BTN_TASK3, m_btnTask3);
    DDX_Control(pDX, IDC_BTN_TASK4, m_btnTask4);
}

BEGIN_MESSAGE_MAP(CMyNanjingDlg, CDialog)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    //}}AFX_MSG_MAP
    ON_BN_CLICKED(IDC_BTN_TASK1, OnBnClickedBtnTask1)
    ON_BN_CLICKED(IDC_BTN_TASK2, OnBnClickedBtnTask2)
    ON_BN_CLICKED(IDC_BTN_TASK3, OnBnClickedBtnTask3)
    ON_BN_CLICKED(IDC_BTN_TASK4, OnBnClickedBtnTask4)
    ON_WM_TIMER()
END_MESSAGE_MAP()


// CMyNanjingDlg message handlers

BOOL CMyNanjingDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE);			// Set big icon
    SetIcon(m_hIcon, FALSE);		// Set small icon

    // Add extra initialization here
    ReadSettings();
    UpdateUiBySettings();
    OnBnClickedBtnTask_X(1);

    return TRUE;  // return TRUE  unless you set the focus to a control
}

BOOL CMyNanjingDlg::ReadSettings()
{
    m_tasks.resize(MAX_NUM_TASKS);

    std::string iniPath = GetIniPathName();
    if (!FileExists(iniPath.c_str())) {

        return FALSE;
    }

    CSimpleIni ini(iniPath.c_str());
    m_config.proxy = ini.GetString("Main", "proxy", "");
    m_config.userId = ini.GetString("Main", "userId", "<userId>");

    for (int i = 0; i < MAX_NUM_TASKS; i++)
    {
        Task &task = m_tasks.at(i);
        CStringA section;
        section.Format("Task-%d", i + 1);

        task.tag = ini.GetString(section, "tag", "<None>");
        if (task.tag != "<None>") {
            // TODO:
        }
    }

    return TRUE;
}

BOOL CMyNanjingDlg::UpdateUiBySettings()
{
    for (int i = 0; i < MAX_NUM_TASKS; i++) {
        UINT buttonid = 0;
        switch (i + 1) {
        case 1: buttonid = IDC_BTN_TASK1; break;
        case 2: buttonid = IDC_BTN_TASK2; break;
        case 3: buttonid = IDC_BTN_TASK3; break;
        case 4: buttonid = IDC_BTN_TASK4; break;
        default:
            ASSERT(FALSE);
            return FALSE;
        }
        this->GetDlgItem(buttonid)->SetWindowText(m_tasks[i].tag.c_str());
    }
    return TRUE;
}

void CMyNanjingDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CMyNanjingDlg::OnPaint()
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
HCURSOR CMyNanjingDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

BOOL CMyNanjingDlg::OnBnClickedBtnTask_X(int num)
{
    CWaitCursor wait;
    auto& task = m_tasks[num - 1];
    if (task.tag == "<None>") {
        return TRUE;
    }


    return TRUE;
}

void CMyNanjingDlg::OnBnClickedBtnTask1()
{
    OnBnClickedBtnTask_X(1);
}

void CMyNanjingDlg::OnBnClickedBtnTask2()
{
    OnBnClickedBtnTask_X(2);
}

void CMyNanjingDlg::OnBnClickedBtnTask3()
{
    OnBnClickedBtnTask_X(3);
}

void CMyNanjingDlg::OnBnClickedBtnTask4()
{
    OnBnClickedBtnTask_X(4);
}

void CMyNanjingDlg::OnTimer(UINT_PTR nIDEvent)
{
    CDialog::OnTimer(nIDEvent);
    CMyNanjingDlg::OnOK();
}
