#include "pch.h"
#include "SetupTest.h"
#include "SetupTestDlg.h"
#include "../Common/WinUtils.h"
#include "../Common/TeamViewerUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// ════════════════════════════════════════════════════════════════
// Construction
// ════════════════════════════════════════════════════════════════

CSetupTestDlg::CSetupTestDlg(CWnd* pParent)
    : CDialogEx(IDD_SETUPTEST_DIALOG, pParent)
    , m_strDevHostname(_T("DELL"))
    , m_strDevVPNIP(_T("7.238.81.240"))
    , m_strShareName(_T("CTrack-software"))
    , m_strPassword(_T("a"))
    , m_strDebuggerPort(_T("4026"))
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

// ════════════════════════════════════════════════════════════════
// DDX / Message Map
// ════════════════════════════════════════════════════════════════

void CSetupTestDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_STATIC_VPN_STATUS, m_staticVPNStatus);
    DDX_Control(pDX, IDC_STATIC_DEBUGGER_STATUS, m_staticDebuggerStatus);
    DDX_Control(pDX, IDC_EDIT_DEV_HOSTNAME, m_editDevHostname);
    DDX_Control(pDX, IDC_EDIT_DEV_VPN_IP, m_editDevVPNIP);
    DDX_Control(pDX, IDC_EDIT_SHARE_NAME, m_editShareName);
    DDX_Control(pDX, IDC_EDIT_PASSWORD, m_editPassword);
    DDX_Control(pDX, IDC_COMBO_DRIVE_LETTER, m_comboDriveLetter);
    DDX_Control(pDX, IDC_EDIT_DEBUGGER_PORT, m_editDebuggerPort);
    DDX_Control(pDX, IDC_EDIT_LOG, m_editLog);
    DDX_Text(pDX, IDC_EDIT_DEV_HOSTNAME, m_strDevHostname);
    DDX_Text(pDX, IDC_EDIT_DEV_VPN_IP, m_strDevVPNIP);
    DDX_Text(pDX, IDC_EDIT_SHARE_NAME, m_strShareName);
    DDX_Text(pDX, IDC_EDIT_PASSWORD, m_strPassword);
    DDX_Text(pDX, IDC_EDIT_DEBUGGER_PORT, m_strDebuggerPort);
}

BEGIN_MESSAGE_MAP(CSetupTestDlg, CDialogEx)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BUTTON_INSTALL_VPN, &CSetupTestDlg::OnBnClickedInstallVPN)
    ON_BN_CLICKED(IDC_BUTTON_INSTALL_DEBUGGER, &CSetupTestDlg::OnBnClickedInstallDebugger)
    ON_BN_CLICKED(IDC_BUTTON_SETUP, &CSetupTestDlg::OnBnClickedSetup)
    ON_BN_CLICKED(IDC_BUTTON_RESTORE, &CSetupTestDlg::OnBnClickedRestore)
END_MESSAGE_MAP()

// ════════════════════════════════════════════════════════════════
// Initialization
// ════════════════════════════════════════════════════════════════

BOOL CSetupTestDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);

    // Initialize logging
    m_log.SetLogControl(&m_editLog);

    // Initialize backup system
    m_backup.Initialize(_T("state_test"));

    // Populate drive letter combo box
    for (TCHAR letter = _T('Z'); letter >= _T('D'); letter--)
    {
        CString s;
        s.Format(_T("%c:"), letter);
        m_comboDriveLetter.AddString(s);
    }
    m_comboDriveLetter.SetCurSel(0);  // Default to Z:

    // Check admin
    if (!CWinUtils::IsRunningAsAdmin())
    {
        m_log.LogError(_T("This program must be run as Administrator!"));
        GetDlgItem(IDC_BUTTON_SETUP)->EnableWindow(FALSE);
        GetDlgItem(IDC_BUTTON_RESTORE)->EnableWindow(FALSE);
    }

    // Check prerequisites
    CheckPrerequisites();

    // Enable/disable Restore button based on saved state
    GetDlgItem(IDC_BUTTON_RESTORE)->EnableWindow(m_backup.HasSavedState());

    m_log.LogSeparator();
    m_log.Log(_T("  Remote Debug Setup - TEST PC"));
    m_log.LogSeparator();
    m_log.Log(_T("  Ready. Check prerequisites, fill in the fields, and click Setup."));

    return TRUE;
}

void CSetupTestDlg::OnPaint()
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

HCURSOR CSetupTestDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

// ════════════════════════════════════════════════════════════════
// Prerequisite Checks
// ════════════════════════════════════════════════════════════════

void CSetupTestDlg::CheckPrerequisites()
{
    DetectVPNStatus();
    DetectDebuggerStatus();
}

void CSetupTestDlg::DetectVPNStatus()
{
    if (CTeamViewerUtils::IsVPNDriverInstalled())
    {
        CString vpnIP = CTeamViewerUtils::GetVPNIPAddress();
        if (!vpnIP.IsEmpty())
        {
            CString status;
            status.Format(_T("Installed & Connected (IP: %s)"), (LPCTSTR)vpnIP);
            m_staticVPNStatus.SetWindowText(status);
        }
        else
        {
            m_staticVPNStatus.SetWindowText(_T("Installed (not connected)"));
        }
        GetDlgItem(IDC_BUTTON_INSTALL_VPN)->EnableWindow(FALSE);
    }
    else if (CTeamViewerUtils::IsTeamViewerInstalled())
    {
        m_staticVPNStatus.SetWindowText(_T("Not installed (TeamViewer found)"));
        GetDlgItem(IDC_BUTTON_INSTALL_VPN)->EnableWindow(TRUE);
    }
    else
    {
        m_staticVPNStatus.SetWindowText(_T("Not installed (TeamViewer not found)"));
        GetDlgItem(IDC_BUTTON_INSTALL_VPN)->EnableWindow(TRUE);
    }
}

void CSetupTestDlg::DetectDebuggerStatus()
{
    CString debuggerPath = CTeamViewerUtils::GetRemoteDebuggerPath();
    if (!debuggerPath.IsEmpty())
    {
        CString status;
        status.Format(_T("Installed (%s)"), (LPCTSTR)debuggerPath);
        // Truncate if too long
        if (status.GetLength() > 80)
        {
            status = _T("Installed (") + debuggerPath.Right(60) + _T(")");
        }
        m_staticDebuggerStatus.SetWindowText(status);
        GetDlgItem(IDC_BUTTON_INSTALL_DEBUGGER)->EnableWindow(FALSE);
    }
    else
    {
        m_staticDebuggerStatus.SetWindowText(_T("Not found"));
        GetDlgItem(IDC_BUTTON_INSTALL_DEBUGGER)->EnableWindow(TRUE);
    }
}

// ════════════════════════════════════════════════════════════════
// Install Buttons
// ════════════════════════════════════════════════════════════════

void CSetupTestDlg::OnBnClickedInstallVPN()
{
    CTeamViewerUtils::InstallVPNDriver();
    // Re-check after user returns
    DetectVPNStatus();
}

void CSetupTestDlg::OnBnClickedInstallDebugger()
{
    CTeamViewerUtils::OpenRemoteToolsDownloadPage();

    AfxMessageBox(
        _T("The download page for Remote Tools for Visual Studio has been opened in your browser.\n\n")
        _T("After installation, click OK to re-check."),
        MB_OK | MB_ICONINFORMATION);

    // Re-check
    DetectDebuggerStatus();
}

// ════════════════════════════════════════════════════════════════
// Input Validation
// ════════════════════════════════════════════════════════════════

bool CSetupTestDlg::ValidateInputs()
{
    UpdateData(TRUE);

    if (m_strDevHostname.IsEmpty())
    {
        AfxMessageBox(_T("Please enter the Dev PC hostname."), MB_OK | MB_ICONWARNING);
        m_editDevHostname.SetFocus();
        return false;
    }

    if (m_strDevVPNIP.IsEmpty())
    {
        AfxMessageBox(_T("Please enter the Dev PC VPN IP address."), MB_OK | MB_ICONWARNING);
        m_editDevVPNIP.SetFocus();
        return false;
    }

    // Basic IP validation (must have 3 dots)
    int dotCount = 0;
    for (int i = 0; i < m_strDevVPNIP.GetLength(); i++)
    {
        if (m_strDevVPNIP[i] == _T('.'))
            dotCount++;
    }
    if (dotCount != 3)
    {
        AfxMessageBox(_T("Please enter a valid IPv4 address for the Dev PC VPN IP."), MB_OK | MB_ICONWARNING);
        m_editDevVPNIP.SetFocus();
        return false;
    }

    if (m_strShareName.IsEmpty())
    {
        AfxMessageBox(_T("Please enter the share name from the Dev PC."), MB_OK | MB_ICONWARNING);
        m_editShareName.SetFocus();
        return false;
    }

    if (m_strPassword.IsEmpty())
    {
        AfxMessageBox(_T("Please enter the RD account password."), MB_OK | MB_ICONWARNING);
        m_editPassword.SetFocus();
        return false;
    }

    if (m_strDebuggerPort.IsEmpty())
    {
        AfxMessageBox(_T("Please enter the Remote Debugger port."), MB_OK | MB_ICONWARNING);
        m_editDebuggerPort.SetFocus();
        return false;
    }

    int port = _ttoi(m_strDebuggerPort);
    if (port < 1024 || port > 65535)
    {
        AfxMessageBox(_T("Debugger port must be between 1024 and 65535."), MB_OK | MB_ICONWARNING);
        m_editDebuggerPort.SetFocus();
        return false;
    }

    return true;
}

TCHAR CSetupTestDlg::GetSelectedDriveLetter()
{
    CString sel;
    int idx = m_comboDriveLetter.GetCurSel();
    if (idx >= 0)
    {
        m_comboDriveLetter.GetLBText(idx, sel);
        if (!sel.IsEmpty())
            return sel[0];
    }
    return _T('Z');
}

// ════════════════════════════════════════════════════════════════
// Setup Button
// ════════════════════════════════════════════════════════════════

void CSetupTestDlg::OnBnClickedSetup()
{
    if (!ValidateInputs())
        return;

    GetDlgItem(IDC_BUTTON_SETUP)->EnableWindow(FALSE);
    GetDlgItem(IDC_BUTTON_RESTORE)->EnableWindow(FALSE);

    CWaitCursor wait;
    m_log.Clear();
    m_log.LogSeparator();
    m_log.Log(_T("  Starting Test PC Setup..."));
    m_log.LogSeparator();
    m_log.Log(_T(""));

    TCHAR driveLetter = GetSelectedDriveLetter();

    // Save configuration to state
    m_backup.SaveState(_T("dev_hostname"), m_strDevHostname);
    m_backup.SaveState(_T("dev_vpn_ip"), m_strDevVPNIP);
    m_backup.SaveState(_T("share_name"), m_strShareName);
    CString dl;
    dl.Format(_T("%c"), driveLetter);
    m_backup.SaveState(_T("drive_letter"), dl);

    int step = 0;
    bool allOk = true;

    // Step 1: Create RD account
    m_log.LogStep(++step, TOTAL_SETUP_STEPS, _T("Creating local RD account..."));
    if (!StepCreateRDAccount()) allOk = false;

    // Step 2: Add RD to Administrators
    m_log.LogStep(++step, TOTAL_SETUP_STEPS, _T("Adding RD to Administrators group..."));
    if (!StepAddRDToAdmins()) allOk = false;

    // Step 3: Create debugger firewall rule
    m_log.LogStep(++step, TOTAL_SETUP_STEPS, _T("Creating Remote Debugger firewall rule..."));
    if (!StepCreateDebuggerFirewallRule()) allOk = false;

    // Step 4: Verify connectivity
    m_log.LogStep(++step, TOTAL_SETUP_STEPS, _T("Verifying VPN connectivity to Dev PC..."));
    StepVerifyConnectivity();  // Don't fail on this, just warn

    // Step 5: Map shared folder
    m_log.LogStep(++step, TOTAL_SETUP_STEPS, _T("Mapping shared folder..."));
    if (!StepMapSharedFolder()) allOk = false;

    // Step 6: Verify mapped drive
    m_log.LogStep(++step, TOTAL_SETUP_STEPS, _T("Verifying mapped drive..."));
    if (!StepVerifyMappedDrive()) allOk = false;

    // Step 7: Locate Remote Debugger
    m_log.LogStep(++step, TOTAL_SETUP_STEPS, _T("Locating Visual Studio Remote Debugger..."));
    StepLocateRemoteDebugger();  // Informational only

    // Step 8: Summary
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

    GetDlgItem(IDC_BUTTON_SETUP)->EnableWindow(TRUE);
    GetDlgItem(IDC_BUTTON_RESTORE)->EnableWindow(m_backup.HasSavedState());
}

// ════════════════════════════════════════════════════════════════
// Setup Steps Implementation
// ════════════════════════════════════════════════════════════════

bool CSetupTestDlg::StepCreateRDAccount()
{
    bool existed = CWinUtils::UserExists(_T("RD"));
    m_backup.SaveState(_T("rd_user_existed"), existed);

    if (existed)
    {
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
                                        _T("Remote Debug"),
                                        _T("Account for remote debugging (matches Dev PC)")))
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

bool CSetupTestDlg::StepAddRDToAdmins()
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

bool CSetupTestDlg::StepCreateDebuggerFirewallRule()
{
    LPCTSTR ruleName = _T("VS Remote Debugger Inbound");
    bool existed = CWinUtils::FirewallRuleExists(ruleName);
    m_backup.SaveState(_T("debugger_firewall_rule_existed"), existed);

    if (CWinUtils::CreateFirewallRule(ruleName,
            _T("Allow incoming Visual Studio Remote Debugger connections"),
            _T("4022-4026"), _T("*")))
    {
        m_log.LogSuccess(_T("Firewall rule created (ports 4022-4026, all profiles)."));
        return true;
    }

    m_log.LogError(_T("Failed to create Remote Debugger firewall rule."));
    return false;
}

bool CSetupTestDlg::StepVerifyConnectivity()
{
    m_log.LogInfo(_T("Pinging Dev PC..."));

    if (CWinUtils::PingHost(m_strDevVPNIP))
    {
        m_log.LogSuccess(_T("Ping successful."));
    }
    else
    {
        m_log.LogWarning(_T("Cannot ping Dev PC. VPN may not be connected."));
        m_log.LogInfo(_T("Continuing anyway (some networks block ICMP)..."));
    }

    // Test SMB port
    m_log.LogInfo(_T("Testing SMB port 445..."));
    if (CWinUtils::TestTcpPort(m_strDevVPNIP, 445))
    {
        m_log.LogSuccess(_T("Port 445 is open on Dev PC."));
        return true;
    }
    else
    {
        m_log.LogWarning(_T("Port 445 is not reachable on Dev PC."));
        m_log.LogInfo(_T("Make sure SetupDevelop has been run on the Dev PC."));
        return false;
    }
}

bool CSetupTestDlg::StepMapSharedFolder()
{
    TCHAR driveLetter = GetSelectedDriveLetter();

    // Check if drive is already mapped
    bool wasAlreadyMapped = CWinUtils::IsDriveMapped(driveLetter);
    m_backup.SaveState(_T("drive_was_mapped"), wasAlreadyMapped);

    // Build UNC path
    CString uncPath;
    uncPath.Format(_T("\\\\%s\\%s"), (LPCTSTR)m_strDevVPNIP, (LPCTSTR)m_strShareName);

    // Build username
    CString userName;
    userName.Format(_T("%s\\RD"), (LPCTSTR)m_strDevHostname);

    CString info;
    info.Format(_T("Mapping %c: -> %s"), driveLetter, (LPCTSTR)uncPath);
    m_log.LogInfo(info);

    if (CWinUtils::MapNetworkDrive(driveLetter, uncPath, userName, m_strPassword))
    {
        CString msg;
        msg.Format(_T("%c: mapped successfully."), driveLetter);
        m_log.LogSuccess(msg);
        return true;
    }

    m_log.LogError(_T("Failed to map drive."));
    m_log.LogInfo(_T("Common causes:"));
    m_log.LogInfo(_T("  Error 5:    Wrong password or missing permissions on Dev PC"));
    m_log.LogInfo(_T("  Error 53:   VPN not connected or port 445 blocked"));
    m_log.LogInfo(_T("  Error 1219: Multiple connections - restart and retry"));
    m_log.LogInfo(_T("  Error 1326: Username/password mismatch"));
    return false;
}

bool CSetupTestDlg::StepVerifyMappedDrive()
{
    TCHAR driveLetter = GetSelectedDriveLetter();
    CString drivePath;
    drivePath.Format(_T("%c:\\"), driveLetter);

    if (GetFileAttributes(drivePath) != INVALID_FILE_ATTRIBUTES)
    {
        m_log.LogSuccess(_T("Mapped drive is accessible."));

        // List some files to confirm
        CString findPath;
        findPath.Format(_T("%c:\\*"), driveLetter);
        WIN32_FIND_DATA fd;
        HANDLE hFind = FindFirstFile(findPath, &fd);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            int fileCount = 0;
            do
            {
                if (++fileCount <= 5)
                {
                    CString entry;
                    entry.Format(_T("  %s %s"),
                        (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? _T("[DIR]") : _T("     "),
                        fd.cFileName);
                    m_log.LogInfo(entry);
                }
            } while (FindNextFile(hFind, &fd));
            FindClose(hFind);

            if (fileCount > 5)
            {
                CString more;
                more.Format(_T("  ... and %d more items"), fileCount - 5);
                m_log.LogInfo(more);
            }
        }
        return true;
    }

    m_log.LogWarning(_T("Mapped drive is not accessible yet."));
    m_log.LogInfo(_T("The drive may need a moment to become available."));
    return false;
}

bool CSetupTestDlg::StepLocateRemoteDebugger()
{
    CString path = CTeamViewerUtils::GetRemoteDebuggerPath();
    if (!path.IsEmpty())
    {
        CString msg;
        msg.Format(_T("Found: %s"), (LPCTSTR)path);
        m_log.LogSuccess(msg);
        return true;
    }

    m_log.LogWarning(_T("Remote Debugger (msvsmon.exe) not found."));
    m_log.LogInfo(_T("Install 'Remote Tools for Visual Studio' from Microsoft."));
    return false;
}

void CSetupTestDlg::StepDisplaySummary()
{
    CString hostname = CWinUtils::GetComputerHostName();
    TCHAR driveLetter = GetSelectedDriveLetter();

    m_log.Log(_T(""));
    m_log.LogSeparator();
    m_log.Log(_T("  SUMMARY"));
    m_log.LogSeparator();

    CString info;
    info.Format(_T("  This PC:     %s"), (LPCTSTR)hostname);
    m_log.Log(info);

    info.Format(_T("  RD Account:  RD (password: %s)"), (LPCTSTR)m_strPassword);
    m_log.Log(info);

    info.Format(_T("  Dev PC:      %s (%s)"), (LPCTSTR)m_strDevHostname, (LPCTSTR)m_strDevVPNIP);
    m_log.Log(info);

    info.Format(_T("  Mapped Drive: %c: -> \\\\%s\\%s"),
                driveLetter, (LPCTSTR)m_strDevVPNIP, (LPCTSTR)m_strShareName);
    m_log.Log(info);

    m_log.Log(_T(""));
    m_log.Log(_T("  NEXT STEPS:"));
    m_log.Log(_T("  1. Start msvsmon.exe as Administrator"));
    m_log.Log(_T("  2. In msvsmon: Tools > Options > Windows Authentication"));
    m_log.Log(_T("  3. In msvsmon: Tools > Permissions > Add RD user"));
    m_log.Log(_T("  4. In Visual Studio on Dev PC:"));
    m_log.Log(_T("     Debug > Attach to Process"));
    m_log.Log(_T("     Connection: Remote (Windows Authentication)"));

    CString vpnIP = CTeamViewerUtils::GetVPNIPAddress();
    if (!vpnIP.IsEmpty())
    {
        info.Format(_T("     Target: %s:%s"), (LPCTSTR)vpnIP, (LPCTSTR)m_strDebuggerPort);
        m_log.Log(info);
    }
}

// ════════════════════════════════════════════════════════════════
// Restore Button
// ════════════════════════════════════════════════════════════════

void CSetupTestDlg::OnBnClickedRestore()
{
    if (AfxMessageBox(_T("This will undo all changes made by Setup.\n\nAre you sure?"),
                      MB_YESNO | MB_ICONQUESTION) != IDYES)
        return;

    GetDlgItem(IDC_BUTTON_SETUP)->EnableWindow(FALSE);
    GetDlgItem(IDC_BUTTON_RESTORE)->EnableWindow(FALSE);

    CWaitCursor wait;
    m_log.Clear();
    m_log.LogSeparator();
    m_log.Log(_T("  Restoring to initial state..."));
    m_log.LogSeparator();
    m_log.Log(_T(""));

    int step = 0;

    // Restore in reverse order
    m_log.LogStep(++step, TOTAL_RESTORE_STEPS, _T("Unmapping network drive..."));
    RestoreMappedDrive();

    m_log.LogStep(++step, TOTAL_RESTORE_STEPS, _T("Removing Remote Debugger firewall rule..."));
    RestoreDebuggerFirewallRule();

    m_log.LogStep(++step, TOTAL_RESTORE_STEPS, _T("Removing RD from Administrators..."));
    RestoreRDFromAdmins();

    m_log.LogStep(++step, TOTAL_RESTORE_STEPS, _T("Removing RD account..."));
    RestoreRDAccount();

    // Step 5: Clear state
    m_log.LogStep(++step, TOTAL_RESTORE_STEPS, _T("Clearing saved state..."));
    m_backup.ClearState();
    m_log.LogSuccess(_T("Saved state cleared."));

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

void CSetupTestDlg::RestoreMappedDrive()
{
    CString dl = m_backup.LoadState(_T("drive_letter"), _T("Z"));
    bool wasMapped = m_backup.LoadStateBool(_T("drive_was_mapped"));

    if (!wasMapped && !dl.IsEmpty())
    {
        TCHAR letter = dl[0];
        CWinUtils::UnmapNetworkDrive(letter);
        CString msg;
        msg.Format(_T("Drive %c: unmapped."), letter);
        m_log.LogSuccess(msg);
    }
    else if (wasMapped)
    {
        m_log.LogInfo(_T("Drive was already mapped before setup, leaving in place."));
    }
    else
    {
        m_log.LogInfo(_T("No drive mapping to remove."));
    }
}

void CSetupTestDlg::RestoreDebuggerFirewallRule()
{
    bool existed = m_backup.LoadStateBool(_T("debugger_firewall_rule_existed"));
    if (!existed)
    {
        CWinUtils::DeleteFirewallRule(_T("VS Remote Debugger Inbound"));
        m_log.LogSuccess(_T("Remote Debugger firewall rule removed."));
    }
    else
    {
        m_log.LogInfo(_T("Rule existed before setup, leaving in place."));
    }
}

void CSetupTestDlg::RestoreRDFromAdmins()
{
    bool wasAdmin = m_backup.LoadStateBool(_T("rd_was_admin"));
    if (!wasAdmin)
    {
        CWinUtils::RemoveUserFromGroup(_T("RD"), _T("Administrators"));
        m_log.LogSuccess(_T("RD removed from Administrators group."));
    }
    else
    {
        m_log.LogInfo(_T("RD was already an Administrator, leaving in place."));
    }
}

void CSetupTestDlg::RestoreRDAccount()
{
    bool existed = m_backup.LoadStateBool(_T("rd_user_existed"));
    if (!existed)
    {
        CWinUtils::DeleteLocalUser(_T("RD"));
        m_log.LogSuccess(_T("RD account deleted."));
    }
    else
    {
        m_log.LogInfo(_T("RD account existed before setup, leaving in place."));
    }
}
