#include "pch.h"
#include "SetupDevelop.h"
#include "SetupDevelopDlg.h"
#include "../Common/WinUtils.h"
#include "../Common/TeamViewerUtils.h"
#include <ShlObj.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// ════════════════════════════════════════════════════════════════
// Construction
// ════════════════════════════════════════════════════════════════

CSetupDevelopDlg::CSetupDevelopDlg(CWnd* pParent)
    : CDialogEx(IDD_SETUPDEVELOP_DIALOG, pParent)
    , m_strPassword(_T("a"))
    , m_strSharePath(_T("C:\\CTrack-software"))
    , m_strShareName(_T("CTrack-software"))
    , m_strVPNSubnet(_T("7.0.0.0/8"))
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

// ════════════════════════════════════════════════════════════════
// DDX / Message Map
// ════════════════════════════════════════════════════════════════

void CSetupDevelopDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_STATIC_VPN_STATUS, m_staticVPNStatus);
    DDX_Control(pDX, IDC_EDIT_TV_ID, m_editTVID);
    DDX_Control(pDX, IDC_EDIT_PASSWORD, m_editPassword);
    DDX_Control(pDX, IDC_EDIT_SHARE_PATH, m_editSharePath);
    DDX_Control(pDX, IDC_EDIT_SHARE_NAME, m_editShareName);
    DDX_Control(pDX, IDC_EDIT_VPN_SUBNET, m_editVPNSubnet);
    DDX_Control(pDX, IDC_EDIT_LOG, m_editLog);
    DDX_Text(pDX, IDC_EDIT_TV_ID, m_strTVID);
    DDX_Text(pDX, IDC_EDIT_PASSWORD, m_strPassword);
    DDX_Text(pDX, IDC_EDIT_SHARE_PATH, m_strSharePath);
    DDX_Text(pDX, IDC_EDIT_SHARE_NAME, m_strShareName);
    DDX_Text(pDX, IDC_EDIT_VPN_SUBNET, m_strVPNSubnet);
}

BEGIN_MESSAGE_MAP(CSetupDevelopDlg, CDialogEx)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BUTTON_CONNECT_VPN, &CSetupDevelopDlg::OnBnClickedConnectVPN)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE, &CSetupDevelopDlg::OnBnClickedBrowse)
    ON_BN_CLICKED(IDC_BUTTON_SETUP, &CSetupDevelopDlg::OnBnClickedSetup)
    ON_BN_CLICKED(IDC_BUTTON_RESTORE, &CSetupDevelopDlg::OnBnClickedRestore)
END_MESSAGE_MAP()

// ════════════════════════════════════════════════════════════════
// Initialization
// ════════════════════════════════════════════════════════════════

BOOL CSetupDevelopDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);

    // Initialize logging
    m_log.SetLogControl(&m_editLog);
    m_log.InitFileLog(_T("SetupDevelop"));

    // Initialize backup system
    m_backup.Initialize(_T("state_develop"));

    // Check admin
    if (!CWinUtils::IsRunningAsAdmin())
    {
        m_log.LogError(_T("This program must be run as Administrator!"));
        GetDlgItem(IDC_BUTTON_SETUP)->EnableWindow(FALSE);
        GetDlgItem(IDC_BUTTON_RESTORE)->EnableWindow(FALSE);
    }

    // Detect VPN status
    DetectVPNStatus();

    // Enable/disable Restore button based on saved state
    GetDlgItem(IDC_BUTTON_RESTORE)->EnableWindow(m_backup.HasSavedState());

    m_log.LogSeparator();
    CString buildInfo;
    buildInfo.Format(_T("  Remote Debug Setup - DEVELOPMENT PC  (build %s %s)"),
                     _T(__DATE__), _T(__TIME__));
    m_log.Log(buildInfo);
    m_log.LogSeparator();
    m_log.Log(_T("  Ready. Fill in the fields and click Setup."));

    return TRUE;
}

void CSetupDevelopDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this);
        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialogEx::OnPaint();
    }
}

HCURSOR CSetupDevelopDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

// ════════════════════════════════════════════════════════════════
// VPN Detection
// ════════════════════════════════════════════════════════════════

void CSetupDevelopDlg::DetectVPNStatus()
{
    CString vpnIP = CTeamViewerUtils::GetVPNIPAddress();
    if (!vpnIP.IsEmpty())
    {
        CString status;
        status.Format(_T("Connected - IP: %s"), (LPCTSTR)vpnIP);
        m_staticVPNStatus.SetWindowText(status);
    }
    else if (CTeamViewerUtils::IsVPNDriverInstalled())
    {
        m_staticVPNStatus.SetWindowText(_T("VPN driver installed but not connected"));
    }
    else if (CTeamViewerUtils::IsTeamViewerInstalled())
    {
        m_staticVPNStatus.SetWindowText(_T("TeamViewer installed, VPN driver not found"));
    }
    else
    {
        m_staticVPNStatus.SetWindowText(_T("TeamViewer not installed"));
    }
}

// ════════════════════════════════════════════════════════════════
// Connect VPN Button
// ════════════════════════════════════════════════════════════════

void CSetupDevelopDlg::OnBnClickedConnectVPN()
{
    UpdateData(TRUE);

    m_strTVID.Trim();
    if (m_strTVID.IsEmpty())
    {
        AfxMessageBox(_T("Please enter the Test PC's TeamViewer ID."), MB_OK | MB_ICONWARNING);
        m_editTVID.SetFocus();
        return;
    }

    CTeamViewerUtils::ConnectVPN(m_strTVID);
    DetectVPNStatus();

    // Copy this PC's VPN IP to clipboard so user can paste it on the Test PC
    CString vpnIP = CTeamViewerUtils::GetVPNIPAddress();
    if (!vpnIP.IsEmpty())
    {
        if (OpenClipboard())
        {
            EmptyClipboard();
            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (vpnIP.GetLength() + 1) * sizeof(TCHAR));
            if (hMem)
            {
                LPTSTR pMem = static_cast<LPTSTR>(GlobalLock(hMem));
                _tcscpy_s(pMem, vpnIP.GetLength() + 1, vpnIP);
                GlobalUnlock(hMem);
#ifdef _UNICODE
                SetClipboardData(CF_UNICODETEXT, hMem);
#else
                SetClipboardData(CF_TEXT, hMem);
#endif
            }
            CloseClipboard();
        }

        CString msg;
        msg.Format(_T("VPN IP address %s has been copied to the clipboard.\n\n")
                   _T("Paste it into the SetupTest program on the Test PC."),
                   (LPCTSTR)vpnIP);
        AfxMessageBox(msg, MB_OK | MB_ICONINFORMATION);
    }
}

// ════════════════════════════════════════════════════════════════
// Browse Button
// ════════════════════════════════════════════════════════════════

void CSetupDevelopDlg::OnBnClickedBrowse()
{
    CFolderPickerDialog dlg(m_strSharePath, 0, this);
    dlg.m_ofn.lpstrTitle = _T("Select folder to share for remote debugging");

    if (dlg.DoModal() == IDOK)
    {
        m_strSharePath = dlg.GetPathName();
        UpdateData(FALSE);

        // Auto-generate share name from folder name
        int lastSlash = m_strSharePath.ReverseFind(_T('\\'));
        if (lastSlash != -1)
        {
            m_strShareName = m_strSharePath.Mid(lastSlash + 1);
            UpdateData(FALSE);
        }
    }
}

// ════════════════════════════════════════════════════════════════
// Input Validation
// ════════════════════════════════════════════════════════════════

bool CSetupDevelopDlg::ValidateInputs()
{
    UpdateData(TRUE);

    if (m_strPassword.IsEmpty())
    {
        AfxMessageBox(_T("Please enter a password for the RD account."), MB_OK | MB_ICONWARNING);
        m_editPassword.SetFocus();
        return false;
    }

    if (m_strSharePath.IsEmpty())
    {
        AfxMessageBox(_T("Please specify a folder to share."), MB_OK | MB_ICONWARNING);
        m_editSharePath.SetFocus();
        return false;
    }

    if (GetFileAttributes(m_strSharePath) == INVALID_FILE_ATTRIBUTES)
    {
        CString msg;
        msg.Format(_T("The folder '%s' does not exist. Create it?"), (LPCTSTR)m_strSharePath);
        if (AfxMessageBox(msg, MB_YESNO | MB_ICONQUESTION) == IDYES)
        {
            if (SHCreateDirectoryEx(nullptr, m_strSharePath, nullptr) != ERROR_SUCCESS)
            {
                AfxMessageBox(_T("Failed to create the folder."), MB_OK | MB_ICONERROR);
                return false;
            }
        }
        else
        {
            return false;
        }
    }

    if (m_strShareName.IsEmpty())
    {
        AfxMessageBox(_T("Please enter a share name."), MB_OK | MB_ICONWARNING);
        m_editShareName.SetFocus();
        return false;
    }

    if (m_strVPNSubnet.IsEmpty())
    {
        AfxMessageBox(_T("Please enter the VPN subnet."), MB_OK | MB_ICONWARNING);
        m_editVPNSubnet.SetFocus();
        return false;
    }

    return true;
}

// ════════════════════════════════════════════════════════════════
// Setup Button
// ════════════════════════════════════════════════════════════════

void CSetupDevelopDlg::OnBnClickedSetup()
{
    if (!ValidateInputs())
        return;

    // Disable buttons during operation
    GetDlgItem(IDC_BUTTON_SETUP)->EnableWindow(FALSE);
    GetDlgItem(IDC_BUTTON_RESTORE)->EnableWindow(FALSE);

    CWaitCursor wait;
    m_log.Clear();
    m_log.SetOperation(_T("setup"));
    {
        CString buildInfo;
        buildInfo.Format(_T("  Build: %s %s"), _T(__DATE__), _T(__TIME__));
        m_log.Log(buildInfo);
    }
    m_log.LogSeparator();
    m_log.Log(_T("  Starting Development PC Setup..."));
    m_log.LogSeparator();
    m_log.Log(_T(""));

    // Log configuration
    {
        CString cfg;
        cfg.Format(_T("Config: SharePath=%s, ShareName=%s, VPNSubnet=%s"),
                   (LPCTSTR)m_strSharePath, (LPCTSTR)m_strShareName, (LPCTSTR)m_strVPNSubnet);
        m_log.LogInfo(cfg);
    }

    // Save configuration to state
    m_backup.SaveState(_T("share_name"), m_strShareName);
    m_backup.SaveState(_T("share_path"), m_strSharePath);
    m_backup.SaveState(_T("vpn_subnet"), m_strVPNSubnet);

    int step = 0;
    bool allOk = true;

    // Step 1: Create RD account
    m_log.LogStep(++step, TOTAL_SETUP_STEPS, _T("Creating local RD account..."));
    if (!StepCreateRDAccount()) allOk = false;

    // Step 2: Add RD to Administrators
    m_log.LogStep(++step, TOTAL_SETUP_STEPS, _T("Adding RD to Administrators group..."));
    if (!StepAddRDToAdmins()) allOk = false;

    // Step 3: Set VPN adapter to Private
    m_log.LogStep(++step, TOTAL_SETUP_STEPS, _T("Configuring VPN adapter network profile..."));
    if (!StepSetVPNAdapterPrivate()) allOk = false;

    // Step 4: Enable Network Discovery
    m_log.LogStep(++step, TOTAL_SETUP_STEPS, _T("Enabling Network Discovery..."));
    if (!StepEnableNetworkDiscovery()) allOk = false;

    // Step 5: Enable File and Printer Sharing
    m_log.LogStep(++step, TOTAL_SETUP_STEPS, _T("Enabling File and Printer Sharing..."));
    if (!StepEnableFileSharing()) allOk = false;

    // Step 6: Set NTLMv2 authentication
    m_log.LogStep(++step, TOTAL_SETUP_STEPS, _T("Setting NTLMv2 authentication level..."));
    if (!StepSetNTLMv2()) allOk = false;

    // Step 7: Disable password protected sharing
    m_log.LogStep(++step, TOTAL_SETUP_STEPS, _T("Configuring password protected sharing..."));
    if (!StepDisablePasswordSharing()) allOk = false;

    // Step 8: Create SMB firewall rule
    m_log.LogStep(++step, TOTAL_SETUP_STEPS, _T("Creating SMB firewall rule (port 445)..."));
    if (!StepCreateSMBFirewallRule()) allOk = false;

    // Step 9: Create Remote Debugger firewall rule
    m_log.LogStep(++step, TOTAL_SETUP_STEPS, _T("Creating Remote Debugger firewall rule (ports 4022-4026)..."));
    if (!StepCreateDebuggerFirewallRule()) allOk = false;

    // Step 10: Create the share
    m_log.LogStep(++step, TOTAL_SETUP_STEPS, _T("Creating network share..."));
    if (!StepCreateShare()) allOk = false;

    // Step 11: Set NTFS permissions
    m_log.LogStep(++step, TOTAL_SETUP_STEPS, _T("Setting NTFS permissions..."));
    if (!StepSetNTFSPermissions()) allOk = false;

    // Step 12: Display summary
    m_log.LogStep(++step, TOTAL_SETUP_STEPS, _T("Setup complete."));
    StepDisplaySummary();

    m_log.Log(_T(""));
    if (allOk)
    {
        m_log.LogSeparator();
        m_log.Log(_T("  ALL STEPS COMPLETED SUCCESSFULLY"));
        m_log.LogSeparator();
    }
    else
    {
        m_log.LogSeparator();
        m_log.Log(_T("  SETUP COMPLETED WITH WARNINGS (see above)"));
        m_log.LogSeparator();
    }

    // Re-enable buttons
    GetDlgItem(IDC_BUTTON_SETUP)->EnableWindow(TRUE);
    GetDlgItem(IDC_BUTTON_RESTORE)->EnableWindow(m_backup.HasSavedState());
}

// ════════════════════════════════════════════════════════════════
// Setup Steps Implementation
// ════════════════════════════════════════════════════════════════

bool CSetupDevelopDlg::StepCreateRDAccount()
{
    bool existed = CWinUtils::UserExists(_T("RD"));
    m_backup.SaveState(_T("rd_user_existed"), existed);

    if (existed)
    {
        // Update password
        if (CWinUtils::SetUserPassword(_T("RD"), m_strPassword))
        {
            m_log.LogSuccess(_T("RD account already exists. Password updated."));
            return true;
        }
        else
        {
            m_log.LogError(_T("Failed to update RD account password."));
            return false;
        }
    }
    else
    {
        if (CWinUtils::CreateLocalUser(_T("RD"), m_strPassword,
                                        _T("Remote Debug"), _T("Account for remote debugging")))
        {
            m_log.LogSuccess(_T("RD account created."));
            return true;
        }
        else
        {
            m_log.LogError(_T("Failed to create RD account."));
            return false;
        }
    }
}

bool CSetupDevelopDlg::StepAddRDToAdmins()
{
    bool wasAdmin = CWinUtils::IsUserInGroup(_T("RD"), _T("Administrators"));
    m_backup.SaveState(_T("rd_was_admin"), wasAdmin);

    if (wasAdmin)
    {
        m_log.LogSuccess(_T("RD is already an Administrator."));
        return true;
    }

    if (CWinUtils::AddUserToGroup(_T("RD"), _T("Administrators")))
    {
        m_log.LogSuccess(_T("RD added to Administrators group."));
        return true;
    }

    m_log.LogError(_T("Failed to add RD to Administrators group."));
    return false;
}

bool CSetupDevelopDlg::StepSetVPNAdapterPrivate()
{
    AdapterInfo vpn = CWinUtils::FindTeamViewerVPNAdapter();
    if (vpn.interfaceIndex < 0)
    {
        m_log.LogWarning(_T("No TeamViewer VPN adapter found."));
        m_log.LogInfo(_T("Make sure TeamViewer VPN is connected, then re-run Setup."));
        return false;
    }

    // Save original state
    CString originalCategory = CWinUtils::GetAdapterNetworkCategory(vpn.interfaceIndex);
    m_backup.SaveState(_T("vpn_adapter_was_private"),
                       originalCategory.Find(_T("Private")) != -1);
    m_backup.SaveState(_T("vpn_adapter_interface_index"), vpn.interfaceIndex);

    CString aliasInfo;
    aliasInfo.Format(_T("VPN adapter: '%s' (index %d, currently %s)"),
                     (LPCTSTR)vpn.alias, vpn.interfaceIndex, (LPCTSTR)originalCategory);
    m_log.LogInfo(aliasInfo);

    if (originalCategory.Find(_T("Private")) != -1)
    {
        m_log.LogSuccess(_T("VPN adapter is already Private."));
        return true;
    }

    if (CWinUtils::SetNetworkProfilePrivate(vpn.interfaceIndex))
    {
        CString msg;
        msg.Format(_T("'%s' changed from %s to Private."),
                   (LPCTSTR)vpn.alias, (LPCTSTR)originalCategory);
        m_log.LogSuccess(msg);
    }
    else
    {
        m_log.LogError(_T("Failed to set VPN adapter to Private."));
        return false;
    }

    // Create a scheduled task to re-set the VPN adapter to Private at every logon.
    // Windows resets the VPN adapter network profile to Public after reboot,
    // which disables Network Discovery and File Sharing.

    // Write a small .ps1 script next to the executable
    TCHAR exePath[MAX_PATH];
    GetModuleFileName(nullptr, exePath, MAX_PATH);
    CString exeDir(exePath);
    exeDir = exeDir.Left(exeDir.ReverseFind(_T('\\')));
    CString scriptPath = exeDir + _T("\\RDS_VPNPrivate.ps1");

    CStdioFile scriptFile;
    if (scriptFile.Open(scriptPath, CFile::modeWrite | CFile::modeCreate | CFile::typeText))
    {
        scriptFile.WriteString(
            _T("# Set TeamViewer VPN adapter to Private network profile\r\n")
            _T("Get-NetAdapter | Where-Object { $_.InterfaceDescription -like '*TeamViewer*' } | ForEach-Object {\r\n")
            _T("    Set-NetConnectionProfile -InterfaceIndex $_.ifIndex -NetworkCategory Private\r\n")
            _T("}\r\n"));
        scriptFile.Close();
    }

    CString taskCmd;
    taskCmd.Format(
        _T("schtasks.exe /create /tn \"RDS_VPNPrivate\" ")
        _T("/tr \"powershell.exe -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File \\\"%s\\\"\" ")
        _T("/sc onlogon /rl highest /f"),
        (LPCTSTR)scriptPath);
    DWORD exitCode = CWinUtils::RunHiddenCommand(taskCmd);

    if (exitCode == 0)
    {
        m_log.LogSuccess(_T("Scheduled task created to keep VPN adapter Private after reboot."));
    }
    else
    {
        m_log.LogWarning(_T("Could not create logon task. VPN profile may revert to Public after reboot."));
        m_log.LogInfo(_T("Re-run Setup after reboot to fix."));
    }

    return true;
}

bool CSetupDevelopDlg::StepEnableNetworkDiscovery()
{
    bool wasEnabled = CWinUtils::IsFirewallRuleGroupEnabled(_T("Network Discovery"));
    m_backup.SaveState(_T("network_discovery_was_enabled"), wasEnabled);

    CWinUtils::EnableFirewallRuleGroup(_T("Network Discovery"), true);
    m_log.LogSuccess(_T("Network Discovery enabled."));
    return true;
}

bool CSetupDevelopDlg::StepEnableFileSharing()
{
    bool wasEnabled = CWinUtils::IsFirewallRuleGroupEnabled(_T("File and Printer Sharing"));
    m_backup.SaveState(_T("file_sharing_was_enabled"), wasEnabled);

    CWinUtils::EnableFirewallRuleGroup(_T("File and Printer Sharing"), true);
    m_log.LogSuccess(_T("File and Printer Sharing enabled."));
    return true;
}

bool CSetupDevelopDlg::StepSetNTLMv2()
{
    // Save current LmCompatibilityLevel for restore
    DWORD currentLevel = 0;
    DWORD size = sizeof(DWORD);
    LONG result = RegGetValue(HKEY_LOCAL_MACHINE,
        _T("SYSTEM\\CurrentControlSet\\Control\\Lsa"),
        _T("LmCompatibilityLevel"), RRF_RT_REG_DWORD,
        nullptr, &currentLevel, &size);

    if (result == ERROR_SUCCESS)
    {
        CString val;
        val.Format(_T("%lu"), currentLevel);
        m_backup.SaveState(_T("lm_compatibility_level"), val);
    }
    else
    {
        m_backup.SaveState(_T("lm_compatibility_level"), _T("not_set"));
    }

    // Set to 3 = Send NTLMv2 response only. Refuse LM & NTLM.
    HKEY hKey;
    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        _T("SYSTEM\\CurrentControlSet\\Control\\Lsa"),
        0, KEY_SET_VALUE, &hKey);

    if (result == ERROR_SUCCESS)
    {
        DWORD newLevel = 3;
        result = RegSetValueEx(hKey, _T("LmCompatibilityLevel"), 0, REG_DWORD,
            (const BYTE*)&newLevel, sizeof(DWORD));
        RegCloseKey(hKey);

        if (result == ERROR_SUCCESS)
        {
            m_log.LogSuccess(_T("LAN Manager auth set to 'Send NTLMv2 only'."));
            return true;
        }
    }

    m_log.LogError(_T("Failed to set NTLMv2 authentication level."));
    return false;
}

bool CSetupDevelopDlg::StepDisablePasswordSharing()
{
    // Save current everyoneincludesanonymous for restore
    DWORD currentVal = 0;
    DWORD size = sizeof(DWORD);
    LONG result = RegGetValue(HKEY_LOCAL_MACHINE,
        _T("SYSTEM\\CurrentControlSet\\Control\\Lsa"),
        _T("everyoneincludesanonymous"), RRF_RT_REG_DWORD,
        nullptr, &currentVal, &size);

    if (result == ERROR_SUCCESS)
    {
        CString val;
        val.Format(_T("%lu"), currentVal);
        m_backup.SaveState(_T("everyone_includes_anonymous"), val);
    }
    else
    {
        m_backup.SaveState(_T("everyone_includes_anonymous"), _T("not_set"));
    }

    // Save current restrictnullsessaccess for restore
    DWORD currentRestrict = 1;
    size = sizeof(DWORD);
    result = RegGetValue(HKEY_LOCAL_MACHINE,
        _T("SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters"),
        _T("restrictnullsessaccess"), RRF_RT_REG_DWORD,
        nullptr, &currentRestrict, &size);

    if (result == ERROR_SUCCESS)
    {
        CString val;
        val.Format(_T("%lu"), currentRestrict);
        m_backup.SaveState(_T("restrict_null_sess_access"), val);
    }
    else
    {
        m_backup.SaveState(_T("restrict_null_sess_access"), _T("not_set"));
    }

    bool ok1 = false, ok2 = false;

    // Set everyoneincludesanonymous = 1
    HKEY hKey;
    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        _T("SYSTEM\\CurrentControlSet\\Control\\Lsa"),
        0, KEY_SET_VALUE, &hKey);
    if (result == ERROR_SUCCESS)
    {
        DWORD val = 1;
        result = RegSetValueEx(hKey, _T("everyoneincludesanonymous"), 0, REG_DWORD,
            (const BYTE*)&val, sizeof(DWORD));
        RegCloseKey(hKey);
        ok1 = (result == ERROR_SUCCESS);
    }

    // Set restrictnullsessaccess = 0
    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        _T("SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters"),
        0, KEY_SET_VALUE, &hKey);
    if (result == ERROR_SUCCESS)
    {
        DWORD val = 0;
        result = RegSetValueEx(hKey, _T("restrictnullsessaccess"), 0, REG_DWORD,
            (const BYTE*)&val, sizeof(DWORD));
        RegCloseKey(hKey);
        ok2 = (result == ERROR_SUCCESS);
    }

    if (ok1 && ok2)
    {
        m_log.LogSuccess(_T("Password protected sharing disabled."));
        return true;
    }

    m_log.LogWarning(_T("Could not fully disable password protected sharing."));
    return false;
}

bool CSetupDevelopDlg::StepCreateSMBFirewallRule()
{
    LPCTSTR ruleName = _T("SMB over TeamViewer VPN");
    bool existed = CWinUtils::FirewallRuleExists(ruleName);
    m_backup.SaveState(_T("smb_firewall_rule_existed"), existed);

    if (CWinUtils::CreateFirewallRule(ruleName,
            _T("Allow SMB file sharing from TeamViewer VPN clients"),
            _T("445"), m_strVPNSubnet))
    {
        CString msg;
        msg.Format(_T("Firewall rule '%s' created (port 445, subnet %s)."),
                   ruleName, (LPCTSTR)m_strVPNSubnet);
        m_log.LogSuccess(msg);
        return true;
    }

    m_log.LogError(_T("Failed to create SMB firewall rule."));
    return false;
}

bool CSetupDevelopDlg::StepCreateDebuggerFirewallRule()
{
    LPCTSTR ruleName = _T("VS Remote Debugger (TeamViewer VPN)");
    bool existed = CWinUtils::FirewallRuleExists(ruleName);
    m_backup.SaveState(_T("debugger_firewall_rule_existed"), existed);

    if (CWinUtils::CreateFirewallRule(ruleName,
            _T("Allow Visual Studio Remote Debugger from TeamViewer VPN clients"),
            _T("4022-4026"), m_strVPNSubnet))
    {
        m_log.LogSuccess(_T("Firewall rule created (ports 4022-4026)."));
        return true;
    }

    m_log.LogError(_T("Failed to create Remote Debugger firewall rule."));
    return false;
}

bool CSetupDevelopDlg::StepCreateShare()
{
    bool existed = CWinUtils::ShareExists(m_strShareName);
    m_backup.SaveState(_T("share_existed"), existed);

    if (existed)
    {
        m_log.LogInfo(_T("Share already exists. Updating permissions..."));
        // Grant Everyone full access (auth is handled by disabling password-protected sharing)
        CString cmd;
        cmd.Format(_T("Grant-SmbShareAccess -Name '%s' -AccountName 'Everyone' -AccessRight Full -Force"),
                   (LPCTSTR)m_strShareName);
        CWinUtils::RunPowerShellCommand(cmd);
        m_log.LogSuccess(_T("Share permissions updated for Everyone."));
        return true;
    }

    if (CWinUtils::CreateSMBShare(m_strShareName, m_strSharePath, _T("Everyone")))
    {
        CString msg;
        msg.Format(_T("Share '%s' created -> %s"),
                   (LPCTSTR)m_strShareName, (LPCTSTR)m_strSharePath);
        m_log.LogSuccess(msg);
        return true;
    }

    m_log.LogError(_T("Failed to create network share."));
    return false;
}

bool CSetupDevelopDlg::StepSetNTFSPermissions()
{
    if (CWinUtils::GrantNTFSPermissions(m_strSharePath, _T("Everyone")))
    {
        m_log.LogSuccess(_T("NTFS permissions: Everyone has Modify access."));
        return true;
    }

    m_log.LogError(_T("Failed to set NTFS permissions for Everyone."));
    return false;
}

void CSetupDevelopDlg::StepDisplaySummary()
{
    CString hostname = CWinUtils::GetComputerHostName();
    CString vpnIP = CTeamViewerUtils::GetVPNIPAddress();

    m_log.Log(_T(""));
    m_log.LogSeparator();
    m_log.Log(_T("  SUMMARY"));
    m_log.LogSeparator();

    CString info;
    info.Format(_T("  Hostname:    %s"), (LPCTSTR)hostname);
    m_log.Log(info);

    info.Format(_T("  RD Account:  RD (password: %s)"), (LPCTSTR)m_strPassword);
    m_log.Log(info);

    info.Format(_T("  Share:       \\\\%s\\%s"), (LPCTSTR)hostname, (LPCTSTR)m_strShareName);
    m_log.Log(info);

    if (!vpnIP.IsEmpty())
    {
        info.Format(_T("  VPN IP:      %s"), (LPCTSTR)vpnIP);
        m_log.Log(info);

        m_log.Log(_T(""));
        m_log.Log(_T("  From Test PC, connect to:"));
        info.Format(_T("    \\\\%s\\%s"), (LPCTSTR)vpnIP, (LPCTSTR)m_strShareName);
        m_log.Log(info);
        info.Format(_T("    Credentials: %s\\RD / %s"), (LPCTSTR)hostname, (LPCTSTR)m_strPassword);
        m_log.Log(info);
    }
    else
    {
        m_log.Log(_T("  VPN IP:      (not detected - check TeamViewer VPN)"));
    }

    m_log.Log(_T(""));
    m_log.Log(_T("  Next: Run SetupTest on the Test PC."));
}

// ════════════════════════════════════════════════════════════════
// Restore Button
// ════════════════════════════════════════════════════════════════

void CSetupDevelopDlg::OnBnClickedRestore()
{
    if (AfxMessageBox(_T("This will undo all changes made by Setup.\n\nAre you sure?"),
                      MB_YESNO | MB_ICONQUESTION) != IDYES)
        return;

    GetDlgItem(IDC_BUTTON_SETUP)->EnableWindow(FALSE);
    GetDlgItem(IDC_BUTTON_RESTORE)->EnableWindow(FALSE);

    CWaitCursor wait;
    m_log.Clear();
    m_log.SetOperation(_T("restore"));
    {
        CString buildInfo;
        buildInfo.Format(_T("  Build: %s %s"), _T(__DATE__), _T(__TIME__));
        m_log.Log(buildInfo);
    }
    m_log.LogSeparator();
    m_log.Log(_T("  Restoring to initial state..."));
    m_log.LogSeparator();
    m_log.Log(_T(""));

    int step = 0;

    // Restore in reverse order
    m_log.LogStep(++step, TOTAL_RESTORE_STEPS, _T("Removing NTFS permissions for RD..."));
    RestoreNTFSPermissions();

    m_log.LogStep(++step, TOTAL_RESTORE_STEPS, _T("Removing network share..."));
    RestoreShare();

    m_log.LogStep(++step, TOTAL_RESTORE_STEPS, _T("Removing Remote Debugger firewall rule..."));
    RestoreDebuggerFirewallRule();

    m_log.LogStep(++step, TOTAL_RESTORE_STEPS, _T("Removing SMB firewall rule..."));
    RestoreSMBFirewallRule();

    m_log.LogStep(++step, TOTAL_RESTORE_STEPS, _T("Restoring File and Printer Sharing..."));
    RestoreFileSharing();

    m_log.LogStep(++step, TOTAL_RESTORE_STEPS, _T("Restoring Network Discovery..."));
    RestoreNetworkDiscovery();

    m_log.LogStep(++step, TOTAL_RESTORE_STEPS, _T("Restoring password protected sharing..."));
    RestorePasswordSharing();

    m_log.LogStep(++step, TOTAL_RESTORE_STEPS, _T("Restoring NTLMv2 authentication level..."));
    RestoreNTLMv2();

    m_log.LogStep(++step, TOTAL_RESTORE_STEPS, _T("Restoring VPN adapter profile..."));
    RestoreVPNAdapterProfile();

    m_log.LogStep(++step, TOTAL_RESTORE_STEPS, _T("Restoring RD account..."));
    RestoreRDAccount();

    // Clear saved state
    m_backup.ClearState();

    m_log.Log(_T(""));
    m_log.LogSeparator();
    m_log.Log(_T("  RESTORE COMPLETE"));
    m_log.LogSeparator();

    GetDlgItem(IDC_BUTTON_SETUP)->EnableWindow(TRUE);
    GetDlgItem(IDC_BUTTON_RESTORE)->EnableWindow(FALSE);
}

// ════════════════════════════════════════════════════════════════
// Restore Steps
// ════════════════════════════════════════════════════════════════

void CSetupDevelopDlg::RestoreNTFSPermissions()
{
    CString sharePath = m_backup.LoadState(_T("share_path"));
    if (!sharePath.IsEmpty())
    {
        CWinUtils::RemoveNTFSPermissions(sharePath, _T("Everyone"));
        m_log.LogSuccess(_T("NTFS permissions for Everyone removed."));
    }
    else
    {
        m_log.LogInfo(_T("No share path in saved state, skipping."));
    }
}

void CSetupDevelopDlg::RestoreShare()
{
    bool existed = m_backup.LoadStateBool(_T("share_existed"));
    CString shareName = m_backup.LoadState(_T("share_name"));

    if (!existed && !shareName.IsEmpty())
    {
        CWinUtils::DeleteSMBShare(shareName);
        m_log.LogSuccess(_T("Network share removed."));
    }
    else if (existed)
    {
        m_log.LogInfo(_T("Share existed before setup, leaving in place."));
    }
    else
    {
        m_log.LogInfo(_T("No share to remove."));
    }
}

void CSetupDevelopDlg::RestoreDebuggerFirewallRule()
{
    bool existed = m_backup.LoadStateBool(_T("debugger_firewall_rule_existed"));
    if (!existed)
    {
        CWinUtils::DeleteFirewallRule(_T("VS Remote Debugger (TeamViewer VPN)"));
        m_log.LogSuccess(_T("Remote Debugger firewall rule removed."));
    }
    else
    {
        m_log.LogInfo(_T("Rule existed before setup, leaving in place."));
    }
}

void CSetupDevelopDlg::RestoreSMBFirewallRule()
{
    bool existed = m_backup.LoadStateBool(_T("smb_firewall_rule_existed"));
    if (!existed)
    {
        CWinUtils::DeleteFirewallRule(_T("SMB over TeamViewer VPN"));
        m_log.LogSuccess(_T("SMB firewall rule removed."));
    }
    else
    {
        m_log.LogInfo(_T("Rule existed before setup, leaving in place."));
    }
}

void CSetupDevelopDlg::RestoreFileSharing()
{
    bool wasEnabled = m_backup.LoadStateBool(_T("file_sharing_was_enabled"));
    if (!wasEnabled)
    {
        CWinUtils::EnableFirewallRuleGroup(_T("File and Printer Sharing"), false);
        m_log.LogSuccess(_T("File and Printer Sharing disabled."));
    }
    else
    {
        m_log.LogInfo(_T("File Sharing was already enabled, leaving as is."));
    }
}

void CSetupDevelopDlg::RestoreNetworkDiscovery()
{
    bool wasEnabled = m_backup.LoadStateBool(_T("network_discovery_was_enabled"));
    if (!wasEnabled)
    {
        CWinUtils::EnableFirewallRuleGroup(_T("Network Discovery"), false);
        m_log.LogSuccess(_T("Network Discovery disabled."));
    }
    else
    {
        m_log.LogInfo(_T("Network Discovery was already enabled, leaving as is."));
    }
}

void CSetupDevelopDlg::RestorePasswordSharing()
{
    CString savedAnon = m_backup.LoadState(_T("everyone_includes_anonymous"));
    CString savedRestrict = m_backup.LoadState(_T("restrict_null_sess_access"));

    if (savedAnon == _T("not_set") && savedRestrict == _T("not_set"))
    {
        m_log.LogInfo(_T("Password sharing settings were not set before, skipping."));
        return;
    }

    HKEY hKey;

    if (savedAnon != _T("not_set") && !savedAnon.IsEmpty())
    {
        DWORD val = (DWORD)_ttol(savedAnon);
        LONG result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
            _T("SYSTEM\\CurrentControlSet\\Control\\Lsa"),
            0, KEY_SET_VALUE, &hKey);
        if (result == ERROR_SUCCESS)
        {
            RegSetValueEx(hKey, _T("everyoneincludesanonymous"), 0, REG_DWORD,
                (const BYTE*)&val, sizeof(DWORD));
            RegCloseKey(hKey);
        }
    }

    if (savedRestrict != _T("not_set") && !savedRestrict.IsEmpty())
    {
        DWORD val = (DWORD)_ttol(savedRestrict);
        LONG result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
            _T("SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters"),
            0, KEY_SET_VALUE, &hKey);
        if (result == ERROR_SUCCESS)
        {
            RegSetValueEx(hKey, _T("restrictnullsessaccess"), 0, REG_DWORD,
                (const BYTE*)&val, sizeof(DWORD));
            RegCloseKey(hKey);
        }
    }

    m_log.LogSuccess(_T("Password protected sharing restored."));
}

void CSetupDevelopDlg::RestoreNTLMv2()
{
    CString savedLevel = m_backup.LoadState(_T("lm_compatibility_level"));

    if (savedLevel == _T("not_set"))
    {
        // Key didn't exist before - delete it
        HKEY hKey;
        LONG result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
            _T("SYSTEM\\CurrentControlSet\\Control\\Lsa"),
            0, KEY_SET_VALUE, &hKey);
        if (result == ERROR_SUCCESS)
        {
            RegDeleteValue(hKey, _T("LmCompatibilityLevel"));
            RegCloseKey(hKey);
        }
        m_log.LogSuccess(_T("NTLMv2 setting removed (was not set before)."));
        return;
    }

    if (!savedLevel.IsEmpty())
    {
        DWORD val = (DWORD)_ttol(savedLevel);
        HKEY hKey;
        LONG result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
            _T("SYSTEM\\CurrentControlSet\\Control\\Lsa"),
            0, KEY_SET_VALUE, &hKey);
        if (result == ERROR_SUCCESS)
        {
            RegSetValueEx(hKey, _T("LmCompatibilityLevel"), 0, REG_DWORD,
                (const BYTE*)&val, sizeof(DWORD));
            RegCloseKey(hKey);
        }
        m_log.LogSuccess(_T("NTLMv2 authentication level restored."));
    }
    else
    {
        m_log.LogInfo(_T("No saved NTLMv2 level, skipping."));
    }
}

void CSetupDevelopDlg::RestoreVPNAdapterProfile()
{
    // Remove the logon task and script file (safe even if they don't exist)
    CWinUtils::RunHiddenCommand(_T("schtasks.exe /delete /tn \"RDS_VPNPrivate\" /f"));

    TCHAR exePath[MAX_PATH];
    GetModuleFileName(nullptr, exePath, MAX_PATH);
    CString exeDir(exePath);
    exeDir = exeDir.Left(exeDir.ReverseFind(_T('\\')));
    DeleteFile(exeDir + _T("\\RDS_VPNPrivate.ps1"));

    m_log.LogSuccess(_T("Logon task 'RDS_VPNPrivate' removed."));

    bool wasPrivate = m_backup.LoadStateBool(_T("vpn_adapter_was_private"));
    if (!wasPrivate)
    {
        int ifIndex = m_backup.LoadStateInt(_T("vpn_adapter_interface_index"), -1);
        if (ifIndex >= 0)
        {
            // Set back to Public
            CString cmd;
            cmd.Format(_T("Set-NetConnectionProfile -InterfaceIndex %d -NetworkCategory Public"), ifIndex);
            CWinUtils::RunPowerShellCommand(cmd);
            m_log.LogSuccess(_T("VPN adapter restored to Public."));
        }
        else
        {
            m_log.LogInfo(_T("No adapter index saved, skipping."));
        }
    }
    else
    {
        m_log.LogInfo(_T("VPN adapter was already Private, leaving as is."));
    }
}

void CSetupDevelopDlg::RestoreRDFromAdmins()
{
    bool wasAdmin = m_backup.LoadStateBool(_T("rd_was_admin"));
    if (!wasAdmin)
    {
        CWinUtils::RemoveUserFromGroup(_T("RD"), _T("Administrators"));
        m_log.LogSuccess(_T("RD removed from Administrators group."));
    }
}

void CSetupDevelopDlg::RestoreRDAccount()
{
    bool existed = m_backup.LoadStateBool(_T("rd_user_existed"));
    if (!existed)
    {
        // First remove from admins
        RestoreRDFromAdmins();

        CWinUtils::DeleteLocalUser(_T("RD"));
        m_log.LogSuccess(_T("RD account deleted."));
    }
    else
    {
        m_log.LogInfo(_T("RD account existed before setup, leaving in place."));
    }
}
