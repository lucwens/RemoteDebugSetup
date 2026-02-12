#pragma once
// WinUtils.h - Windows configuration helpers for firewall, user accounts, shares, network

#include <afxwin.h>
#include <vector>

struct AdapterInfo
{
    CString name;
    CString alias;
    CString ipAddress;
    int     interfaceIndex;
    CString networkCategory;  // "Public", "Private", "DomainAuthenticated"
};

class CWinUtils
{
public:
    // ── Admin Check ──
    static bool IsRunningAsAdmin();

    // ── User Account Management (NetAPI32) ──
    static bool UserExists(LPCTSTR userName);
    static bool CreateLocalUser(LPCTSTR userName, LPCTSTR password, LPCTSTR fullName, LPCTSTR description);
    static bool DeleteLocalUser(LPCTSTR userName);
    static bool SetUserPassword(LPCTSTR userName, LPCTSTR password);
    static bool IsUserInGroup(LPCTSTR userName, LPCTSTR groupName);
    static bool AddUserToGroup(LPCTSTR userName, LPCTSTR groupName);
    static bool RemoveUserFromGroup(LPCTSTR userName, LPCTSTR groupName);

    // ── Firewall Management (COM INetFwPolicy2) ──
    static bool FirewallRuleExists(LPCTSTR ruleName);
    static bool CreateFirewallRule(LPCTSTR ruleName, LPCTSTR description,
                                   LPCTSTR ports, LPCTSTR remoteAddresses);
    static bool DeleteFirewallRule(LPCTSTR ruleName);

    // ── Network Discovery & File Sharing ──
    static bool EnableFirewallRuleGroup(LPCTSTR groupName, bool enable);
    static bool IsFirewallRuleGroupEnabled(LPCTSTR groupName);

    // ── Network Adapter / Profile ──
    static std::vector<AdapterInfo> GetNetworkAdapters();
    static AdapterInfo FindTeamViewerVPNAdapter();
    static bool SetNetworkProfilePrivate(int interfaceIndex);
    static CString GetAdapterNetworkCategory(int interfaceIndex);

    // ── SMB Share Management (NetAPI32) ──
    static bool ShareExists(LPCTSTR shareName);
    static bool CreateSMBShare(LPCTSTR shareName, LPCTSTR path, LPCTSTR userName);
    static bool DeleteSMBShare(LPCTSTR shareName);

    // ── NTFS Permissions ──
    static bool GrantNTFSPermissions(LPCTSTR path, LPCTSTR userName);
    static bool RemoveNTFSPermissions(LPCTSTR path, LPCTSTR userName);

    // ── Drive Mapping (WNet) ──
    static bool MapNetworkDrive(TCHAR driveLetter, LPCTSTR uncPath,
                                LPCTSTR userName, LPCTSTR password);
    static bool UnmapNetworkDrive(TCHAR driveLetter);
    static bool IsDriveMapped(TCHAR driveLetter);

    // ── Connectivity ──
    static bool PingHost(LPCTSTR ipAddress);
    static bool TestTcpPort(LPCTSTR ipAddress, int port, int timeoutMs = 3000);

    // ── Utility ──
    static CString GetComputerHostName();
    static CString RunPowerShellCommand(LPCTSTR command);
    static CString GetLastErrorMessage(DWORD errorCode = 0);
};
