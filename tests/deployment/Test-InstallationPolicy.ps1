#requires -Version 7.2
<#
Standalone consumer of the production installation policy. Registry fixtures use
only a unique HKCU test key. Optional current-user integration owns a fresh real
installation below dist/, never starts JobTracker, and uninstalls only that copy.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$InstallerPath,
    [Parameter(Mandatory)][string]$ManifestPath,
    [Parameter(Mandatory)][string]$WorkDirectory,
    [Parameter(Mandatory)][string]$IsccPath,
    [switch]$RunCurrentUserIntegration
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$work = [IO.Path]::TrimEndingDirectorySeparator([IO.Path]::GetFullPath($WorkDirectory))
if (-not $work.StartsWith((Join-Path $repository 'dist') + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Test output must be below this repository''s dist/ directory.'
}
if (Test-Path -LiteralPath $work) { throw 'Choose a fresh test output directory.' }
$ancestor = $work
while ($ancestor) {
    if ((Test-Path -LiteralPath $ancestor) -and
        ((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw "Reparse point in test output: $ancestor"
    }
    $ancestor = [IO.Path]::GetDirectoryName($ancestor)
}
$installer = (Get-Item -LiteralPath $InstallerPath).FullName
. (Join-Path $PSScriptRoot 'OfflineInstallerEvidence.ps1')
$manifest = Get-OfflineInstallerEvidence -InstallerPath $installer -MetadataPath $ManifestPath
$compiler = (Get-Item -LiteralPath $IsccPath).FullName
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$elevated = ([Security.Principal.WindowsPrincipal]::new($identity)).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if ($RunCurrentUserIntegration -and $elevated) { throw 'Current-user integration must run without OS elevation.' }
New-Item -ItemType Directory -Path $work | Out-Null
$results = [Collections.Generic.List[object]]::new()
$hive = [Microsoft.Win32.RegistryKey]::OpenBaseKey([Microsoft.Win32.RegistryHive]::CurrentUser, [Microsoft.Win32.RegistryView]::Registry64)
$fixtureKey = 'Software\JobTrackerInstallerTests\' + [Guid]::NewGuid().ToString('N')
$binaries = @{}

function Assert-Test([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}
function Test-Case([string]$Name, [scriptblock]$Body) {
    try {
        & $Body
        $results.Add([ordered]@{ name = $Name; result = 'PASS' })
        Write-Output "PASS: $Name"
    } catch {
        $results.Add([ordered]@{ name = $Name; result = 'FAIL'; error = $_.Exception.Message })
        throw
    } finally {
        $results | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $work 'results.json') -Encoding utf8
    }
}
function Invoke-Setup([string]$Program, [string[]]$Arguments, [string]$Log) {
    # Wait for the Inno loader and its children. Killing only the loader could
    # leave Setup modifying the fixture while its version metadata is restored.
    $process = Start-Process -FilePath $Program -ArgumentList (@('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/SP-', "/LOG=`"$Log`"") + $Arguments) -WindowStyle Hidden -PassThru -Wait
    return $process.ExitCode
}
function Invoke-Policy([string]$Name, [hashtable]$Parameters, [string]$Language = 'english') {
    $log = Join-Path $work "$Name.log"
    $arguments = @("/LANG=$Language", "/DIR=`"$work/unused`"")
    foreach ($entry in $Parameters.GetEnumerator()) { $arguments += "/$($entry.Key)=`"$($entry.Value)`"" }
    $code = Invoke-Setup $binaries['0.1.0'] $arguments $log
    Assert-Test ($code -eq 0) "Policy test exit $code; see $log"
    $content = Get-Content -LiteralPath $log -Raw
    Assert-Test ($content -match 'TEST PASS:' -and $content -notmatch 'TEST FAILURE:') "Missing policy assertions: $log"
}
function New-Record([string]$Name, [string]$Scope, [string]$Version) {
    $directory = Join-Path $work $Name
    New-Item -ItemType Directory -Path "$directory/bin" | Out-Null
    Copy-Item -LiteralPath $binaries[$Version] -Destination "$directory/bin/JobTrackerApp.exe"
    Set-Content -LiteralPath "$directory/JobTracker.install.ini" -Value "[Installation]`r`nScope=$Scope`r`nVersion=$Version"
    $keyPath = "$fixtureKey\$Name"
    $key = $hive.CreateSubKey($keyPath)
    try {
        $key.SetValue('DisplayVersion', $Version)
        $key.SetValue('InstallLocation', "$directory\")
        $key.SetValue('Inno Setup: App Path', $directory)
    } finally { $key.Dispose() }
    return @{ directory = $directory; key = $keyPath }
}
function Set-RecordValue([string]$KeyPath, [string]$Name, [string]$Value) {
    $key = $hive.OpenSubKey($KeyPath, $true)
    try { $key.SetValue($Name, $Value) } finally { $key.Dispose() }
}

try {
    foreach ($version in @('0.0.9', '0.1.0', '0.2.0')) {
        $arguments = @("/DPipelineFile=$repository/Installer/PayloadSafety.iss", "/DPolicyFile=$repository/Installer/InstallationPolicy.iss",
            "/DTestOutput=$work", "/DTestVersion=$version", "$PSScriptRoot/InstallationPolicyHarness.iss")
        $output = @(& $compiler @arguments 2>&1)
        $code = $LASTEXITCODE
        $output | Set-Content -LiteralPath "$work/compile-$version.log" -Encoding utf8
        Assert-Test ($code -eq 0) "Test harness compile failed: $work/compile-$version.log"
        $binaries[$version] = Join-Path $work "policy-$version-test-only.exe"
    }
    Test-Case 'strict-numeric-version-parser' { Invoke-Policy 'parse' @{ Case = 'parse' } }
    $other = Join-Path $work 'other-installation'
    foreach ($scope in @('CurrentUser', 'AllUsers')) {
        $record = New-Record "record-$scope" $scope '0.1.0'
        foreach ($incoming in @('0.1.0', '0.2.0', '0.0.9')) {
            $expected = if ($incoming -eq '0.0.9') { 'InstallDowngrade' } else { '' }
            Test-Case "$scope-same-scope-$incoming" {
                Invoke-Policy "$scope-$incoming" @{ Case='record'; Key=$record.key; Scope=$scope; Path=$record.directory; Incoming=$incoming; SameScope='yes'; Expected=$expected }
            }
        }
        foreach ($incoming in @('0.1.0', '0.2.0', '0.0.9')) {
            Test-Case "$scope-cross-scope-$incoming" {
                Invoke-Policy "$scope-cross-$incoming" @{ Case='record'; Key=$record.key; Scope=$scope; Path=$other; Incoming=$incoming; Expected='InstallScopeConflict' }
            }
        }
        Test-Case "$scope-cross-scope-russian" {
            Invoke-Policy "$scope-cross-russian" @{ Case='record'; Key=$record.key; Scope=$scope; Path=$other; Incoming='0.1.0'; Expected='InstallScopeConflict' } 'russian'
        }
        foreach ($path in @($record.directory, "$($record.directory)\child", $work)) {
            $index = $results.Count
            Test-Case "$scope-overlap-$index" {
                Invoke-Policy "overlap-$index" @{ Case='record'; Key=$record.key; Scope=$scope; Path=$path; Incoming='0.1.0'; Expected='InstallScopeConflict' }
            }
        }
        Test-Case "$scope-relocation-requires-uninstall" {
            Invoke-Policy "$scope-relocate" @{ Case='record'; Key=$record.key; Scope=$scope; Path=$other; Incoming='0.1.0'; SameScope='yes'; Expected='InstallMove' }
        }
    }
    $record = New-Record 'invalid-record' 'CurrentUser' '0.1.0'
    foreach ($bad in @('invalid', '0.1', '0.2.0', '0.1.0.0')) {
        Set-RecordValue $record.key 'DisplayVersion' $bad
        Test-Case "untrusted-registry-version-$bad" {
            Invoke-Policy "bad-version-$bad" @{ Case='record'; Key=$record.key; Scope='CurrentUser'; Path=$record.directory; Incoming='0.2.0'; SameScope='yes'; Expected='InstallStateInvalid' }
        }
    }
    Set-RecordValue $record.key 'DisplayVersion' '0.1.0'
    Set-RecordValue $record.key 'Inno Setup: App Path' $other
    Test-Case 'conflicting-registered-paths' {
        Invoke-Policy 'bad-path' @{ Case='record'; Key=$record.key; Scope='CurrentUser'; Path=$record.directory; Incoming='0.1.0'; SameScope='yes'; Expected='InstallStateInvalid' }
    }
    Set-RecordValue $record.key 'Inno Setup: App Path' $record.directory
    Set-Content -LiteralPath "$($record.directory)/JobTracker.install.ini" -Value "[Installation]`r`nScope=AllUsers"
    Test-Case 'conflicting-scope-marker' {
        Invoke-Policy 'bad-marker' @{ Case='record'; Key=$record.key; Scope='CurrentUser'; Path=$record.directory; Incoming='0.1.0'; SameScope='yes'; Expected='InstallStateInvalid' }
    }
    foreach ($bad in @('', 'invalid', '0.1', '0.2.0', '0.1.0.0')) {
        $markerText = "[Installation]`r`nScope=CurrentUser"
        if ($bad) { $markerText += "`r`nVersion=$bad" }
        Set-Content -LiteralPath "$($record.directory)/JobTracker.install.ini" -Value $markerText
        Test-Case "untrusted-marker-version-$bad" {
            Invoke-Policy "bad-marker-version-$bad" @{ Case='record'; Key=$record.key; Scope='CurrentUser'; Path=$record.directory; Incoming='0.1.0'; SameScope='yes'; Expected='InstallStateInvalid' }
        }
    }
    Set-Content -LiteralPath "$($record.directory)/JobTracker.install.ini" -Value "[Installation]`r`nScope=CurrentUser`r`nVersion=0.1.0"
    Test-Case 'unloaded-profile-fails-closed' { Invoke-Policy 'profile' @{ Case='unloaded-profile'; Expected='InstallProfileUnavailable' } }
    Test-Case 'case-and-trailing-separator-alias' {
        Invoke-Policy 'case' @{ Case='path'; Path="$($record.directory.ToUpperInvariant())\"; Canonical=$record.directory }
    }
    Test-Case 'short-path-alias' { Invoke-Policy 'shortpath' @{ Case='shortpath'; Path=$record.directory } }
    foreach ($bad in @('relative\folder', '\\server\share\app', "$other\..\elsewhere", "$other.", "$other ", "$other`:stream")) {
        $index = $results.Count
        Test-Case "ambiguous-path-$index" { Invoke-Policy "bad-path-$index" @{ Case='path'; Path=$bad; Expected='InstallStateInvalid' } }
    }
    $unusedDrive = @('Z','Y','X','W','V') | Where-Object { -not (Test-Path "${_}:\") } | Select-Object -First 1
    if ($unusedDrive) {
        Test-Case 'absent-drive-terminates' { Invoke-Policy 'absent-drive' @{ Case='path'; Path="${unusedDrive}:\missing\app"; Expected='InstallStateInvalid' } }
    }
    $junction = Join-Path $work 'alias-junction'
    New-Item -ItemType Junction -Path $junction -Target $record.directory | Out-Null
    Test-Case 'junction-rejected' { Invoke-Policy 'junction' @{ Case='path'; Path="$junction\child"; Expected='PayloadUnsafePath' } }
    $empty = Join-Path $work 'empty'
    New-Item -ItemType Directory -Path $empty | Out-Null
    Test-Case 'empty-destination-accepted' { Invoke-Policy 'empty' @{ Case='empty'; Path=$empty } }
    Test-Case 'nonempty-destination-preserved' { Invoke-Policy 'nonempty' @{ Case='empty'; Path=$record.directory; Expected='InstallFolderNotEmpty' } }
    Set-Content -LiteralPath "$($record.directory)/bin/JobTrackerApp.exe" -Value 'Corrupt test executable'
    Test-Case 'damaged-executable-fails-closed' {
        Invoke-Policy 'corrupt-exe' @{ Case='record'; Key=$record.key; Scope='CurrentUser'; Path=$record.directory; Incoming='0.1.0'; SameScope='yes'; Expected='InstallStateInvalid' }
    }
    Test-Case 'lifecycle-lock-reentrant-in-owner' { Invoke-Policy 'lock' @{ Case='lock' } }
    $mutex = [Threading.Mutex]::new($false, 'Global\JobTracker.InstallationState')
    try {
        Test-Case 'concurrent-lifecycle-change-rejected' { Invoke-Policy 'busy' @{ Case='lock'; Expected='InstallBusy' } }
    } finally { $mutex.Dispose() }

    if ($RunCurrentUserIntegration) {
        $realWork = Join-Path $work 'currentuser'
        $target = Join-Path $realWork 'application'
        $realKey = 'Software\Microsoft\Windows\CurrentVersion\Uninstall\JobTracker.CurrentUser_is1'
        $localTest = Join-Path $PSScriptRoot 'Test-LocalInstaller.ps1'
        Test-Case 'real-currentuser-install' {
            & $localTest -InstallerPath $installer -ManifestPath $ManifestPath -WorkDirectory $realWork -Scope CurrentUser -Action Install -WithDesktopShortcut
        }
        Test-Case 'read-only-loaded-HKU-profile-discovery' {
            Invoke-Policy 'loaded-profile' @{ Case='loaded-profile'; Sid=$identity.User.Value; Expected='InstallScopeConflict' }
        }
        Test-Case 'read-only-host-allusers-audit' {
            # Enumeration order can encounter the owned CurrentUser fixture or
            # an unloaded profile first. Both must block; this never installs.
            Invoke-Policy 'allusers-audit' @{ Case='allusers-audit'; Expected='InstallScopeConflict'; AlternativeExpected='InstallProfileUnavailable' }
        }
        $exe = Join-Path $target 'bin/JobTrackerApp.exe'
        $backup = Join-Path $realWork 'original-JobTrackerApp.exe'
        Copy-Item -LiteralPath $exe -Destination $backup
        $marker = Join-Path $target 'JobTracker.install.ini'
        $originalMarker = Get-Content -LiteralPath $marker -Raw
        $actual = $hive.OpenSubKey($realKey)
        try { Assert-Test ($actual.GetValue('InstallLocation').TrimEnd('\') -ieq $target) 'Refusing to mutate an unowned installation.' }
        finally { $actual.Dispose() }
        try {
            foreach ($version in @('0.0.9', '0.2.0')) {
                # Simulated prior release: coherent PE/registry/marker in our fixture.
                Copy-Item -LiteralPath $binaries[$version] -Destination $exe -Force
                Set-RecordValue $realKey 'DisplayVersion' $version
                Set-Content -LiteralPath $marker -Value "[Installation]`r`nScope=CurrentUser`r`nVersion=$version"
                $before = (Get-FileHash -LiteralPath $exe).Hash
                Test-Case "real-version-order-$version" {
                    $log = Join-Path $realWork "version-$version.log"
                    $code = Invoke-Setup $installer @('/CURRENTUSER', '/LANG=english') $log
                    if ($version -eq '0.0.9') {
                        Assert-Test ($code -eq 0) "Upgrade exit $code"
                        & $localTest -InstallerPath $installer -ManifestPath $ManifestPath -WorkDirectory $realWork -Scope CurrentUser -Action Verify
                    } else {
                        Assert-Test ($code -ne 0 -and (Get-Content -LiteralPath $log -Raw) -match 'Downgrades are blocked') 'Downgrade was not rejected by version policy.'
                        Assert-Test ((Get-FileHash -LiteralPath $exe).Hash -ceq $before) 'Rejected downgrade changed executable bytes.'
                        $key = $hive.OpenSubKey($realKey)
                        try { Assert-Test ($key.GetValue('DisplayVersion') -ceq $version) 'Rejected downgrade changed registry version.' }
                        finally { $key.Dispose() }
                    }
                }
            }
        } finally {
            Copy-Item -LiteralPath $backup -Destination $exe -Force
            Set-RecordValue $realKey 'DisplayVersion' $manifest.version
            Set-Content -LiteralPath $marker -Value $originalMarker -NoNewline
        }
        Test-Case 'real-reinstall-and-previous-path-reuse' {
            # Repair a damaged ancillary file while identity remains verifiable.
            Set-Content -LiteralPath "$target/bin/qt.conf" -Value 'Damaged fixture configuration'
            $code = Invoke-Setup $installer @('/CURRENTUSER', '/LANG=english') "$realWork/repair.log"
            Assert-Test ($code -eq 0) 'Repair failed.'
            & $localTest -InstallerPath $installer -ManifestPath $ManifestPath -WorkDirectory $realWork -Scope CurrentUser -Action Reinstall
        }
        Test-Case 'real-language-change-removes-owned-shortcuts' {
            $code = Invoke-Setup $installer @('/CURRENTUSER', '/LANG=russian', '/TASKS=desktopicon') "$realWork/russian.log"
            Assert-Test ($code -eq 0) 'Russian reinstall failed.'
            $programs = [Environment]::GetFolderPath('Programs')
            $desktop = [Environment]::GetFolderPath('DesktopDirectory')
            Assert-Test (-not (Test-Path -LiteralPath "$programs/JobTracker/JobTracker (current user).lnk")) 'Old English Start Menu shortcut remains.'
            Assert-Test (-not (Test-Path -LiteralPath "$desktop/JobTracker (current user).lnk")) 'Old English desktop shortcut remains.'
            Assert-Test (Test-Path -LiteralPath "$programs/JobTracker/JobTracker (текущий пользователь).lnk") 'Russian shortcut missing.'
            $code = Invoke-Setup $installer @('/CURRENTUSER', '/LANG=english', '/TASKS=') "$realWork/english.log"
            Assert-Test ($code -eq 0) 'English reinstall failed.'
            Assert-Test (-not (Test-Path -LiteralPath "$programs/JobTracker/JobTracker (текущий пользователь).lnk")) 'Old Russian Start Menu shortcut remains.'
            Assert-Test (-not (Test-Path -LiteralPath "$desktop/JobTracker (текущий пользователь).lnk")) 'Old Russian desktop shortcut remains.'
        }
        Test-Case 'real-destination-conflict-preserves-installation' {
            & $localTest -InstallerPath $installer -ManifestPath $ManifestPath -WorkDirectory $realWork -Scope CurrentUser -Action ScopeConflict
        }
        Test-Case 'real-currentuser-uninstall-preserves-sentinels' {
            & $localTest -InstallerPath $installer -ManifestPath $ManifestPath -WorkDirectory $realWork -Scope CurrentUser -Action Uninstall
        }
    }
    [ordered]@{ result='PASS'; checks=$results.Count; currentUserIntegration=[bool]$RunCurrentUserIntegration; elevated=$elevated;
        user=$identity.Name; policySha256=(Get-FileHash -LiteralPath "$repository/Installer/InstallationPolicy.iss").Hash;
        installerSha256=(Get-FileHash -LiteralPath $installer).Hash } |
        ConvertTo-Json | Set-Content -LiteralPath "$work/summary.json" -Encoding utf8
    Write-Output "PASS: $($results.Count) installation policy checks."
} finally {
    # Delete only this invocation's newly created, GUID-qualified test key.
    $hive.DeleteSubKeyTree($fixtureKey, $false)
    $hive.Dispose()
}
