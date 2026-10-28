#requires -Version 7.2
<#
Standalone native lifecycle checks using current production sources and tiny
synthetic files. Default mode substitutes logical scope and four disposable
shell folders in private source copies; it is not genuine Windows scope/UAC
evidence. -RunWindowsScopeIntegration additionally uses real /CURRENTUSER and
/ALLUSERS and real shell folders if the caller is already elevated.
The prerequisite is a test-only stub in both modes. No built Setup is required.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$WorkDirectory,
    [Parameter(Mandatory)][string]$IsccPath,
    [switch]$RunWindowsScopeIntegration
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
. (Join-Path $PSScriptRoot 'OfflineInstallerEvidence.ps1')
$work = [IO.Path]::TrimEndingDirectorySeparator([IO.Path]::GetFullPath($WorkDirectory))
if (-not $work.StartsWith((Join-Path $repository 'build/installer-tests') + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Shortcut lifecycle fixtures must stay below build/installer-tests/.'
}
Assert-NoReparsePoint $work
if (Test-Path -LiteralPath $work) { throw 'Choose a fresh fixture directory.' }
$compiler = (Get-Item -LiteralPath $IsccPath).FullName
$sourceDirectory = Join-Path $repository 'Installer/Windows'
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$elevated = ([Security.Principal.WindowsPrincipal]::new($identity)).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
New-Item -ItemType Directory -Path $work | Out-Null
$results = [Collections.Generic.List[object]]::new()
$realLinks = [Collections.Generic.List[object]]::new()
$setups = @{}
$sourceHashes = [ordered]@{}
foreach ($name in @('JobTracker.iss','PayloadSafety.iss','InstallationPolicy.iss','InstallOwnership.iss','PersonalData.iss')) {
    $sourceHashes[$name] = (Get-FileHash -LiteralPath (Join-Path $sourceDirectory $name)).Hash
}
function Assert-Test([bool]$Condition, [string]$Message) { if (-not $Condition) { throw $Message } }
function Assert-FixturePath([string]$Path) {
    $resolved = [IO.Path]::GetFullPath($Path)
    Assert-Test ($resolved.StartsWith($work + '\', [StringComparison]::OrdinalIgnoreCase)) "Unsafe fixture path: $Path"
    Assert-NoReparsePoint $resolved
}
function Record-Result([string]$Mode, [string]$Name, [string]$Result, [string]$Detail = '') {
    $results.Add([ordered]@{mode=$Mode;name=$Name;result=$Result;detail=$Detail})
    $results | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $work 'results.json') -Encoding utf8
    Write-Output ('{0}: {1}/{2} {3}' -f $Result,$Mode,$Name,$Detail)
}
function Test-Case([string]$Mode, [string]$Name, [scriptblock]$Body) {
    try { & $Body; Record-Result $Mode $Name 'PASS' }
    catch { Record-Result $Mode $Name 'FAIL' $_.Exception.Message; throw }
}
function Read-Marker([string]$Directory) {
    $values = @{}; $section = ''
    foreach ($line in Get-Content -LiteralPath (Join-Path $Directory 'JobTracker.install.ini')) {
        if ($line -match '^\[([^\]]+)\]$') { $section = $Matches[1] }
        elseif ($line -match '^([^=]+)=(.*)$') { $values["$section/$($Matches[1])"] = $Matches[2] }
    }
    return $values
}
function Get-Folder([string]$Mode, [string]$Scenario, [string]$Scope, [string]$Kind) {
    if ($Mode -eq 'fixture-shells') { return Join-Path $Scenario "shells/$Scope/$Kind" }
    $name = switch ("$Scope/$Kind") {
        'CurrentUser/Programs' { 'Programs' }; 'CurrentUser/Desktop' { 'DesktopDirectory' }
        'AllUsers/Programs' { 'CommonPrograms' }; 'AllUsers/Desktop' { 'CommonDesktopDirectory' }
    }
    return [Environment]::GetFolderPath($name)
}
function Invoke-Lifecycle([string]$Program, [string[]]$Arguments, [string]$Name) {
    $log = Join-Path $work "$Name.log"
    $options = @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/SP-','/LANG=english',('/LOG="' + $log + '"')) + $Arguments
    $process = Start-Process -FilePath $Program -ArgumentList $options -WindowStyle Hidden -Wait -PassThru
    $content = Get-Content -LiteralPath $log -Raw
    Assert-Test ($content -notmatch 'Manual-close|RequireClosedJobTracker|close this copy of JobTracker manually') 'Discarded application-close policy ran.'
    return [pscustomobject]@{exit=$process.ExitCode;log=$content;path=$log}
}
function Invoke-Install([string]$Mode, [string]$Scenario, [string]$Directory, [string]$Scope, [bool]$Desktop, [string]$Name, [bool]$Success = $true) {
    Assert-FixturePath $Directory
    $arguments = @(('/DIR="' + $Directory + '"'), $(if ($Desktop) { '/TASKS=desktopicon' } else { '/TASKS=' }))
    if ($Mode -eq 'fixture-shells') {
        $arguments += @('/CURRENTUSER', "/FixtureScope=$Scope",
            ('/FixturePrograms="' + (Get-Folder $Mode $Scenario $Scope 'Programs') + '"'),
            ('/FixtureDesktop="' + (Get-Folder $Mode $Scenario $Scope 'Desktop') + '"'))
    } else { $arguments += "/$($Scope.ToUpperInvariant())" }
    $operation = Invoke-Lifecycle $setups[$Mode] $arguments $Name
    Assert-Test (($operation.exit -eq 0) -eq $Success) "Unexpected Setup exit $($operation.exit); see $($operation.path)"
    $record = Read-Marker $Directory
    if ($Success) {
        Assert-Test ($record['Installation/Path'] -ieq $Directory -and $record['Installation/Scope'] -ceq $Scope) 'Local scope/path metadata mismatch.'
        Assert-Test (Test-Path -LiteralPath $record['Installation/Uninstaller']) 'Local uninstaller is missing.'
    } else { Assert-Test ($operation.log -match 'incomplete|failed|Cannot|could not') 'Failure lacked an incomplete-operation diagnostic.' }
    if ($Mode -eq 'windows-shells') {
        foreach ($key in @('StartMenu','Desktop')) {
            $path = [string]$record["Shortcuts/$key"]
            if ($path -and (Test-Path -LiteralPath $path)) {
                $realLinks.Add([pscustomobject]@{path=$path;target=(Join-Path $Directory 'bin/JobTrackerApp.exe')})
            }
        }
    }
    return $record
}
function Assert-Placement([string]$Mode, [string]$Scenario, [string]$Directory, [string]$Scope, [bool]$Desktop) {
    $record = Read-Marker $Directory
    foreach ($key in @('StartMenu','Desktop')) {
        $path = [string]$record["Shortcuts/$key"]
        if ($key -eq 'Desktop' -and -not $Desktop) { Assert-Test ($path -eq '') 'Deselected desktop task retained an ownership record.'; continue }
        $kind = if ($key -eq 'StartMenu') { 'Programs' } else { 'Desktop' }
        $folder = Get-Folder $Mode $Scenario $Scope $kind
        if ($key -eq 'StartMenu') { $folder = Join-Path $folder 'JobTracker' }
        Assert-Test ([IO.Path]::GetDirectoryName($path) -ieq $folder) "$key shortcut is outside the selected scope's shell folder: $path"
        Assert-Test (Test-Path -LiteralPath $path) "Recorded $key shortcut is absent: $path"
        Assert-Test ((Get-InstallerShortcutInfo $path).Target -ieq (Join-Path $Directory 'bin/JobTrackerApp.exe')) "$key shortcut has the wrong target."
    }
    return $record
}
function Invoke-Uninstall([string]$Directory, [string]$Name) {
    Assert-FixturePath $Directory
    $record = Read-Marker $Directory
    $program = [string]$record['Installation/Uninstaller']
    Assert-Test ([IO.Path]::GetDirectoryName($program) -ieq $Directory) 'Refusing a foreign uninstaller.'
    $links = @(foreach ($key in @('StartMenu','Desktop')) {
        $path = [string]$record["Shortcuts/$key"]
        if ($path -and (Test-Path -LiteralPath $path) -and
            (Get-InstallerShortcutInfo $path).Target -ieq (Join-Path $Directory 'bin/JobTrackerApp.exe')) { $path }
    })
    $operation = Invoke-Lifecycle $program @() $Name
    Assert-Test ($operation.exit -eq 0) "Uninstall exit $($operation.exit); see $($operation.path)"
    Assert-Test ($operation.log -match 'JobTracker personal data has been preserved') 'Silent uninstall did not report data retention.'
    Assert-Test (-not (Test-Path -LiteralPath (Join-Path $Directory 'bin/JobTrackerApp.exe'))) 'Owned program remains.'
    foreach ($path in $links) { Assert-Test (-not (Test-Path -LiteralPath $path)) "Owned shortcut remains: $path" }
}
function Set-ShortcutTarget([string]$Path, [string]$Target) {
    $shell = New-Object -ComObject WScript.Shell; $link = $null
    try { $link = $shell.CreateShortcut($Path); $link.TargetPath = $Target; $link.Save() }
    finally {
        if ($null -ne $link) { [Runtime.InteropServices.Marshal]::FinalReleaseComObject($link) | Out-Null }
        [Runtime.InteropServices.Marshal]::FinalReleaseComObject($shell) | Out-Null
    }
}
function Compile-Consumer([string]$Mode) {
    $directory = Join-Path $work "consumer-$Mode"
    New-Item -ItemType Directory -Path $directory | Out-Null
    foreach ($name in @('JobTracker.iss','PayloadSafety.iss','InstallationPolicy.iss','InstallOwnership.iss','PersonalData.iss')) {
        $text = Get-Content -LiteralPath (Join-Path $sourceDirectory $name) -Raw
        if ($Mode -eq 'fixture-shells' -and $name -in @('JobTracker.iss','InstallOwnership.iss')) {
            $text = $text.Replace('IsAdminInstallMode', 'ShortcutFixtureAllUsers')
            $text = $text.Replace("'{autoprograms}\JobTracker'", "'{param:FixturePrograms}\JobTracker'")
            $text = $text.Replace("'{autodesktop}'", "'{param:FixtureDesktop}'")
            if ($name -eq 'JobTracker.iss') {
                $text = $text.Replace('#include "PayloadSafety.iss"', '#include "PayloadSafety.iss"' + [Environment]::NewLine + '#include "ShortcutFixtureScope.iss"')
            }
        }
        Set-Content -LiteralPath (Join-Path $directory $name) -Value $text -NoNewline -Encoding utf8
    }
    @'
// TEST ONLY: native engine remains /CURRENTUSER; only logical scope is varied.
[Code]
function ShortcutFixtureAllUsers: Boolean;
begin
  Result := ExpandConstant('{param:FixtureScope|CurrentUser}') = 'AllUsers';
end;
'@ | Set-Content -LiteralPath (Join-Path $directory 'ShortcutFixtureScope.iss') -Encoding utf8
    @'
// TEST ONLY: no machine runtime query, package extraction or prerequisite run.
[CustomMessages]
english.MsvcCurrentUserInfo=TEST ONLY synthetic payload.
english.MsvcAllUsersInfo=TEST ONLY synthetic payload.
russian.MsvcCurrentUserInfo=TEST ONLY synthetic payload.
russian.MsvcAllUsersInfo=TEST ONLY synthetic payload.
[Code]
procedure CleanupMsvcRuntime;
begin
end;
function PrepareMsvcRuntime(const AllUsers: Boolean; var NeedsRestart: Boolean): String;
begin
  Result := '';
end;
'@ | Set-Content -LiteralPath (Join-Path $directory 'MsvcRuntime.iss') -Encoding utf8
    $identityFile = Join-Path $directory 'identity.iss'
    @"
#define JobTrackerProductName "JobTracker"
#define JobTrackerReleaseVersion "1.0.0"
#define JobTrackerWindowsVersion "1.0.0.0"
#define JobTrackerPublisher ""
#define JobTrackerExecutableName "JobTrackerApp.exe"
#define JobTrackerSetupIconFile "$sourceDirectory\JobTracker.ico"
#define JobTrackerUninstallDisplayIcon "{app}\bin\JobTrackerApp.exe,0"
"@ | Set-Content -LiteralPath $identityFile -Encoding utf8
    $runtimeFile = Join-Path $directory 'runtime.iss'
    '// TEST ONLY: synthetic payload has no runtime inputs.' | Set-Content -LiteralPath $runtimeFile -Encoding utf8
    $payloadSource = Join-Path $directory 'synthetic-app.txt'
    'Synthetic file only; this is not a runnable application.' | Set-Content -LiteralPath $payloadSource -Encoding utf8
    $payloadHash = (Get-FileHash -LiteralPath $payloadSource).Hash.ToLowerInvariant()
    $noticeHash = (Get-FileHash -LiteralPath (Join-Path $sourceDirectory 'MSVC-NOTICE.txt')).Hash.ToLowerInvariant()
    $payloadFile = Join-Path $directory 'payload.iss'
    @"
[Files]
Source: "$payloadSource"; DestDir: "{app}\bin"; DestName: "JobTrackerApp.exe"; Hash: "$payloadHash"; Flags: ignoreversion uninsneveruninstall; AfterInstall: RecordInstalledFile('bin\JobTrackerApp.exe')
[Code]
procedure RegisterCurrentPayload;
begin
  RegisterOwnedCandidate('bin\JobTrackerApp.exe', '$payloadHash');
  RegisterOwnedCandidate('licenses\MSVC-NOTICE.txt', '$noticeHash');
end;
procedure ValidatePayloadDestination;
begin
  RequireSafePayloadPath(ExpandConstant('{app}\bin\JobTrackerApp.exe'));
  RequireSafePayloadPath(ExpandConstant('{app}\licenses\MSVC-NOTICE.txt'));
end;
"@ | Set-Content -LiteralPath $payloadFile -Encoding utf8
    $harness = Join-Path $directory 'ShortcutLifecycleHarness.iss'
    @"
; TEST ONLY consumer; substitutions are restricted to this private source copy.
#include "JobTracker.iss"
[Setup]
SourceDir=$sourceDirectory
"@ | Set-Content -LiteralPath $harness -Encoding utf8
    $arguments = @("/DJobTrackerIdentityFile=$identityFile", "/DJobTrackerOutputDirectory=$directory",
        "/DJobTrackerMsvcFile=$runtimeFile", "/DJobTrackerPayloadFile=$payloadFile", $harness)
    $output = @(& $compiler @arguments 2>&1); $code = $LASTEXITCODE
    $output | Set-Content -LiteralPath (Join-Path $directory 'compile.log') -Encoding utf8
    Assert-Test ($code -eq 0) "Native shortcut consumer compilation failed: $directory/compile.log"
    $setups[$Mode] = Join-Path $directory 'JobTracker-Setup-1.0.0.exe'
}
function Test-ScopeTransitions([string]$Mode) {
    foreach ($initial in @('CurrentUser','AllUsers')) {
        $next = if ($initial -eq 'CurrentUser') { 'AllUsers' } else { 'CurrentUser' }
        Test-Case $Mode "$initial-to-$next-reinstall-and-uninstall" {
            $scenario = Join-Path $work "$Mode-$initial-transition"
            $directory = Join-Path $scenario 'application'; $other = Join-Path $scenario 'other-copy'
            $old = Invoke-Install $Mode $scenario $directory $initial $true "$Mode-$initial-initial"
            $otherRecord = Invoke-Install $Mode $scenario $other $initial $true "$Mode-$initial-other"
            $null = Assert-Placement $Mode $scenario $directory $initial $true
            $null = Invoke-Install $Mode $scenario $directory $next $true "$Mode-$initial-transition"
            $new = Assert-Placement $Mode $scenario $directory $next $true
            foreach ($key in @('StartMenu','Desktop')) {
                Assert-Test ($old["Shortcuts/$key"] -ine $new["Shortcuts/$key"]) "Scope transition reused old $key location."
                Assert-Test (-not (Test-Path -LiteralPath $old["Shortcuts/$key"])) "Old scope's $key shortcut was orphaned."
                Assert-Test (Test-Path -LiteralPath $otherRecord["Shortcuts/$key"]) "Transition removed another copy's $key shortcut."
            }
            $repeat = Invoke-Install $Mode $scenario $directory $next $true "$Mode-$initial-reinstall"
            Assert-Test ($repeat['Shortcuts/StartMenu'] -ceq $new['Shortcuts/StartMenu'] -and
                $repeat['Shortcuts/Desktop'] -ceq $new['Shortcuts/Desktop']) 'Same-scope reinstall duplicated shortcuts.'
            Invoke-Uninstall $directory "$Mode-$initial-uninstall"
            $null = Assert-Placement $Mode $scenario $other $initial $true
            Invoke-Uninstall $other "$Mode-$initial-other-uninstall"
        }
    }
    Test-Case $Mode 'desktop-checked-unchecked-checked' {
        $scenario = Join-Path $work "$Mode-desktop-choice"; $directory = Join-Path $scenario 'application'
        $old = Invoke-Install $Mode $scenario $directory 'CurrentUser' $true "$Mode-desktop-on"
        $null = Invoke-Install $Mode $scenario $directory 'CurrentUser' $false "$Mode-desktop-off"
        $null = Assert-Placement $Mode $scenario $directory 'CurrentUser' $false
        Assert-Test (-not (Test-Path -LiteralPath $old['Shortcuts/Desktop'])) 'Deselected desktop shortcut remained.'
        $null = Invoke-Install $Mode $scenario $directory 'CurrentUser' $true "$Mode-desktop-on-again"
        $null = Assert-Placement $Mode $scenario $directory 'CurrentUser' $true
        Invoke-Uninstall $directory "$Mode-desktop-uninstall"
    }
    Test-Case $Mode 'retargeted-shortcuts-survive-scope-and-desktop-changes' {
        $scenario = Join-Path $work "$Mode-retargeted"; $directory = Join-Path $scenario 'application'
        $other = Join-Path $scenario 'other-copy'
        $old = Invoke-Install $Mode $scenario $directory 'CurrentUser' $true "$Mode-retarget-initial"
        $null = Invoke-Install $Mode $scenario $other 'CurrentUser' $true "$Mode-retarget-other"
        $target = Join-Path $other 'bin/JobTrackerApp.exe'
        foreach ($key in @('StartMenu','Desktop')) { Set-ShortcutTarget $old["Shortcuts/$key"] $target }
        $null = Invoke-Install $Mode $scenario $directory 'AllUsers' $false "$Mode-retarget-transition"
        $null = Assert-Placement $Mode $scenario $directory 'AllUsers' $false
        foreach ($key in @('StartMenu','Desktop')) { Assert-Test ((Get-InstallerShortcutInfo $old["Shortcuts/$key"]).Target -ieq $target) 'User-retargeted shortcut changed.' }
        $null = Invoke-Install $Mode $scenario $directory 'CurrentUser' $true "$Mode-retarget-back"
        $new = Assert-Placement $Mode $scenario $directory 'CurrentUser' $true
        Assert-Test ($new['Shortcuts/Desktop'] -ine $old['Shortcuts/Desktop']) 'Rechecked desktop overwrote the retargeted shortcut.'
        Invoke-Uninstall $directory "$Mode-retarget-uninstall"
        foreach ($key in @('StartMenu','Desktop')) { Assert-Test ((Get-InstallerShortcutInfo $old["Shortcuts/$key"]).Target -ieq $target) 'Uninstall removed a user-retargeted shortcut.' }
        Invoke-Uninstall $other "$Mode-retarget-other-uninstall"
    }
}
try {
    Test-Case 'fixture-shells' 'compile-current-production-lifecycle' { Compile-Consumer 'fixture-shells' }
    Test-ScopeTransitions 'fixture-shells'
    foreach ($initial in @('CurrentUser','AllUsers')) {
        $next = if ($initial -eq 'CurrentUser') { 'AllUsers' } else { 'CurrentUser' }
        Test-Case 'fixture-shells' "$initial-to-$next-removal-failure-keeps-old-record" {
            $scenario = Join-Path $work "locked-$initial"; $directory = Join-Path $scenario 'application'
            $old = Invoke-Install 'fixture-shells' $scenario $directory $initial $true "locked-$initial-initial"
            $blocked = [string]$old['Shortcuts/StartMenu']
            $stream = [IO.File]::Open($blocked, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::Read)
            try {
                $failed = Invoke-Install 'fixture-shells' $scenario $directory $next $true "locked-$initial-transition" $false
                Assert-Test ($failed['Shortcuts/StartMenu'] -ceq $blocked -and (Test-Path -LiteralPath $blocked)) 'Failed removal lost the old owned shortcut record.'
                Assert-Test ($failed['Shortcuts/Desktop'] -ceq $old['Shortcuts/Desktop']) 'An earlier failure orphaned the remaining desktop shortcut.'
            } finally { $stream.Dispose() }
            $null = Invoke-Install 'fixture-shells' $scenario $directory $next $true "locked-$initial-retry"
            $null = Assert-Placement 'fixture-shells' $scenario $directory $next $true
            Assert-Test (-not (Test-Path -LiteralPath $blocked)) 'Retry left the previously locked old shortcut.'
            Invoke-Uninstall $directory "locked-$initial-uninstall"
        }
        Test-Case 'fixture-shells' "$initial-to-$next-desktop-failure-retains-mixed-records" {
            $scenario = Join-Path $work "desktop-transition-$initial"; $directory = Join-Path $scenario 'application'
            $old = Invoke-Install 'fixture-shells' $scenario $directory $initial $true "desktop-transition-$initial-initial"
            $blocked = [string]$old['Shortcuts/Desktop']
            $stream = [IO.File]::Open($blocked, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::Read)
            try {
                $failed = Invoke-Install 'fixture-shells' $scenario $directory $next $true "desktop-transition-$initial-failure" $false
                $newStart = [string]$failed['Shortcuts/StartMenu']
                $newFolder = Join-Path (Get-Folder 'fixture-shells' $scenario $next 'Programs') 'JobTracker'
                Assert-Test ([IO.Path]::GetDirectoryName($newStart) -ieq $newFolder -and
                    (Test-Path -LiteralPath $newStart) -and -not (Test-Path -LiteralPath $old['Shortcuts/StartMenu'])) 'First shortcut was not migrated before the desktop failure.'
                Assert-Test ((Get-InstallerShortcutInfo $newStart).Target -ieq (Join-Path $directory 'bin/JobTrackerApp.exe')) 'Migrated Start Menu link has the wrong target.'
                Assert-Test ($failed['Shortcuts/Desktop'] -ceq $blocked -and
                    (Test-Path -LiteralPath $blocked)) 'Partial transition lost the old desktop ownership record.'
            } finally { $stream.Dispose() }
            $retry = Invoke-Install 'fixture-shells' $scenario $directory $next $true "desktop-transition-$initial-retry"
            $null = Assert-Placement 'fixture-shells' $scenario $directory $next $true
            Assert-Test ($retry['Shortcuts/StartMenu'] -ceq $newStart -and
                -not (Test-Path -LiteralPath $blocked)) 'Retry did not preserve successful work and finish the old desktop removal.'
            Invoke-Uninstall $directory "desktop-transition-$initial-uninstall"
        }
    }
    foreach ($task in @($false,$true)) {
        Test-Case 'fixture-shells' "desktop-task-$task-read-failure-retains-record" {
            $scenario = Join-Path $work "desktop-readlocked-$task"; $directory = Join-Path $scenario 'application'
            $old = Invoke-Install 'fixture-shells' $scenario $directory 'CurrentUser' $true "desktop-readlocked-$task-initial"
            $path = [string]$old['Shortcuts/Desktop']
            $stream = [IO.File]::Open($path, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::None)
            try {
                $failed = Invoke-Install 'fixture-shells' $scenario $directory 'CurrentUser' $task "desktop-readlocked-$task-operation" $false
                Assert-Test ($failed['Shortcuts/Desktop'] -ceq $path -and (Test-Path -LiteralPath $path)) 'Read failure erased the shortcut ownership record.'
                Assert-Test (@(Get-ChildItem -LiteralPath (Split-Path -Parent $path) -Filter '*.lnk').Count -eq 1) 'Unreadable recorded shortcut produced an untracked replacement.'
            } finally { $stream.Dispose() }
            $null = Invoke-Install 'fixture-shells' $scenario $directory 'CurrentUser' $task "desktop-readlocked-$task-retry"
            $null = Assert-Placement 'fixture-shells' $scenario $directory 'CurrentUser' $task
            Invoke-Uninstall $directory "desktop-readlocked-$task-uninstall"
        }
    }
    Test-Case 'fixture-shells' 'desktop-deselection-delete-failure-retains-record' {
        $scenario = Join-Path $work 'desktop-locked'; $directory = Join-Path $scenario 'application'
        $old = Invoke-Install 'fixture-shells' $scenario $directory 'CurrentUser' $true 'desktop-locked-initial'
        $path = [string]$old['Shortcuts/Desktop']
        $stream = [IO.File]::Open($path, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::Read)
        try {
            $failed = Invoke-Install 'fixture-shells' $scenario $directory 'CurrentUser' $false 'desktop-locked-off' $false
            Assert-Test ($failed['Shortcuts/Desktop'] -ceq $path -and (Test-Path -LiteralPath $path)) 'Deletion failure erased its ownership record.'
        } finally { $stream.Dispose() }
        $null = Invoke-Install 'fixture-shells' $scenario $directory 'CurrentUser' $false 'desktop-locked-retry'
        $null = Assert-Placement 'fixture-shells' $scenario $directory 'CurrentUser' $false
        Invoke-Uninstall $directory 'desktop-locked-uninstall'
    }
    Test-Case 'fixture-shells' 'unverifiable-desktop-is-preserved-and-reported' {
        $scenario = Join-Path $work 'desktop-unverifiable'; $directory = Join-Path $scenario 'application'
        $old = Invoke-Install 'fixture-shells' $scenario $directory 'CurrentUser' $true 'desktop-unverifiable-initial'
        $path = [string]$old['Shortcuts/Desktop']; $bytes = [IO.File]::ReadAllBytes($path)
        [IO.File]::WriteAllText($path, 'Not a valid Shell Link; preserve this unverified file.')
        $hash = (Get-FileHash -LiteralPath $path).Hash
        $failed = Invoke-Install 'fixture-shells' $scenario $directory 'CurrentUser' $false 'desktop-unverifiable-off' $false
        Assert-Test ($failed['Shortcuts/Desktop'] -ceq $path -and (Get-FileHash -LiteralPath $path).Hash -ceq $hash) 'Unverified shortcut was changed or lost its retry record.'
        [IO.File]::WriteAllBytes($path, $bytes)
        $null = Invoke-Install 'fixture-shells' $scenario $directory 'CurrentUser' $false 'desktop-unverifiable-retry'
        Invoke-Uninstall $directory 'desktop-unverifiable-uninstall'
    }
    Test-Case 'fixture-shells' 'creation-failure-keeps-intended-location-and-other-old-record' {
        $scenario = Join-Path $work 'creation-blocked'; $directory = Join-Path $scenario 'application'
        $old = Invoke-Install 'fixture-shells' $scenario $directory 'CurrentUser' $true 'creation-blocked-initial'
        $blocked = Get-Folder 'fixture-shells' $scenario 'AllUsers' 'Programs'
        New-Item -ItemType Directory -Path (Split-Path -Parent $blocked) | Out-Null
        Set-Content -LiteralPath $blocked -Value 'A file obstructs creation of the destination shell folder.'
        $failed = Invoke-Install 'fixture-shells' $scenario $directory 'AllUsers' $true 'creation-blocked-transition' $false
        Assert-Test ([IO.Path]::GetDirectoryName([string]$failed['Shortcuts/StartMenu']) -ieq (Join-Path $blocked 'JobTracker')) 'Creation failure did not retain the intended resulting path.'
        Assert-Test (-not (Test-Path -LiteralPath $old['Shortcuts/StartMenu'])) 'Old Start Menu link was not handled before its metadata changed.'
        Assert-Test ($failed['Shortcuts/Desktop'] -ceq $old['Shortcuts/Desktop'] -and (Test-Path -LiteralPath $old['Shortcuts/Desktop'])) 'Creation failure orphaned the untouched old desktop link.'
        Assert-FixturePath $blocked
        Move-Item -LiteralPath $blocked -Destination (Join-Path $scenario 'preserved-creation-obstruction.txt')
        $null = Invoke-Install 'fixture-shells' $scenario $directory 'AllUsers' $true 'creation-blocked-retry'
        $null = Assert-Placement 'fixture-shells' $scenario $directory 'AllUsers' $true
        Invoke-Uninstall $directory 'creation-blocked-uninstall'
    }
    if ($RunWindowsScopeIntegration -and $elevated) {
        Test-Case 'windows-shells' 'compile-native-scope-lifecycle' { Compile-Consumer 'windows-shells' }
        Test-ScopeTransitions 'windows-shells'
    } else {
        $reason = if (-not $RunWindowsScopeIntegration) { '-RunWindowsScopeIntegration was not selected.' } else { 'The Windows token is not elevated; /ALLUSERS requires administrator approval.' }
        Record-Result 'windows-shells' 'actual-currentuser-allusers-transitions' 'NOT_RUN' $reason
    }
    Record-Result 'interactive' 'alternate-credential-UAC-and-dialogs' 'NOT_RUN' 'Unattended fixtures do not exercise interactive UAC credential switching or confirmation dialogs.'
    foreach ($name in $sourceHashes.Keys) {
        Assert-Test ((Get-FileHash -LiteralPath (Join-Path $sourceDirectory $name)).Hash -ceq $sourceHashes[$name]) "Production source changed during the native suite: $name"
    }
    [ordered]@{
        result='PASS'; passed=@($results | Where-Object result -eq 'PASS').Count
        notRun=@($results | Where-Object result -eq 'NOT_RUN').Count
        elevated=$elevated; user=$identity.Name; sourceHashes=$sourceHashes; installerSha256=$null
        fixtureSubstitutions=@('logical scope','user/common shell folders','runtime prerequisite')
        windowsScopeIntegration=[bool]($RunWindowsScopeIntegration -and $elevated)
        personalDataTouched=$false; applicationLaunched=$false
    } | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $work 'summary.json') -Encoding utf8
} finally {
    foreach ($link in $realLinks) {
        if (Test-Path -LiteralPath $link.path) {
            Assert-NoReparsePoint $link.path
            $target = (Get-InstallerShortcutInfo $link.path).Target
            if ($target.StartsWith($work + '\', [StringComparison]::OrdinalIgnoreCase)) { Remove-Item -LiteralPath $link.path }
        }
    }
}
