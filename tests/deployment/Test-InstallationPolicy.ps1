#requires -Version 7.2
<#
Native consumer of the production local installation policy. Every fixture is
under a fresh build/installer-tests/ directory. No registry fixture, profile
scan, application launch or personal-data access is used. Optional integration
installs and silently removes only the copies created by Test-LocalInstaller.
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
if (-not $work.StartsWith((Join-Path $repository 'build/installer-tests') + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Policy fixtures must be below this repository''s build/installer-tests/ directory.'
}
if (Test-Path -LiteralPath $work) { throw 'Choose a fresh test output directory.' }
. (Join-Path $PSScriptRoot 'OfflineInstallerEvidence.ps1')
Assert-NoReparsePoint $work
$installer = (Get-Item -LiteralPath $InstallerPath).FullName
$manifest = Get-OfflineInstallerEvidence -InstallerPath $installer -MetadataPath $ManifestPath
$compiler = (Get-Item -LiteralPath $IsccPath).FullName
$productExe = Join-Path $manifest.payloadDirectory 'bin/JobTrackerApp.exe'
Assert-NoReparsePoint $productExe
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$elevated = ([Security.Principal.WindowsPrincipal]::new($identity)).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if ($RunCurrentUserIntegration -and $elevated) { throw 'Current-user integration must run without OS elevation.' }
New-Item -ItemType Directory -Path $work | Out-Null
$results = [Collections.Generic.List[object]]::new()

function Assert-Test([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}
function Test-Case([string]$Name, [scriptblock]$Body) {
    try {
        & $Body
        $results.Add([ordered]@{name=$Name;result='PASS'})
        Write-Output "PASS: $Name"
    } catch {
        $results.Add([ordered]@{name=$Name;result='FAIL';error=$_.Exception.Message})
        throw
    } finally {
        $results | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $work 'results.json') -Encoding utf8
    }
}
function Invoke-Policy([string]$Name, [hashtable]$Parameters, [string]$Language = 'english') {
    $log = Join-Path $work "$Name.log"
    $arguments = @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/SP-', "/LANG=$Language", "/DIR=`"$work/unused`"", "/LOG=`"$log`"")
    foreach ($entry in $Parameters.GetEnumerator()) { $arguments += "/$($entry.Key)=`"$($entry.Value)`"" }
    $process = Start-Process -FilePath $harness -ArgumentList $arguments -WindowStyle Hidden -PassThru -Wait
    Assert-Test ($process.ExitCode -eq 0) "Policy test exit $($process.ExitCode); see $log"
    $content = Get-Content -LiteralPath $log -Raw
    Assert-Test ($content -match 'TEST PASS:' -and $content -notmatch 'TEST FAILURE:') "Missing policy assertions: $log"
}
function Write-Marker([string]$Directory, [string]$Scope, [string]$Version, [string]$Product = 'JobTracker') {
    Set-Content -LiteralPath (Join-Path $Directory 'JobTracker.install.ini') -Encoding utf8 -Value @"
[Installation]
Product=$Product
Format=2
Path=$Directory
Scope=$Scope
Version=$Version
"@
}
function New-Record([string]$Name, [string]$Scope = 'CurrentUser', [string]$Version = '0.1.0') {
    $directory = Join-Path $work $Name
    New-Item -ItemType Directory -Path $directory | Out-Null
    Write-Marker $directory $Scope $Version
    Set-Content -LiteralPath (Join-Path $directory 'unrelated.txt') -Value 'Keep unknown files.'
    return $directory
}
function Check-Policy([string]$Name, [string]$Path, [string]$Incoming = '0.1.0', [string]$Expected = '', [bool]$AllUsers = $false) {
    $parameters = @{Case='policy';Path=$Path;Incoming=$Incoming;Expected=$Expected;AllUsers=$(if ($AllUsers) {'yes'} else {'no'})}
    Invoke-Policy $Name $parameters
}

$arguments = @("/DPipelineFile=$repository/Installer/Windows/PayloadSafety.iss", "/DPolicyFile=$repository/Installer/Windows/InstallationPolicy.iss",
    "/DTestOutput=$work", '/DTestVersion=0.1.0', "$PSScriptRoot/InstallationPolicyHarness.iss")
$output = @(& $compiler @arguments 2>&1)
$compileCode = $LASTEXITCODE
$output | Set-Content -LiteralPath (Join-Path $work 'compile.log') -Encoding utf8
Assert-Test ($compileCode -eq 0) "Policy harness compile failed: $work/compile.log"
$harness = Join-Path $work 'policy-0.1.0-test-only.exe'

Test-Case 'strict-numeric-version-parser' { Invoke-Policy 'parse' @{Case='parse'} }
$driveRoot = [IO.Path]::GetPathRoot($work)
$rootCandidate = Join-Path $driveRoot ('JobTracker-policy-' + [Guid]::NewGuid().ToString('N'))
Assert-Test (-not (Test-Path -LiteralPath $rootCandidate)) 'Root-level path regression requires a nonexistent destination.'
Test-Case 'new-drive-root-path-is-stable' { Invoke-Policy 'root-child' @{Case='path';Path=$rootCandidate;Canonical=$rootCandidate} }
$nestedRootCandidate = Join-Path $rootCandidate 'nested/JobTracker'
Test-Case 'new-nested-drive-root-path-is-stable' { Invoke-Policy 'root-nested' @{Case='path';Path=$nestedRootCandidate;Canonical=$nestedRootCandidate} }
Test-Case 'new-drive-root-path-with-trailing-separator' { Invoke-Policy 'root-child-trailing' @{Case='path';Path="$rootCandidate\";Canonical=$rootCandidate} }
Test-Case 'drive-root-canonical-path' { Invoke-Policy 'drive-root' @{Case='path';Path=$driveRoot;Canonical=$driveRoot} }
Test-Case 'drive-root-installation-rejected' { Check-Policy 'root-install' $driveRoot '0.1.0' 'InstallFolderNotEmpty' }
Test-Case 'wizard-selected-new-root-directory' { Check-Policy 'wizard-root' $rootCandidate }
Test-Case 'wizard-selected-new-root-directory-allusers' { Check-Policy 'wizard-root-allusers' $rootCandidate '0.1.0' '' $true }
Test-Case 'wizard-selected-new-nested-root-directory' { Check-Policy 'wizard-root-nested' $nestedRootCandidate }
Test-Case 'root-path-validation-does-not-create-directories' {
    Assert-Test (-not (Test-Path -LiteralPath $rootCandidate)) 'Policy validation created the proposed destination.'
}
$newPath = Join-Path $work 'new-destination'
Test-Case 'first-install-new-directory' { Check-Policy 'new' $newPath }
$empty = Join-Path $work 'empty-directory'
New-Item -ItemType Directory -Path $empty | Out-Null
Test-Case 'first-install-empty-directory' { Check-Policy 'empty' $empty }

foreach ($scope in @('CurrentUser', 'AllUsers')) {
    $record = New-Record "record-$scope" $scope
    foreach ($allUsers in @($false, $true)) {
        $mode = if ($allUsers) { 'allusers' } else { 'currentuser' }
        foreach ($incoming in @('0.1.0', '0.2.0', '0.0.9')) {
            $expected = if ($incoming -eq '0.0.9') { 'InstallDowngrade' } else { '' }
            $name = "$scope-$mode-$incoming"
            Test-Case $name { Check-Policy $name $record $incoming $expected $allUsers }
        }
        $separate = Join-Path $work "separate-$scope-$mode"
        $name = "$scope-$mode-older-separate-directory"
        Test-Case $name { Check-Policy $name $separate '0.0.9' '' $allUsers }
    }
    Test-Case "$scope-missing-executable-and-uninstaller-repair" { Check-Policy "$scope-missing-files" $record }
    Test-Case "$scope-nested-child-rejected" { Check-Policy "$scope-child" (Join-Path $record 'child') '0.1.0' 'InstallPathConflict' }
}

$damaged = New-Record 'damaged-executable'
New-Item -ItemType Directory -Path (Join-Path $damaged 'bin') | Out-Null
Set-Content -LiteralPath (Join-Path $damaged 'bin/JobTrackerApp.exe') -Value 'Damaged owned executable.'
Test-Case 'damaged-executable-with-marker-repair' { Check-Policy 'damaged-exe' $damaged }

$exeOnly = Join-Path $work 'executable-only'
New-Item -ItemType Directory -Path (Join-Path $exeOnly 'bin') | Out-Null
Copy-Item -LiteralPath $productExe -Destination (Join-Path $exeOnly 'bin/JobTrackerApp.exe')
Test-Case 'actual-product-executable-identifies-copy' { Invoke-Policy 'exe-identity' @{Case='identity';Path=$exeOnly;Identified='yes'} }
Test-Case 'missing-metadata-executable-only-repair' { Check-Policy 'exe-only' $exeOnly $manifest.version }
Write-Marker $exeOnly 'CurrentUser' 'damaged'
Test-Case 'damaged-marker-version-with-executable-repair' { Check-Policy 'damaged-marker' $exeOnly $manifest.version }
Write-Marker $exeOnly 'CurrentUser' '65535.0.0'
Test-Case 'newer-marker-cannot-be-bypassed-by-older-executable' { Check-Policy 'newer-marker' $exeOnly $manifest.version 'InstallDowngrade' }

$ledgerOnly = Join-Path $work 'ledger-only'
New-Item -ItemType Directory -Path $ledgerOnly | Out-Null
Set-Content -LiteralPath (Join-Path $ledgerOnly 'JobTracker.ownership.ini') -Encoding utf8 -Value "[Identity]`r`nProduct=JobTracker-2`r`nPath=$ledgerOnly`r`nCount=0"
Test-Case 'interrupted-copy-ledger-only-repair' { Check-Policy 'ledger-only' $ledgerOnly }

$unknown = Join-Path $work 'unknown-directory'
New-Item -ItemType Directory -Path (Join-Path $unknown 'bin') | Out-Null
Set-Content -LiteralPath (Join-Path $unknown 'bin/JobTrackerApp.exe') -Value 'This is not a recognized executable.'
Set-Content -LiteralPath (Join-Path $unknown 'unrelated.txt') -Value 'Keep unknown files.'
$unknownHash = (Get-FileHash -LiteralPath (Join-Path $unknown 'unrelated.txt')).Hash
Test-Case 'unrelated-nonempty-folder-rejected' { Check-Policy 'unknown' $unknown '0.1.0' 'InstallFolderNotEmpty' }
Write-Marker $unknown 'CurrentUser' '0.1.0' 'AnotherProduct'
Test-Case 'unrelated-product-marker-rejected' { Check-Policy 'unknown-product' $unknown '0.1.0' 'InstallFolderNotEmpty' }
Assert-Test ((Get-FileHash -LiteralPath (Join-Path $unknown 'unrelated.txt')).Hash -ceq $unknownHash) 'Rejected destination changed unrelated files.'

$outer = New-Record 'outer-install'
$inner = Join-Path $outer 'subfolder/inner-install'
New-Item -ItemType Directory -Path $inner | Out-Null
Write-Marker $inner 'AllUsers' '0.1.0'
Test-Case 'outer-install-containing-another-copy-rejected' { Check-Policy 'outer' $outer '0.1.0' 'InstallPathConflict' }

$deleted = New-Record 'manually-deleted-location'
$preserved = Join-Path $work 'saved-manually-deleted-copy'
Assert-Test ([IO.Path]::GetFullPath($deleted).StartsWith($work + '\', [StringComparison]::OrdinalIgnoreCase) -and
    [IO.Path]::GetFullPath($preserved).StartsWith($work + '\', [StringComparison]::OrdinalIgnoreCase)) 'Unsafe simulated folder deletion.'
Move-Item -LiteralPath $deleted -Destination $preserved
Test-Case 'fresh-install-after-program-folder-removed' { Check-Policy 'deleted-location' $deleted '0.0.9' }
Test-Case 'older-copy-allowed-beside-known-newer-copy' { Check-Policy 'older-independent' (Join-Path $work 'older-independent') '0.0.9' }

$regular = New-Record 'path-alias-target'
Test-Case 'case-and-trailing-separator-alias' { Invoke-Policy 'case-alias' @{Case='path';Path="$($regular.ToUpperInvariant())\";Canonical=$regular} }
Test-Case 'short-path-alias' { Invoke-Policy 'shortpath' @{Case='shortpath';Path=$regular} }
foreach ($bad in @('relative\folder', '\\server\share\app', "$regular\..\elsewhere", "$regular.", "$regular ", "$regular`:stream")) {
    $name = "ambiguous-path-$($results.Count)"
    Test-Case $name { Invoke-Policy $name @{Case='path';Path=$bad;Expected='InstallStateInvalid'} }
}
$unusedDrive = [char[]](65..90) | Where-Object { -not (Test-Path "${_}:\") } | Select-Object -First 1
if ($unusedDrive) {
    Test-Case 'absent-selected-drive-rejected' { Invoke-Policy 'absent-drive' @{Case='path';Path="${unusedDrive}:\missing\app";Expected='InstallStateInvalid'} }
    $moved = New-Record 'copy-with-disconnected-origin'
    $marker = Join-Path $moved 'JobTracker.install.ini'
    (Get-Content -LiteralPath $marker -Raw).Replace("Path=$moved", "Path=${unusedDrive}:\disconnected-original") | Set-Content -LiteralPath $marker
    Test-Case 'disconnected-source-does-not-block-new-local-copy-policy' { Check-Policy 'disconnected-origin' $moved }
}
$junction = Join-Path $work 'alias-junction'
New-Item -ItemType Junction -Path $junction -Target $regular | Out-Null
Test-Case 'junction-rejected' { Invoke-Policy 'junction' @{Case='path';Path="$junction\child";Expected='PayloadUnsafePath'} }

$data = Join-Path $work 'profile/Local/JobTracker/Data'
foreach ($other in @($data, (Join-Path $data 'child'), [IO.Path]::GetDirectoryName($data))) {
    $name = "personal-data-overlap-$($results.Count)"
    Test-Case $name { Invoke-Policy $name @{Case='overlap';Path=$data;Other=$other;Overlap='yes'} }
}
Test-Case 'personal-data-sibling-does-not-overlap' { Invoke-Policy 'data-sibling' @{Case='overlap';Path=$data;Other="$data-copy";Overlap='no'} }
Test-Case 'separate-program-data-folders-do-not-overlap' { Invoke-Policy 'data-separate' @{Case='overlap';Path=$data;Other=$regular;Overlap='no'} }

Test-Case 'installer-mutex-reentrant-in-owner' { Invoke-Policy 'lock' @{Case='lock'} }
$mutex = [Threading.Mutex]::new($false, 'Global\JobTracker.InstallationState')
try {
    Test-Case 'concurrent-installer-change-rejected' { Invoke-Policy 'busy' @{Case='lock';Expected='InstallBusy'} }
} finally { $mutex.Dispose() }

if ($RunCurrentUserIntegration) {
    $localTest = Join-Path $PSScriptRoot 'Test-LocalInstaller.ps1'
    $realWork = Join-Path $work 'currentuser'
    foreach ($action in @('Install', 'Reinstall', 'AdditionalCopy', 'RepairMissingUninstaller', 'RepairMissingLog', 'Uninstall')) {
        Test-Case "real-currentuser-$action" {
            & $localTest -InstallerPath $installer -ManifestPath $ManifestPath -WorkDirectory $realWork -Scope CurrentUser -Action $action -WithDesktopShortcut
        }
    }
}
[ordered]@{result='PASS';checks=$results.Count;currentUserIntegration=[bool]$RunCurrentUserIntegration;elevated=$elevated;
    user=$identity.Name;policySha256=(Get-FileHash -LiteralPath "$repository/Installer/Windows/InstallationPolicy.iss").Hash;
    installerSha256=(Get-FileHash -LiteralPath $installer).Hash} |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $work 'summary.json') -Encoding utf8
Write-Output "PASS: $($results.Count) local installation policy checks."
