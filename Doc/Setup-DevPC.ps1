#Requires -RunAsAdministrator
<#
.SYNOPSIS
    Remote Debugging Setup - Development PC
.DESCRIPTION
    Configures the Development PC for remote debugging over TeamViewer VPN:
    - Creates the RD local account
    - Sets TeamViewer VPN adapter to Private
    - Enables network discovery and file sharing
    - Creates firewall rules for SMB (port 445)
    - Shares a specified folder
.NOTES
    Run this script as Administrator on the DEVELOPMENT PC.
    TeamViewer VPN must be connected before running this script.
#>

# ============================================================
# CONFIGURATION - Edit these values before running
# ============================================================
$RD_Password    = "a"                           # Password for the RD account (must match Test PC)
$SharePath      = "C:\CTrack-software"          # Folder to share
$ShareName      = "CTrack-software"             # Network share name
$VPNSubnet      = "7.0.0.0/8"                   # TeamViewer VPN subnet

# ============================================================

Write-Host ""
Write-Host "======================================================" -ForegroundColor Cyan
Write-Host "  Remote Debugging Setup - DEVELOPMENT PC" -ForegroundColor Cyan
Write-Host "======================================================" -ForegroundColor Cyan
Write-Host ""

# ----------------------------------------------------------
# 1. Create local RD account
# ----------------------------------------------------------
Write-Host "[1/6] Creating local RD account..." -ForegroundColor Yellow

$userExists = Get-LocalUser -Name "RD" -ErrorAction SilentlyContinue
if ($userExists) {
    Write-Host "  RD account already exists. Updating password..." -ForegroundColor Green
    Set-LocalUser -Name "RD" -Password (ConvertTo-SecureString $RD_Password -AsPlainText -Force)
} else {
    New-LocalUser -Name "RD" `
        -Password (ConvertTo-SecureString $RD_Password -AsPlainText -Force) `
        -FullName "Remote Debug" `
        -Description "Account for remote debugging" `
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
# 2. Find and fix TeamViewer VPN adapter
# ----------------------------------------------------------
Write-Host ""
Write-Host "[2/6] Configuring TeamViewer VPN network profile..." -ForegroundColor Yellow

# Find the VPN adapter - look for LocalNetwork connectivity (not Internet)
$vpnProfiles = Get-NetConnectionProfile | Where-Object { 
    $_.IPv4Connectivity -eq "LocalNetwork" -or 
    $_.InterfaceAlias -like "*TeamViewer*" -or
    $_.Name -like "*Local Area Connection*"
}

if ($vpnProfiles) {
    foreach ($profile in $vpnProfiles) {
        if ($profile.NetworkCategory -ne "Private") {
            Set-NetConnectionProfile -InterfaceIndex $profile.InterfaceIndex -NetworkCategory Private
            Write-Host "  '$($profile.InterfaceAlias)' changed from $($profile.NetworkCategory) to Private." -ForegroundColor Green
        } else {
            Write-Host "  '$($profile.InterfaceAlias)' is already Private." -ForegroundColor Green
        }
    }
} else {
    Write-Host "  WARNING: No TeamViewer VPN adapter found." -ForegroundColor Red
    Write-Host "  Make sure TeamViewer VPN is connected, then re-run this script." -ForegroundColor Red
    Write-Host "  Current adapters:" -ForegroundColor Gray
    Get-NetConnectionProfile | Format-Table InterfaceAlias, NetworkCategory, IPv4Connectivity -AutoSize
}

# ----------------------------------------------------------
# 3. Enable Network Discovery and File Sharing
# ----------------------------------------------------------
Write-Host ""
Write-Host "[3/6] Enabling Network Discovery and File Sharing..." -ForegroundColor Yellow

# Enable for Private profile
Get-NetFirewallRule -DisplayGroup "Network Discovery" | 
    Where-Object { $_.Profile -match "Private" } | 
    Set-NetFirewallRule -Enabled True -ErrorAction SilentlyContinue

Get-NetFirewallRule -DisplayGroup "File and Printer Sharing" | 
    Where-Object { $_.Profile -match "Private" } | 
    Set-NetFirewallRule -Enabled True -ErrorAction SilentlyContinue

# Also enable for Public (TeamViewer VPN sometimes still shows as Public initially)
Get-NetFirewallRule -DisplayGroup "Network Discovery" | 
    Where-Object { $_.Profile -match "Public" } | 
    Set-NetFirewallRule -Enabled True -ErrorAction SilentlyContinue

Get-NetFirewallRule -DisplayGroup "File and Printer Sharing" | 
    Where-Object { $_.Profile -match "Public" } | 
    Set-NetFirewallRule -Enabled True -ErrorAction SilentlyContinue

Write-Host "  Network Discovery and File Sharing enabled." -ForegroundColor Green

# ----------------------------------------------------------
# 4. Create firewall rule for SMB over VPN
# ----------------------------------------------------------
Write-Host ""
Write-Host "[4/6] Creating firewall rule for SMB (port 445)..." -ForegroundColor Yellow

$ruleName = "SMB over TeamViewer VPN"
$existingRule = Get-NetFirewallRule -DisplayName $ruleName -ErrorAction SilentlyContinue

if ($existingRule) {
    Remove-NetFirewallRule -DisplayName $ruleName
    Write-Host "  Removed existing rule." -ForegroundColor Gray
}

New-NetFirewallRule `
    -DisplayName $ruleName `
    -Direction Inbound `
    -Protocol TCP `
    -LocalPort 445 `
    -Action Allow `
    -Profile Domain,Private,Public `
    -RemoteAddress $VPNSubnet `
    -Description "Allow SMB file sharing from TeamViewer VPN clients" | Out-Null

Write-Host "  Firewall rule '$ruleName' created (port 445, subnet $VPNSubnet)." -ForegroundColor Green

# ----------------------------------------------------------
# 5. Create firewall rule for Remote Debugger
# ----------------------------------------------------------
Write-Host ""
Write-Host "[5/6] Creating firewall rule for VS Remote Debugger..." -ForegroundColor Yellow

$ruleNameDbg = "VS Remote Debugger (TeamViewer VPN)"
$existingRuleDbg = Get-NetFirewallRule -DisplayName $ruleNameDbg -ErrorAction SilentlyContinue

if ($existingRuleDbg) {
    Remove-NetFirewallRule -DisplayName $ruleNameDbg
}

# Allow ports 4022-4026 to cover VS 2019/2022/future versions
New-NetFirewallRule `
    -DisplayName $ruleNameDbg `
    -Direction Inbound `
    -Protocol TCP `
    -LocalPort 4022-4026 `
    -Action Allow `
    -Profile Domain,Private,Public `
    -RemoteAddress $VPNSubnet `
    -Description "Allow Visual Studio Remote Debugger from TeamViewer VPN clients" | Out-Null

Write-Host "  Firewall rule '$ruleNameDbg' created (ports 4022-4026)." -ForegroundColor Green

# ----------------------------------------------------------
# 6. Share the folder
# ----------------------------------------------------------
Write-Host ""
Write-Host "[6/6] Sharing folder '$SharePath' as '$ShareName'..." -ForegroundColor Yellow

if (-not (Test-Path $SharePath)) {
    Write-Host "  WARNING: Path '$SharePath' does not exist. Skipping share creation." -ForegroundColor Red
} else {
    # Remove existing share if present
    $existingShare = Get-SmbShare -Name $ShareName -ErrorAction SilentlyContinue
    if ($existingShare) {
        Write-Host "  Share '$ShareName' already exists. Updating permissions..." -ForegroundColor Gray
    } else {
        New-SmbShare -Name $ShareName -Path $SharePath -FullAccess "RD" -Description "Remote Debug Share" | Out-Null
        Write-Host "  Share '$ShareName' created." -ForegroundColor Green
    }

    # Ensure RD has share-level permissions
    Grant-SmbShareAccess -Name $ShareName -AccountName "RD" -AccessRight Full -Force | Out-Null
    Write-Host "  Share permissions: RD has Full Control." -ForegroundColor Green

    # Ensure RD has NTFS permissions
    $acl = Get-Acl $SharePath
    $rdRule = New-Object System.Security.AccessControl.FileSystemAccessRule(
        "RD", "Modify", "ContainerInherit,ObjectInherit", "None", "Allow"
    )
    $acl.AddAccessRule($rdRule)
    Set-Acl -Path $SharePath -AclObject $acl
    Write-Host "  NTFS permissions: RD has Modify access." -ForegroundColor Green
}

# ----------------------------------------------------------
# Summary
# ----------------------------------------------------------
Write-Host ""
Write-Host "======================================================" -ForegroundColor Cyan
Write-Host "  SETUP COMPLETE" -ForegroundColor Cyan
Write-Host "======================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "  Hostname:    $(hostname)" -ForegroundColor White
Write-Host "  RD Account:  RD (password: $RD_Password)" -ForegroundColor White
Write-Host "  Share:       \\$(hostname)\$ShareName" -ForegroundColor White
Write-Host ""

# Show VPN IP
$vpnAdapter = Get-NetIPAddress -InterfaceAlias "Local Area Connection" -AddressFamily IPv4 -ErrorAction SilentlyContinue
if ($vpnAdapter) {
    Write-Host "  VPN IP:      $($vpnAdapter.IPAddress)" -ForegroundColor White
    Write-Host ""
    Write-Host "  From Test PC, connect to:" -ForegroundColor Green
    Write-Host "    \\$($vpnAdapter.IPAddress)\$ShareName" -ForegroundColor Green
    Write-Host "    Credentials: $(hostname)\RD / $RD_Password" -ForegroundColor Green
} else {
    Write-Host "  VPN IP:      (could not detect - check TeamViewer VPN)" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "  From Test PC, connect to:" -ForegroundColor Green
    Write-Host "    \\<VPN_IP>\$ShareName" -ForegroundColor Green
    Write-Host "    Credentials: $(hostname)\RD / $RD_Password" -ForegroundColor Green
}

Write-Host ""
Write-Host "  Next: Run Setup-TestPC.ps1 on the Test PC." -ForegroundColor Yellow
Write-Host ""
