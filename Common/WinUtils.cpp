#include "pch.h"
#include "WinUtils.h"

#include <lm.h>          // NetUserAdd, NetShareAdd, etc.
#include <lmaccess.h>
#include <lmshare.h>
#include <lmerr.h>
#include <netfw.h>       // INetFwPolicy2
#include <comdef.h>
#include <Winnetwk.h>    // WNetAddConnection2
#include <iphlpapi.h>    // GetAdaptersAddresses
#include <icmpapi.h>     // IcmpSendEcho
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <AclAPI.h>      // SetEntriesInAcl
#include <sddl.h>

#pragma comment(lib, "netapi32.lib")
#pragma comment(lib, "mpr.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "advapi32.lib")

// ════════════════════════════════════════════════════════════════
// Admin Check
// ════════════════════════════════════════════════════════════════

bool CWinUtils::IsRunningAsAdmin()
{
    BOOL isAdmin = FALSE;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    PSID adminGroup = nullptr;

    if (AllocateAndInitializeSid(&ntAuthority, 2,
        SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0, &adminGroup))
    {
        CheckTokenMembership(nullptr, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin != FALSE;
}

// ════════════════════════════════════════════════════════════════
// User Account Management
// ════════════════════════════════════════════════════════════════

bool CWinUtils::UserExists(LPCTSTR userName)
{
    LPBYTE bufPtr = nullptr;
    NET_API_STATUS status = NetUserGetInfo(nullptr, userName, 0, &bufPtr);
    if (bufPtr)
        NetApiBufferFree(bufPtr);
    return (status == NERR_Success);
}

bool CWinUtils::CreateLocalUser(LPCTSTR userName, LPCTSTR password,
                                 LPCTSTR fullName, LPCTSTR description)
{
    USER_INFO_1 ui = {};
    ui.usri1_name     = const_cast<LPWSTR>(userName);
    ui.usri1_password  = const_cast<LPWSTR>(password);
    ui.usri1_priv      = USER_PRIV_USER;
    ui.usri1_home_dir  = nullptr;
    ui.usri1_comment   = const_cast<LPWSTR>(description);
    ui.usri1_flags     = UF_SCRIPT | UF_DONT_EXPIRE_PASSWD;
    ui.usri1_script_path = nullptr;

    DWORD paramErr = 0;
    NET_API_STATUS status = NetUserAdd(nullptr, 1, reinterpret_cast<LPBYTE>(&ui), &paramErr);

    if (status == NERR_Success)
    {
        // Set full name via level 1011
        USER_INFO_1011 fullNameInfo = {};
        fullNameInfo.usri1011_full_name = const_cast<LPWSTR>(fullName);
        NetUserSetInfo(nullptr, userName, 1011, reinterpret_cast<LPBYTE>(&fullNameInfo), nullptr);
        return true;
    }

    return (status == NERR_UserExists);  // Already exists is OK
}

bool CWinUtils::DeleteLocalUser(LPCTSTR userName)
{
    NET_API_STATUS status = NetUserDel(nullptr, const_cast<LPWSTR>(userName));
    return (status == NERR_Success || status == NERR_UserNotFound);
}

bool CWinUtils::SetUserPassword(LPCTSTR userName, LPCTSTR password)
{
    USER_INFO_1003 pwInfo = {};
    pwInfo.usri1003_password = const_cast<LPWSTR>(password);
    NET_API_STATUS status = NetUserSetInfo(nullptr, const_cast<LPWSTR>(userName),
                                           1003, reinterpret_cast<LPBYTE>(&pwInfo), nullptr);
    return (status == NERR_Success);
}

bool CWinUtils::IsUserInGroup(LPCTSTR userName, LPCTSTR groupName)
{
    LPLOCALGROUP_MEMBERS_INFO_3 pBuf = nullptr;
    DWORD entriesRead = 0, totalEntries = 0;
    NET_API_STATUS status = NetLocalGroupGetMembers(nullptr,
        const_cast<LPWSTR>(groupName), 3,
        reinterpret_cast<LPBYTE*>(&pBuf),
        MAX_PREFERRED_LENGTH, &entriesRead, &totalEntries, nullptr);

    bool found = false;
    if (status == NERR_Success && pBuf)
    {
        for (DWORD i = 0; i < entriesRead; i++)
        {
            CString memberName(pBuf[i].lgrmi3_domainandname);
            if (memberName.Find(userName) != -1)
            {
                found = true;
                break;
            }
        }
        NetApiBufferFree(pBuf);
    }
    return found;
}

bool CWinUtils::AddUserToGroup(LPCTSTR userName, LPCTSTR groupName)
{
    LOCALGROUP_MEMBERS_INFO_3 member = {};
    member.lgrmi3_domainandname = const_cast<LPWSTR>(userName);

    NET_API_STATUS status = NetLocalGroupAddMembers(nullptr,
        const_cast<LPWSTR>(groupName), 3,
        reinterpret_cast<LPBYTE>(&member), 1);

    return (status == NERR_Success || status == ERROR_MEMBER_IN_ALIAS);
}

bool CWinUtils::RemoveUserFromGroup(LPCTSTR userName, LPCTSTR groupName)
{
    LOCALGROUP_MEMBERS_INFO_3 member = {};
    member.lgrmi3_domainandname = const_cast<LPWSTR>(userName);

    NET_API_STATUS status = NetLocalGroupDelMembers(nullptr,
        const_cast<LPWSTR>(groupName), 3,
        reinterpret_cast<LPBYTE>(&member), 1);

    return (status == NERR_Success || status == ERROR_MEMBER_NOT_IN_ALIAS);
}

// ════════════════════════════════════════════════════════════════
// Firewall Management (COM)
// ════════════════════════════════════════════════════════════════

bool CWinUtils::FirewallRuleExists(LPCTSTR ruleName)
{
    bool exists = false;
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool comInit = SUCCEEDED(hr) || hr == S_FALSE;

    INetFwPolicy2* pPolicy = nullptr;
    hr = CoCreateInstance(__uuidof(NetFwPolicy2), nullptr, CLSCTX_INPROC_SERVER,
                          __uuidof(INetFwPolicy2), reinterpret_cast<void**>(&pPolicy));
    if (SUCCEEDED(hr) && pPolicy)
    {
        INetFwRules* pRules = nullptr;
        if (SUCCEEDED(pPolicy->get_Rules(&pRules)) && pRules)
        {
            INetFwRule* pRule = nullptr;
            _bstr_t bstrName(ruleName);
            hr = pRules->Item(bstrName, &pRule);
            if (SUCCEEDED(hr) && pRule)
            {
                exists = true;
                pRule->Release();
            }
            pRules->Release();
        }
        pPolicy->Release();
    }

    if (comInit) CoUninitialize();
    return exists;
}

bool CWinUtils::CreateFirewallRule(LPCTSTR ruleName, LPCTSTR description,
                                    LPCTSTR ports, LPCTSTR remoteAddresses)
{
    // First delete any existing rule with the same name
    DeleteFirewallRule(ruleName);

    bool success = false;
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool comInit = SUCCEEDED(hr) || hr == S_FALSE;

    INetFwPolicy2* pPolicy = nullptr;
    hr = CoCreateInstance(__uuidof(NetFwPolicy2), nullptr, CLSCTX_INPROC_SERVER,
                          __uuidof(INetFwPolicy2), reinterpret_cast<void**>(&pPolicy));
    if (SUCCEEDED(hr) && pPolicy)
    {
        INetFwRules* pRules = nullptr;
        if (SUCCEEDED(pPolicy->get_Rules(&pRules)) && pRules)
        {
            INetFwRule* pRule = nullptr;
            hr = CoCreateInstance(__uuidof(NetFwRule), nullptr, CLSCTX_INPROC_SERVER,
                                  __uuidof(INetFwRule), reinterpret_cast<void**>(&pRule));
            if (SUCCEEDED(hr) && pRule)
            {
                pRule->put_Name(_bstr_t(ruleName));
                pRule->put_Description(_bstr_t(description));
                pRule->put_Protocol(NET_FW_IP_PROTOCOL_TCP);
                pRule->put_LocalPorts(_bstr_t(ports));
                pRule->put_Direction(NET_FW_RULE_DIR_IN);
                pRule->put_Action(NET_FW_ACTION_ALLOW);
                pRule->put_Profiles(NET_FW_PROFILE2_ALL);
                pRule->put_Enabled(VARIANT_TRUE);

                if (remoteAddresses && _tcslen(remoteAddresses) > 0)
                    pRule->put_RemoteAddresses(_bstr_t(remoteAddresses));

                hr = pRules->Add(pRule);
                success = SUCCEEDED(hr);
                pRule->Release();
            }
            pRules->Release();
        }
        pPolicy->Release();
    }

    if (comInit) CoUninitialize();
    return success;
}

bool CWinUtils::DeleteFirewallRule(LPCTSTR ruleName)
{
    bool success = false;
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool comInit = SUCCEEDED(hr) || hr == S_FALSE;

    INetFwPolicy2* pPolicy = nullptr;
    hr = CoCreateInstance(__uuidof(NetFwPolicy2), nullptr, CLSCTX_INPROC_SERVER,
                          __uuidof(INetFwPolicy2), reinterpret_cast<void**>(&pPolicy));
    if (SUCCEEDED(hr) && pPolicy)
    {
        INetFwRules* pRules = nullptr;
        if (SUCCEEDED(pPolicy->get_Rules(&pRules)) && pRules)
        {
            hr = pRules->Remove(_bstr_t(ruleName));
            success = SUCCEEDED(hr);
            pRules->Release();
        }
        pPolicy->Release();
    }

    if (comInit) CoUninitialize();
    return success;
}

// ════════════════════════════════════════════════════════════════
// Firewall Rule Groups (Network Discovery, File Sharing)
// ════════════════════════════════════════════════════════════════

bool CWinUtils::EnableFirewallRuleGroup(LPCTSTR groupName, bool enable)
{
    // Use PowerShell to enable/disable firewall rule groups
    CString cmd;
    if (enable)
    {
        cmd.Format(_T("Get-NetFirewallRule -DisplayGroup '%s' | ")
                   _T("Where-Object { $_.Profile -match 'Private|Public' } | ")
                   _T("Set-NetFirewallRule -Enabled True"),
                   groupName);
    }
    else
    {
        cmd.Format(_T("Get-NetFirewallRule -DisplayGroup '%s' | ")
                   _T("Where-Object { $_.Profile -match 'Private|Public' } | ")
                   _T("Set-NetFirewallRule -Enabled False"),
                   groupName);
    }
    CString result = RunPowerShellCommand(cmd);
    return true;  // PowerShell doesn't return a clean error code for this
}

bool CWinUtils::IsFirewallRuleGroupEnabled(LPCTSTR groupName)
{
    CString cmd;
    cmd.Format(_T("(Get-NetFirewallRule -DisplayGroup '%s' | ")
               _T("Where-Object { $_.Profile -match 'Private' -and $_.Enabled -eq 'True' }).Count"),
               groupName);
    CString result = RunPowerShellCommand(cmd);
    result.Trim();
    return (_ttoi(result) > 0);
}

// ════════════════════════════════════════════════════════════════
// Network Adapter / Profile
// ════════════════════════════════════════════════════════════════

std::vector<AdapterInfo> CWinUtils::GetNetworkAdapters()
{
    std::vector<AdapterInfo> adapters;

    // Use PowerShell to get adapter info with network category
    CString cmd(_T("Get-NetConnectionProfile | ForEach-Object { ")
                _T("$ip = (Get-NetIPAddress -InterfaceIndex $_.InterfaceIndex -AddressFamily IPv4 -ErrorAction SilentlyContinue | Select-Object -First 1).IPAddress; ")
                _T("\"$($_.InterfaceIndex)|$($_.InterfaceAlias)|$($_.Name)|$($_.NetworkCategory)|$ip\" }"));
    CString result = RunPowerShellCommand(cmd);

    int pos = 0;
    CString line = result.Tokenize(_T("\r\n"), pos);
    while (!line.IsEmpty())
    {
        line.Trim();
        if (!line.IsEmpty())
        {
            // Parse: index|alias|name|category|ip
            int p = 0;
            CString idx = line.Tokenize(_T("|"), p);
            CString alias = line.Tokenize(_T("|"), p);
            CString name = line.Tokenize(_T("|"), p);
            CString category = line.Tokenize(_T("|"), p);
            CString ip = line.Tokenize(_T("|"), p);

            if (!idx.IsEmpty())
            {
                AdapterInfo ai;
                ai.interfaceIndex = _ttoi(idx);
                ai.alias = alias;
                ai.name = name;
                ai.networkCategory = category;
                ai.ipAddress = ip;
                adapters.push_back(ai);
            }
        }
        line = result.Tokenize(_T("\r\n"), pos);
    }

    return adapters;
}

AdapterInfo CWinUtils::FindTeamViewerVPNAdapter()
{
    AdapterInfo vpnAdapter = {};
    vpnAdapter.interfaceIndex = -1;

    auto adapters = GetNetworkAdapters();
    for (const auto& a : adapters)
    {
        // TeamViewer VPN adapters typically show as "Local Area Connection" with
        // LocalNetwork connectivity, or have "TeamViewer" in the name
        CString aliasLower = a.alias;
        aliasLower.MakeLower();
        CString nameLower = a.name;
        nameLower.MakeLower();

        if (aliasLower.Find(_T("teamviewer")) != -1 ||
            nameLower.Find(_T("teamviewer")) != -1)
        {
            vpnAdapter = a;
            return vpnAdapter;
        }

        // Also check for VPN-like adapters on the 7.x.x.x subnet
        if (!a.ipAddress.IsEmpty() && a.ipAddress.Left(2) == _T("7."))
        {
            vpnAdapter = a;
            return vpnAdapter;
        }
    }

    // Fallback: look for "Local Area Connection" (common TeamViewer VPN alias)
    for (const auto& a : adapters)
    {
        if (a.alias.Find(_T("Local Area Connection")) != -1)
        {
            vpnAdapter = a;
            return vpnAdapter;
        }
    }

    return vpnAdapter;
}

bool CWinUtils::SetNetworkProfilePrivate(int interfaceIndex)
{
    CString cmd;
    cmd.Format(_T("Set-NetConnectionProfile -InterfaceIndex %d -NetworkCategory Private"), interfaceIndex);
    RunPowerShellCommand(cmd);
    // Verify
    CString cat = GetAdapterNetworkCategory(interfaceIndex);
    return (cat.Find(_T("Private")) != -1);
}

CString CWinUtils::GetAdapterNetworkCategory(int interfaceIndex)
{
    CString cmd;
    cmd.Format(_T("(Get-NetConnectionProfile -InterfaceIndex %d).NetworkCategory"), interfaceIndex);
    CString result = RunPowerShellCommand(cmd);
    result.Trim();
    return result;
}

// ════════════════════════════════════════════════════════════════
// SMB Share Management
// ════════════════════════════════════════════════════════════════

bool CWinUtils::ShareExists(LPCTSTR shareName)
{
    LPBYTE buf = nullptr;
    NET_API_STATUS status = NetShareGetInfo(nullptr, const_cast<LPWSTR>(shareName),
                                             0, &buf);
    if (buf) NetApiBufferFree(buf);
    return (status == NERR_Success);
}

bool CWinUtils::CreateSMBShare(LPCTSTR shareName, LPCTSTR path, LPCTSTR userName)
{
    // Build a security descriptor that grants full control to the specified user
    // For simplicity, we'll create the share with Everyone=FullControl
    // and rely on NTFS permissions for actual access control
    SHARE_INFO_2 si = {};
    si.shi2_netname     = const_cast<LPWSTR>(shareName);
    si.shi2_type        = STYPE_DISKTREE;
    si.shi2_remark      = const_cast<LPWSTR>(_T("Remote Debug Share"));
    si.shi2_permissions  = ACCESS_ALL;
    si.shi2_max_uses     = static_cast<DWORD>(-1);
    si.shi2_current_uses = 0;
    si.shi2_path         = const_cast<LPWSTR>(path);
    si.shi2_passwd       = nullptr;

    DWORD paramErr = 0;
    NET_API_STATUS status = NetShareAdd(nullptr, 2, reinterpret_cast<LPBYTE>(&si), &paramErr);

    if (status == NERR_Success || status == NERR_DuplicateShare)
    {
        // Grant share-level permissions to the user via PowerShell
        CString cmd;
        cmd.Format(_T("Grant-SmbShareAccess -Name '%s' -AccountName '%s' -AccessRight Full -Force"),
                   shareName, userName);
        RunPowerShellCommand(cmd);
        return true;
    }

    return false;
}

bool CWinUtils::DeleteSMBShare(LPCTSTR shareName)
{
    NET_API_STATUS status = NetShareDel(nullptr, const_cast<LPWSTR>(shareName), 0);
    return (status == NERR_Success || status == NERR_NetNameNotFound);
}

// ════════════════════════════════════════════════════════════════
// NTFS Permissions
// ════════════════════════════════════════════════════════════════

bool CWinUtils::GrantNTFSPermissions(LPCTSTR path, LPCTSTR userName)
{
    // Use icacls to grant Modify with inheritance (OI=Object Inherit, CI=Container Inherit).
    // This is fast: it sets the inheritable ACE on the root folder only and lets
    // Windows propagate via normal inheritance, avoiding the stall that
    // SetNamedSecurityInfo causes when it walks every child object.
    CString cmd;
    cmd.Format(_T("icacls.exe \"%s\" /grant \"%s\":(OI)(CI)M"), path, userName);
    DWORD exitCode = RunHiddenCommand(cmd);
    return (exitCode == 0);
}

bool CWinUtils::RemoveNTFSPermissions(LPCTSTR path, LPCTSTR userName)
{
    // Use icacls to remove the user's ACE (including inherited entries).
    CString cmd;
    cmd.Format(_T("icacls.exe \"%s\" /remove \"%s\""), path, userName);
    DWORD exitCode = RunHiddenCommand(cmd);
    return (exitCode == 0);
}

// ════════════════════════════════════════════════════════════════
// ── Run a command hidden and wait for it to finish ──
static DWORD RunHiddenCmd(LPCTSTR commandLine)
{
    return CWinUtils::RunHiddenCommand(commandLine);
}

DWORD CWinUtils::RunHiddenCommand(LPCTSTR commandLine)
{
    CString cmd(commandLine);
    STARTUPINFO si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};

    if (CreateProcess(nullptr, cmd.GetBuffer(), nullptr, nullptr, FALSE,
                      CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
    {
        cmd.ReleaseBuffer();
        WaitForSingleObject(pi.hProcess, 30000);
        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return exitCode;
    }
    cmd.ReleaseBuffer();
    return (DWORD)-1;
}

// ── Enable shared drive mappings between elevated and non-elevated sessions ──
// Sets the EnableLinkedConnections registry key. Takes effect after re-login.
static void EnsureLinkedConnections()
{
    HKEY hKey = nullptr;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"),
                     0, KEY_READ | KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
    {
        DWORD value = 0, size = sizeof(value);
        if (RegQueryValueEx(hKey, _T("EnableLinkedConnections"), nullptr, nullptr,
                            reinterpret_cast<LPBYTE>(&value), &size) != ERROR_SUCCESS
            || value != 1)
        {
            value = 1;
            RegSetValueEx(hKey, _T("EnableLinkedConnections"), 0, REG_DWORD,
                          reinterpret_cast<const BYTE*>(&value), sizeof(value));
        }
        RegCloseKey(hKey);
    }
}

// Drive Mapping
// ════════════════════════════════════════════════════════════════

bool CWinUtils::MapNetworkDrive(TCHAR driveLetter, LPCTSTR uncPath,
                                 LPCTSTR userName, LPCTSTR password)
{
    CString localName;
    localName.Format(_T("%c:"), driveLetter);

    // First disconnect any existing mapping on this letter
    UnmapNetworkDrive(driveLetter);

    NETRESOURCE nr = {};
    nr.dwType = RESOURCETYPE_DISK;
    nr.lpLocalName = localName.GetBuffer();
    nr.lpRemoteName = const_cast<LPTSTR>(uncPath);
    nr.lpProvider = nullptr;

    DWORD result = WNetAddConnection2(&nr, password, userName,
                                       CONNECT_UPDATE_PROFILE);
    localName.ReleaseBuffer();

    if (result != NO_ERROR)
        return false;

    // Set EnableLinkedConnections for future logins (shares drive mappings
    // between elevated and non-elevated sessions after reboot).
    if (IsRunningAsAdmin())
        EnsureLinkedConnections();

    return true;
}

bool CWinUtils::UnmapNetworkDrive(TCHAR driveLetter)
{
    CString localName;
    localName.Format(_T("%c:"), driveLetter);

    DWORD result = WNetCancelConnection2(localName, CONNECT_UPDATE_PROFILE, TRUE);

    // Also unmap in non-elevated (Explorer) session when running as admin.
    // Uses schtasks with /rl limited /it to run in the interactive session.
    if (IsRunningAsAdmin())
    {
        CString cmd;
        cmd.Format(
            _T("schtasks.exe /create /tn \"RDS_UnmapDrive\" /tr \"net.exe use %c: /del /y\" /sc once /st 00:00 /f /rl limited /it"),
            driveLetter);
        RunHiddenCmd(cmd);
        RunHiddenCmd(_T("schtasks.exe /run /tn \"RDS_UnmapDrive\""));
        Sleep(2000);
        RunHiddenCmd(_T("schtasks.exe /delete /tn \"RDS_UnmapDrive\" /f"));
    }

    return (result == NO_ERROR || result == ERROR_NOT_CONNECTED);
}

bool CWinUtils::IsDriveMapped(TCHAR driveLetter)
{
    CString localName;
    localName.Format(_T("%c:"), driveLetter);

    TCHAR remoteName[MAX_PATH];
    DWORD bufSize = MAX_PATH;
    DWORD result = WNetGetConnection(localName, remoteName, &bufSize);
    return (result == NO_ERROR);
}

// ════════════════════════════════════════════════════════════════
// Connectivity
// ════════════════════════════════════════════════════════════════

bool CWinUtils::PingHost(LPCTSTR ipAddress)
{
    // Convert IP string to address
    IN_ADDR addr;
    CStringA ipA(ipAddress);
    if (inet_pton(AF_INET, ipA, &addr) != 1)
        return false;

    HANDLE hIcmp = IcmpCreateFile();
    if (hIcmp == INVALID_HANDLE_VALUE)
        return false;

    char sendData[] = "PingTest";
    DWORD replySize = sizeof(ICMP_ECHO_REPLY) + sizeof(sendData) + 8;
    LPVOID replyBuf = malloc(replySize);
    if (!replyBuf)
    {
        IcmpCloseHandle(hIcmp);
        return false;
    }

    DWORD retVal = IcmpSendEcho(hIcmp, addr.S_un.S_addr, sendData,
                                 sizeof(sendData), nullptr, replyBuf, replySize, 2000);

    bool success = (retVal > 0);
    free(replyBuf);
    IcmpCloseHandle(hIcmp);
    return success;
}

bool CWinUtils::TestTcpPort(LPCTSTR ipAddress, int port, int timeoutMs)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return false;

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
    {
        WSACleanup();
        return false;
    }

    // Set non-blocking
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<u_short>(port));
    CStringA ipA(ipAddress);
    inet_pton(AF_INET, ipA, &addr.sin_addr);

    connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(sock, &writeSet);

    timeval tv;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;

    bool connected = (select(0, nullptr, &writeSet, nullptr, &tv) > 0);

    closesocket(sock);
    WSACleanup();
    return connected;
}

// ════════════════════════════════════════════════════════════════
// Utility
// ════════════════════════════════════════════════════════════════

CString CWinUtils::GetComputerHostName()
{
    TCHAR name[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
    if (GetComputerName(name, &size))
        return CString(name);
    return _T("UNKNOWN");
}

CString CWinUtils::RunPowerShellCommand(LPCTSTR command)
{
    CString result;

    // Create a temp file for output
    TCHAR tempPath[MAX_PATH], tempFile[MAX_PATH];
    GetTempPath(MAX_PATH, tempPath);
    GetTempFileName(tempPath, _T("rds"), 0, tempFile);

    // Build PowerShell command line
    CString cmdLine;
    cmdLine.Format(
        _T("powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \"& { %s } 2>&1 | Out-File -FilePath '%s' -Encoding ascii\""),
        command, tempFile);

    // Execute
    STARTUPINFO si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {};
    if (CreateProcess(nullptr, cmdLine.GetBuffer(), nullptr, nullptr, FALSE,
                      CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
    {
        cmdLine.ReleaseBuffer();
        WaitForSingleObject(pi.hProcess, 30000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        // Read output
        CStdioFile file;
        if (file.Open(tempFile, CFile::modeRead | CFile::typeText))
        {
            CString line;
            while (file.ReadString(line))
            {
                if (!result.IsEmpty())
                    result += _T("\r\n");
                result += line;
            }
            file.Close();
        }
    }
    else
    {
        cmdLine.ReleaseBuffer();
    }

    DeleteFile(tempFile);
    return result;
}

CString CWinUtils::GetLastErrorMessage(DWORD errorCode)
{
    if (errorCode == 0)
        errorCode = ::GetLastError();

    LPTSTR msgBuf = nullptr;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                  nullptr, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  reinterpret_cast<LPTSTR>(&msgBuf), 0, nullptr);

    CString msg;
    if (msgBuf)
    {
        msg = msgBuf;
        msg.Trim();
        LocalFree(msgBuf);
    }
    else
    {
        msg.Format(_T("Error code: %lu"), errorCode);
    }
    return msg;
}
