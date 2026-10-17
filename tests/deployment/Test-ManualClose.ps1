#requires -Version 7.2
<#
Standalone native manual-close tests. Real processes are synthetic .NET Framework
executables that exit cooperatively; JobTracker/Qt and personal data are never
opened. Optional CurrentUser integration uses the existing owned fixture helper.
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
$metadata = Get-OfflineInstallerEvidence -InstallerPath $installer -MetadataPath $ManifestPath
$compiler = (Get-Item -LiteralPath $IsccPath).FullName
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$elevated = ([Security.Principal.WindowsPrincipal]::new($identity)).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if ($RunCurrentUserIntegration -and $elevated) { throw 'CurrentUser checks must run without OS elevation.' }
New-Item -ItemType Directory -Path $work | Out-Null
$results = [Collections.Generic.List[object]]::new()
$activeFixtures = [Collections.Generic.List[object]]::new()
$harness = Join-Path $work 'manual-close-test-only.exe'
$fixtureExe = Join-Path $work 'JobTrackerApp.exe'

function Assert-Test([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}
function Test-Case([string]$Name, [scriptblock]$Body) {
    try {
        & $Body
        $results.Add([ordered]@{ name=$Name; result='PASS' })
        Write-Output "PASS: $Name"
    } catch {
        $results.Add([ordered]@{ name=$Name; result='FAIL'; error=$_.Exception.Message })
        throw
    } finally {
        $results | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath "$work/results.json" -Encoding utf8
    }
}
function Wait-File([string]$Path) {
    $timer = [Diagnostics.Stopwatch]::StartNew()
    while (-not (Test-Path -LiteralPath $Path)) {
        if ($timer.Elapsed.TotalSeconds -gt 30) { throw "Timed out waiting for $Path" }
        Start-Sleep -Milliseconds 50
    }
}
function Start-Fixture([string]$Executable, [string]$Name) {
    $ready = Join-Path $work "$Name-ready.txt"
    $stop = Join-Path $work "$Name-stop.txt"
    $process = Start-Process -FilePath $Executable -ArgumentList @("`"$ready`"", "`"$stop`"") -WindowStyle Hidden -PassThru
    $fixture = [pscustomobject]@{ process=$process; stop=$stop; executable=$Executable }
    $activeFixtures.Add($fixture)
    Wait-File $ready
    Assert-Test (-not $process.HasExited) 'Fixture exited before verification.'
    return $fixture
}
function Close-Fixture($Fixture) {
    Set-Content -LiteralPath $Fixture.stop -Value 'close normally'
    Assert-Test ($Fixture.process.WaitForExit(10000)) 'Fixture failed to close cooperatively; it is not force-killed.'
    Assert-Test ($Fixture.process.ExitCode -eq 0) 'Fixture exited abnormally.'
}
function Invoke-Setup([string]$Program, [string[]]$Arguments, [string]$Name, [switch]$Unsuppressed) {
    $options = @('/VERYSILENT', '/NORESTART', '/SP-', "/LOG=`"$work/$Name.log`"")
    if (-not $Unsuppressed) { $options += '/SUPPRESSMSGBOXES' }
    $process = Start-Process -FilePath $Program -ArgumentList ($options + $Arguments) -WindowStyle Hidden -PassThru -Wait
    $log = Get-Content -LiteralPath "$work/$Name.log" -Raw
    Assert-Test ($log -notmatch '(?m)^\d[^\r\n]*   Message box ') 'A native modal dialog invalidated this unattended check.'
    return $process.ExitCode
}
function Invoke-Probe([string]$Name, [string]$Path, [bool]$Busy) {
    $expected = if ($Busy) { 'yes' } else { 'no' }
    $code = Invoke-Setup $harness @('/Case=probe', "/Path=`"$Path`"", "/Busy=$expected", "/DIR=`"$work/probe-output`"") $Name
    Assert-Test ($code -eq 0) "Probe failed: $Name ($code)"
}
function Snapshot-Tree([string]$Directory) {
    return @((Get-ChildItem -LiteralPath $Directory -File -Recurse | Sort-Object FullName | ForEach-Object {
        $_.FullName.Substring($Directory.Length) + ' ' + (Get-FileHash -LiteralPath $_.FullName).Hash
    })) -join "`n"
}

try {
    Test-Case 'compile-test-consumers' {
        $versionFile = Join-Path $work 'AssemblyVersion.cs'
        '[assembly:System.Reflection.AssemblyFileVersion("' + $metadata.version + '.0")]' |
            Set-Content -LiteralPath $versionFile -Encoding utf8
        $csc = Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
        $fixtureSource = Join-Path $PSScriptRoot 'RunningApplicationFixture.cs'
        $output = @(& $csc /nologo /target:exe /platform:x64 "/out:$fixtureExe" $versionFile $fixtureSource 2>&1)
        $code = $LASTEXITCODE
        $output | Set-Content -LiteralPath "$work/compile-fixture.log" -Encoding utf8
        Assert-Test ($code -eq 0) 'Fixture compilation failed.'
        Set-Content -LiteralPath "$work/payload.txt" -Value 'installed fixture payload'
        $output = @(& $compiler "/DTestOutput=$work" "/DTestPayload=$work/payload.txt" "$PSScriptRoot/ManualCloseHarness.iss" 2>&1)
        $code = $LASTEXITCODE
        $output | Set-Content -LiteralPath "$work/compile-harness.log" -Encoding utf8
        Assert-Test ($code -eq 0) 'Manual-close harness compilation failed.'
    }
    Test-Case 'closed-and-absent-files' {
        Invoke-Probe 'closed' $fixtureExe $false
        Invoke-Probe 'absent' "$work/absent/JobTrackerApp.exe" $false
    }
    $running = Start-Fixture $fixtureExe 'main'
    Test-Case 'running-image-detected-without-termination' {
        Invoke-Probe 'running' $fixtureExe $true
        Assert-Test (-not $running.process.HasExited) 'Gate closed the running process.'
    }
    Test-Case 'same-filename-other-path-is-unaffected' {
        New-Item -ItemType Directory -Path "$work/other" | Out-Null
        Copy-Item -LiteralPath $fixtureExe -Destination "$work/other/JobTrackerApp.exe"
        Invoke-Probe 'unrelated' "$work/other/JobTrackerApp.exe" $false
        Assert-Test (-not $running.process.HasExited) 'Unrelated process was closed.'
    }
    foreach ($language in @('english','russian')) {
        Test-Case "cancel-preserves-running-file-$language" {
            $before = (Get-FileHash -LiteralPath $fixtureExe).Hash
            $code = Invoke-Setup $harness @('/Case=wait', "/LANG=$language", "/Path=`"$fixtureExe`"", "/DIR=`"$work/cancel-$language`"") "cancel-$language"
            Assert-Test ($code -ne 0) 'Silent running-file check did not cancel.'
            Assert-Test (-not (Test-Path -LiteralPath "$work/cancel-$language/payload.txt")) 'Cancellation installed files.'
            Assert-Test ((Get-FileHash -LiteralPath $fixtureExe).Hash -ceq $before) 'Cancellation changed the executable.'
            Assert-Test (-not $running.process.HasExited) 'Cancellation terminated the process.'
            $log = Get-Content -LiteralPath "$work/cancel-$language.log" -Raw
            $expected = if ($language -eq 'russian') { 'закройте эту копию JobTracker' } else { 'close this copy of JobTracker' }
            Assert-Test ($log -match $expected) 'Localized manual-close message missing.'
        }
    }
    Close-Fixture $running
    Test-Case 'retry-after-cooperative-close' {
        Invoke-Probe 'closed-after-running' $fixtureExe $false
        $code = Invoke-Setup $harness @('/Case=wait', "/Path=`"$fixtureExe`"", "/DIR=`"$work/retry`"") 'retry'
        Assert-Test ($code -eq 0 -and (Test-Path -LiteralPath "$work/retry/payload.txt")) 'Retry after closing failed.'
    }
    Test-Case 'unverifiable-readonly-file-fails-closed' {
        $file = "$work/readonly.exe"
        Copy-Item -LiteralPath $fixtureExe -Destination $file
        (Get-Item -LiteralPath $file).IsReadOnly = $true
        try { Invoke-Probe 'readonly' $file $true }
        finally { (Get-Item -LiteralPath $file).IsReadOnly = $false }
    }
    foreach ($case in @('late-install','late-file','late-uninstall')) {
        Test-Case "$case-rechecks-after-preflight" {
            $target = Join-Path $work $case
            New-Item -ItemType Directory -Path "$target/bin" | Out-Null
            Copy-Item -LiteralPath $fixtureExe -Destination "$target/bin/JobTrackerApp.exe"
            Set-Content -LiteralPath "$target/payload.txt" -Value 'preserve before copy'
            Set-Content -LiteralPath "$target/later-payload.txt" -Value 'preserve subsequent copy'
            $before = Snapshot-Tree $target
            $ready = "$work/$case-boundary.txt"
            $continue = "$work/$case-continue.txt"
            $arguments = @('/VERYSILENT','/NORESTART','/SP-', "/Case=$case", "/DIR=`"$target`"", "/Ready=`"$ready`"", "/Continue=`"$continue`"", "/LOG=`"$work/$case.log`"")
            if ($case -eq 'late-uninstall') { $arguments += '/SUPPRESSMSGBOXES' }
            # Start-Job waits for the entire loader process tree, not only its stub.
            $job = Start-Job -ScriptBlock {
                param($program, $options)
                (Start-Process -FilePath $program -ArgumentList $options -WindowStyle Hidden -PassThru -Wait).ExitCode
            } -ArgumentList $harness, $arguments
            try {
                Wait-File $ready
                $late = Start-Fixture "$target/bin/JobTrackerApp.exe" $case
                Set-Content -LiteralPath $continue -Value 'continue'
                $null = Wait-Job $job -Timeout 45
                Assert-Test ($job.State -eq 'Completed') 'Boundary test did not finish; process tree is not killed.'
                $code = Receive-Job $job -ErrorAction Stop
                Assert-Test ($code -ne 0) 'Late-started application did not stop the operation.'
                if ($case -eq 'late-file') {
                    $log = Get-Content -LiteralPath "$work/$case.log" -Raw
                    Assert-Test ($code -eq 5 -and $log -match 'Manual-close native cancellation acknowledged' -and
                        $log -match 'Rolling back changes') 'Late copy did not enter native cancellation/rollback.'
                    Assert-Test ($log -notmatch '(?m)^\d[^\r\n]*   Message box ') 'Late copy opened a native modal dialog.'
                }
                Assert-Test ((Snapshot-Tree $target) -ceq $before) 'Boundary abort changed fixture bytes.'
                Assert-Test (-not $late.process.HasExited) 'Boundary abort closed the process.'
                Close-Fixture $late
            } finally {
                if ($job.State -eq 'Completed') { Remove-Job $job }
            }
        }
    }
    foreach ($option in @('/CLOSEAPPLICATIONS','/FORCECLOSEAPPLICATIONS','/RESTARTAPPLICATIONS','/NOCANCEL','/LOADINF=')) {
        Test-Case "reject-automatic-close-option-$($option.Trim('/='))" {
            $argument = $option
            if ($option -eq '/LOADINF=') {
                Set-Content -LiteralPath "$work/unsafe.inf" -Value "[Setup]`r`nCloseApplications=yes`r`nForceCloseApplications=yes`r`nRestartApplications=yes"
                $argument += "`"$work/unsafe.inf`""
            }
            $name = 'option-' + $option.Trim('/=')
            $code = Invoke-Setup $installer @('/CURRENTUSER', '/LANG=english', $argument, "/DIR=`"$work/options`"") $name -Unsuppressed
            Assert-Test ($code -ne 0 -and (Get-Content -LiteralPath "$work/$name.log" -Raw) -match 'not supported by this installer') 'Unsafe option was not rejected.'
            Assert-Test (-not (Test-Path -LiteralPath "$work/options")) 'Rejected option wrote application files.'
        }
    }
    if ($RunCurrentUserIntegration) {
        $localTest = Join-Path $PSScriptRoot 'Test-LocalInstaller.ps1'
        $realWork = Join-Path $work 'currentuser'
        Test-Case 'real-currentuser-install' {
            & $localTest -InstallerPath $installer -ManifestPath $ManifestPath -WorkDirectory $realWork -Scope CurrentUser -Action Install
        }
        $target = Join-Path $realWork 'application'
        $exe = Join-Path $target 'bin/JobTrackerApp.exe'
        $backup = Join-Path $realWork 'original-JobTrackerApp.exe'
        Copy-Item -LiteralPath $exe -Destination $backup
        try {
            Copy-Item -LiteralPath $fixtureExe -Destination $exe -Force
            $realRunning = Start-Fixture $exe 'real'
            $before = Snapshot-Tree $target
            foreach ($operation in @('reinstall','uninstall')) {
                Test-Case "real-running-$operation-cancels-before-changes" {
                    $program = if ($operation -eq 'reinstall') { $installer } else { Join-Path $target 'unins000.exe' }
                    $code = Invoke-Setup $program @('/CURRENTUSER','/LANG=english') "real-$operation" -Unsuppressed:($operation -eq 'uninstall')
                    Assert-Test ($code -ne 0) 'Running-image operation succeeded unexpectedly.'
                    $log = Get-Content -LiteralPath "$work/real-$operation.log" -Raw
                    Assert-Test ($log -match ('Manual-close check blocked: ' + [regex]::Escape($exe) + ' \(Windows error (32|33|1224)\)') -and
                        $log -match 'Operation cancelled: the installation files could not be confirmed available') 'Operation was not rejected by the exact-file manual-close gate.'
                    Assert-Test ((Snapshot-Tree $target) -ceq $before) 'Cancellation changed installed bytes.'
                    Assert-Test (-not $realRunning.process.HasExited) 'Installer/uninstaller closed the application.'
                    $key = Get-ItemProperty -LiteralPath 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\JobTracker.CurrentUser_is1'
                    Assert-Test ($key.InstallLocation.TrimEnd('\') -ieq $target) 'Cancellation changed registration.'
                }
            }
            Close-Fixture $realRunning
        } finally {
            if ($null -ne (Get-Variable realRunning -ErrorAction SilentlyContinue) -and -not $realRunning.process.HasExited) { Close-Fixture $realRunning }
            Copy-Item -LiteralPath $backup -Destination $exe -Force
        }
        $otherRunning = Start-Fixture $fixtureExe 'other-during-real'
        try {
            Test-Case 'real-reinstall-allows-unrelated-copy' {
                & $localTest -InstallerPath $installer -ManifestPath $ManifestPath -WorkDirectory $realWork -Scope CurrentUser -Action Reinstall
                Assert-Test (-not $otherRunning.process.HasExited) 'Unrelated copy was terminated.'
            }
            Test-Case 'real-uninstall-after-close-preserves-sentinels' {
                & $localTest -InstallerPath $installer -ManifestPath $ManifestPath -WorkDirectory $realWork -Scope CurrentUser -Action Uninstall
                Assert-Test (-not $otherRunning.process.HasExited) 'Uninstall closed an unrelated copy.'
            }
        } finally { Close-Fixture $otherRunning }
    }
    [ordered]@{ result='PASS'; checks=$results.Count; currentUserIntegration=[bool]$RunCurrentUserIntegration;
        elevated=$elevated; installerSha256=$metadata.sha256; manualCloseSha256=$metadata.sourceHashes.'ManualClose.iss' } |
        ConvertTo-Json | Set-Content -LiteralPath "$work/summary.json" -Encoding utf8
    Write-Output "PASS: $($results.Count) manual-close checks."
} finally {
    foreach ($fixture in $activeFixtures) {
        if (-not $fixture.process.HasExited) { Close-Fixture $fixture }
    }
}
