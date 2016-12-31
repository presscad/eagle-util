// MyNanjingDlg.cpp : implementation file
//

#include "stdafx.h"

#define _SCL_SECURE_NO_WARNINGS // supress warning C4996 in debug build
#include "MyNanjing.h"
#include "MyNanjingDlg.h"
#include "Ini/SimpleIni.h"
#include "basic/FileUtil.h"
#include "common/common_utils.h"
#include <boost/process.hpp>
#include <boost/iostreams/stream.hpp>
#include <SQLiteCpp/SQLiteCpp.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


using namespace std;

using namespace boost::process;
using namespace boost::process::initializers;
using namespace boost::iostreams;

static const string ADB = "nox_adb";


std::string GetIniPathName()
{
    std::string modulePathName = utils::GetCurModulePathname();
    std::string curDir = utils::GetCurDirectory();
    if (curDir.back() != '\\') {
        curDir += '\\';
    }
    std::string pathname = curDir + utils::GetFileTitle(modulePathName.c_str()) + ".ini";
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
    ON_WM_CLOSE()
    ON_WM_DESTROY()
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
    if (!utils::FileExists(iniPath.c_str())) {
        m_stcStatus.SetWindowTextA(("INI error: " + iniPath + " does not exist").c_str());
        return FALSE;
    }

    utils::CSimpleIni ini(iniPath.c_str());
    m_config.adb = ini.GetString("Main", "adb", "");
    m_config.devices = util::StringSplit(ini.GetString("Main", "devices", ""), ',');
    if (m_config.devices.empty()) {
        m_stcStatus.SetWindowTextA("INI error: invalid devices in main section");
        return FALSE;
    }

    for (int i = 0; i < MAX_NUM_TASKS; i++) {
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

void CMyNanjingDlg::OnTimer(UINT_PTR nIDEvent)
{
    CDialog::OnTimer(nIDEvent);
    CMyNanjingDlg::OnOK();
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

static string ExecuteCmd(string exe, string cmd)
{
    CWaitCursor wait;
    const string std_out_name = "~stdout.txt";

    try {
        file_descriptor_sink sink_out(std_out_name);
        child c = execute(
            run_exe(exe),
            start_in_dir("."),
            bind_stdout(sink_out),
            bind_stderr(sink_out),
#ifdef _WIN32
            show_window(SW_HIDE),
#endif
            set_cmd_line(cmd)
        );
        if (c.process_handle()) {
            wait_for_exit(c);
        }
        else {
            return "error: invalid process handle returned";
        }
    }
    catch (const exception& e) {
        return string("error: ") + e.what();
    }

    string std_out;
    util::ReadAllFromFile(std_out_name, std_out);
    util::Rm(std_out_name, false);
    return std_out;
}

static void AppendTextToEditCtrl(CEdit& edit, string text)
{
    util::StringReplace(text, "\n", "\r\n");

    // add CR/LF to text
    CString strLine;
    strLine.Format(_T("%s\r\n"), text.c_str());

    // get the initial text length
    int nLength = edit.GetWindowTextLength();
    // put the selection at the end of text
    edit.SetSel(nLength, nLength);
    // replace the selection
    edit.ReplaceSel(strLine);
}

static void ClearEditCtrl(CEdit& edit)
{
    edit.SetWindowTextA("");
    AppendTextToEditCtrl(edit, "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
    edit.SetWindowTextA("");
}

// ADB Connect ...
void CMyNanjingDlg::OnBnClickedBtnTask1()
{
    ClearEditCtrl(m_edResult);

    string cmd = ADB + " kill-server";
    AppendTextToEditCtrl(m_edResult, "> " + cmd);
    string response = ExecuteCmd(m_config.adb, cmd);
    AppendTextToEditCtrl(m_edResult, response);

    for (auto device : m_config.devices) {
        cmd = ADB + " connect " + device;
        AppendTextToEditCtrl(m_edResult, "> " + cmd);
        response = ExecuteCmd(m_config.adb, cmd);
        AppendTextToEditCtrl(m_edResult, response);
    }
}

static string TodayAsStr()
{
    auto ts = util::GetCurTimestamp();
    CStringA str;
    str.Format("%04d-%02u-%02u", ts.year, ts.month, ts.day);
    return (const char *)str;
}

static string RandomStepsStr()
{
    std::srand((unsigned)std::time(0));
    return to_string(21000 + std::rand() % 5000);
}

static string UpdateTodayStep(string db_pathname)
{
    string today_str = TodayAsStr();
    int n_steps = -1;

    try {
        SQLite::Database db(db_pathname, SQLite::OPEN_READWRITE);

        try {
            auto col_steps = db.execAndGet("SELECT steps FROM stepinfo WHERE curdate=\'" + today_str + "\'");
            if (!col_steps.isNull()) {
                n_steps = col_steps.getInt();
            }
        }
        catch (std::exception&) {} // no rows case

        if (-1 == n_steps) {
            // insert new entry
            string sql = string("INSERT INTO stepinfo (steps, curdate) VALUES (") + RandomStepsStr() + ", \'" + today_str + "\')";
            db.exec(sql);
        }
        else if (n_steps < 20000) {
            // update existing entry
            string sql = "UPDATE stepinfo SET steps = " + RandomStepsStr() + " WHERE curdate = '" + today_str + "\'";
            db.exec(sql);
        }
    }
    catch (std::exception& e) {
        return string("error: ") + e.what();
    }
    return "";
}

// copy db to local, modify and copy to device
void CMyNanjingDlg::OnBnClickedBtnTask2()
{
    ClearEditCtrl(m_edResult);
    string cmd = ADB + " -s " + m_config.devices.front() + " pull /data/data/com.hoperun.intelligenceportal/databases/step.db ~step.db";
    AppendTextToEditCtrl(m_edResult, "> " + cmd);
    string response = ExecuteCmd(m_config.adb, cmd);
    AppendTextToEditCtrl(m_edResult, response);
    if (response.find("error:") != string::npos) {
        return;
    }

    AppendTextToEditCtrl(m_edResult, "> UpdateTodayStep ...");
    response = UpdateTodayStep("~step.db");
    AppendTextToEditCtrl(m_edResult, response);
    if (response.find("error:") != string::npos) {
        return;
    }

    for (auto device : m_config.devices) {
        string cmd = ADB + " -s " + device + " push ~step.db /data/data/com.hoperun.intelligenceportal/databases/step.db";
        AppendTextToEditCtrl(m_edResult, "> " + cmd);
        string response = ExecuteCmd(m_config.adb, cmd);
        AppendTextToEditCtrl(m_edResult, response);
    }

    util::Rm("~step.db", false);
}

void CMyNanjingDlg::OnBnClickedBtnTask3()
{
    OnBnClickedBtnTask_X(3);
}

static boost::process::pipe create_async_pipe()
{
#if defined(BOOST_WINDOWS_API)
    std::string name = "\\\\.\\pipe\\boost_process_async_io";
    HANDLE handle1 = ::CreateNamedPipeA(name.c_str(), PIPE_ACCESS_INBOUND |
        FILE_FLAG_OVERLAPPED, 0, 1, 8192, 8192, 0, NULL);
    HANDLE handle2 = ::CreateFileA(name.c_str(), GENERIC_WRITE, 0, NULL,
        OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    return make_pipe(handle1, handle2);
#elif defined(BOOST_POSIX_API)
    return create_pipe();
#endif
}

// click the "Apply" button, last step
void CMyNanjingDlg::OnBnClickedBtnTask4()
{
    for (auto device : m_config.devices) {
        string cmd = ADB + " -s " + device + " shell input tap 246 236";

        boost::process::pipe p = create_async_pipe();
        execute(
            run_exe(m_config.adb),
#ifdef _WIN32
            show_window(SW_HIDE),
#endif
            set_cmd_line(cmd)
        );
    }
}

void CMyNanjingDlg::OnDestroy()
{
    CDialog::OnDestroy();

    ExecuteCmd(m_config.adb, ADB + " kill-server");
    util::Rm("~step.db", false);
}
