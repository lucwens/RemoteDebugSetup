#pragma once
// TeamViewerUtils.h - TeamViewer VPN detection and management

#include <afxwin.h>

class CTeamViewerUtils
{
public:
    // Check if TeamViewer is installed (checks registry)
    static bool IsTeamViewerInstalled();

    // Check if the TeamViewer VPN driver/adapter is installed
    static bool IsVPNDriverInstalled();

    // Get the TeamViewer VPN adapter's local IP address
    static CString GetVPNIPAddress();

    // Get the TeamViewer installation path
    static CString GetTeamViewerPath();

    // Attempt to install the VPN driver (launches TeamViewer config)
    static bool InstallVPNDriver();

    // Initiate a VPN connection to a partner (launches TeamViewer with instructions)
    static bool ConnectVPN(const CString& partnerID);

    // Check if Visual Studio Remote Debugger (msvsmon.exe) is installed
    static bool IsRemoteDebuggerInstalled();

    // Get the path to msvsmon.exe
    static CString GetRemoteDebuggerPath();

    // Open the Microsoft download page for Remote Tools
    static void OpenRemoteToolsDownloadPage();
};
