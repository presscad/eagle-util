// IE-ProxyDlg.h : header file
//

#pragma once
#include "afxwin.h"


#define MAX_NUM_TASKS   4

struct Task
{
    CString displayName;
    CString proxy;
    UINT    port;
    CString bypass;
};

// CIEProxyDlg dialog
class CIEProxyDlg : public CDialog
{
    // Construction
public:
    CIEProxyDlg(CWnd* pParent = NULL);	// standard constructor

    // Dialog Data
    enum { IDD = IDD_MYNANJING_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


    // Implementation
protected:
    HICON m_hIcon;
    CStatic m_stcStatus;
    CArray<Task> m_tasks;

    BOOL OnBnClickedBtnTask_X(int num);
    BOOL ReadSettings();
    BOOL UpdateUiBySettings();

    // Generated message map functions
    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg LRESULT OnGetTravelInfo(WPARAM wParam, LPARAM lParam); 
    DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedBtnTask1();
    afx_msg void OnBnClickedBtnTask2();
    afx_msg void OnBnClickedBtnTask3();
    afx_msg void OnBnClickedBtnTask4();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
};
