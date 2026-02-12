#include "pch.h"
#include "TeamViewerUtils.h"
#include "WinUtils.h"

bool CTeamViewerUtils::IsTeamViewerInstalled()
{
    // Check registry for TeamViewer installation
    HKEY hKey = nullptr;
    bool found = false;

    // Check 64-bit registry
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\TeamViewer"),
                     0, KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
    {
        found = true;
        RegCloseKey(hKey);
    }

    // Check 32-bit registry
    if (!found && RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\WOW6432Node\\TeamViewer"),
                                0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        found = true;
        RegCloseKey(hKey);
    }

    // Also check if the executable exists
    if (!found)
    {
        CString path = GetTeamViewerPath();
        found = !path.IsEmpty();
    }

    return found;
}

bool CTeamViewerUtils::IsVPNDriverInstalled()
{
    // Check if a TeamViewer VPN network adapter exists
    AdapterInfo vpn = CWinUtils::FindTeamViewerVPNAdapter();
    if (vpn.interfaceIndex >= 0)
        return true;

    // Also check via PowerShell for the adapter even if disconnected
    CString result = CWinUtils::RunPowerShellCommand(
        _T("Get-NetAdapter -Name '*TeamViewer*' -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Name"));
    result.Trim();
    if (!result.IsEmpty())
        return true;

    // Check for the VPN adapter in Device Manager
    result = CWinUtils::RunPowerShellCommand(
        _T("Get-WmiObject Win32_NetworkAdapter | Where-Object { $_.Name -like '*TeamViewer*' } | Select-Object -ExpandProperty Name"));
    result.Trim();
    return !result.IsEmpty();
}

CString CTeamViewerUtils::GetVPNIPAddress()
{
    AdapterInfo vpn = CWinUtils::FindTeamViewerVPNAdapter();
    if (vpn.interfaceIndex >= 0 && !vpn.ipAddress.IsEmpty())
        return vpn.ipAddress;

    // Fallback: look for any adapter on the 7.x.x.x subnet
    CString result = CWinUtils::RunPowerShellCommand(
        _T("Get-NetIPAddress -AddressFamily IPv4 | Where-Object { $_.IPAddress -like '7.*' } | ")
        _T("Select-Object -First 1 -ExpandProperty IPAddress"));
    result.Trim();
    return result;
}

CString CTeamViewerUtils::GetTeamViewerPath()
{
    // Common installation paths
    LPCTSTR paths[] = {
        _T("C:\\Program Files\\TeamViewer\\TeamViewer.exe"),
        _T("C:\\Program Files (x86)\\TeamViewer\\TeamViewer.exe"),
    };

    for (auto& p : paths)
    {
        if (GetFileAttributes(p) != INVALID_FILE_ATTRIBUTES)
            return CString(p);
    }

    // Check registry for install location
    HKEY hKey = nullptr;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\TeamViewer"),
                     0, KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
    {
        TCHAR path[MAX_PATH];
        DWORD size = sizeof(path);
        if (RegQueryValueEx(hKey, _T("InstallationDirectory"), nullptr, nullptr,
                            reinterpret_cast<LPBYTE>(path), &size) == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            CString fullPath;
            fullPath.Format(_T("%s\\TeamViewer.exe"), path);
            if (GetFileAttributes(fullPath) != INVALID_FILE_ATTRIBUTES)
                return fullPath;
        }
        RegCloseKey(hKey);
    }

    return _T("");
}

bool CTeamViewerUtils::InstallVPNDriver()
{
    CString tvPath = GetTeamViewerPath();
    if (tvPath.IsEmpty())
    {
        AfxMessageBox(_T("TeamViewer is not installed. Please install TeamViewer first.\n\n")
                      _T("After installing TeamViewer:\n")
                      _T("1. Open TeamViewer\n")
                      _T("2. Go to Settings > Advanced > Advanced network settings\n")
                      _T("3. Click 'Install' next to 'Install VPN driver'"),
                      MB_OK | MB_ICONINFORMATION);
        return false;
    }

    // Show instructions to the user
    CString msg;
    msg.Format(_T("To install the TeamViewer VPN driver:\n\n")
               _T("1. Open TeamViewer (%s)\n")
               _T("2. Go to Settings (gear icon) > Advanced\n")
               _T("3. Click 'Open advanced settings'\n")
               _T("4. Scroll to 'Advanced network settings'\n")
               _T("5. Click 'Install' next to 'Install VPN driver'\n\n")
               _T("Would you like to open TeamViewer now?"),
               (LPCTSTR)tvPath);

    if (AfxMessageBox(msg, MB_YESNO | MB_ICONQUESTION) == IDYES)
    {
        ShellExecute(nullptr, _T("open"), tvPath, nullptr, nullptr, SW_SHOWNORMAL);
    }

    return true;
}

bool CTeamViewerUtils::IsRemoteDebuggerInstalled()
{
    return !GetRemoteDebuggerPath().IsEmpty();
}

CString CTeamViewerUtils::GetRemoteDebuggerPath()
{
    // Search common paths for msvsmon.exe
    LPCTSTR searchPaths[] = {
        // VS 2026 (hypothetical)
        _T("C:\\Program Files\\Microsoft Visual Studio\\2026\\Enterprise\\Common7\\IDE\\Remote Debugger\\x64\\msvsmon.exe"),
        _T("C:\\Program Files\\Microsoft Visual Studio\\2026\\Professional\\Common7\\IDE\\Remote Debugger\\x64\\msvsmon.exe"),
        _T("C:\\Program Files\\Microsoft Visual Studio\\2026\\Community\\Common7\\IDE\\Remote Debugger\\x64\\msvsmon.exe"),
        // VS 2022
        _T("C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\Common7\\IDE\\Remote Debugger\\x64\\msvsmon.exe"),
        _T("C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\Common7\\IDE\\Remote Debugger\\x64\\msvsmon.exe"),
        _T("C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\Common7\\IDE\\Remote Debugger\\x64\\msvsmon.exe"),
        _T("C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\Common7\\IDE\\Remote Debugger\\x64\\msvsmon.exe"),
        // Standalone Remote Tools
        _T("C:\\Program Files\\Microsoft Visual Studio 18.0\\Common7\\IDE\\Remote Debugger\\x64\\msvsmon.exe"),
        _T("C:\\Program Files\\Microsoft Visual Studio 17.0\\Common7\\IDE\\Remote Debugger\\x64\\msvsmon.exe"),
    };

    for (auto& p : searchPaths)
    {
        if (GetFileAttributes(p) != INVALID_FILE_ATTRIBUTES)
            return CString(p);
    }

    // Fallback: search with PowerShell
    CString result = CWinUtils::RunPowerShellCommand(
        _T("Get-ChildItem -Path '${env:ProgramFiles}' -Filter 'msvsmon.exe' -Recurse -ErrorAction SilentlyContinue | ")
        _T("Select-Object -First 1 -ExpandProperty FullName"));
    result.Trim();
    if (!result.IsEmpty() && GetFileAttributes(result) != INVALID_FILE_ATTRIBUTES)
        return result;

    return _T("");
}

void CTeamViewerUtils::OpenRemoteToolsDownloadPage()
{
    // Open the Microsoft download page for Remote Tools for Visual Studio
    ShellExecute(nullptr, _T("open"),
                 _T("https://visualstudio.microsoft.com/downloads/#remote-tools-for-visual-studio-2022"),
                 nullptr, nullptr, SW_SHOWNORMAL);
}
