// IE-ProxyDlg.h : header file
//

#pragma once
#include "afxwin.h"
#include <afxbutton.h>
#include <string>
#include <vector>


#define MAX_NUM_TASKS   4

using namespace std;

struct MainConfig
{
    string adb;
    vector<string> devices;
};

struct Task
{
    string tag;

};


// CMyNanjingDlg dialog
class CMyNanjingDlg : public CDialog
{
    // Construction
public:
    CMyNanjingDlg(CWnd* pParent = NULL);	// standard constructor

    // Dialog Data
    enum { IDD = IDD_MYNANJING_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


    // Implementation
protected:
    HICON m_hIcon;
    vector<Task> m_tasks;
    MainConfig m_config;

    BOOL OnBnClickedBtnTask_X(int num);
    BOOL ReadSettings();
    BOOL UpdateUiBySettings();

    // Generated message map functions
    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedBtnTask1();
    afx_msg void OnBnClickedBtnTask2();
    afx_msg void OnBnClickedBtnTask3();
    afx_msg void OnBnClickedBtnTask4();
    afx_msg void OnTimer(UINT_PTR nIDEvent);

    CStatic m_stcStatus;
    CEdit m_edResult;
    CMFCButton m_btnTask1;
    CMFCButton m_btnTask2;
    CMFCButton m_btnTask3;
    CMFCButton m_btnTask4;
};
