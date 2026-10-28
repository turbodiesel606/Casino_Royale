#requires -Version 7.2
<#
Run production installer/uninstaller logic against small synthetic payloads.
Synthetic mapped executables exit cooperatively. Real JobTracker and personal
data are never opened. Filesystem fixtures stay beneath build/installer-tests/.
Shortcuts are removed by their owning uninstaller, except one exact test-owned
retargeted link whose preservation is verified before test cleanup.
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
    throw 'Operations fixtures must be below this repository''s build/installer-tests/ directory.'
}
if (Test-Path -LiteralPath $work) { throw 'Choose a fresh test output directory.' }
. (Join-Path $PSScriptRoot 'OfflineInstallerEvidence.ps1')
Assert-NoReparsePoint $work
$installer = (Get-Item -LiteralPath $InstallerPath).FullName
$metadata = Get-OfflineInstallerEvidence -InstallerPath $installer -MetadataPath $ManifestPath
$compiler = (Get-Item -LiteralPath $IsccPath).FullName
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$elevated = ([Security.Principal.WindowsPrincipal]::new($identity)).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if ($elevated) { throw 'Operations fixtures intentionally use unelevated current-user installation only.' }
New-Item -ItemType Directory -Path $work | Out-Null
$results = [Collections.Generic.List[object]]::new()
$activeFixtures = [Collections.Generic.List[object]]::new()
$candidates = @{}
$payloads = @{}
$sourceDirectory = Join-Path $repository 'Installer/Windows'

function Assert-Test([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}
function Assert-FixturePath([string]$Path) {
    $resolved = [IO.Path]::GetFullPath($Path)
    Assert-Test ($resolved.StartsWith($work + '\', [StringComparison]::OrdinalIgnoreCase)) "Path escaped test fixtures: $Path"
    Assert-NoReparsePoint $resolved
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
        $results | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $work 'results.json') -Encoding utf8
    }
}
function Start-Fixture([string]$Executable, [string]$Name) {
    Assert-FixturePath $Executable
    $ready = Join-Path $work "$Name-ready.txt"
    $stop = Join-Path $work "$Name-stop.txt"
    $process = Start-Process -FilePath $Executable -ArgumentList @("`"$ready`"", "`"$stop`"") -WindowStyle Hidden -PassThru
    $fixture = [pscustomobject]@{process=$process;stop=$stop;executable=$Executable}
    $activeFixtures.Add($fixture)
    $timer = [Diagnostics.Stopwatch]::StartNew()
    while (-not (Test-Path -LiteralPath $ready)) {
        if ($timer.Elapsed.TotalSeconds -gt 30) { throw "Timed out waiting for synthetic process readiness: $ready" }
        Start-Sleep -Milliseconds 50
    }
    Assert-Test (-not $process.HasExited) 'Synthetic process exited before verification.'
    return $fixture
}
function Close-Fixture($Fixture) {
    if (-not $Fixture.process.HasExited) {
        Set-Content -LiteralPath $Fixture.stop -Value 'close normally'
        Assert-Test ($Fixture.process.WaitForExit(10000)) 'Synthetic application did not close cooperatively; it was not terminated.'
    }
    Assert-Test ($Fixture.process.ExitCode -eq 0) 'Synthetic application exited abnormally.'
}
function Read-Marker([string]$Directory) {
    $values = @{}
    $section = ''
    foreach ($line in Get-Content -LiteralPath (Join-Path $Directory 'JobTracker.install.ini')) {
        if ($line -match '^\[([^\]]+)\]$') { $section = $Matches[1] }
        elseif ($line -match '^([^=]+)=(.*)$') { $values["$section/$($Matches[1])"] = $Matches[2] }
    }
    return $values
}
function Snapshot-Tree([string]$Directory) {
    Assert-FixturePath $Directory
    return @((Get-ChildItem -LiteralPath $Directory -File -Recurse | Sort-Object FullName | ForEach-Object {
        $_.FullName.Substring($Directory.Length) + ' ' + (Get-FileHash -LiteralPath $_.FullName).Hash
    })) -join "`n"
}
function Invoke-Operation([string]$Program, [string[]]$Arguments, [string]$Name) {
    $log = Join-Path $work "$Name.log"
    $options = @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/SP-', '/LANG=english', "/LOG=`"$log`"") + $Arguments
    $process = Start-Process -FilePath $Program -ArgumentList $options -WindowStyle Hidden -PassThru -Wait
    $content = Get-Content -LiteralPath $log -Raw
    Assert-Test ($content -notmatch 'Manual-close|RequireClosedJobTracker|close this copy of JobTracker manually') 'Discarded preliminary application-close policy ran.'
    return [pscustomobject]@{exit=$process.ExitCode;log=$content;path=$log}
}
function Install-Copy([string]$Directory, [string]$Version, [string]$Name, [switch]$WithDesktopShortcut) {
    Assert-FixturePath $Directory
    $arguments = @('/CURRENTUSER', "/DIR=`"$Directory`"", $(if ($WithDesktopShortcut) { '/TASKS=desktopicon' } else { '/TASKS=' }))
    $operation = Invoke-Operation $candidates[$Version] $arguments $Name
    Assert-Test ($operation.exit -eq 0) "Synthetic setup failed ($($operation.exit)); see $($operation.path)"
    $local = Read-Marker $Directory
    Assert-Test ($local['Installation/Path'] -ieq $Directory -and $local['Installation/Version'] -ceq $Version) 'Synthetic installation marker mismatch.'
    Assert-Test (Test-Path -LiteralPath $local['Installation/Uninstaller']) 'Synthetic installation has no local uninstaller.'
    foreach ($file in $payloads[$Version]) {
        Assert-Test ((Get-FileHash -LiteralPath (Join-Path $Directory $file.path)).Hash -ieq $file.sha256) "Synthetic payload mismatch: $($file.path)"
    }
    return $local
}
function Uninstall-Copy([string]$Directory, [string]$Name, [bool]$ExpectSuccess = $true) {
    Assert-FixturePath $Directory
    $local = Read-Marker $Directory
    $program = [string]$local['Installation/Uninstaller']
    Assert-Test ([IO.Path]::GetDirectoryName($program) -ieq $Directory) 'Refusing to invoke a foreign uninstaller from metadata.'
    $ownedLinks = @(foreach ($kind in @('StartMenu', 'Desktop')) {
        $path = [string]$local["Shortcuts/$kind"]
        if ($path -and (Test-Path -LiteralPath $path) -and
            (Get-InstallerShortcutInfo $path).Target -ieq (Join-Path $Directory 'bin/JobTrackerApp.exe')) { $path }
    })
    $operation = Invoke-Operation $program @() $Name
    Assert-Test (($operation.exit -eq 0) -eq $ExpectSuccess) "Unexpected uninstall exit $($operation.exit); see $($operation.path)"
    Assert-Test ($operation.log -notmatch 'Personal-data cleanup confirmed|Personal-data cleanup complete|Data cleanup started') 'Silent uninstaller attempted data cleanup.'
    if ($ExpectSuccess) {
        Assert-Test (-not (Test-Path -LiteralPath (Join-Path $Directory 'bin/JobTrackerApp.exe'))) 'Owned executable remains after successful uninstall.'
        foreach ($path in $ownedLinks) { Assert-Test (-not (Test-Path -LiteralPath $path)) "Owned shortcut remains after successful uninstall: $path" }
    } else {
        Assert-Test ($operation.log -match 'Program removal incomplete' -and (Test-Path -LiteralPath $program)) 'Incomplete removal was not reported or retry uninstaller was lost.'
        Assert-Test (Test-Path -LiteralPath (Join-Path $Directory 'JobTracker.ownership.ini')) 'Incomplete removal lost its ownership ledger.'
    }
    return $operation
}
function Assert-LinkTarget([string]$Path, [string]$Target) {
    Assert-Test (Test-Path -LiteralPath $Path) "Shortcut missing: $Path"
    $link = Get-InstallerShortcutInfo $Path
    Assert-Test ($link.Target -ieq $Target) "Shortcut target changed: $Path"
}

try {
    Test-Case 'compile-production-lifecycle-consumers' {
        $csc = Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
        foreach ($version in @('1.0.0', '2.0.0')) {
            $directory = Join-Path $work "input-$version"
            New-Item -ItemType Directory -Path $directory | Out-Null
            $fixtureExe = Join-Path $directory 'JobTrackerApp.exe'
            $versionFile = Join-Path $directory 'AssemblyVersion.cs'
            @"
[assembly:System.Reflection.AssemblyFileVersion("$version.0")]
[assembly:System.Reflection.AssemblyProduct("JobTracker")]
"@ | Set-Content -LiteralPath $versionFile -Encoding utf8
            $output = @(& $csc /nologo /target:exe /platform:x64 "/out:$fixtureExe" $versionFile (Join-Path $PSScriptRoot 'RunningApplicationFixture.cs') 2>&1)
            $code = $LASTEXITCODE
            $output | Set-Content -LiteralPath (Join-Path $directory 'compile-fixture.log') -Encoding utf8
            Assert-Test ($code -eq 0) "Synthetic application compilation failed for $version"
            $files = [Collections.Generic.List[object]]::new()
            $files.Add([pscustomobject]@{path='bin\JobTrackerApp.exe';source=$fixtureExe;sha256=(Get-FileHash -LiteralPath $fixtureExe).Hash.ToLowerInvariant()})
            foreach ($name in @('first.txt', 'locked.txt', $(if ($version -eq '1.0.0') {'old-only.txt'} else {'new-only.txt'}))) {
                $source = Join-Path $directory $name
                Set-Content -LiteralPath $source -Value "Owned $name payload version $version"
                $files.Add([pscustomobject]@{path="payload\$name";source=$source;sha256=(Get-FileHash -LiteralPath $source).Hash.ToLowerInvariant()})
            }
            $payloads[$version] = @($files)
            $identityFile = Join-Path $directory 'identity.iss'
            @"
#define JobTrackerProductName "JobTracker"
#define JobTrackerReleaseVersion "$version"
#define JobTrackerWindowsVersion "$version.0"
#define JobTrackerPublisher ""
#define JobTrackerExecutableName "JobTrackerApp.exe"
#define JobTrackerSetupIconFile "$sourceDirectory\JobTracker.ico"
#define JobTrackerUninstallDisplayIcon "{app}\bin\JobTrackerApp.exe,0"
"@ | Set-Content -LiteralPath $identityFile -Encoding utf8
            $runtime = Join-Path $directory 'runtime.iss'
            @(
                '// TEST ONLY: /CURRENTUSER; no prerequisite files or execution.'
                '#define MsvcMinimumComponents "14, 0, 0, 0"'
                '#define MsvcRedistHash "0000000000000000000000000000000000000000000000000000000000000000"'
                '#define MsvcRedistSize 0'
                '[Code]'
                'function MsvcSystemFilesCompatible: Boolean;'
                'begin'
                '  Result := False;'
                'end;'
            ) | Set-Content -LiteralPath $runtime -Encoding utf8
            $payload = [Collections.Generic.List[string]]::new()
            $payload.Add('[Files]')
            foreach ($file in $files) {
                $payload.Add(('Source: "{0}"; DestDir: "{{app}}\{1}"; Hash: "{2}"; Flags: ignoreversion uninsneveruninstall; AfterInstall: RecordInstalledFile(''{3}'')' -f
                    $file.source, [IO.Path]::GetDirectoryName($file.path), $file.sha256, $file.path))
            }
            $payload.Add('[Code]')
            $payload.Add('procedure RegisterCurrentPayload;')
            $payload.Add('begin')
            foreach ($file in $files) { $payload.Add(('  RegisterOwnedCandidate(''{0}'', ''{1}'');' -f $file.path, $file.sha256)) }
            $noticeHash = (Get-FileHash -LiteralPath (Join-Path $sourceDirectory 'MSVC-NOTICE.txt')).Hash.ToLowerInvariant()
            $payload.Add(('  RegisterOwnedCandidate(''licenses\MSVC-NOTICE.txt'', ''{0}'');' -f $noticeHash))
            $payload.Add('end;')
            $payload.Add('procedure ValidatePayloadDestination;')
            $payload.Add('begin')
            foreach ($file in $files) { $payload.Add(('  RequireSafePayloadPath(ExpandConstant(''{{app}}\{0}''));' -f $file.path)) }
            $payload.Add('  RequireSafePayloadPath(ExpandConstant(''{app}\licenses\MSVC-NOTICE.txt''));')
            $payload.Add('end;')
            $payloadFile = Join-Path $directory 'payload.iss'
            $payload | Set-Content -LiteralPath $payloadFile -Encoding utf8
            $arguments = @("/DJobTrackerIdentityFile=$identityFile", "/DJobTrackerOutputDirectory=$directory", "/DJobTrackerMsvcFile=$runtime",
                "/DJobTrackerPayloadFile=$payloadFile", "/DTestSourceDirectory=$sourceDirectory", (Join-Path $PSScriptRoot 'InstallOperationsHarness.iss'))
            $output = @(& $compiler @arguments 2>&1)
            $code = $LASTEXITCODE
            $output | Set-Content -LiteralPath (Join-Path $directory 'compile-setup.log') -Encoding utf8
            Assert-Test ($code -eq 0) "Operations Setup compile failed for $version; see $directory/compile-setup.log"
            $candidates[$version] = Join-Path $directory "JobTracker-Setup-$version.exe"
        }
    }

    $main = Join-Path $work 'running-copy'
    $mainRecord = Install-Copy $main '1.0.0' 'running-first-install' -WithDesktopShortcut
    $running = Start-Fixture (Join-Path $main 'bin/JobTrackerApp.exe') 'running'
    Test-Case 'another-directory-install-while-copy-runs' {
        $other = Join-Path $work 'other-copy'
        $otherRecord = Install-Copy $other '1.0.0' 'other-install' -WithDesktopShortcut
        Assert-Test ($mainRecord['Shortcuts/StartMenu'] -ine $otherRecord['Shortcuts/StartMenu']) 'Independent copies share a shortcut.'
        Assert-Test ($mainRecord['Shortcuts/Desktop'] -ine $otherRecord['Shortcuts/Desktop']) 'Independent copies share a desktop shortcut.'
        Assert-Test (-not $running.process.HasExited) 'Installing another copy terminated the running application.'
        $null = Uninstall-Copy $other 'other-uninstall'
        Assert-LinkTarget $mainRecord['Shortcuts/StartMenu'] (Join-Path $main 'bin/JobTrackerApp.exe')
        Assert-LinkTarget $mainRecord['Shortcuts/Desktop'] (Join-Path $main 'bin/JobTrackerApp.exe')
        Assert-Test (-not $running.process.HasExited) 'Uninstalling another copy terminated the running application.'
    }
    Test-Case 'same-directory-running-update-attempts-real-write' {
        $operation = Invoke-Operation $candidates['2.0.0'] @('/CURRENTUSER', "/DIR=`"$main`"") 'running-update'
        Assert-Test ($operation.exit -ne 0 -and $operation.log.Contains('JobTrackerApp.exe') -and
            $operation.log -match '(?i)error|failed|denied|incomplete') 'Expected a reported native write failure for the mapped executable.'
        Assert-Test (-not $running.process.HasExited) 'Update closed the synthetic application.'
    }
    Test-Case 'running-uninstall-attempts-removal-and-retains-retry' {
        $operation = Uninstall-Copy $main 'running-uninstall' $false
        Assert-Test ($operation.log.Contains('JobTrackerApp.exe')) 'Running-file failure omitted the actual path.'
        Assert-Test (-not (Test-Path -LiteralPath (Join-Path $main 'payload/first.txt'))) 'Uninstall did not attempt removable owned files.'
        Assert-Test (-not $running.process.HasExited) 'Uninstall terminated the synthetic application.'
    }
    Close-Fixture $running
    Test-Case 'retry-after-actual-file-obstruction-removed' { $null = Uninstall-Copy $main 'running-uninstall-retry' }

    foreach ($condition in @('locked', 'readonly')) {
        $directory = Join-Path $work "partial-$condition"
        $null = Install-Copy $directory '1.0.0' "$condition-install"
        $file = Join-Path $directory 'payload/locked.txt'
        $stream = $null
        if ($condition -eq 'locked') { $stream = [IO.File]::Open($file, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::Read) }
        else { (Get-Item -LiteralPath $file).IsReadOnly = $true }
        try {
            Test-Case "actual-$condition-file-partial-removal" {
                $operation = Uninstall-Copy $directory "$condition-uninstall" $false
                Assert-Test ($operation.log.Contains($file)) 'Incomplete removal omitted the blocked path.'
                Assert-Test (Test-Path -LiteralPath $file) 'Blocked file was unexpectedly removed.'
                Assert-Test (-not (Test-Path -LiteralPath (Join-Path $directory 'payload/first.txt'))) 'A blocked file prevented all other program removal.'
            }
        } finally {
            if ($null -ne $stream) { $stream.Dispose() }
            if (Test-Path -LiteralPath $file) { (Get-Item -LiteralPath $file).IsReadOnly = $false }
        }
        Test-Case "$condition-removal-retry" { $null = Uninstall-Copy $directory "$condition-retry" }
    }

    Test-Case 'same-directory-upgrade-history-and-downgrade' {
        $directory = Join-Path $work 'upgrade-copy'
        $before = Install-Copy $directory '1.0.0' 'upgrade-install'
        $logPath = [IO.Path]::ChangeExtension([string]$before['Installation/Uninstaller'], '.dat')
        $logSize = (Get-Item -LiteralPath $logPath).Length
        $null = Install-Copy $directory '2.0.0' 'upgrade-newer'
        Assert-Test ((Get-Item -LiteralPath $logPath).Length -gt $logSize -and
            (Test-Path -LiteralPath (Join-Path $directory 'payload/old-only.txt'))) 'Upgrade discarded valid older installation history/files.'
        Assert-Test (@(Get-ChildItem -LiteralPath $directory -Filter 'unins*.preserved-*' -File).Count -eq 0) 'Ordinary upgrade quarantined a valid local uninstall log.'
        $snapshot = Snapshot-Tree $directory
        $operation = Invoke-Operation $candidates['1.0.0'] @('/CURRENTUSER', "/DIR=`"$directory`"") 'upgrade-downgrade'
        Assert-Test ($operation.exit -ne 0 -and $operation.log -match 'reliably identified newer') 'Same-directory downgrade was not rejected.'
        Assert-Test ((Snapshot-Tree $directory) -ceq $snapshot) 'Rejected downgrade changed selected copy.'
        $separate = Join-Path $work 'separate-older'
        $null = Install-Copy $separate '1.0.0' 'separate-older-install'
        $null = Uninstall-Copy $separate 'separate-older-uninstall'
        $null = Uninstall-Copy $directory 'upgrade-uninstall'
        Assert-Test (-not (Test-Path -LiteralPath (Join-Path $directory 'payload/old-only.txt'))) 'Older owned file was lost from additive bookkeeping.'
    }

    foreach ($operationName in @('copied', 'moved')) {
        Test-Case "$operationName-uninstaller-refuses-source-paths-and-repair-is-local" {
            $source = Join-Path $work "$operationName-source"
            $destination = Join-Path $work "$operationName-destination"
            $original = Install-Copy $source '1.0.0' "$operationName-install"
            Set-Content -LiteralPath (Join-Path $source 'unrelated.txt') -Value 'Keep unrelated source and destination files.'
            $snapshot = Snapshot-Tree $source
            Assert-FixturePath $source
            Assert-FixturePath $destination
            if ($operationName -eq 'copied') { Copy-Item -LiteralPath $source -Destination $destination -Recurse }
            else { Move-Item -LiteralPath $source -Destination $destination }
            $localUninstaller = Join-Path $destination ([IO.Path]::GetFileName([string]$original['Installation/Uninstaller']))
            $operation = Invoke-Operation $localUninstaller @() "$operationName-unsafe-uninstall"
            Assert-Test ($operation.exit -ne 0 -and $operation.log -match 'different installation path') 'Relocated uninstaller was not rejected before destructive work.'
            Assert-Test ((Snapshot-Tree $destination) -ceq $snapshot) 'Rejected relocated uninstall changed its copied files.'
            if ($operationName -eq 'copied') { Assert-Test ((Snapshot-Tree $source) -ceq $snapshot) 'Copied uninstaller changed the original installation.' }
            Assert-LinkTarget $original['Shortcuts/StartMenu'] (Join-Path $source 'bin/JobTrackerApp.exe')
            $repaired = Install-Copy $destination '1.0.0' "$operationName-repair"
            Assert-Test ($repaired['Shortcuts/StartMenu'] -ine $original['Shortcuts/StartMenu']) 'Relocated repair adopted the source shortcut.'
            $null = Uninstall-Copy $destination "$operationName-local-uninstall"
            Assert-Test (Test-Path -LiteralPath (Join-Path $destination 'unrelated.txt')) 'Relocated uninstall removed unknown data.'
            Assert-LinkTarget $original['Shortcuts/StartMenu'] (Join-Path $source 'bin/JobTrackerApp.exe')
            if ($operationName -eq 'copied') {
                Assert-Test ((Snapshot-Tree $source) -ceq $snapshot) 'Repaired copied uninstall changed original files.'
                $null = Uninstall-Copy $source "$operationName-source-uninstall"
            } else {
                # Reinstall this owned source path to remove its stale original
                # shortcuts through the normal ownership-aware lifecycle.
                $null = Install-Copy $source '1.0.0' 'moved-source-fresh-install'
                $null = Uninstall-Copy $source 'moved-source-uninstall'
            }
        }
    }

    Test-Case 'missing-ledger-reconstructs-only-known-payload-files' {
        $directory = Join-Path $work 'missing-ledger'
        $null = Install-Copy $directory '1.0.0' 'missing-ledger-install'
        $unknown = Join-Path $directory 'unrelated.txt'
        Set-Content -LiteralPath $unknown -Value 'Preserve unowned file.'
        $hash = (Get-FileHash -LiteralPath $unknown).Hash
        $ledger = Join-Path $directory 'JobTracker.ownership.ini'
        Assert-FixturePath $ledger
        Move-Item -LiteralPath $ledger -Destination (Join-Path $work 'missing-ledger-original.ini')
        $null = Uninstall-Copy $directory 'missing-ledger-uninstall'
        Assert-Test ((Get-FileHash -LiteralPath $unknown).Hash -ceq $hash) 'Reconstructed cleanup removed or changed an unknown file.'
    }

    Test-Case 'missing-ledger-and-exclusive-file-still-reach-removal' {
        $directory = Join-Path $work 'missing-ledger-locked'
        $local = Install-Copy $directory '1.0.0' 'missing-ledger-locked-install'
        $ledger = Join-Path $directory 'JobTracker.ownership.ini'
        Assert-FixturePath $ledger
        Move-Item -LiteralPath $ledger -Destination (Join-Path $work 'missing-ledger-locked-original.ini')
        $file = Join-Path $directory 'payload/locked.txt'
        $stream = [IO.File]::Open($file, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::None)
        try {
            $operation = Invoke-Operation $local['Installation/Uninstaller'] @() 'missing-ledger-locked-uninstall'
            Assert-Test ($operation.exit -ne 0 -and $operation.log.Contains($file) -and
                $operation.log -match 'Program removal incomplete') 'Exclusive reconstruction failure was not reported as an attempted removal.'
            $dataChoice = $operation.log.IndexOf('JobTracker personal data has been preserved.')
            $programFailure = $operation.log.IndexOf('Program removal incomplete')
            Assert-Test ($dataChoice -ge 0 -and $programFailure -gt $dataChoice) 'A program-file reconstruction failure prevented the data-choice phase.'
            Assert-Test (-not (Test-Path -LiteralPath (Join-Path $directory 'payload/first.txt'))) 'Reconstruction failure prevented removal of other identifiable files.'
            Assert-Test (Test-Path -LiteralPath $local['Installation/Uninstaller']) 'Failed reconstruction lost the retry uninstaller.'
        } finally { $stream.Dispose() }
        $null = Uninstall-Copy $directory 'missing-ledger-locked-retry'
    }

    Test-Case 'metadata-hardlinks-and-occupied-write-names-preserve-outside-bytes' {
        $directory = Join-Path $work 'metadata-hardlinks'
        $null = Install-Copy $directory '1.0.0' 'metadata-hardlinks-install'
        $outsideRecords = [Collections.Generic.List[object]]::new()
        foreach ($name in @('JobTracker.install.ini', 'JobTracker.ownership.ini')) {
            $path = Join-Path $directory $name
            $outside = Join-Path $work "outside-$name"
            Assert-FixturePath $path
            Assert-FixturePath $outside
            Move-Item -LiteralPath $path -Destination $outside
            New-Item -ItemType HardLink -Path $path -Target $outside | Out-Null
            $outsideRecords.Add([pscustomobject]@{path=$outside;hash=(Get-FileHash -LiteralPath $outside).Hash})
            foreach ($index in @(0, 1)) {
                $occupied = "$path.write-$index"
                New-Item -ItemType HardLink -Path $occupied -Target $outside | Out-Null
            }
        }
        $null = Install-Copy $directory '1.0.0' 'metadata-hardlinks-repair'
        foreach ($record in $outsideRecords) {
            Assert-Test ((Get-FileHash -LiteralPath $record.path).Hash -ceq $record.hash) 'Metadata repair changed bytes through an outside hardlink.'
        }
        $null = Uninstall-Copy $directory 'metadata-hardlinks-uninstall'
        foreach ($record in $outsideRecords) {
            Assert-Test ((Get-FileHash -LiteralPath $record.path).Hash -ceq $record.hash) 'Metadata removal changed outside hardlink bytes.'
        }
    }

    Test-Case 'unicode-directory-metadata-roundtrip-and-repair' {
        $directory = Join-Path $work 'копия-Əlaqə-测试'
        $first = Install-Copy $directory '1.0.0' 'unicode-install'
        Assert-LinkTarget $first['Shortcuts/StartMenu'] (Join-Path $directory 'bin/JobTrackerApp.exe')
        $second = Install-Copy $directory '1.0.0' 'unicode-repair'
        Assert-Test ($second['Installation/Path'] -ceq $directory -and
            $second['Installation/Id'] -ceq $first['Installation/Id']) 'Unicode metadata did not roundtrip through repair.'
        Assert-Test ($second['Shortcuts/StartMenu'] -ceq $first['Shortcuts/StartMenu'] -and
            $second['Shortcuts/Desktop'] -ceq $first['Shortcuts/Desktop']) 'Unicode repair created duplicate shortcuts.'
        $null = Uninstall-Copy $directory 'unicode-uninstall'
    }

    Test-Case 'owned-payload-hardlink-repair-preserves-outside-bytes' {
        $directory = Join-Path $work 'payload-hardlink'
        $null = Install-Copy $directory '1.0.0' 'payload-hardlink-install'
        $path = Join-Path $directory 'payload/locked.txt'
        $outside = Join-Path $work 'outside-owned-payload.txt'
        Assert-FixturePath $path
        Assert-FixturePath $outside
        Move-Item -LiteralPath $path -Destination $outside
        Set-Content -LiteralPath $outside -Value 'Unrelated outside bytes must survive repair.'
        $hash = (Get-FileHash -LiteralPath $outside).Hash
        New-Item -ItemType HardLink -Path $path -Target $outside | Out-Null
        $null = Install-Copy $directory '1.0.0' 'payload-hardlink-repair'
        Assert-Test ((Get-FileHash -LiteralPath $outside).Hash -ceq $hash) 'Native payload replacement changed outside hardlink bytes.'
        $null = Uninstall-Copy $directory 'payload-hardlink-uninstall'
        Assert-Test ((Get-FileHash -LiteralPath $outside).Hash -ceq $hash) 'Payload removal changed outside hardlink bytes.'
    }

    Test-Case 'damaged-uninstall-data-is-preserved-and-rebuilt-locally' {
        $directory = Join-Path $work 'damaged-uninstall-data'
        $local = Install-Copy $directory '1.0.0' 'damaged-data-install'
        $unknown = Join-Path $directory 'unrelated.txt'
        Set-Content -LiteralPath $unknown -Value 'Preserve unrelated files during uninstall-data reconstruction.'
        $data = [IO.Path]::ChangeExtension([string]$local['Installation/Uninstaller'], '.dat')
        Assert-FixturePath $data
        Copy-Item -LiteralPath $data -Destination (Join-Path $work 'damaged-uninstall-original.dat')
        Set-Content -LiteralPath $data -Value 'Deliberately damaged fixture uninstall data.'
        $corruptHash = (Get-FileHash -LiteralPath $data).Hash
        $null = Install-Copy $directory '1.0.0' 'damaged-data-repair'
        $saved = @(Get-ChildItem -LiteralPath $directory -Filter 'unins*.dat.preserved-*' -File)
        Assert-Test ($saved.Count -eq 1 -and (Get-FileHash -LiteralPath $saved[0].FullName).Hash -ceq $corruptHash) 'Damaged uninstall bytes were overwritten instead of preserved.'
        Assert-Test (Test-Path -LiteralPath $unknown) 'Uninstall-data reconstruction removed an unrelated file.'
        $null = Uninstall-Copy $directory 'damaged-data-uninstall'
        Assert-Test (Test-Path -LiteralPath $unknown) 'Rebuilt uninstaller adopted an unrelated file.'
    }

    Test-Case 'partial-program-deletion-repairs-existing-copy' {
        $directory = Join-Path $work 'partial-program'
        $null = Install-Copy $directory '1.0.0' 'partial-program-install'
        foreach ($relative in @('bin/JobTrackerApp.exe', 'payload/first.txt')) {
            $path = Join-Path $directory $relative
            Assert-FixturePath $path
            Move-Item -LiteralPath $path -Destination (Join-Path $work ('partial-original-' + [IO.Path]::GetFileName($path)))
        }
        $unknown = Join-Path $directory 'unrelated.txt'
        Set-Content -LiteralPath $unknown -Value 'Keep this unrelated file during repair.'
        $null = Install-Copy $directory '1.0.0' 'partial-program-repair'
        Assert-Test (Test-Path -LiteralPath $unknown) 'Partial repair removed an unrelated file.'
        $null = Uninstall-Copy $directory 'partial-program-uninstall'
    }

    Test-Case 'removed-program-folder-reinstalls-despite-stale-shortcuts' {
        $directory = Join-Path $work 'removed-program'
        $backup = Join-Path $work 'removed-program-backup'
        $old = Install-Copy $directory '1.0.0' 'removed-program-install'
        Assert-FixturePath $directory
        Assert-FixturePath $backup
        Move-Item -LiteralPath $directory -Destination $backup
        $fresh = Install-Copy $directory '1.0.0' 'removed-program-fresh-install'
        Assert-Test ($fresh['Shortcuts/StartMenu'] -ceq $old['Shortcuts/StartMenu']) 'A stale shortcut prevented safe same-location reuse.'
        $snapshot = Snapshot-Tree $directory
        $movedUninstaller = Join-Path $backup ([IO.Path]::GetFileName([string]$old['Installation/Uninstaller']))
        $operation = Invoke-Operation $movedUninstaller @() 'removed-program-old-uninstaller'
        Assert-Test ($operation.exit -ne 0 -and $operation.log -match 'different installation path') 'Moved old uninstaller could target the recreated source path.'
        Assert-Test ((Snapshot-Tree $directory) -ceq $snapshot) 'Moved old uninstaller changed the fresh installation.'
        $null = Uninstall-Copy $directory 'removed-program-uninstall'
    }

    Test-Case 'missing-ledger-preserves-unowned-payload-collision' {
        $directory = Join-Path $work 'unowned-collision'
        $null = Install-Copy $directory '1.0.0' 'unowned-collision-install'
        $ledger = Join-Path $directory 'JobTracker.ownership.ini'
        Assert-FixturePath $ledger
        Move-Item -LiteralPath $ledger -Destination (Join-Path $work 'unowned-collision-original.ini')
        $collision = Join-Path $directory 'payload/locked.txt'
        Set-Content -LiteralPath $collision -Value 'Unknown replacement must be preserved without assuming historical ownership.'
        $hash = (Get-FileHash -LiteralPath $collision).Hash
        $null = Install-Copy $directory '1.0.0' 'unowned-collision-repair'
        $saved = @(Get-ChildItem -LiteralPath (Join-Path $directory 'payload') -Filter 'locked.txt.preserved-*' -File)
        Assert-Test ($saved.Count -eq 1 -and (Get-FileHash -LiteralPath $saved[0].FullName).Hash -ceq $hash) 'Repair overwrote an unowned colliding file.'
        $null = Uninstall-Copy $directory 'unowned-collision-uninstall'
        Assert-Test ((Get-FileHash -LiteralPath $saved[0].FullName).Hash -ceq $hash) 'Rebuilt ownership adopted the preserved collision.'
    }

    Test-Case 'retargeted-shortcut-and-other-copy-survive-uninstall' {
        $first = Join-Path $work 'retarget-first'
        $second = Join-Path $work 'retarget-second'
        $firstRecord = Install-Copy $first '1.0.0' 'retarget-first-install'
        $secondRecord = Install-Copy $second '1.0.0' 'retarget-second-install'
        $path = [string]$firstRecord['Shortcuts/StartMenu']
        $secondExe = Join-Path $second 'bin/JobTrackerApp.exe'
        $shell = New-Object -ComObject WScript.Shell
        $link = $shell.CreateShortcut($path)
        $link.TargetPath = $secondExe
        $link.Save()
        $null = Uninstall-Copy $first 'retarget-first-uninstall'
        Assert-LinkTarget $path $secondExe
        Assert-LinkTarget $secondRecord['Shortcuts/StartMenu'] $secondExe
        Assert-Test (Test-Path -LiteralPath $secondExe) 'Removing first copy removed second copy.'
        $null = Uninstall-Copy $second 'retarget-second-uninstall'
        Assert-LinkTarget $path $secondExe
        # This exact link was created/retargeted here, and its target was checked.
        Remove-Item -LiteralPath $path
    }

    if ($RunCurrentUserIntegration) {
        $localTest = Join-Path $PSScriptRoot 'Test-LocalInstaller.ps1'
        $realWork = Join-Path $work 'real-currentuser'
        foreach ($action in @('Install', 'AdditionalCopy', 'RepairMissingUninstaller', 'RepairMissingLog', 'Uninstall')) {
            Test-Case "real-currentuser-$action" {
                & $localTest -InstallerPath $installer -ManifestPath $ManifestPath -WorkDirectory $realWork -Scope CurrentUser -Action $action
            }
        }
    }
    [ordered]@{result='PASS';checks=$results.Count;currentUserIntegration=[bool]$RunCurrentUserIntegration;
        elevated=$elevated;installerSha256=$metadata.sha256;ownershipSha256=(Get-FileHash -LiteralPath (Join-Path $sourceDirectory 'InstallOwnership.iss')).Hash;
        personalDataTouched=$false;applicationLaunched='synthetic cooperative .NET fixtures only'} |
        ConvertTo-Json | Set-Content -LiteralPath (Join-Path $work 'summary.json') -Encoding utf8
    Write-Output "PASS: $($results.Count) native installation operations checks."
} finally {
    foreach ($fixture in $activeFixtures) {
        if (-not $fixture.process.HasExited) { Close-Fixture $fixture }
    }
}
