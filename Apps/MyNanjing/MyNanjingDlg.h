// IE-ProxyDlg.h : header file
//

#pragma once
#include "afxwin.h"


#define MAX_NUM_PROXY   4

struct Proxy
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
    enum { IDD = IDD_IEPROXY_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


    // Implementation
protected:
    HICON m_hIcon;
    CStatic m_stcStatus;
    CArray<Proxy> m_proxies;

    BOOL OnBnClickedBtnProxy_X(int num);
    BOOL ReadSettings();
    BOOL UpdateUiBySettings();

    // Generated message map functions
    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg LRESULT OnGetProxyStatus(WPARAM wParam, LPARAM lParam); 
    DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedBtnNoProxy();
    afx_msg void OnBnClickedBtnProxy1();
    afx_msg void OnBnClickedBtnProxy2();
    afx_msg void OnBnClickedBtnProxy3();
    afx_msg void OnBnClickedBtnProxy4();
    afx_msg void OnBnClickedBtnSysProxySetting();
    afx_msg void OnTimer(UINT nIDEvent);
};
