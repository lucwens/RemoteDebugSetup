#pragma once

#include "../Common/LogUtils.h"
#include "../Common/RegistryBackup.h"

class CSetupTestDlg : public CDialogEx
{
public:
    CSetupTestDlg(CWnd* pParent = nullptr);

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_SETUPTEST_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    HICON m_hIcon;

    // DDX Controls
    CStatic   m_staticVPNStatus;
    CStatic   m_staticDebuggerStatus;
    CEdit     m_editDevHostname;
    CEdit     m_editDevVPNIP;
    CEdit     m_editShareName;
    CEdit     m_editPassword;
    CComboBox m_comboDriveLetter;
    CEdit     m_editDebuggerPort;
    CEdit     m_editLog;

    // DDX Data
    CString m_strDevHostname;
    CString m_strDevVPNIP;
    CString m_strShareName;
    CString m_strPassword;
    CString m_strDebuggerPort;

    // Utility objects
    CLogUtils       m_log;
    CRegistryBackup m_backup;

    // Internal state
    static const int TOTAL_SETUP_STEPS = 8;
    static const int TOTAL_RESTORE_STEPS = 5;

    // Event handlers
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnBnClickedInstallVPN();
    afx_msg void OnBnClickedInstallDebugger();
    afx_msg void OnBnClickedSetup();
    afx_msg void OnBnClickedRestore();

    // Prerequisite checks
    void CheckPrerequisites();
    void DetectVPNStatus();
    void DetectDebuggerStatus();

    // Validation
    bool ValidateInputs();
    TCHAR GetSelectedDriveLetter();

    // Setup steps
    bool StepCreateRDAccount();
    bool StepAddRDToAdmins();
    bool StepCreateDebuggerFirewallRule();
    bool StepVerifyConnectivity();
    bool StepMapSharedFolder();
    bool StepVerifyMappedDrive();
    bool StepLocateRemoteDebugger();
    void StepDisplaySummary();

    // Restore steps
    void RestoreMappedDrive();
    void RestoreDebuggerFirewallRule();
    void RestoreRDFromAdmins();
    void RestoreRDAccount();

    DECLARE_MESSAGE_MAP()
};
