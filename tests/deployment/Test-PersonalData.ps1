#requires -Version 7.2
<# Native deletion uses fresh build/installer-tests/ fixtures only.
The identity probe reads Windows account/profile paths but never opens actual JobTracker Data.
Production has no fixture overrides. #>
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
if (-not $work.StartsWith((Join-Path $repository 'build/installer-tests') + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Fixtures must be below this repository''s build/installer-tests/ directory.'
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
if ($RunCurrentUserIntegration -and (-not $InstallerPath -or -not $ManifestPath)) {
    throw 'Integration requires InstallerPath and ManifestPath.'
}
if ($RunCurrentUserIntegration) {
    . (Join-Path $PSScriptRoot 'OfflineInstallerEvidence.ps1')
    $metadata = Get-OfflineInstallerEvidence -InstallerPath $InstallerPath -MetadataPath $ManifestPath
}
$compiler = (Get-Item -LiteralPath $IsccPath).FullName
$personalDataSource = Join-Path $repository 'Installer/Windows/PersonalData.iss'
$personalDataHash = (Get-FileHash -LiteralPath $personalDataSource -Algorithm SHA256).Hash
New-Item -ItemType Directory -Path $work | Out-Null
$results = [Collections.Generic.List[object]]::new()
$ownershipEvidence = [Collections.Generic.List[object]]::new()
$administrators = [Security.Principal.SecurityIdentifier]::new('S-1-5-32-544')
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
function Record-NotRun([string]$Name, [string]$Reason) {
    $results.Add([ordered]@{ name=$Name; result='NOT_RUN'; reason=$Reason })
    $results | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath "$work/results.json"
    Write-Output "NOT_RUN: $Name -- $Reason"
}
function Assert-FixtureOwner([string]$Path, [Security.Principal.SecurityIdentifier]$Owner, [string]$Case) {
    $actual = (Get-Acl -LiteralPath $Path).GetOwner([Security.Principal.SecurityIdentifier]).Value
    Assert-Test ($actual -ceq $Owner.Value) "Unexpected fixture owner for $Path; expected $($Owner.Value), got $actual."
    $ownershipEvidence.Add([ordered]@{case=$Case;path=$Path;owner=$actual;expected=$Owner.Value})
    $ownershipEvidence | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath "$work/ownership.json"
}
function Set-FixtureOwner([string]$Path, [Security.Principal.SecurityIdentifier]$Owner) {
    # Fixture creation only. The production cleanup never sets ownership or ACLs.
    $acl = Get-Acl -LiteralPath $Path
    if ($acl.GetOwner([Security.Principal.SecurityIdentifier]).Value -cne $Owner.Value) {
        $acl.SetOwner($Owner)
        Set-Acl -LiteralPath $Path -AclObject $acl -ErrorAction Stop
    }
}
function New-Data([string]$Name, [switch]$Absent, [switch]$Empty) {
    $local = Join-Path $work $Name
    $parent = Join-Path $local 'JobTracker'
    $root = Join-Path $parent 'Data'
    $other = Join-Path $work "other-profile-$Name/JobTracker/Data"
    New-Item -ItemType Directory -Path $parent | Out-Null
    if (-not $Absent) {
        New-Item -ItemType Directory -Path $root | Out-Null
        if (-not $Empty) {
            New-Item -ItemType Directory -Path "$root/Resumes" | Out-Null
            Set-Content -LiteralPath "$root/database.sqlite" -Value 'Synthetic database: do not open with SQLite'
            Set-Content -LiteralPath "$root/database.sqlite-wal" -Value 'Synthetic WAL'
            Set-Content -LiteralPath "$root/Resumes/резюме.pdf" -Value 'Synthetic CV'
        }
    }
    Set-Content -LiteralPath "$parent/settings.json" -Value 'Must preserve JobTracker parent siblings'
    Set-Content -LiteralPath "$local/outside.txt" -Value 'Must remain unchanged'
    New-Item -ItemType Directory -Path $other | Out-Null
    Set-Content -LiteralPath "$other/other-user.sqlite" -Value 'Other profile must remain unchanged'
    # Ordinary fixtures intentionally use TOKEN_USER ownership, including an
    # empty Data root. Administrative fixtures change only their named objects
    # afterward and retain that verified ownership throughout cleanup.
    foreach ($path in @($local) + @(Get-ChildItem -LiteralPath $local -Recurse -Force | ForEach-Object FullName)) {
        Set-FixtureOwner $path $identity.User
    }
    if (-not $Absent) { Assert-FixtureOwner $root $identity.User "$Name-before-cleanup" }
    return [pscustomobject]@{ Local=$local; Parent=$parent; Root=$root; OtherRoot=$other }
}
function Snapshot([string]$Root) {
    return @(Get-ChildItem -LiteralPath $Root -File -Recurse | Sort-Object FullName | ForEach-Object {
        $_.FullName.Substring($Root.Length) + ' ' + (Get-FileHash -LiteralPath $_.FullName).Hash
    }) -join "\n"
}
function Assert-Outside($Fixture) {
    Assert-Test ((Get-Content -LiteralPath "$($Fixture.Local)/outside.txt" -Raw).Trim() -ceq 'Must remain unchanged') 'Outside file changed.'
    Assert-Test ((Get-Content -LiteralPath "$($Fixture.Parent)/settings.json" -Raw).Trim() -ceq 'Must preserve JobTracker parent siblings') 'JobTracker sibling changed.'
    Assert-Test ((Get-Content -LiteralPath "$($Fixture.OtherRoot)/other-user.sqlite" -Raw).Trim() -ceq 'Other profile must remain unchanged') 'Other profile changed.'
}
function Invoke-Native([string]$Program, [string[]]$Options, [string]$Name) {
    $arguments = @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/SP-','/LANG=english', "/LOG=""$work/$Name.log""") + $Options
    $process = Start-Process -FilePath $Program -ArgumentList $arguments -WindowStyle Hidden -PassThru -Wait
    $log = Get-Content -LiteralPath "$work/$Name.log" -Raw
    Assert-Test ($log -notmatch '(?m)^\d[^\r\n]*   Message box ') 'Unexpected modal dialog.'
    Assert-Test ($log -notmatch 'TEST FAILURE:') "Native assertion failed: $Name"
    Assert-Test ($process.ExitCode -eq 0) "Native process failed: $Name ($($process.ExitCode))"
}
function Invoke-Cleanup($Fixture, [string]$Name, [string]$Expected = '') {
    Invoke-Native $harness @('/Case=delete', "/LocalData=""$($Fixture.Local)""", "/Root=""$($Fixture.Root)""",
        "/Expected=""$Expected""", "/DIR=""$work/unused-program-$Name""") $Name
}
function Assert-NativeX64([string]$Path) {
    $bytes = [IO.File]::ReadAllBytes($Path)
    $pe = [BitConverter]::ToInt32($bytes, 0x3c)
    Assert-Test ([BitConverter]::ToUInt32($bytes, $pe) -eq 0x4550 -and
        [BitConverter]::ToUInt16($bytes, $pe + 4) -eq 0x8664 -and
        [BitConverter]::ToUInt16($bytes, $pe + 24) -eq 0x20b) "Expected native AMD64/PE32+: $Path"
}
Test-Case 'compile-native-consumers' {
    Set-Content -LiteralPath "$work/program.txt" -Value 'Synthetic installed program'
    foreach ($name in @('PersonalDataHarness','PersonalDataStateHarness')) {
        $output = @(& $compiler "/DTestOutput=$work" (Join-Path $PSScriptRoot "$name.iss") 2>&1)
        $code = $LASTEXITCODE
        $output | Set-Content -LiteralPath "$work/compile-$name.log"
        Assert-Test ($code -eq 0) "Harness compilation failed: $name"
    }
    $policy = Get-Content -LiteralPath $personalDataSource -Raw
    $boundary = @'
procedure TestResolvePersonalDataIdentity(var Identity: TPersonalDataIdentity);
var Needed: Cardinal;
begin
  TestIdentityReads := TestIdentityReads + 1;
  if TestCase = 'identity-error' then RaiseException('Synthetic target-account resolution failure.');
  ClearPersonalDataIdentity(Identity);
  RequirePolicyTest(DataOpenProcessToken(THandle(-1), 8, Identity.Token) <> 0, 'fixture token');
  Needed := 256;
  Identity.SidBuffer := DataAllocate($40, Needed);
  RequirePolicyTest(Identity.SidBuffer <> 0, 'fixture SID memory');
  RequirePolicyTest(DataTokenInformation(Identity.Token, 1, Identity.SidBuffer, Needed, Needed) <> 0, 'fixture SID');
  DataReadPointer(Identity.Sid, Identity.SidBuffer, SizeOf(Identity.Sid));
  Identity.Account := 'fixture-user';
  Identity.ExecutionAccount := 'fixture-execution-account';
  Identity.Profile := TestBase + '\profile';
  Identity.LocalData := TestBase;
  Identity.FallbackToExecutionAccount := Pos('fallback-', TestCase) = 1;
end;

procedure TestDeleteConfirmedPersonalData(const LocalData, Root: String; const UserSid: THandle);
begin
  RequirePolicyTest(TestLastAction = 'consent', 'cleanup begins immediately after explicit deletion confirmation');
  RequirePolicyTest((LocalData = TestBase) and (Root = TestRoot), 'only selected synthetic user Data is targeted');
  TestCleanups := TestCleanups + 1;
  TestLastAction := 'cleanup';
  DeleteConfirmedPersonalData(LocalData, Root, UserSid);
end;

'@
    $anchor = 'procedure PersonalDataAfterUninstall(CurStep: TUninstallStep);'
    Assert-Test (($policy.Split($anchor).Count - 1) -eq 1) 'Review callback-boundary anchor.'
    $policy = $policy.Replace($anchor, $boundary + $anchor)
    $substitutions = [ordered]@{
        'if UninstallSilent then begin' = 'if TestUninstallSilent then begin'
        '        ResolvePersonalDataIdentity(Identity);' = '        TestResolvePersonalDataIdentity(Identity);'
        '        DeleteConfirmedPersonalData(Identity.LocalData, Root, Identity.Sid);' = '        TestDeleteConfirmedPersonalData(Identity.LocalData, Root, Identity.Sid);'
    }
    foreach ($old in $substitutions.Keys) {
        Assert-Test (($policy.Split($old).Count - 1) -eq 1) "Review policy substitution: $old"
        $policy = $policy.Replace($old, $substitutions[$old])
    }
    Assert-Test ([regex]::Matches($policy, '\bSuppressibleMsgBox\b').Count -eq 4) 'Review dialog boundary substitution.'
    $policy = [regex]::Replace($policy, '\bSuppressibleMsgBox\b', 'TestSuppressibleMsgBox')
    $policyPath = Join-Path $work 'PersonalData.test-only.iss'
    Set-Content -LiteralPath $policyPath -Value $policy -Encoding utf8
    [ordered]@{ sourceSha256=$personalDataHash; substitutions=$substitutions; dialogCalls=4
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
Test-Case 'native-Windows-account-profile-and-known-folder' {
    Invoke-Native (Join-Path $work 'personal-data-state-test-only.exe') @(
        "/ExpectedLocalData=""$([Environment]::GetFolderPath('LocalApplicationData'))""",
        "/ExpectedProfile=""$([Environment]::GetFolderPath('UserProfile'))""",
        "/ExpectedSid=$($identity.User.Value)") 'identity'
    Assert-Test ((Get-Content -LiteralPath "$work/identity.txt" -Raw) -ceq 'PASS') 'Identity probe did not finish.'
}
foreach ($language in @('english','russian')) {
    Test-Case "explicit-native-Data-only-delete-$language" {
        $fixture = New-Data "delete-$language"
        Invoke-Native $harness @('/Case=delete', "/LANG=$language", "/LocalData=""$($fixture.Local)""",
            "/Root=""$($fixture.Root)""", "/DIR=""$work/unused-program-$language""") "delete-$language"
        Assert-Test (-not (Test-Path -LiteralPath $fixture.Root)) 'Confirmed Data tree remains.'
        Assert-Outside $fixture
    }
}
foreach ($kind in @('missing','empty')) {
    Test-Case "$kind-Data-is-complete" {
        $fixture = New-Data $kind -Absent:($kind -eq 'missing') -Empty:($kind -eq 'empty')
        Invoke-Cleanup $fixture $kind
        Assert-Test (-not (Test-Path -LiteralPath $fixture.Root)) 'Data remains.'
        Assert-Outside $fixture
    }
}
foreach ($kind in @('root','file','empty-root')) {
    $name = "administrators-owned-$kind"
    $fixture = New-Data $name -Empty:($kind -eq 'empty-root')
    $ownedPath = if ($kind -eq 'file') { Join-Path $fixture.Root 'database.sqlite' } else { $fixture.Root }
    $prepared = $false
    try {
        Set-FixtureOwner $ownedPath $administrators
        Assert-FixtureOwner $ownedPath $administrators "$name-before-cleanup"
        $prepared = $true
    } catch {
        Record-NotRun "$name-delete-when-permitted" ("The current Windows token cannot prepare and verify Administrators ownership. Run this standalone suite with an elevated test token; no cleanup was attempted for this fixture. " + $_.Exception.Message)
    }
    if ($prepared) {
        Test-Case "$name-delete-when-permitted" {
            if ($kind -eq 'file') { Assert-FixtureOwner $fixture.Root $identity.User "$name-user-owned-root" }
            # No ownership normalization or ACL changes occur after this check.
            Assert-FixtureOwner $ownedPath $administrators "$name-immediately-before-native-call"
            Invoke-Cleanup $fixture $name
            Assert-Test (-not (Test-Path -LiteralPath $fixture.Root)) 'Administrators-owned Data was rejected despite fixture deletion permissions.'
            Assert-Outside $fixture
        }
    }
}
Test-Case 'locked-file-reports-partial-cleanup' {
    $fixture = New-Data 'locked'
    $lock = [IO.File]::Open("$($fixture.Root)/database.sqlite", 'Open', 'Read', 'None')
    try { Invoke-Cleanup $fixture 'locked' 'Windows error 32' } finally { $lock.Dispose() }
    Assert-Test (Test-Path -LiteralPath "$($fixture.Root)/database.sqlite") 'Locked data disappeared.'
    Assert-Test (-not (Test-Path -LiteralPath "$($fixture.Root)/database.sqlite-wal")) 'Unlocked sibling was not attempted.'
    Assert-Outside $fixture
}
Test-Case 'missing-confirmed-account-identity-is-rejected' {
    $fixture = New-Data 'invalid-account'
    $before = Snapshot $fixture.Root
    Invoke-Native $harness @('/Case=invalid-account', "/LocalData=""$($fixture.Local)""", "/Root=""$($fixture.Root)""",
        "/DIR=""$work/invalid-account-program""") 'invalid-account'
    Assert-Test ((Snapshot $fixture.Root) -ceq $before) 'Missing confirmed account identity changed data.'
    Assert-Outside $fixture
}
Test-Case 'another-profile-is-outside-confirmed-LocalData' {
    $fixture = New-Data 'other-profile-boundary'
    $before = Snapshot $fixture.Root
    Invoke-Cleanup ([pscustomobject]@{Local=$fixture.Local;Root=$fixture.OtherRoot}) 'other-profile-boundary' 'Unexpected personal-data root'
    Assert-Test ((Snapshot $fixture.Root) -ceq $before) 'Other-profile target rejection changed selected-user data.'
    Assert-Outside $fixture
}
Test-Case 'readonly-file-reports-partial-cleanup' {
    $fixture = New-Data 'readonly'
    $file = Get-Item -LiteralPath "$($fixture.Root)/Resumes/резюме.pdf"
    $file.IsReadOnly = $true
    Invoke-Cleanup $fixture 'readonly' 'Windows error 5'
    Assert-Test (Test-Path -LiteralPath $file.FullName) 'Read-only data disappeared.'
    Assert-Test (-not (Test-Path -LiteralPath "$($fixture.Root)/database.sqlite")) 'Unlocked sibling was not attempted.'
    Assert-Outside $fixture
}
Test-Case 'access-denied-file-reports-partial-cleanup' {
    $fixture = New-Data 'access-denied'
    $file = Join-Path $fixture.Root 'Resumes/резюме.pdf'
    $deniedAcl = Get-Acl -LiteralPath $file
    $parentPath = [IO.Path]::GetDirectoryName($file)
    $deniedParentAcl = Get-Acl -LiteralPath $parentPath
    $rule = [Security.AccessControl.FileSystemAccessRule]::new($identity.User,
        ([Security.AccessControl.FileSystemRights]::Delete -bor [Security.AccessControl.FileSystemRights]::ReadAttributes),
        [Security.AccessControl.AccessControlType]::Deny)
    $deniedAcl.AddAccessRule($rule)
    $deniedParentAcl.AddAccessRule([Security.AccessControl.FileSystemAccessRule]::new($identity.User,
        [Security.AccessControl.FileSystemRights]::DeleteSubdirectoriesAndFiles,
        [Security.AccessControl.AccessControlType]::Deny))
    Set-Acl -LiteralPath $parentPath -AclObject $deniedParentAcl
    Set-Acl -LiteralPath $file -AclObject $deniedAcl
    $deniedFileDescriptor = (Get-Acl -LiteralPath $file).Sddl
    $deniedParentDescriptor = (Get-Acl -LiteralPath $parentPath).Sddl
    Invoke-Cleanup $fixture 'access-denied' 'Windows error 5'
    Assert-Test (Test-Path -LiteralPath $file) 'Denied file disappeared.'
    Assert-Test ((Get-Acl -LiteralPath $file).Sddl -ceq $deniedFileDescriptor -and
        (Get-Acl -LiteralPath $parentPath).Sddl -ceq $deniedParentDescriptor) 'Cleanup changed the denied fixture permissions.'
    Assert-Test (-not (Test-Path -LiteralPath "$($fixture.Root)/database.sqlite")) 'Unlocked sibling was not attempted.'
    Assert-Outside $fixture
}
Test-Case 'hardlink-is-preserved-with-outside-bytes' {
    $fixture = New-Data 'hardlink'
    New-Item -ItemType HardLink -Path "$($fixture.Root)/linked.txt" -Target "$($fixture.Local)/outside.txt" | Out-Null
    Invoke-Cleanup $fixture 'hardlink' 'Hard-linked personal data'
    Assert-Test (Test-Path -LiteralPath "$($fixture.Root)/linked.txt") 'Hardlink removed.'
    Assert-Outside $fixture
}
foreach ($position in @('root','descendant','ancestor')) {
    Test-Case "junction-$position" {
        $fixture = New-Data "junction-$position"
        $outside = Join-Path $fixture.Local 'outside-directory'
        New-Item -ItemType Directory -Path $outside | Out-Null
        Set-Content -LiteralPath "$outside/sentinel.txt" -Value 'Outside junction target'
        $before = Snapshot $fixture.Root
        if ($position -eq 'root') {
            $candidate = New-Data 'root-alias' -Absent
            New-Item -ItemType Junction -Path $candidate.Root -Target $outside | Out-Null
        } elseif ($position -eq 'ancestor') {
            $alias = Join-Path $work 'ancestor-alias'
            New-Item -ItemType Junction -Path $alias -Target $fixture.Local | Out-Null
            $candidate = [pscustomobject]@{ Local=$alias; Root="$alias\JobTracker\Data" }
        } else {
            New-Item -ItemType Junction -Path "$($fixture.Root)/redirect" -Target $outside | Out-Null
            $candidate = $fixture
        }
        $expected = if ($position -eq 'ancestor') { 'destination' } else { 'Reparse point' }
        Invoke-Cleanup $candidate "junction-$position" $expected
        Assert-Test ((Get-Content -LiteralPath "$outside/sentinel.txt" -Raw).Trim() -ceq 'Outside junction target') 'Junction target changed.'
        if ($position -ne 'descendant') { Assert-Test ((Snapshot $fixture.Root) -ceq $before) 'Root/ancestor refusal changed original data.' }
        Assert-Outside $fixture
    }
}
foreach ($kind in @('broad-parent','jobtracker-parent','wrong-leaf','dotdot','trailing-dot')) {
    Test-Case "unsafe-$kind" {
        $fixture = New-Data "unsafe-$kind"
        $before = Snapshot $fixture.Root
        $root = switch ($kind) {
            'broad-parent' { $fixture.Local }
            'jobtracker-parent' { $fixture.Parent }
            'wrong-leaf' { "$($fixture.Local)\Unrelated" }
            'dotdot' { "$($fixture.Root)\.." }
            'trailing-dot' { "$($fixture.Root)." }
        }
        Invoke-Cleanup ([pscustomobject]@{ Local=$fixture.Local; Root=$root }) "unsafe-$kind" 'Unexpected personal-data root'
        Assert-Test ((Snapshot $fixture.Root) -ceq $before) 'Unsafe-root refusal changed data.'
    }
}
foreach ($partial in @($false, $true)) {
    $scenario = if ($partial) { 'partial' } else { 'complete' }
    Test-Case "native-silent-uninstall-$scenario-preserves-data" {
        $fixture = New-Data "lifecycle-$scenario"
        $before = Snapshot $fixture.Root
        $program = Join-Path $work "lifecycle-program-$scenario"
        Invoke-Native $harness @('/Case=lifecycle', "/DIR=""$program""") "install-$scenario"
        $lock = $null
        if ($partial) { $lock = [IO.File]::Open("$program/bin/program.txt", 'Open', 'Read', 'None') }
        try {
            Invoke-Native "$program/unins000.exe" @("/Expected=$scenario", "/Result=""$work/uninstall-$scenario.txt""",
                '/DELETEPERSONALDATA=yes') "uninstall-$scenario"
        } finally { if ($null -ne $lock) { $lock.Dispose() } }
        Assert-Test ((Get-Content -LiteralPath "$work/uninstall-$scenario.txt" -Raw) -ceq 'PASS') 'Lifecycle assertion did not execute.'
        Assert-Test ((Snapshot $fixture.Root) -ceq $before) 'Silent uninstall changed personal-data fixture.'
        Assert-Test ((Get-Content -LiteralPath "$work/uninstall-$scenario.log" -Raw) -match 'personal data has been preserved') 'Silent preservation was not reached.'
    }
}
foreach ($language in @('english','russian')) {
    foreach ($scenario in @('no','close','yes','silent','allusers','elevated','running-copy','remaining-copy',
            'incomplete-program','disconnected-copy','alternate-account','identity-error',
            'fallback-no','fallback-close','fallback-yes','fallback-keep','pre-uninstall','repeat-callback','locked-data','absent')) {
        Test-Case "callback-$language-$scenario" {
            $fixture = New-Data "callback-$language-$scenario" -Absent:($scenario -eq 'absent')
            $before = Snapshot $fixture.Parent
            $lock = $null
            if ($scenario -eq 'locked-data') { $lock = [IO.File]::Open("$($fixture.Root)/database.sqlite", 'Open', 'Read', 'None') }
            try {
                Invoke-Native (Join-Path $work 'personal-data-policy-test-only.exe') @("/LANG=$language", "/Case=$scenario") "callback-$language-$scenario"
            } finally { if ($null -ne $lock) { $lock.Dispose() } }
            Assert-Test ((Get-Content -LiteralPath "$($fixture.Local)/callback.txt" -Raw) -match '^PASS prompts=') 'Callback did not complete.'
            if ($scenario -in @('no','close','silent','identity-error','fallback-no','fallback-close','fallback-keep','pre-uninstall')) {
                Assert-Test ((Snapshot $fixture.Parent) -ceq $before) 'Preservation callback changed data.'
            }
            Assert-Outside $fixture
        }
    }
}
if ($RunCurrentUserIntegration) {
    $localTest = Join-Path $PSScriptRoot 'Test-LocalInstaller.ps1'
    $integration = Join-Path $work 'currentuser'
    Test-Case 'real-currentuser-silent-preservation' {
        $fixture = New-Data 'currentuser-data'
        $before = Snapshot $fixture.Root
        foreach ($action in @('Install','Reinstall','Uninstall')) {
            & $localTest -InstallerPath $InstallerPath -ManifestPath $ManifestPath -WorkDirectory $integration -Scope CurrentUser -Action $action
        }
        Assert-Test ((Get-Content -LiteralPath "$integration/uninstall.log" -Raw) -match 'personal data has been preserved') 'Real uninstaller did not reach silent preservation.'
        Assert-Test ((Snapshot $fixture.Root) -ceq $before) 'Lifecycle changed synthetic sibling data.'
    }
}
Assert-Test ((Get-FileHash -LiteralPath $personalDataSource -Algorithm SHA256).Hash -ceq $personalDataHash) 'Production cleanup source changed during tests.'
[ordered]@{
    passed=@($results | Where-Object result -eq 'PASS').Count
    notRun=@($results | Where-Object result -eq 'NOT_RUN')
    elevated=$elevated; currentUserIntegration=[bool]$RunCurrentUserIntegration
    personalDataSha256=$personalDataHash; architecture='AMD64/PE32+'
    installerSha256=if ($RunCurrentUserIntegration) { $metadata.sha256 } else { $null }
    notes='Synthetic fixtures only. Native read-only Windows identity, deletion, locked-file, ACL, and reparse checks. Callback identity/dialog boundaries simulated; actual UAC credential switching and interactive UI remain manual acceptance.'
} | ConvertTo-Json | Set-Content -LiteralPath "$work/summary.json"
Write-Output "PASS: $(@($results | Where-Object result -eq 'PASS').Count) personal-data checks; NOT_RUN: $(@($results | Where-Object result -eq 'NOT_RUN').Count)."
