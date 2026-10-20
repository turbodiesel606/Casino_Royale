#requires -Version 7.2
<#
Exercise the real offline installer without launching JobTracker or reading its data.
Each scope uses a fresh fixture below build/installer-tests/. Only its own generated
installation is uninstalled; logs, state and synthetic personal-data sentinels are retained.
Run CurrentUser without elevation, AllUsers from an elevated PowerShell session.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$InstallerPath,
    [Parameter(Mandatory)][string]$ManifestPath,
    [Parameter(Mandatory)][string]$WorkDirectory,
    [Parameter(Mandatory)][ValidateSet('CurrentUser', 'AllUsers')][string]$Scope,
    [Parameter(Mandatory)][ValidateSet('Install', 'Verify', 'Reinstall', 'ScopeConflict', 'Uninstall')][string]$Action,
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
$ancestor = $work
while ($ancestor) {
    if ((Test-Path -LiteralPath $ancestor) -and
        ((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw "Reparse point in fixture path: $ancestor"
    }
    $ancestor = [IO.Path]::GetDirectoryName($ancestor)
}
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$elevated = ([Security.Principal.WindowsPrincipal]::new($identity)).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (($Scope -eq 'AllUsers') -ne $elevated) {
    throw 'AllUsers requires elevation; CurrentUser must run without elevation to verify its privilege contract.'
}
$installer = (Get-Item -LiteralPath $InstallerPath).FullName
. (Join-Path $PSScriptRoot 'OfflineInstallerEvidence.ps1')
$manifest = Get-OfflineInstallerEvidence -InstallerPath $installer -MetadataPath $ManifestPath
$installerMetadata = $manifest
$target = Join-Path $work 'application'
$statePath = Join-Path $work 'fixture.json'
$allUsers = $Scope -eq 'AllUsers'
$registryHive = if ($allUsers) { [Microsoft.Win32.RegistryHive]::LocalMachine } else { [Microsoft.Win32.RegistryHive]::CurrentUser }
$root = [Microsoft.Win32.RegistryKey]::OpenBaseKey($registryHive, [Microsoft.Win32.RegistryView]::Registry64)
$uninstallKey = "Software\Microsoft\Windows\CurrentVersion\Uninstall\JobTracker.${Scope}_is1"
$caption = if ($Language -eq 'russian') {
    if ($allUsers) { 'все пользователи' } else { 'текущий пользователь' }
} else {
    if ($allUsers) { 'all users' } else { 'current user' }
}
$shortcutName = "JobTracker ($caption).lnk"
$programsFolder = if ($allUsers) { 'CommonPrograms' } else { 'Programs' }
$desktopFolder = if ($allUsers) { 'CommonDesktopDirectory' } else { 'DesktopDirectory' }
$shortcut = Join-Path ([Environment]::GetFolderPath($programsFolder)) "JobTracker/$shortcutName"
$desktopShortcut = Join-Path ([Environment]::GetFolderPath($desktopFolder)) $shortcutName
$executable = Join-Path $target 'bin/JobTrackerApp.exe'

function Assert-Check([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}

function Invoke-Installer([string]$Program, [string[]]$ExtraArguments, [string]$LogName) {
    $arguments = @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/SP-', "/LOG=`"$work/$LogName.log`"") + $ExtraArguments
    $process = Start-Process -FilePath $Program -ArgumentList $arguments -WindowStyle Hidden -PassThru -Wait
    return $process.ExitCode
}

function Assert-Installed {
    $key = $root.OpenSubKey($uninstallKey)
    Assert-Check ($null -ne $key) 'Missing scope-specific uninstall registration.'
    try {
        Assert-Check ($key.GetValue('InstallLocation').TrimEnd('\') -ieq $target) 'Registered install location mismatch.'
        Assert-Check ($key.GetValue('DisplayVersion') -ceq $manifest.version) 'Registered version mismatch.'
        Assert-Check ($key.GetValue('DisplayName') -ceq "JobTracker ($caption)") 'Uninstall display name/scope mismatch.'
        Assert-Check ($key.GetValue('DisplayIcon') -ieq "$executable,0") 'Uninstall icon mismatch.'
    } finally { $key.Dispose() }
    foreach ($file in $manifest.files) {
        $installedFile = Join-Path $target $file.path
        Assert-Check ((Get-FileHash -LiteralPath $installedFile -Algorithm SHA256).Hash.ToLowerInvariant() -ceq $file.sha256) "Installed payload mismatch: $($file.path)"
    }
    if ($installerMetadata.PSObject.Properties.Name -contains 'msvcRuntime') {
        foreach ($file in $installerMetadata.msvcRuntime.files) {
            $runtimeFile = Join-Path $target "bin/$($file.name)"
            if ($allUsers) {
                Assert-Check (-not (Test-Path -LiteralPath $runtimeFile)) 'AllUsers installed an App-Local runtime.'
            } else {
                Assert-Check ((Get-FileHash -LiteralPath $runtimeFile -Algorithm SHA256).Hash -ieq $file.sha256) "App-Local runtime mismatch: $($file.name)"
            }
        }
        Assert-Check (Test-Path -LiteralPath (Join-Path $target 'licenses/MSVC-NOTICE.txt')) 'Runtime notice is missing.'
    }
    Assert-Check (Test-Path -LiteralPath $shortcut) 'Required Start Menu shortcut missing.'
    $shell = New-Object -ComObject WScript.Shell
    $link = $shell.CreateShortcut($shortcut)
    Assert-Check ($link.TargetPath -ieq $executable) 'Shortcut points to the wrong installation.'
    Assert-Check ($link.WorkingDirectory -ieq (Join-Path $target 'bin')) 'Shortcut working directory mismatch.'
    Assert-Check ($link.IconLocation -ieq "$executable,0") 'Shortcut icon mismatch.'
    Assert-Check ((Get-Content -LiteralPath (Join-Path $target 'JobTracker.install.ini') -Raw) -match "Scope=$Scope") 'Scope marker mismatch.'
}

try {
    if ($Action -eq 'Install') {
        Assert-Check (-not (Test-Path -LiteralPath $work)) 'Fixture already exists; choose a new work directory.'
        $existing = $root.OpenSubKey($uninstallKey)
        if ($null -ne $existing) { $existing.Dispose(); throw 'An existing installation is registered; no changes made.' }
        Assert-Check (-not (Test-Path -LiteralPath $shortcut)) 'Start Menu shortcut already exists; no changes made.'
        Assert-Check (-not (Test-Path -LiteralPath $desktopShortcut)) 'Desktop shortcut already exists; no changes made.'
        New-Item -ItemType Directory -Path $work | Out-Null
        [ordered]@{
            scope = $Scope
            language = $Language
            target = $target
            sid = $identity.User.Value
            installerSha256 = (Get-FileHash -LiteralPath $installer -Algorithm SHA256).Hash
            manifestSha256 = (Get-FileHash -LiteralPath $ManifestPath -Algorithm SHA256).Hash
        } | ConvertTo-Json | Set-Content -LiteralPath $statePath -Encoding utf8
        Set-Content -LiteralPath (Join-Path $work 'personal-data-sentinel.txt') -Value 'Keep this synthetic personal data.'
    } else {
        $state = Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json
        Assert-Check ($state.scope -ceq $Scope -and $state.target -ieq $target -and $state.sid -ceq $identity.User.Value) 'Fixture owner/scope/path mismatch.'
        Assert-Check ($state.language -ceq $Language) 'Fixture language mismatch.'
        Assert-Check ($state.installerSha256 -ceq (Get-FileHash -LiteralPath $installer -Algorithm SHA256).Hash) 'Installer changed since fixture creation.'
        Assert-Check ($state.manifestSha256 -ceq (Get-FileHash -LiteralPath $ManifestPath -Algorithm SHA256).Hash) 'Manifest changed since fixture creation.'
        Assert-Installed
    }
    if ($Action -in 'Install', 'Reinstall') {
        $arguments = @("/$($Scope.ToUpperInvariant())", "/LANG=$language", "/SAVEINF=`"$work/$Action.inf`"", '/NOICONS')
        if ($Action -eq 'Install') {
            $arguments += "/DIR=`"$target`""
            if ($WithDesktopShortcut) { $arguments += '/TASKS=desktopicon' }
        }
        $exitCode = Invoke-Installer $installer $arguments $Action
        Assert-Check ($exitCode -eq 0) "Installer returned $exitCode; see $work/$Action.log."
        Assert-Installed
        if (-not $allUsers -and $installerMetadata.PSObject.Properties.Name -contains 'msvcRuntime') {
            $runtimeLog = Get-Content -LiteralPath (Join-Path $work "$Action.log") -Raw
            Assert-Check ($runtimeLog -match 'current-user App-Local runtime; no prerequisite execution' -and
                $runtimeLog -notmatch 'MSVC prerequisite returned:') 'Current-user runtime policy was not followed.'
        }
        if ($Action -eq 'Install') {
            Assert-Check ((Test-Path -LiteralPath $desktopShortcut) -eq [bool]$WithDesktopShortcut) 'Desktop shortcut default/opt-in mismatch.'
            if ($WithDesktopShortcut) {
                $shell = New-Object -ComObject WScript.Shell
                $link = $shell.CreateShortcut($desktopShortcut)
                Assert-Check ($link.TargetPath -ieq $executable -and $link.IconLocation -ieq "$executable,0") 'Desktop shortcut target/icon mismatch.'
            }
            Set-Content -LiteralPath (Join-Path $target 'unmanaged-sentinel.txt') -Value 'Preserve unknown files.'
        }
        $inf = Get-Content -LiteralPath (Join-Path $work "$Action.inf") -Raw
        if (-not $WithDesktopShortcut -or $Action -eq 'Reinstall') {
            Assert-Check ($inf -match '(?m)^Tasks=\s*$') 'Desktop task was not unchecked by default.'
        }
    } elseif ($Action -eq 'ScopeConflict') {
        $conflict = Join-Path $work ('conflict-' + [Guid]::NewGuid().ToString('N'))
        New-Item -ItemType Directory -Path $conflict | Out-Null
        $oppositeScope = if ($allUsers) { 'CurrentUser' } else { 'AllUsers' }
        $marker = Join-Path $conflict 'JobTracker.install.ini'
        Set-Content -LiteralPath $marker -Value "[Installation]`r`nScope=$oppositeScope"
        $before = (Get-FileHash -LiteralPath $marker -Algorithm SHA256).Hash
        $exitCode = Invoke-Installer $installer @("/$($Scope.ToUpperInvariant())", "/LANG=$language", "/DIR=`"$conflict`"") 'ScopeConflict'
        Assert-Check ($exitCode -ne 0) 'Conflicting scope was accepted.'
        Assert-Check (-not (Test-Path -LiteralPath (Join-Path $conflict 'bin/JobTrackerApp.exe'))) 'Conflict check wrote application files.'
        Assert-Check ((Get-FileHash -LiteralPath $marker -Algorithm SHA256).Hash -ceq $before) 'Conflict marker was overwritten.'
        Assert-Installed
    } elseif ($Action -eq 'Uninstall') {
        # Validate the exact resolved tree before invoking its generated uninstaller.
        $resolvedTarget = (Resolve-Path -LiteralPath $target).Path
        Assert-Check ($resolvedTarget -ieq (Join-Path $work 'application')) 'Unsafe uninstall target.'
        Assert-Check (-not ((Get-Item -LiteralPath $target -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) 'Uninstall target is a reparse point.'
        $links = @(Get-ChildItem -LiteralPath $target -Recurse -Force | Where-Object { $_.Attributes -band [IO.FileAttributes]::ReparsePoint })
        Assert-Check ($links.Count -eq 0) 'Reparse point inside uninstall fixture.'
        $exitCode = Invoke-Installer (Join-Path $target 'unins000.exe') @() 'Uninstall'
        Assert-Check ($exitCode -eq 0) "Uninstaller returned $exitCode."
        $remaining = $root.OpenSubKey($uninstallKey)
        if ($null -ne $remaining) { $remaining.Dispose(); throw 'Uninstall registration remains.' }
        foreach ($file in $manifest.files) {
            Assert-Check (-not (Test-Path -LiteralPath (Join-Path $target $file.path))) "Payload remains after uninstall: $($file.path)"
        }
        if ($installerMetadata.PSObject.Properties.Name -contains 'msvcRuntime') {
            foreach ($file in $installerMetadata.msvcRuntime.files) {
                Assert-Check (-not (Test-Path -LiteralPath (Join-Path $target "bin/$($file.name)"))) 'App-Local runtime remains after uninstall.'
            }
            Assert-Check (-not (Test-Path -LiteralPath (Join-Path $target 'licenses/MSVC-NOTICE.txt'))) 'Runtime notice remains after uninstall.'
        }
        Assert-Check (-not (Test-Path -LiteralPath $shortcut)) 'Start Menu shortcut remains.'
        Assert-Check (-not (Test-Path -LiteralPath $desktopShortcut)) 'Desktop shortcut remains.'
        Assert-Check (Test-Path -LiteralPath (Join-Path $target 'unmanaged-sentinel.txt')) 'Uninstall removed an unknown file.'
    }
    Assert-Check ((Get-Content -LiteralPath (Join-Path $work 'personal-data-sentinel.txt') -Raw).Trim() -ceq 'Keep this synthetic personal data.') 'Synthetic personal data changed.'
    [ordered]@{ action = $Action; scope = $Scope; language = $Language; result = 'PASS'; elevated = $elevated; user = $identity.Name; payloadFiles = $manifest.files.Count } |
        ConvertTo-Json | Set-Content -LiteralPath (Join-Path $work "$Action.result.json") -Encoding utf8
    Write-Output "PASS: $Scope $Action; $($manifest.files.Count) payload files; fixture $work."
} finally { $root.Dispose() }
