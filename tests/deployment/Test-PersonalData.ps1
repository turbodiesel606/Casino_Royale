#requires -Version 7.2
<# Native offline cleanup tests use only new dist/ fixtures. Never resolve or
open the account's real JobTracker data. Production has no fixture overrides. #>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$WorkDirectory,
    [Parameter(Mandatory)][string]$IsccPath,
    [string]$InstallerPath,
    [string]$ManifestPath,
    [switch]$RunCurrentUserIntegration
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$work = [IO.Path]::TrimEndingDirectorySeparator([IO.Path]::GetFullPath($WorkDirectory))
if (-not $work.StartsWith((Join-Path $repository 'dist') + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Fixtures must be below this repository dist directory.'
}
if (Test-Path -LiteralPath $work) { throw 'Choose a fresh fixture directory.' }
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
if ($elevated) { throw 'Run native deletion checks without Windows elevation.' }
if ($RunCurrentUserIntegration -and (-not $InstallerPath -or -not $ManifestPath)) {
    throw 'Integration requires InstallerPath and ManifestPath.'
}
if ($RunCurrentUserIntegration) {
    . (Join-Path $PSScriptRoot 'OfflineInstallerEvidence.ps1')
    $metadata = Get-OfflineInstallerEvidence -InstallerPath $InstallerPath -MetadataPath $ManifestPath
}
$compiler = (Get-Item -LiteralPath $IsccPath).FullName
$personalDataSource = Join-Path $repository 'Installer/PersonalData.iss'
$personalDataHash = (Get-FileHash -LiteralPath $personalDataSource -Algorithm SHA256).Hash
New-Item -ItemType Directory -Path $work | Out-Null
$results = [Collections.Generic.List[object]]::new()
$harness = Join-Path $work 'personal-data-test-only.exe'
function Assert-Test([bool]$Condition, [string]$Detail) { if (-not $Condition) { throw $Detail } }
function Test-Case([string]$Name, [scriptblock]$Body) {
    try {
        & $Body
        $results.Add([ordered]@{ name=$Name; result='PASS' })
        Write-Output "PASS: $Name"
    } catch {
        $results.Add([ordered]@{ name=$Name; result='FAIL'; error=$_.Exception.Message })
        throw
    } finally {
        $results | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath "$work/results.json"
    }
}
function New-Data([string]$Name) {
    $local = Join-Path $work $Name
    $root = Join-Path $local 'JobTracker'
    New-Item -ItemType Directory -Path "$root/Data/Resumes" | Out-Null
    Set-Content -LiteralPath "$root/Data/database.sqlite" -Value 'Synthetic database: do not open with SQLite'
    Set-Content -LiteralPath "$root/Data/database.sqlite-wal" -Value 'Synthetic WAL'
    Set-Content -LiteralPath "$root/Data/Resumes/резюме.pdf" -Value 'Synthetic CV'
    Set-Content -LiteralPath "$root/settings.json" -Value 'Synthetic future user settings'
    Set-Content -LiteralPath "$local/outside.txt" -Value 'Must remain unchanged'
    return [pscustomobject]@{ Local=$local; Root=$root }
}
function Snapshot([string]$Root) {
    return @(Get-ChildItem -LiteralPath $Root -File -Recurse | Sort-Object FullName | ForEach-Object {
        $_.FullName.Substring($Root.Length) + ' ' + (Get-FileHash -LiteralPath $_.FullName).Hash
    }) -join "`n"
}
function Invoke-Native([string]$Program, [string[]]$Options, [string]$Name) {
    $arguments = @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/SP-','/LANG=english', "/LOG=`"$work/$Name.log`"") + $Options
    $process = Start-Process -FilePath $Program -ArgumentList $arguments -WindowStyle Hidden -PassThru -Wait
    $log = Get-Content -LiteralPath "$work/$Name.log" -Raw
    Assert-Test ($log -notmatch '(?m)^\d[^\r\n]*   Message box ') 'Unexpected modal dialog.'
    Assert-Test ($log -notmatch 'TEST FAILURE:') "Native assertion failed: $Name"
    Assert-Test ($process.ExitCode -eq 0) "Native process failed: $Name ($($process.ExitCode))"
}
function Invoke-Cleanup($Fixture, [string]$Name, [string]$Expected = '') {
    Invoke-Native $harness @('/Case=delete', "/LocalData=`"$($Fixture.Local)`"", "/Root=`"$($Fixture.Root)`"",
        "/Expected=`"$Expected`"", "/DIR=`"$work/unused-program-$Name`"") $Name
}
function Assert-NativeX64([string]$Path) {
    $bytes = [IO.File]::ReadAllBytes($Path)
    $pe = [BitConverter]::ToInt32($bytes, 0x3c)
    Assert-Test ([BitConverter]::ToUInt32($bytes, $pe) -eq 0x4550 -and
        [BitConverter]::ToUInt16($bytes, $pe + 4) -eq 0x8664 -and
        [BitConverter]::ToUInt16($bytes, $pe + 24) -eq 0x20b) "Expected native AMD64/PE32+: $Path"
}

Test-Case 'compile-native-consumer' {
    Set-Content -LiteralPath "$work/program.txt" -Value 'Synthetic installed program'
    $output = @(& $compiler "/DTestOutput=$work" (Join-Path $PSScriptRoot 'PersonalDataHarness.iss') 2>&1)
    $code = $LASTEXITCODE
    $output | Set-Content -LiteralPath "$work/compile.log"
    Assert-Test ($code -eq 0) 'Harness compilation failed.'
    $output = @(& $compiler "/DTestOutput=$work" (Join-Path $PSScriptRoot 'PersonalDataStateHarness.iss') 2>&1)
    $code = $LASTEXITCODE
    $output | Set-Content -LiteralPath "$work/compile-state.log"
    Assert-Test ($code -eq 0) 'State harness compilation failed.'
    # Only this private test include redirects process/UI/known-folder/registry
    # boundaries. Native ownership, path, handle and deletion code stays intact.
    # Exact counts fail closed when production changes require test review.
    $policy = Get-Content -LiteralPath $personalDataSource -Raw
    $substitutions = [ordered]@{
        IsAdmin=2; IsAdminInstallMode=3; UninstallSilent=1; ExpandConstant=5
        SuppressibleMsgBox=2; ReadInstallRecord=1; InstallKeyExists=4
    }
    foreach ($name in $substitutions.Keys) {
        $pattern = '\b' + $name + '\b'
        Assert-Test ([regex]::Matches($policy, $pattern).Count -eq $substitutions[$name]) "Review policy boundary substitution: $name"
        $policy = [regex]::Replace($policy, $pattern, "Test$name")
    }
    $policyPath = Join-Path $work 'PersonalData.test-only.iss'
    Set-Content -LiteralPath $policyPath -Value $policy -Encoding utf8
    [ordered]@{
        sourceSha256=$personalDataHash; substitutions=$substitutions
        testIncludeSha256=(Get-FileHash -LiteralPath $policyPath -Algorithm SHA256).Hash
    } | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath "$work/policy-boundaries.json"
    $output = @(& $compiler "/DTestOutput=$work" "/DTestPersonalDataFile=$policyPath" (Join-Path $PSScriptRoot 'PersonalDataPolicyHarness.iss') 2>&1)
    $code = $LASTEXITCODE
    $output | Set-Content -LiteralPath "$work/compile-policy.log"
    Assert-Test ($code -eq 0) 'Callback policy harness compilation failed.'
    foreach ($name in @('personal-data-test-only.exe','personal-data-state-test-only.exe','personal-data-policy-test-only.exe')) {
        Assert-NativeX64 (Join-Path $work $name)
    }
}
foreach ($language in @('english','russian')) {
    Test-Case "explicit-native-delete-$language" {
        $fixture = New-Data "delete-$language"
        $outsideHash = (Get-FileHash -LiteralPath "$($fixture.Local)/outside.txt").Hash
        Invoke-Native $harness @('/Case=delete', "/LANG=$language", "/LocalData=`"$($fixture.Local)`"",
            "/Root=`"$($fixture.Root)`"", "/DIR=`"$work/unused-program-$language`"") "delete-$language"
        Assert-Test (-not (Test-Path -LiteralPath $fixture.Root)) 'Confirmed tree remains.'
        Assert-Test ((Get-FileHash -LiteralPath "$($fixture.Local)/outside.txt").Hash -ceq $outsideHash) 'Outside file changed.'
    }
}
Test-Case 'empty-root' {
    $local = Join-Path $work 'empty'
    New-Item -ItemType Directory -Path "$local/JobTracker" | Out-Null
    Invoke-Cleanup ([pscustomobject]@{Local=$local; Root="$local\JobTracker"}) 'empty'
    Assert-Test (-not (Test-Path -LiteralPath "$local/JobTracker")) 'Empty root remains.'
}
Test-Case 'locked-file-refuses-before-any-delete' {
    $fixture = New-Data 'locked'
    $before = Snapshot $fixture.Root
    $lock = [IO.File]::Open("$($fixture.Root)/Data/database.sqlite", 'Open', 'Read', 'None')
    try { Invoke-Cleanup $fixture 'locked' 'Cannot lock personal-data path' }
    finally { $lock.Dispose() }
    Assert-Test ((Snapshot $fixture.Root) -ceq $before) 'Locked preflight changed data.'
}
Test-Case 'native-owner-SID-mismatch' {
    $fixture = New-Data 'owner-mismatch'
    $before = Snapshot $fixture.Root
    Invoke-Native $harness @('/Case=owner-mismatch', "/LocalData=`"$($fixture.Local)`"", "/Root=`"$($fixture.Root)`"",
        "/DIR=`"$work/owner-mismatch-program`"") 'owner-mismatch'
    Assert-Test ((Snapshot $fixture.Root) -ceq $before) 'Owner mismatch changed data.'
}
Test-Case 'readonly-refuses-before-any-delete' {
    $fixture = New-Data 'readonly'
    $before = Snapshot $fixture.Root
    $file = Get-Item -LiteralPath "$($fixture.Root)/Data/Resumes/резюме.pdf"
    $file.IsReadOnly = $true
    try { Invoke-Cleanup $fixture 'readonly' 'Read-only personal data' }
    finally { $file.IsReadOnly = $false }
    Assert-Test ((Snapshot $fixture.Root) -ceq $before) 'Readonly preflight changed data.'
}
Test-Case 'delete-access-denied-preserves-entire-tree' {
    $fixture = New-Data 'access-denied'
    $before = Snapshot $fixture.Root
    $file = Join-Path $fixture.Root 'Data/Resumes/резюме.pdf'
    $originalAcl = Get-Acl -LiteralPath $file
    $deniedAcl = Get-Acl -LiteralPath $file
    # DELETE can also be granted by the parent. Deny the directly requested
    # FILE_READ_ATTRIBUTES right so inherited DELETE_CHILD cannot bypass this case.
    $rule = [Security.AccessControl.FileSystemAccessRule]::new($identity.User,
        ([Security.AccessControl.FileSystemRights]::Delete -bor [Security.AccessControl.FileSystemRights]::ReadAttributes),
        [Security.AccessControl.AccessControlType]::Deny)
    $deniedAcl.AddAccessRule($rule)
    try {
        Set-Acl -LiteralPath $file -AclObject $deniedAcl
        Invoke-Cleanup $fixture 'access-denied' 'Cannot lock personal-data path'
    } finally { Set-Acl -LiteralPath $file -AclObject $originalAcl }
    Assert-Test ((Snapshot $fixture.Root) -ceq $before) 'Access-denied preflight changed data.'
}
Test-Case 'hardlink-refuses-with-outside-bytes-preserved' {
    $fixture = New-Data 'hardlink'
    New-Item -ItemType HardLink -Path "$($fixture.Root)/linked.txt" -Target "$($fixture.Local)/outside.txt" | Out-Null
    $before = Snapshot $fixture.Root
    Invoke-Cleanup $fixture 'hardlink' 'Hard-linked personal data'
    Assert-Test ((Snapshot $fixture.Root) -ceq $before) 'Hardlink refusal changed data.'
    Assert-Test ((Get-Content -LiteralPath "$($fixture.Local)/outside.txt" -Raw).Trim() -ceq 'Must remain unchanged') 'Outside hardlink bytes changed.'
}
foreach ($position in @('root','descendant','ancestor')) {
    Test-Case "junction-$position" {
        $fixture = New-Data "junction-$position"
        $outside = Join-Path $fixture.Local 'outside-directory'
        New-Item -ItemType Directory -Path $outside | Out-Null
        Set-Content -LiteralPath "$outside/sentinel.txt" -Value 'Outside junction target'
        $before = Snapshot $fixture.Root
        if ($position -eq 'root') {
            $alias = Join-Path $work 'root-alias'
            New-Item -ItemType Directory -Path $alias | Out-Null
            New-Item -ItemType Junction -Path "$alias/JobTracker" -Target $outside | Out-Null
            $candidate = [pscustomobject]@{ Local=$alias; Root="$alias\JobTracker" }
        } elseif ($position -eq 'ancestor') {
            $alias = Join-Path $work 'ancestor-alias'
            New-Item -ItemType Junction -Path $alias -Target $fixture.Local | Out-Null
            $candidate = [pscustomobject]@{ Local=$alias; Root="$alias\JobTracker" }
        } else {
            New-Item -ItemType Junction -Path "$($fixture.Root)/redirect" -Target $outside | Out-Null
            $candidate = $fixture
        }
        $expected = if ($position -eq 'ancestor') { 'destination' } else { 'Reparse point' }
        Invoke-Cleanup $candidate "junction-$position" $expected
        Assert-Test ((Get-Content -LiteralPath "$outside/sentinel.txt" -Raw).Trim() -ceq 'Outside junction target') 'Junction target changed.'
        Assert-Test ((Snapshot $fixture.Root) -ceq $before) 'Junction refusal changed original data.'
    }
}
foreach ($kind in @('broad-parent','wrong-leaf','dotdot','trailing-dot')) {
    Test-Case "unsafe-$kind" {
        $fixture = New-Data "unsafe-$kind"
        $before = Snapshot $fixture.Root
        $root = switch ($kind) {
            'broad-parent' { $fixture.Local }
            'wrong-leaf' { "$($fixture.Local)\Unrelated" }
            'dotdot' { "$($fixture.Local)\JobTracker\.." }
            'trailing-dot' { "$($fixture.Root)." }
        }
        Invoke-Cleanup ([pscustomobject]@{ Local=$fixture.Local; Root=$root }) "unsafe-$kind" 'Unexpected personal-data root'
        Assert-Test ((Snapshot $fixture.Root) -ceq $before) 'Unsafe-root refusal changed data.'
    }
}
Test-Case 'pre-completion-callback-does-not-delete' {
    $fixture = New-Data 'cancelled'
    $before = Snapshot $fixture.Root
    Invoke-Native $harness @('/Case=silent', "/LocalData=`"$($fixture.Local)`"", "/Root=`"$($fixture.Root)`"",
        "/DIR=`"$work/pre-completion-program`"") 'pre-completion'
    Assert-Test ((Snapshot $fixture.Root) -ceq $before) 'Pre-completion callback changed data.'
}
foreach ($partial in @($false, $true)) {
    $uninstallScenario = if ($partial) { 'partial' } else { 'complete' }
    Test-Case "native-uninstall-$uninstallScenario-preserves-data" {
        $fixture = New-Data "lifecycle-$uninstallScenario"
        $before = Snapshot $fixture.Root
        $program = Join-Path $work "lifecycle-program-$uninstallScenario"
        Invoke-Native $harness @('/Case=lifecycle', "/DIR=`"$program`"") "install-$uninstallScenario"
        $lock = $null
        if ($partial) { $lock = [IO.File]::Open("$program/bin/program.txt", 'Open', 'Read', 'None') }
        try {
            Invoke-Native "$program/unins000.exe" @("/Expected=$uninstallScenario", "/Result=`"$work/uninstall-$uninstallScenario.txt`"", '/DELETEPERSONALDATA=yes') "uninstall-$uninstallScenario"
        } finally { if ($null -ne $lock) { $lock.Dispose() } }
        Assert-Test ((Get-Content -LiteralPath "$work/uninstall-$uninstallScenario.txt" -Raw) -ceq 'PASS') 'Lifecycle assertion did not execute.'
        Assert-Test ((Snapshot $fixture.Root) -ceq $before) 'Native uninstall changed personal-data fixture.'
        Assert-Test ((Get-Content -LiteralPath "$work/uninstall-$uninstallScenario.log" -Raw) -match 'personal data has been preserved') 'Production silent policy was not reached.'
    }
}
foreach ($language in @('english','russian')) {
    foreach ($scenario in @('no','close','yes','silent','allusers','elevated',
            'core-allusers','core-elevated','account-missing','account-invalid','account-mismatch',
            'restart-required','incomplete','shared-cu64','shared-cu32','shared-lm64','shared-lm32',
            'registry-denied','late-shared','late-program','pre-completion')) {
        Test-Case "callback-$language-$scenario" {
            $fixture = New-Data "callback-$language-$scenario"
            $before = Snapshot $fixture.Root
            $outside = (Get-FileHash -LiteralPath "$($fixture.Local)/outside.txt").Hash
            Invoke-Native (Join-Path $work 'personal-data-policy-test-only.exe') @("/LANG=$language", "/Case=$scenario") "callback-$language-$scenario"
            Assert-Test ((Get-Content -LiteralPath "$($fixture.Local)/callback.txt" -Raw) -match '^PASS prompts=') 'Callback assertions did not complete.'
            if ($scenario -eq 'yes') {
                Assert-Test (-not (Test-Path -LiteralPath $fixture.Root)) 'Confirmed synthetic tree remains.'
            } else {
                Assert-Test ((Snapshot $fixture.Root) -ceq $before) 'Preservation callback changed data.'
            }
            Assert-Test ((Get-FileHash -LiteralPath "$($fixture.Local)/outside.txt").Hash -ceq $outside) 'Callback changed outside bytes.'
        }
    }
}
if ($RunCurrentUserIntegration) {
    $localTest = Join-Path $PSScriptRoot 'Test-LocalInstaller.ps1'
    $integration = Join-Path $work 'currentuser'
    $stateHarness = Join-Path $work 'personal-data-state-test-only.exe'
    Test-Case 'real-currentuser-default-preservation-and-shared-gate' {
        $fixture = New-Data 'currentuser-data'
        $before = Snapshot $fixture.Root
        & $localTest -InstallerPath $InstallerPath -ManifestPath $ManifestPath -WorkDirectory $integration -Scope CurrentUser -Action Install
        try {
            Assert-Test ((Snapshot $fixture.Root) -ceq $before) 'Install changed synthetic sibling data.'
            Invoke-Native $stateHarness @("/DIR=`"$integration/application`"", '/Verified=yes', '/Shared=yes') 'registered-identity'
            Invoke-Native $stateHarness @("/DIR=`"$work/unrelated`"", '/Verified=no', '/Shared=yes') 'different-installation'
            & $localTest -InstallerPath $InstallerPath -ManifestPath $ManifestPath -WorkDirectory $integration -Scope CurrentUser -Action Reinstall
            Assert-Test ((Snapshot $fixture.Root) -ceq $before) 'Repair changed synthetic sibling data.'
        } finally {
            & $localTest -InstallerPath $InstallerPath -ManifestPath $ManifestPath -WorkDirectory $integration -Scope CurrentUser -Action Uninstall
        }
        Invoke-Native $stateHarness @("/DIR=`"$integration/application`"", '/Verified=no', '/Shared=no') 'unregistered-identity'
        Assert-Test ((Get-Content -LiteralPath "$integration/uninstall.log" -Raw) -match 'personal data has been preserved') 'Real uninstaller did not reach preservation event.'
        Assert-Test ((Snapshot $fixture.Root) -ceq $before) 'Uninstall changed synthetic sibling data.'
    }
}
Assert-Test ((Get-FileHash -LiteralPath $personalDataSource -Algorithm SHA256).Hash -ceq $personalDataHash) 'Production cleanup source changed during tests.'
[ordered]@{
    passed=$results.Count; elevated=$elevated; currentUserIntegration=[bool]$RunCurrentUserIntegration
    personalDataSha256=$personalDataHash; architecture='AMD64/PE32+'
    installerSha256=if ($RunCurrentUserIntegration) { $metadata.sha256 } else { $null }
    notes='Synthetic dist fixtures only. Callback scope/account/dialog boundaries are simulated in a private test include; real native core and optional owned CurrentUser lifecycle. No actual personal-data, interactive UI, AllUsers/UAC or clean-host acceptance.'
} | ConvertTo-Json | Set-Content -LiteralPath "$work/summary.json"
Write-Output "PASS: $($results.Count) personal-data safety checks."
