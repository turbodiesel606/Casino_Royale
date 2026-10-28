#requires -Version 7.2
<#
Exercise a real offline Setup using only owned fixtures below build/installer-tests/.
Never launch JobTracker or read/write the real personal-data store. Uninstall is
silent and must preserve data; destructive data-core tests use separate fixtures.
Run CurrentUser without elevation, AllUsers from an elevated PowerShell session.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$InstallerPath,
    [Parameter(Mandatory)][string]$ManifestPath,
    [Parameter(Mandatory)][string]$WorkDirectory,
    [Parameter(Mandatory)][ValidateSet('CurrentUser', 'AllUsers')][string]$Scope,
    [Parameter(Mandatory)][ValidateSet('Install', 'Verify', 'Reinstall', 'AdditionalCopy', 'RepairMissingUninstaller', 'RepairMissingLog', 'Uninstall')][string]$Action,
    [ValidateSet('english', 'russian')][string]$Language = 'english',
    [switch]$WithDesktopShortcut
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$work = [IO.Path]::TrimEndingDirectorySeparator([IO.Path]::GetFullPath($WorkDirectory))
if (-not $work.StartsWith((Join-Path $repository 'build/installer-tests') + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Installer fixtures must be below this repository''s build/installer-tests/ directory.'
}
. (Join-Path $PSScriptRoot 'OfflineInstallerEvidence.ps1')
Assert-NoReparsePoint $work
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$elevated = ([Security.Principal.WindowsPrincipal]::new($identity)).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (($Scope -eq 'AllUsers') -ne $elevated) {
    throw 'AllUsers requires elevation; CurrentUser must run without elevation to verify its privilege contract.'
}
$installer = (Get-Item -LiteralPath $InstallerPath).FullName
$manifest = Get-OfflineInstallerEvidence -InstallerPath $installer -MetadataPath $ManifestPath
$target = Join-Path $work 'application'
$statePath = Join-Path $work 'fixture.json'

function Assert-Check([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}

function Get-RegistrationSnapshot {
    $entries = [Collections.Generic.List[string]]::new()
    foreach ($hive in @([Microsoft.Win32.RegistryHive]::CurrentUser, [Microsoft.Win32.RegistryHive]::LocalMachine)) {
        foreach ($view in @([Microsoft.Win32.RegistryView]::Registry32, [Microsoft.Win32.RegistryView]::Registry64)) {
            $base = [Microsoft.Win32.RegistryKey]::OpenBaseKey($hive, $view)
            $root = $null
            try {
                $root = $base.OpenSubKey('Software\Microsoft\Windows\CurrentVersion\Uninstall')
                if ($null -eq $root) { continue }
                foreach ($name in $root.GetSubKeyNames()) {
                    $entry = $root.OpenSubKey($name)
                    try {
                        if ($name -like 'JobTracker*' -or [string]$entry.GetValue('DisplayName') -like 'JobTracker*') {
                            $values = @($entry.GetValueNames() | Sort-Object | ForEach-Object { "$_=$($entry.GetValue($_))" }) -join ';'
                            $entries.Add("$hive/$view/$name/$values")
                        }
                    } finally { if ($null -ne $entry) { $entry.Dispose() } }
                }
            } finally {
                if ($null -ne $root) { $root.Dispose() }
                $base.Dispose()
            }
        }
    }
    return (@($entries | Sort-Object) -join "`n")
}

function Read-Installation([string]$Directory, [string]$FileName = 'JobTracker.install.ini') {
    $values = @{}
    $section = ''
    foreach ($line in Get-Content -LiteralPath (Join-Path $Directory $FileName)) {
        if ($line -match '^\[([^\]]+)\]$') { $section = $Matches[1] }
        elseif ($line -match '^([^=]+)=(.*)$') { $values["$section/$($Matches[1])"] = $Matches[2] }
    }
    return $values
}

function Invoke-Installer([string]$Program, [string[]]$ExtraArguments, [string]$LogName) {
    $arguments = @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/SP-', "/LOG=`"$work/$LogName.log`"") + $ExtraArguments
    $process = Start-Process -FilePath $Program -ArgumentList $arguments -WindowStyle Hidden -PassThru -Wait
    return $process.ExitCode
}

function Install-Copy([string]$Directory, [string]$LogName) {
    $arguments = @("/$($Scope.ToUpperInvariant())", "/LANG=$Language", "/DIR=`"$Directory`"")
    if ($WithDesktopShortcut) { $arguments += '/TASKS=desktopicon' } else { $arguments += '/TASKS=' }
    $exitCode = Invoke-Installer $installer $arguments $LogName
    Assert-Check ($exitCode -eq 0) "Installer returned $exitCode; see $work/$LogName.log."
}

function Assert-Installed([string]$Directory) {
    $local = Read-Installation $Directory
    Assert-Check ($local['Installation/Format'] -eq '2' -and $local['Installation/Product'] -ceq 'JobTracker') 'Local product/format marker mismatch.'
    Assert-Check ($local['Installation/Path'] -ieq $Directory -and $local['Installation/Scope'] -ceq $Scope -and
        $local['Installation/Version'] -ceq $manifest.version) 'Local installation path/scope/version mismatch.'
    Assert-Check ($local['Installation/Id'] -match '^[a-f0-9]{16}$') 'Per-copy identity missing.'
    $ledger = Read-Installation $Directory 'JobTracker.ownership.ini'
    Assert-Check ($ledger['Identity/Product'] -ceq 'JobTracker-2' -and $ledger['Identity/Path'] -ieq $Directory) 'Local ownership identity mismatch.'
    $owned = @(for ($index = 0; $index -lt [int]$ledger['Identity/Count']; ++$index) { $ledger["Files/$index"] })
    foreach ($file in $manifest.files) {
        $installedFile = Join-Path $Directory $file.path
        Assert-Check ((Get-FileHash -LiteralPath $installedFile -Algorithm SHA256).Hash -ieq $file.sha256) "Installed payload mismatch: $($file.path)"
        Assert-Check ($owned -contains $file.path.Replace('/', '\')) "Payload ownership missing: $($file.path)"
    }
    foreach ($file in $manifest.msvcRuntime.files) {
        $runtimeFile = Join-Path $Directory "bin/$($file.name)"
        Assert-Check ((Get-FileHash -LiteralPath $runtimeFile -Algorithm SHA256).Hash -ieq $file.sha256) "App-Local runtime mismatch in $Scope`: $($file.name)"
        Assert-Check ($owned -contains "bin\$($file.name)") "Runtime ownership missing: $($file.name)"
    }
    Assert-Check (Test-Path -LiteralPath (Join-Path $Directory 'licenses/MSVC-NOTICE.txt')) 'Runtime notice is missing.'
    $executable = Join-Path $Directory 'bin/JobTrackerApp.exe'
    foreach ($kind in @('StartMenu', 'Desktop')) {
        $path = $local["Shortcuts/$kind"]
        if ($kind -eq 'Desktop' -and -not $WithDesktopShortcut) { continue }
        Assert-Check ($path -and (Test-Path -LiteralPath $path)) "$kind shortcut missing."
        $link = Get-InstallerShortcutInfo $path
        Assert-Check ($link.Target -ieq $executable -and $link.WorkingDirectory -ieq (Join-Path $Directory 'bin')) "$kind shortcut points to the wrong installation."
        Assert-Check ($link.IconFilename -ieq $executable -and $link.IconIndex -eq 0) "$kind shortcut icon mismatch."
    }
    $uninstaller = [string]$local['Installation/Uninstaller']
    Assert-Check ([IO.Path]::GetDirectoryName($uninstaller) -ieq $Directory -and
        [IO.Path]::GetFileName($uninstaller) -match '^unins\d+\.exe$' -and
        (Test-Path -LiteralPath $uninstaller)) 'A working local uninstaller was not recorded.'
    Assert-Check (Test-Path -LiteralPath ([IO.Path]::ChangeExtension($uninstaller, '.dat'))) 'Local uninstall data is missing.'
    return $local
}

function Uninstall-Copy([string]$Directory, [string]$LogName) {
    $resolved = (Resolve-Path -LiteralPath $Directory).Path
    Assert-Check ($resolved -ieq $Directory -and $Directory.StartsWith($work + '\', [StringComparison]::OrdinalIgnoreCase)) 'Unsafe uninstall fixture.'
    Assert-NoReparsePoint $Directory
    $links = @(Get-ChildItem -LiteralPath $Directory -Recurse -Force | Where-Object { $_.Attributes -band [IO.FileAttributes]::ReparsePoint })
    Assert-Check ($links.Count -eq 0) 'Reparse point inside uninstall fixture.'
    $local = Read-Installation $Directory
    $uninstaller = [string]$local['Installation/Uninstaller']
    Assert-Check ([IO.Path]::GetDirectoryName($uninstaller) -ieq $Directory) 'Uninstaller is outside its fixture.'
    $exitCode = Invoke-Installer $uninstaller @() $LogName
    Assert-Check ($exitCode -eq 0) "Uninstaller returned $exitCode; see $work/$LogName.log."
    foreach ($file in $manifest.files) {
        Assert-Check (-not (Test-Path -LiteralPath (Join-Path $Directory $file.path))) "Payload remains after uninstall: $($file.path)"
    }
    foreach ($file in $manifest.msvcRuntime.files) {
        Assert-Check (-not (Test-Path -LiteralPath (Join-Path $Directory "bin/$($file.name)"))) 'App-Local runtime remains after uninstall.'
    }
    foreach ($kind in @('StartMenu', 'Desktop')) {
        $path = [string]$local["Shortcuts/$kind"]
        if ($path) { Assert-Check (-not (Test-Path -LiteralPath $path)) "Owned $kind shortcut remains." }
    }
    Assert-Check (Test-Path -LiteralPath (Join-Path $Directory 'unmanaged-sentinel.txt')) 'Uninstall removed an unrelated file.'
    $log = Get-Content -LiteralPath (Join-Path $work "$LogName.log") -Raw
    Assert-Check ($log -notmatch 'confirmed.*personal.data.*delet|personal.data.*deleted|Data cleanup started') 'Silent uninstall attempted personal-data deletion.'
}

$registrationBefore = Get-RegistrationSnapshot
if ($Action -eq 'Install') {
    Assert-Check (-not (Test-Path -LiteralPath $work)) 'Fixture already exists; choose a fresh directory.'
    New-Item -ItemType Directory -Path $work | Out-Null
    [ordered]@{
        scope=$Scope; language=$Language; target=$target; sid=$identity.User.Value; desktop=[bool]$WithDesktopShortcut
        installerSha256=(Get-FileHash -LiteralPath $installer).Hash
        manifestSha256=(Get-FileHash -LiteralPath $ManifestPath).Hash
    } | ConvertTo-Json | Set-Content -LiteralPath $statePath -Encoding utf8
    Set-Content -LiteralPath (Join-Path $work 'outside-sentinel.txt') -Value 'Preserve files outside the program folder.'
} else {
    $state = Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json
    Assert-Check ($state.scope -ceq $Scope -and $state.target -ieq $target -and $state.sid -ceq $identity.User.Value) 'Fixture owner/scope/path mismatch.'
    Assert-Check ($state.language -ceq $Language -and $state.desktop -eq [bool]$WithDesktopShortcut) 'Fixture language/task mismatch.'
    Assert-Check ($state.installerSha256 -ceq (Get-FileHash -LiteralPath $installer).Hash -and
        $state.manifestSha256 -ceq (Get-FileHash -LiteralPath $ManifestPath).Hash) 'Installer or evidence changed since fixture creation.'
}

switch ($Action) {
    'Install' {
        Install-Copy $target $Action
        $null = Assert-Installed $target
        Set-Content -LiteralPath (Join-Path $target 'unmanaged-sentinel.txt') -Value 'Preserve unknown files.'
    }
    'Verify' { $null = Assert-Installed $target }
    'Reinstall' {
        $before = Assert-Installed $target
        $history = [IO.Path]::ChangeExtension([string]$before['Installation/Uninstaller'], '.dat')
        $historySize = (Get-Item -LiteralPath $history).Length
        $preservedHistory = @(Get-ChildItem -LiteralPath $target -Filter 'unins*.preserved-*' -File | Sort-Object Name | ForEach-Object Name) -join ';'
        Install-Copy $target $Action
        $after = Assert-Installed $target
        Assert-Check ($after['Installation/Uninstaller'] -ieq $before['Installation/Uninstaller']) 'Normal repair replaced valid local uninstall identity.'
        Assert-Check ((Get-Item -LiteralPath $history).Length -gt $historySize) 'Normal repair did not append valid uninstall history.'
        $preservedAfter = @(Get-ChildItem -LiteralPath $target -Filter 'unins*.preserved-*' -File | Sort-Object Name | ForEach-Object Name) -join ';'
        Assert-Check ($preservedAfter -ceq $preservedHistory) 'Normal repair quarantined valid uninstall history.'
    }
    { $_ -in 'RepairMissingUninstaller', 'RepairMissingLog' } {
        $before = Assert-Installed $target
        $missing = [string]$before['Installation/Uninstaller']
        if ($Action -eq 'RepairMissingLog') { $missing = [IO.Path]::ChangeExtension($missing, '.dat') }
        Assert-Check ([IO.Path]::GetDirectoryName($missing) -ieq $target) 'Unsafe repair fixture file.'
        $saved = Join-Path $work ("$Action-original-" + [Guid]::NewGuid().ToString('N') + [IO.Path]::GetExtension($missing))
        Move-Item -LiteralPath $missing -Destination $saved
        Install-Copy $target $Action
        $null = Assert-Installed $target
        Assert-Check (Test-Path -LiteralPath (Join-Path $target 'unmanaged-sentinel.txt')) 'Repair removed an unrelated file.'
    }
    'AdditionalCopy' {
        $first = Assert-Installed $target
        $second = Join-Path $work ('second-' + [Guid]::NewGuid().ToString('N'))
        Install-Copy $second $Action
        $other = Assert-Installed $second
        Assert-Check ($other['Installation/Id'] -cne $first['Installation/Id'] -and
            $other['Shortcuts/StartMenu'] -ine $first['Shortcuts/StartMenu']) 'Independent copies share an identity or shortcut.'
        Set-Content -LiteralPath (Join-Path $second 'unmanaged-sentinel.txt') -Value 'Preserve unknown files.'
        Uninstall-Copy $second 'AdditionalCopy-Uninstall'
        $null = Assert-Installed $target
    }
    'Uninstall' { Uninstall-Copy $target $Action }
}
Assert-Check ((Get-RegistrationSnapshot) -ceq $registrationBefore) 'Setup/uninstall changed JobTracker Windows installed-apps registration.'
Assert-Check ((Get-Content -LiteralPath (Join-Path $work 'outside-sentinel.txt') -Raw).Trim() -ceq 'Preserve files outside the program folder.') 'Outside sentinel changed.'
[ordered]@{ action=$Action; scope=$Scope; language=$Language; result='PASS'; elevated=$elevated; user=$identity.Name; payloadFiles=$manifest.files.Count } |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $work "$Action.result.json") -Encoding utf8
Write-Output "PASS: $Scope $Action; $($manifest.files.Count) payload files; fixture $work."
