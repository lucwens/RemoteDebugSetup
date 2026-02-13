#pragma once

#include "../Common/LogUtils.h"
#include "../Common/RegistryBackup.h"

class CSetupDevelopDlg : public CDialogEx
{
public:
    CSetupDevelopDlg(CWnd* pParent = nullptr);

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_SETUPDEVELOP_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    HICON m_hIcon;

    // DDX Controls
    CStatic m_staticVPNStatus;
    CEdit   m_editTVID;
    CEdit   m_editPassword;
    CEdit   m_editSharePath;
    CEdit   m_editShareName;
    CEdit   m_editVPNSubnet;
    CRichEditCtrl m_editLog;

    // DDX Data
    CString m_strTVID;
    CString m_strPassword;
    CString m_strSharePath;
    CString m_strShareName;
    CString m_strVPNSubnet;

    // Utility objects
    CLogUtils       m_log;
    CRegistryBackup m_backup;

    // Internal state
    static const int TOTAL_SETUP_STEPS = 12;
    static const int TOTAL_RESTORE_STEPS = 10;

    // Event handlers
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnBnClickedConnectVPN();
    afx_msg void OnBnClickedBrowse();
    afx_msg void OnBnClickedSetup();
    afx_msg void OnBnClickedRestore();

    // Setup step methods
    void DetectVPNStatus();
    bool ValidateInputs();

    // Setup steps
    bool StepCreateRDAccount();
    bool StepAddRDToAdmins();
    bool StepSetVPNAdapterPrivate();
    bool StepEnableNetworkDiscovery();
    bool StepEnableFileSharing();
    bool StepSetNTLMv2();
    bool StepDisablePasswordSharing();
    bool StepCreateSMBFirewallRule();
    bool StepCreateDebuggerFirewallRule();
    bool StepCreateShare();
    bool StepSetNTFSPermissions();
    void StepDisplaySummary();

    // Restore steps
    void RestoreNTFSPermissions();
    void RestoreShare();
    void RestoreDebuggerFirewallRule();
    void RestoreSMBFirewallRule();
    void RestoreFileSharing();
    void RestoreNetworkDiscovery();
    void RestorePasswordSharing();
    void RestoreNTLMv2();
    void RestoreVPNAdapterProfile();
    void RestoreRDFromAdmins();
    void RestoreRDAccount();

    DECLARE_MESSAGE_MAP()
};
