// MyNanjingDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MyNanjing.h"
#include "MyNanjingDlg.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "common/simple_thread_pool.hpp"
#include "Ini/SimpleIni.h"
#include "basic/FileUtil.h"
#include "common/common_utils.h"
#include "net/curl_utils.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#define DEFAULT_INI_TEXT    \
    "[Main]\n" \
    "proxy =\n" \
    "userId =\n" \
    "[Task-1]\n" \
    "tag = Travel Info\n" \
    "url = http://58.213.141.220:10001/greentravel-api/getUserTravelInfo \n" \
    "method = POST\n" \
    "httpHeaders = content-type: application/x-www-form-urlencoded\n" \
    "postFields = userId=<userId>\n" \
    "[Task-2]\n" \
    "tag = Walk Credit\n" \
    "url = http://58.213.141.220:10001/greentravel-api/applyPointsByType \n" \
    "method = POST\n" \
    "httpHeaders = content-type: application/x-www-form-urlencoded\n" \
    "postFields = userId=<userId>&applyType=3\n" \
    "threadsNum = 4\n"


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
            task.url = ini.GetString(section, "url", "");
            task.method = ini.GetString(section, "method", "GET");

            string str = ini.GetString(section, "httpHeaders", "");
            if (!str.empty()) {
                util::ParseCsvLine(task.httpHeaders, str, '\n');
            }

            str = ini.GetString(section, "postFields", "");
            if (!str.empty()) {
                util::StringReplace(str, "<userId>", m_config.userId);
                util::ParseCsvLine(task.postFields, str, '\n');
            }

            task.threadsNum = ini.GetInt(section, "threadsNum", 1);
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

void CMyNanjingDlg::UpdateTaskButtons(const std::string& result_json)
{
    using namespace rapidjson;

    struct Flags {
        bool walk_avai;
        bool metro_avai;
        bool bus_avai;
        bool bicycle_avai;
    } flags{};

    if (!result_json.empty()) {
        Document document;
        document.Parse(result_json.c_str());
        if (document.IsObject() && document.HasMember("result") && document["result"].GetInt() == 0) {
            auto& data = document["data"];
            flags.walk_avai = data["bicycleApplyAvaliable"].GetBool();
            flags.metro_avai = data["metroApplyAvaliable"].GetBool();
            flags.bus_avai = data["busApplyAvaliable"].GetBool();
            flags.bicycle_avai = data["bicycleApplyAvaliable"].GetBool();
        }
    }

    auto get_button = [this](const string& text) {
        vector<CMFCButton*> btns = { &m_btnTask1, &m_btnTask2, &m_btnTask3, &m_btnTask4 };
        int i = 0;
        for (auto& task : this->m_tasks) {
            CString tag = task.tag.c_str(), t = text.c_str();
            tag.MakeLower();
            t.MakeLower();
            if (tag.Find(t) >= 0) {
                return btns[i];
            }

            ++i;
        }
        return (CMFCButton *)nullptr;
    };

    auto set_button_color = [this](CMFCButton *p_btn, bool available) {
        if (available) {
            p_btn->SetFaceColor(RGB(15, 128, 15), true);
        }
        else {
            p_btn->SetFaceColor(RGB(128, 128, 128), true);
        }
    };

    auto p_btn = get_button("Walk");
    if (p_btn) {
        set_button_color(p_btn, flags.walk_avai);
    }
    p_btn = get_button("Metro");
    if (p_btn) {
        set_button_color(p_btn, flags.metro_avai);
    }
    p_btn = get_button("Bus");
    if (p_btn) {
        set_button_color(p_btn, flags.bus_avai);
    }
    p_btn = get_button("Bicycle");
    if (p_btn) {
        set_button_color(p_btn, flags.bicycle_avai);
    }
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
    if (task.url.empty()) {
        return TRUE;
    }

    struct Data {
        int index{};
        string result_str;

        // parsed data
        int credit{};
    };
    vector<Data> threads_data;
    util::SimpleDataQueue<int> indices;
    for (int i = 0; i < task.threadsNum; ++i) {
        indices.Add(i);
        threads_data.push_back(Data());
        threads_data.back().index = i;
    }

    util::CreateSimpleThreadPool("dummy", (unsigned)task.threadsNum, [&task, &threads_data, &indices, this]() {
        while (true) {
            int index;
            if (false == indices.Get(index)) {
                break;
            }
            auto& data = threads_data[index];

            auto handle = net::GetCurlHandle(m_config.proxy);
            if (!task.method.empty()) {
                net::CurlSetMethod(handle, task.method);
            }
            for (auto& header : task.httpHeaders) {
                net::CurlAppendHeader(handle, header);
            }
            for (auto& field : task.postFields) {
                net::CurlAppendPostField(handle, field);
            }

            data.result_str = net::SendRequestAndReceive(handle, task.url);
        }
    }).JoinAll();

    using namespace rapidjson;
    int credit = 0;
    string result_json;
    for (auto& data : threads_data) {
        Document document;
        document.Parse(data.result_str.c_str());
        if (!document.IsObject()) {
            continue;
        }
        if (!document.HasMember("result")) {
            continue;
        }
        int result = document["result"].GetInt();
        if (result == 0) {
            data.credit = document["data"]["points"].GetInt();
        }

        if (credit < data.credit) {
            credit = data.credit;
            result_json = data.result_str;
        }
    }

    if (credit > 0) {
        string text = "Credit: " + to_string(credit);
        m_stcStatus.SetWindowTextA(text.c_str());

        if (!result_json.empty()) {
            Document document;
            document.Parse(result_json.c_str());

            StringBuffer buffer;
            PrettyWriter<StringBuffer> writer(buffer);
            document.Accept(writer);
            result_json = buffer.GetString();
            util::StringReplace(result_json, "\n", "\r\n");
            m_edResult.SetWindowTextA(result_json.c_str());

            UpdateTaskButtons(result_json);
        }
    }
    else {
        auto& data = threads_data.front(); // simply show the 1st one
        if (!data.result_str.empty()) {
            Document document;
            document.Parse(data.result_str.c_str());

            string text = string("Message: ") + document["message"].GetString();
            wstring w_text = util::StrToWStr(text);
            ::SetWindowTextW(m_stcStatus.GetSafeHwnd(), w_text.c_str());

            StringBuffer buffer;
            PrettyWriter<StringBuffer> writer(buffer);
            document.Accept(writer);
            string str = buffer.GetString();
            util::StringReplace(str, "\n", "\r\n");

            wstring w_str = util::StrToWStr(str);
            ::SetWindowTextW(m_edResult.GetSafeHwnd(), w_str.c_str());
        }
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
