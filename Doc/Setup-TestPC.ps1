#Requires -RunAsAdministrator
<#
.SYNOPSIS
    Remote Debugging Setup - Test PC
.DESCRIPTION
    Configures the Test PC for remote debugging over TeamViewer VPN:
    - Creates the RD local account (matching the Dev PC)
    - Creates firewall rules for the Remote Debugger
    - Maps the shared folder from the Dev PC
    - Verifies connectivity
.NOTES
    Run this script as Administrator on the TEST PC.
    TeamViewer VPN must be connected before running this script.
#>

# ============================================================
# CONFIGURATION - Edit these values before running
# ============================================================
$RD_Password    = "a"                           # Must match the Dev PC password
$DevPC_Hostname = "DELL"                        # Hostname of the Development PC
$DevPC_VPN_IP   = "7.238.81.240"                # VPN IP of the Development PC
$ShareName      = "CTrack-software"             # Share name on the Dev PC
$DriveLetter    = "Z"                           # Drive letter to map
$DebuggerPort   = 4026                          # Remote Debugger port

# ============================================================

Write-Host ""
Write-Host "======================================================" -ForegroundColor Cyan
Write-Host "  Remote Debugging Setup - TEST PC" -ForegroundColor Cyan
Write-Host "======================================================" -ForegroundColor Cyan
Write-Host ""

# ----------------------------------------------------------
# 1. Create local RD account (must match Dev PC)
# ----------------------------------------------------------
Write-Host "[1/5] Creating local RD account..." -ForegroundColor Yellow

$userExists = Get-LocalUser -Name "RD" -ErrorAction SilentlyContinue
if ($userExists) {
    Write-Host "  RD account already exists. Updating password..." -ForegroundColor Green
    Set-LocalUser -Name "RD" -Password (ConvertTo-SecureString $RD_Password -AsPlainText -Force)
} else {
    New-LocalUser -Name "RD" `
        -Password (ConvertTo-SecureString $RD_Password -AsPlainText -Force) `
        -FullName "Remote Debug" `
        -Description "Account for remote debugging (matches Dev PC)" `
        -PasswordNeverExpires
    Write-Host "  RD account created." -ForegroundColor Green
}

# Add to Administrators group
$isAdmin = Get-LocalGroupMember -Group "Administrators" -ErrorAction SilentlyContinue | 
           Where-Object { $_.Name -like "*\RD" }
if (-not $isAdmin) {
    Add-LocalGroupMember -Group "Administrators" -Member "RD"
    Write-Host "  RD added to Administrators group." -ForegroundColor Green
} else {
    Write-Host "  RD is already an Administrator." -ForegroundColor Green
}

# ----------------------------------------------------------
# 2. Create firewall rule for Remote Debugger
# ----------------------------------------------------------
Write-Host ""
Write-Host "[2/5] Creating firewall rule for VS Remote Debugger..." -ForegroundColor Yellow

$ruleName = "VS Remote Debugger Inbound"
$existingRule = Get-NetFirewallRule -DisplayName $ruleName -ErrorAction SilentlyContinue

if ($existingRule) {
    Remove-NetFirewallRule -DisplayName $ruleName
}

New-NetFirewallRule `
    -DisplayName $ruleName `
    -Direction Inbound `
    -Protocol TCP `
    -LocalPort 4022-4026 `
    -Action Allow `
    -Profile Domain,Private,Public `
    -Description "Allow incoming Visual Studio Remote Debugger connections" | Out-Null

Write-Host "  Firewall rule created (ports 4022-4026, all profiles)." -ForegroundColor Green

# ----------------------------------------------------------
# 3. Verify VPN connectivity
# ----------------------------------------------------------
Write-Host ""
Write-Host "[3/5] Verifying VPN connectivity to Dev PC ($DevPC_VPN_IP)..." -ForegroundColor Yellow

$pingResult = Test-Connection -ComputerName $DevPC_VPN_IP -Count 2 -Quiet -ErrorAction SilentlyContinue
if ($pingResult) {
    Write-Host "  Ping successful." -ForegroundColor Green
} else {
    Write-Host "  WARNING: Cannot ping $DevPC_VPN_IP" -ForegroundColor Red
    Write-Host "  Make sure TeamViewer VPN is connected." -ForegroundColor Red
    Write-Host "  Continuing anyway (some networks block ICMP)..." -ForegroundColor Yellow
}

# Verify SMB port
Write-Host "  Testing SMB port 445..." -ForegroundColor Gray
$smbTest = Test-NetConnection -ComputerName $DevPC_VPN_IP -Port 445 -WarningAction SilentlyContinue
if ($smbTest.TcpTestSucceeded) {
    Write-Host "  Port 445 is open on Dev PC." -ForegroundColor Green
} else {
    Write-Host "  WARNING: Port 445 is not reachable on Dev PC." -ForegroundColor Red
    Write-Host "  Run Setup-DevPC.ps1 on the Dev PC first." -ForegroundColor Red
}

# ----------------------------------------------------------
# 4. Map shared folder
# ----------------------------------------------------------
Write-Host ""
Write-Host "[4/5] Mapping shared folder \\$DevPC_VPN_IP\$ShareName as ${DriveLetter}:..." -ForegroundColor Yellow

# Remove existing mapping if present
$existingMapping = Get-PSDrive -Name $DriveLetter -ErrorAction SilentlyContinue
if ($existingMapping) {
    net use "${DriveLetter}:" /delete /y 2>$null | Out-Null
    Write-Host "  Removed existing ${DriveLetter}: mapping." -ForegroundColor Gray
}

# Also clear any existing connections to this server
net use "\\$DevPC_VPN_IP\$ShareName" /delete /y 2>$null | Out-Null

# Map the drive
$mapResult = net use "${DriveLetter}:" "\\$DevPC_VPN_IP\$ShareName" /user:${DevPC_Hostname}\RD $RD_Password /persistent:yes 2>&1

if ($LASTEXITCODE -eq 0) {
    Write-Host "  ${DriveLetter}: mapped successfully." -ForegroundColor Green
} else {
    Write-Host "  WARNING: Failed to map drive." -ForegroundColor Red
    Write-Host "  Error: $mapResult" -ForegroundColor Red
    Write-Host ""
    Write-Host "  Common fixes:" -ForegroundColor Yellow
    Write-Host "    Error 5:    Wrong password or missing permissions on Dev PC" -ForegroundColor Gray
    Write-Host "    Error 53:   VPN not connected or port 445 blocked" -ForegroundColor Gray
    Write-Host "    Error 1219: Run 'net use * /delete' then re-run this script" -ForegroundColor Gray
    Write-Host "    Error 1326: Username/password mismatch" -ForegroundColor Gray
}

# ----------------------------------------------------------
# 5. Locate Remote Debugger
# ----------------------------------------------------------
Write-Host ""
Write-Host "[5/5] Locating Visual Studio Remote Debugger..." -ForegroundColor Yellow

$debuggerPaths = @(
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\Remote Debugger\x64\msvsmon.exe",
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional\Common7\IDE\Remote Debugger\x64\msvsmon.exe",
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\Common7\IDE\Remote Debugger\x64\msvsmon.exe",
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\Remote Debugger\x64\msvsmon.exe",
    # Standalone Remote Tools install location
    "${env:ProgramFiles}\Microsoft Visual Studio 17.0\Common7\IDE\Remote Debugger\x64\msvsmon.exe"
)

$debuggerFound = $false
foreach ($path in $debuggerPaths) {
    if (Test-Path $path) {
        Write-Host "  Found: $path" -ForegroundColor Green
        $debuggerFound = $true
        break
    }
}

if (-not $debuggerFound) {
    # Try searching
    $found = Get-ChildItem -Path "${env:ProgramFiles}" -Filter "msvsmon.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($found) {
        Write-Host "  Found: $($found.FullName)" -ForegroundColor Green
        $debuggerFound = $true
    } else {
        Write-Host "  WARNING: Remote Debugger (msvsmon.exe) not found." -ForegroundColor Red
        Write-Host "  Install 'Remote Tools for Visual Studio 2022' from Microsoft." -ForegroundColor Yellow
        Write-Host "  Or copy from Dev PC: Common7\IDE\Remote Debugger\" -ForegroundColor Yellow
    }
}

# ----------------------------------------------------------
# Summary
# ----------------------------------------------------------
Write-Host ""
Write-Host "======================================================" -ForegroundColor Cyan
Write-Host "  SETUP COMPLETE" -ForegroundColor Cyan
Write-Host "======================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "  This PC:     $(hostname)" -ForegroundColor White
Write-Host "  RD Account:  RD (password: $RD_Password)" -ForegroundColor White
Write-Host "  Dev PC:      $DevPC_Hostname ($DevPC_VPN_IP)" -ForegroundColor White

# Check if drive mapping succeeded
if (Test-Path "${DriveLetter}:\") {
    Write-Host "  Mapped Drive: ${DriveLetter}: -> \\$DevPC_VPN_IP\$ShareName" -ForegroundColor White
} else {
    Write-Host "  Mapped Drive: FAILED (see errors above)" -ForegroundColor Red
}

Write-Host ""
Write-Host "  NEXT STEPS:" -ForegroundColor Yellow
Write-Host "  1. Start msvsmon.exe as Administrator" -ForegroundColor White
Write-Host "  2. In msvsmon: Tools > Options > Windows Authentication" -ForegroundColor White
Write-Host "  3. In msvsmon: Tools > Permissions > Add RD user" -ForegroundColor White
Write-Host "  4. In Visual Studio on Dev PC:" -ForegroundColor White
Write-Host "     Debug > Attach to Process" -ForegroundColor White
Write-Host "     Connection: Remote (Windows Authentication)" -ForegroundColor White
Write-Host "     Target: ${DevPC_VPN_IP}:${DebuggerPort}" -ForegroundColor White
Write-Host ""
