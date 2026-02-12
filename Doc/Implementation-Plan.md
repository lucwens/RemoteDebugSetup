# Remote Debug Setup - Implementation Plan

## Overview

Two MFC dialog-based C++ applications for Visual Studio 2026 that automate the setup of remote debugging between a **Development PC** and a **Test PC** connected via TeamViewer VPN.

| Program | Runs On | Purpose |
|---------|---------|---------|
| **SetupDevelop** | Development PC | Shares a local folder and configures the Dev PC so the Test PC can access it over TeamViewer VPN |
| **SetupTest** | Test PC | Installs prerequisites (TeamViewer VPN driver, VS Remote Debugger), maps the shared folder, and configures the debugger |

Both programs must run **elevated** (Administrator) and provide a **Restore** button to undo all changes.

---

## Solution Structure

```
RemoteDebugSetup.sln                    (VS 2026 Solution)
├── SetupDevelop/                        (MFC Dialog App project)
│   ├── SetupDevelop.vcxproj
│   ├── SetupDevelop.rc                  (Dialog resource)
│   ├── resource.h
│   ├── SetupDevelop.h / .cpp            (CWinApp)
│   ├── SetupDevelopDlg.h / .cpp         (Main dialog)
│   ├── targetver.h
│   ├── pch.h / pch.cpp                  (Precompiled header)
│   ├── framework.h
│   ├── SetupDevelop.manifest            (requireAdministrator)
│   └── res/
│       └── SetupDevelop.ico
├── SetupTest/                           (MFC Dialog App project)
│   ├── SetupTest.vcxproj
│   ├── SetupTest.rc
│   ├── resource.h
│   ├── SetupTest.h / .cpp
│   ├── SetupTestDlg.h / .cpp
│   ├── targetver.h
│   ├── pch.h / pch.cpp
│   ├── framework.h
│   ├── SetupTest.manifest               (requireAdministrator)
│   └── res/
│       └── SetupTest.ico
├── Common/                              (Shared utility code)
│   ├── WinUtils.h / .cpp               (Firewall, user account, network helpers)
│   ├── TeamViewerUtils.h / .cpp         (TeamViewer VPN detection & installation)
│   ├── RegistryBackup.h / .cpp          (Save/restore state)
│   └── LogUtils.h / .cpp               (Logging to edit control + file)
└── Doc/
    └── Implementation-Plan.md           (This document)
```

---

## Common Utilities (`Common/`)

Shared code used by both programs. Linked as additional source files (not a separate lib) for simplicity.

### WinUtils (Windows Configuration Helpers)

| Function | Description |
|----------|-------------|
| `CreateLocalUser(name, password, fullName)` | Creates a local Windows account using `NetUserAdd` |
| `DeleteLocalUser(name)` | Removes a local account using `NetUserDel` |
| `UserExists(name) → bool` | Checks if a local user exists via `NetUserGetInfo` |
| `AddUserToGroup(user, group)` | Adds user to local group (e.g., Administrators) via `NetLocalGroupAddMembers` |
| `RemoveUserFromGroup(user, group)` | Removes user from group |
| `IsUserInGroup(user, group) → bool` | Checks group membership |
| `SetNetworkProfilePrivate(adapterAlias)` | Sets network adapter to Private via WMI/PowerShell |
| `GetNetworkAdapters() → vector<AdapterInfo>` | Enumerates adapters with profile type |
| `EnableNetworkDiscovery(enable)` | Toggles network discovery firewall rules |
| `EnableFileSharing(enable)` | Toggles file and printer sharing rules |
| `CreateFirewallRule(name, port, protocol, subnet)` | Creates inbound firewall rule via `INetFwPolicy2` COM |
| `DeleteFirewallRule(name)` | Removes a named firewall rule |
| `FirewallRuleExists(name) → bool` | Checks if rule exists |
| `CreateSMBShare(shareName, path, user)` | Creates a network share via `NetShareAdd` |
| `DeleteSMBShare(shareName)` | Removes a share via `NetShareDel` |
| `ShareExists(shareName) → bool` | Checks if share exists |
| `SetNTFSPermissions(path, user, access)` | Modifies NTFS ACL for a folder |
| `MapNetworkDrive(driveLetter, uncPath, user, password)` | Maps a drive via `WNetAddConnection2` |
| `UnmapNetworkDrive(driveLetter)` | Unmaps a drive via `WNetCancelConnection2` |
| `RunElevated() → bool` | Checks if running as Administrator |
| `GetComputerName() → CString` | Returns local hostname |
| `PingHost(ip) → bool` | Tests ICMP connectivity |
| `TestTcpPort(ip, port) → bool` | Tests if a TCP port is reachable |
| `RunPowerShellCommand(cmd) → CString` | Executes a PowerShell command and captures output |

### TeamViewerUtils

| Function | Description |
|----------|-------------|
| `IsTeamViewerInstalled() → bool` | Checks registry for TeamViewer installation |
| `IsVPNDriverInstalled() → bool` | Checks for TeamViewer VPN network adapter |
| `GetTeamViewerVPNAdapter() → AdapterInfo` | Returns the VPN adapter details |
| `GetVPNIPAddress() → CString` | Returns the local VPN IP address |
| `InstallVPNDriver()` | Launches TeamViewer VPN driver installation (guided) |

### RegistryBackup (State Save/Restore)

Stores original system state before modifications so everything can be reverted.

| Function | Description |
|----------|-------------|
| `SaveState(key, value)` | Persists a key-value pair to a JSON config file |
| `LoadState(key) → CString` | Reads back a saved value |
| `HasSavedState() → bool` | Returns true if a backup state file exists |
| `ClearState()` | Deletes the backup file |
| `GetStateFilePath() → CString` | Returns path: `%APPDATA%\RemoteDebugSetup\state.json` |

**State items tracked:**

For SetupDevelop:
- `rd_user_existed` — whether the RD account existed before setup
- `rd_was_admin` — whether RD was already in Administrators group
- `vpn_adapter_was_private` — original network category of VPN adapter
- `network_discovery_was_enabled` — original state
- `file_sharing_was_enabled` — original state
- `smb_firewall_rule_existed` — whether rule existed
- `debugger_firewall_rule_existed` — whether rule existed
- `share_existed` — whether the share already existed
- `share_name` — the share name created
- `share_path` — the shared folder path
- `original_ntfs_acl` — serialized original ACL

For SetupTest:
- `rd_user_existed`
- `rd_was_admin`
- `debugger_firewall_rule_existed`
- `drive_was_mapped` — whether a drive was already mapped to this letter
- `drive_letter` — the mapped drive letter
- `original_drive_mapping` — what was previously on that letter
- `vs_remote_tools_installed_by_us` — whether we installed the Remote Tools

### LogUtils

| Function | Description |
|----------|-------------|
| `Log(level, message)` | Appends to a CEdit control (in the dialog) and optionally to a log file |
| `SetLogControl(CEdit*)` | Sets the edit control target |
| `LogSuccess(msg)` | Green-prefixed success message |
| `LogWarning(msg)` | Yellow-prefixed warning |
| `LogError(msg)` | Red-prefixed error |

---

## SetupDevelop — Detailed Design

### Dialog Layout

```
┌─────────────────────────────────────────────────────────────────┐
│  Remote Debug Setup — Development PC                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  TeamViewer VPN Status:  [●  Connected — IP: 7.238.81.240  ]   │
│                                                                 │
│  ── Configuration ──────────────────────────────────────────    │
│                                                                 │
│  RD Account Password:    [________a________]                    │
│                                                                 │
│  Folder to Share:        [_C:\CTrack-software_] [Browse...]     │
│                                                                 │
│  Share Name:             [_CTrack-software____]                 │
│                                                                 │
│  VPN Subnet:             [_7.0.0.0/8__________]                 │
│                                                                 │
│  ── Status Log ─────────────────────────────────────────────    │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ [1/6] Creating local RD account...                      │    │
│  │   ✔ RD account created.                                 │    │
│  │ [2/6] Configuring VPN adapter to Private...             │    │
│  │   ✔ Adapter set to Private.                             │    │
│  │ ...                                                     │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                 │
│  [  Setup  ]    [  Restore  ]    [  Close  ]                    │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### Setup Steps (executed when "Setup" button is clicked)

| Step | Action | Win32 API / Mechanism |
|------|--------|-----------------------|
| 1 | **Check admin elevation** | `IsUserAnAdmin()` — abort with message if not elevated |
| 2 | **Detect TeamViewer VPN** | Enumerate network adapters, look for TeamViewer VPN adapter; show VPN IP |
| 3 | **Create RD local account** | `NetUserAdd` (level 1); set `USER_PRIV_USER`; if exists, `NetUserSetInfo` to update password |
| 4 | **Add RD to Administrators** | `NetLocalGroupAddMembers` with group "Administrators" |
| 5 | **Set VPN adapter to Private** | PowerShell: `Set-NetConnectionProfile -InterfaceIndex N -NetworkCategory Private` |
| 6 | **Enable Network Discovery** | Enable firewall rule group "Network Discovery" for Private and Public profiles |
| 7 | **Enable File and Printer Sharing** | Enable firewall rule group "File and Printer Sharing" for Private and Public profiles |
| 8 | **Create SMB firewall rule** | `INetFwPolicy2` COM: inbound TCP 445, remote address = VPN subnet, all profiles |
| 9 | **Create Remote Debugger firewall rule** | `INetFwPolicy2` COM: inbound TCP 4022-4026, remote address = VPN subnet, all profiles |
| 10 | **Share the folder** | `NetShareAdd` level 502; set share permissions for RD (Full Control) |
| 11 | **Set NTFS permissions** | `SetNamedSecurityInfo` / `AddAccessAllowedAceEx` for RD with Modify access |
| 12 | **Display summary** | Show hostname, VPN IP, share UNC path, credentials in the log |

### Restore Steps (executed when "Restore" button is clicked)

Reads saved state and undoes each change in reverse order:

| Step | Action |
|------|--------|
| 1 | Remove NTFS permissions for RD |
| 2 | Remove the SMB share |
| 3 | Delete the Remote Debugger firewall rule |
| 4 | Delete the SMB firewall rule |
| 5 | Restore original network discovery/file sharing state |
| 6 | Restore original VPN adapter network profile |
| 7 | Remove RD from Administrators (if wasn't admin before) |
| 8 | Delete RD account (if didn't exist before) |
| 9 | Clear saved state |

### Input Validation

- Folder path must exist
- Share name must be a valid NetBIOS name (no special chars, ≤ 80 chars)
- Password must not be empty
- VPN subnet must be valid CIDR notation
- TeamViewer VPN must be connected (warning if not, but allow to proceed)

---

## SetupTest — Detailed Design

### Dialog Layout

```
┌─────────────────────────────────────────────────────────────────┐
│  Remote Debug Setup — Test PC                                   │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ── Prerequisites ──────────────────────────────────────────    │
│                                                                 │
│  TeamViewer VPN:         [● Installed]     [Install...]         │
│  VS Remote Debugger:     [● Installed]     [Install...]         │
│                                                                 │
│  ── Dev PC Connection ──────────────────────────────────────    │
│                                                                 │
│  Dev PC Hostname:        [_DELL________________]                │
│  Dev PC VPN IP:          [_7.238.81.240________]                │
│  Share Name:             [_CTrack-software_____]                │
│  RD Account Password:    [________a____________]                │
│  Map to Drive Letter:    [Z ▼]                                  │
│  Debugger Port:          [_4026________________]                │
│                                                                 │
│  ── Status Log ─────────────────────────────────────────────    │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ [1/5] Creating local RD account...                      │    │
│  │   ✔ RD account created.                                 │    │
│  │ [2/5] Creating firewall rule for Remote Debugger...     │    │
│  │   ✔ Firewall rule created (ports 4022-4026).            │    │
│  │ ...                                                     │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                 │
│  [  Setup  ]    [  Restore  ]    [  Close  ]                    │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### Prerequisite Checks (performed on dialog initialization)

| Check | How | If Not Found |
|-------|-----|--------------|
| **TeamViewer installed** | Registry: `HKLM\SOFTWARE\TeamViewer` or `HKLM\SOFTWARE\WOW6432Node\TeamViewer` | Show warning — user must install TeamViewer manually |
| **TeamViewer VPN driver** | Enumerate adapters: look for "TeamViewer VPN" adapter via `GetAdaptersInfo` / WMI | Enable **[Install...]** button → runs `teamviewer.exe --install-vpn` or guides user |
| **VS Remote Debug Tools** | Search for `msvsmon.exe` in known paths:<br>• `%ProgramFiles%\Microsoft Visual Studio\2022\*\Common7\IDE\Remote Debugger\x64\msvsmon.exe`<br>• `%ProgramFiles%\Microsoft Visual Studio 17.0\...`<br>• Recursive search in `%ProgramFiles%` | Enable **[Install...]** button → opens Microsoft download page for Remote Tools for VS 2022/2026 |

### Setup Steps (executed when "Setup" button is clicked)

| Step | Action | Win32 API / Mechanism |
|------|--------|-----------------------|
| 1 | **Check admin elevation** | `IsUserAnAdmin()` |
| 2 | **Detect TeamViewer VPN** | Verify VPN adapter exists and has an IP; show VPN IP |
| 3 | **Create RD local account** | `NetUserAdd` / `NetUserSetInfo` (same password as Dev PC) |
| 4 | **Add RD to Administrators** | `NetLocalGroupAddMembers` |
| 5 | **Create Remote Debugger firewall rule** | `INetFwPolicy2` COM: inbound TCP 4022-4026, all profiles |
| 6 | **Verify VPN connectivity** | Ping Dev PC VPN IP; test TCP port 445 |
| 7 | **Map shared folder** | `WNetAddConnection2` with `CONNECT_UPDATE_PROFILE` for persistence; credentials = `DEVPCNAME\RD` |
| 8 | **Verify mapped drive** | Check if the mapped drive letter is accessible |
| 9 | **Locate Remote Debugger** | Find `msvsmon.exe` and display its path |
| 10 | **Display summary** | Show mapped drive, debugger location, next steps |

### Restore Steps

| Step | Action |
|------|--------|
| 1 | Unmap the network drive |
| 2 | Delete the Remote Debugger firewall rule |
| 3 | Remove RD from Administrators (if wasn't admin before) |
| 4 | Delete RD account (if didn't exist before) |
| 5 | Clear saved state |

Note: VS Remote Tools and TeamViewer VPN driver are NOT uninstalled during restore (they are heavyweight installs that the user may want to keep).

### Input Validation

- Dev PC VPN IP must be a valid IPv4 address
- Dev PC hostname must not be empty
- Share name must not be empty
- Password must not be empty
- Drive letter must be A-Z and not already in use (or warn)
- Debugger port must be 1024-65535

---

## Implementation Steps

### Phase 1: Solution & Project Scaffolding
1. Create VS 2026 solution file (`RemoteDebugSetup.sln`)
2. Create `SetupDevelop.vcxproj` and `SetupTest.vcxproj` with MFC dialog app settings
3. Set up precompiled headers, framework includes, app manifests (requireAdministrator)
4. Create the `Common/` folder with header/source stubs

### Phase 2: Common Utilities
5. Implement `LogUtils` — logging to CEdit control
6. Implement `RegistryBackup` — JSON-based state file under `%APPDATA%`
7. Implement `WinUtils` — all firewall, user account, share, network, and NTFS helpers
8. Implement `TeamViewerUtils` — VPN detection and installation

### Phase 3: SetupDevelop
9. Create dialog resource (`IDD_SETUPDEVELOP_DIALOG`) with all controls
10. Implement dialog class with DDX/DDV bindings
11. Implement OnInitDialog — detect VPN, load saved state if any
12. Implement OnSetup — execute all 12 setup steps with logging
13. Implement OnRestore — undo all changes using saved state
14. Implement Browse button for folder selection

### Phase 4: SetupTest
15. Create dialog resource (`IDD_SETUPTEST_DIALOG`) with all controls
16. Implement dialog class with DDX/DDV bindings
17. Implement OnInitDialog — run prerequisite checks, detect VPN
18. Implement Install buttons — launch VPN driver install, open Remote Tools download
19. Implement OnSetup — execute all 10 setup steps with logging
20. Implement OnRestore — undo all changes using saved state

### Phase 5: Testing & Polish
21. Test both programs individually
22. Verify restore functionality undoes all changes cleanly
23. Add error handling for edge cases (VPN not connected, share already exists, etc.)
24. Final code review and cleanup

---

## Technical Notes

### Elevation (UAC)
Both applications embed an application manifest (`requireAdministrator`) so Windows prompts for elevation automatically:
```xml
<requestedExecutionLevel level="requireAdministrator" uiAccess="false"/>
```

### Firewall Management
Use the Windows Firewall COM API (`INetFwPolicy2`, `INetFwRules`, `INetFwRule`) rather than shelling out to `netsh`:
- Header: `#include <netfw.h>`
- Link: `ole32.lib`, `oleaut32.lib`
- More reliable and provides proper error codes

### User Account Management
Use the NetAPI32 functions:
- `NetUserAdd`, `NetUserDel`, `NetUserGetInfo`, `NetUserSetInfo`
- `NetLocalGroupAddMembers`, `NetLocalGroupDelMembers`, `NetLocalGroupGetMembers`
- Link: `netapi32.lib`

### Network Share Management
Use NetAPI32:
- `NetShareAdd`, `NetShareDel`, `NetShareGetInfo`
- Link: `netapi32.lib`

### Drive Mapping
Use WNet API:
- `WNetAddConnection2`, `WNetCancelConnection2`
- Link: `mpr.lib`

### Network Profile Change
The network profile (Public/Private) is best changed via PowerShell since there is no clean C++ API:
```cpp
RunPowerShellCommand(L"Set-NetConnectionProfile -InterfaceIndex 5 -NetworkCategory Private");
```

### NTFS Permissions
Use the Security API:
- `GetNamedSecurityInfo`, `SetNamedSecurityInfo`
- `AddAccessAllowedAceEx`, `BuildExplicitAccessWithName`, `SetEntriesInAcl`
- Link: `advapi32.lib`

### State Persistence Format
A simple JSON file at `%APPDATA%\RemoteDebugSetup\state_develop.json` (or `state_test.json`):
```json
{
  "version": 1,
  "timestamp": "2026-02-12T10:30:00Z",
  "rd_user_existed": false,
  "rd_was_admin": false,
  "vpn_adapter_was_private": false,
  "vpn_adapter_interface_index": 5,
  "share_existed": false,
  "share_name": "CTrack-software",
  "share_path": "C:\\CTrack-software",
  "smb_firewall_rule_existed": false,
  "debugger_firewall_rule_existed": false,
  "network_discovery_was_enabled": false,
  "file_sharing_was_enabled": false
}
```

Since MFC doesn't include a JSON library, a minimal JSON writer/reader will be implemented (or use simple INI-style key=value format for practicality).

### Supported Platforms
- Windows 10 (version 1903+)
- Windows 11
- x64 builds only (matches VS Remote Debugger x64)
- Visual Studio 2026 (v144 platform toolset)

---

## Dependencies & Linked Libraries

| Library | Purpose |
|---------|---------|
| `netapi32.lib` | User accounts, groups, shares |
| `mpr.lib` | Drive mapping (WNet) |
| `ole32.lib` | COM initialization for firewall API |
| `oleaut32.lib` | COM automation |
| `iphlpapi.lib` | IP helper (adapter enumeration) |
| `advapi32.lib` | Security/ACL functions |
| `ws2_32.lib` | Winsock (for TCP port testing) |
| `icmp.lib` / `iphlpapi.lib` | ICMP ping |

All are standard Windows SDK libraries — no third-party dependencies required.
