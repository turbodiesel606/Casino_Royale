#requires -Version 7.2
<# Standalone runtime policy/integrity/native-download checks. Never installs
the system prerequisite, registers JobTracker, or launches the application. #>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$BuildDirectory,
    [Parameter(Mandatory)][string]$InstallerDirectory,
    [Parameter(Mandatory)][string]$RedistributablePath,
    [Parameter(Mandatory)][string]$WorkDirectory,
    [Parameter(Mandatory)][string]$IsccPath,
    [switch]$CheckOfficialDownload
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$work = [IO.Path]::GetFullPath($WorkDirectory)
if (-not $work.StartsWith((Join-Path $repository 'dist') + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Runtime test fixtures must be below dist/.'
}
if (Test-Path -LiteralPath $work) { throw 'Test output exists; choose a fresh directory.' }
$ancestor = $work
while ($ancestor) {
    if ((Test-Path -LiteralPath $ancestor) -and
        ((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw 'Reparse point in test destination.'
    }
    $ancestor = [IO.Path]::GetDirectoryName($ancestor)
}
$installerDirectory = [IO.Path]::GetFullPath($InstallerDirectory)
$redist = (Get-Item -LiteralPath $RedistributablePath).FullName
$metadata = Get-Content -LiteralPath (Join-Path $installerDirectory 'installer-build.json') -Raw | ConvertFrom-Json
New-Item -ItemType Directory -Path $work | Out-Null
$results = [Collections.Generic.List[object]]::new()
function Assert-Check([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}
function Record-Pass([string]$Name, [object]$Evidence) {
    $results.Add([ordered]@{name=$Name;result='PASS';evidence=$Evidence})
    $results | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $work 'results.json')
    Write-Output "PASS: $Name"
}

# Verify import names and symbols for the entire verified common payload and
# the CRT itself. This catches a new Qt/toolchain dependency the lock cannot serve.
$cache = Get-Content -LiteralPath (Join-Path $BuildDirectory 'CMakeCache.txt') -Raw
$instance = [regex]::Match($cache, '(?m)^CMAKE_GENERATOR_INSTANCE:INTERNAL=(.+?)\r?$').Groups[1].Value
$dumpbin = Join-Path $instance "VC/Tools/MSVC/$($metadata.msvcRuntime.toolset)/bin/Hostx64/x64/dumpbin.exe"
$exports = @{}
$crtFiles = @(Get-ChildItem -LiteralPath (Join-Path $installerDirectory 'msvc-runtime') -Filter '*.dll' -File)
foreach ($file in $crtFiles) {
    $lines = @(& $dumpbin /nologo /exports $file.FullName)
    Assert-Check ($LASTEXITCODE -eq 0) "Export inspection failed: $($file.Name)"
    $exports[$file.Name] = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    foreach ($line in $lines) {
        if ($line -match '^\s+\d+\s+[0-9A-F]+\s+[0-9A-F]+\s+(\S+)') { [void]$exports[$file.Name].Add($Matches[1]) }
    }
    Assert-Check ($exports[$file.Name].Count -gt 0) "No exports parsed for $($file.Name)."
}
$binaries = @(Get-ChildItem -LiteralPath $metadata.verifiedPayloadDirectory -Recurse -File | Where-Object Extension -in '.exe','.dll') + $crtFiles
$imports = [Collections.Generic.List[object]]::new()
foreach ($binary in $binaries) {
    $lines = @(& $dumpbin /nologo /imports $binary.FullName)
    Assert-Check ($LASTEXITCODE -eq 0) "Import inspection failed: $($binary.Name)"
    $dll = ''
    foreach ($line in $lines) {
        if ($line -match '^\s{4}(\S+\.dll)\s*$') {
            $dll = $Matches[1]
            if ($dll -match '^(msvcp|msvcr\d|vcruntime|concrt|vccorlib|vcomp)') {
                Assert-Check ($exports.ContainsKey($dll)) "Unbundled CRT dependency: $($binary.Name) -> $dll"
            } else { $dll = '' }
        } elseif ($dll -and $line -match '^\s+[0-9A-F]+\s+(\S+)\s*$') {
            $symbol = $Matches[1]
            Assert-Check ($exports[$dll].Contains($symbol)) "Missing CRT symbol: $($binary.Name) -> $dll!$symbol"
            $imports.Add([ordered]@{binary=$binary.FullName;dll=$dll;symbol=$symbol})
        }
    }
}
Assert-Check ($imports.Count -gt 0) 'No CRT imports were inspected.'
$imports | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $work 'msvc-imports.json')
Record-Pass 'payload-and-crt-import-closure' ([ordered]@{binaries=$binaries.Count;imports=$imports.Count})

$compileArguments = @(
    "/DPipelineFile=$(Join-Path $repository 'packaging/windows/OnlinePayload.iss')"
    "/DRuntimeInput=$(Join-Path $installerDirectory 'msvc-runtime/JobTrackerMsvcRuntime.iss')"
    "/DRuntimeCode=$(Join-Path $repository 'packaging/windows/MsvcRuntime.iss')"
    "/DTestOutput=$work"
    "/DRedistFile=$redist"
    (Join-Path $PSScriptRoot 'MsvcRuntimeHarness.iss')
)
$output = @(& $IsccPath @compileArguments 2>&1)
$compileExit = $LASTEXITCODE
$output | Set-Content -LiteralPath (Join-Path $work 'compile.log')
Assert-Check ($compileExit -eq 0) "Harness compile failed ($compileExit); see $work/compile.log."
Record-Pass 'harness-compilation' $compileExit
$harness = Join-Path $work 'msvc-runtime-test-only.exe'
function Invoke-Case([string]$Name, [bool]$Success, [string]$Pattern, [string]$Language = 'english') {
    $directory = Join-Path $work $Name
    $target = Join-Path $directory 'application'
    New-Item -ItemType Directory -Path $target | Out-Null
    $sentinel = Join-Path $target 'sentinel.txt'
    Set-Content -LiteralPath $sentinel -Value 'Preserve existing data.'
    if ($Name -eq 'destination-conflict') {
        New-Item -ItemType Directory -Path (Join-Path $target 'bin') | Out-Null
        Set-Content -LiteralPath (Join-Path $target 'bin/msvcp140.dll') -Value 'Preserve existing runtime.'
    }
    if ($Name -eq 'reparse') {
        $outside = Join-Path $directory 'outside'
        New-Item -ItemType Directory -Path $outside | Out-Null
        Set-Content -LiteralPath (Join-Path $outside 'sentinel.txt') -Value 'Preserve outside.'
        New-Item -ItemType Junction -Path (Join-Path $target 'bin') -Target $outside | Out-Null
    }
    $log = Join-Path $directory 'setup.log'
    $arguments = @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/SP-', "/LANG=$Language", "/DIR=`"$target`"", "/LOG=`"$log`"", "/Case=$Name")
    $process = Start-Process -FilePath $harness -ArgumentList $arguments -WindowStyle Hidden -Wait -PassThru
    $logText = Get-Content -LiteralPath $log -Raw
    Assert-Check (($process.ExitCode -eq 0) -eq $Success) "${Name}: exit $($process.ExitCode); see $log."
    Assert-Check ($logText -match $Pattern -and $logText -notmatch 'TEST FAILURE') "${Name}: missing expected result or assertion failure."
    Assert-Check ($logText -notmatch 'MSVC prerequisite returned:') 'Harness executed the machine prerequisite.'
    Assert-Check ((Get-Content -LiteralPath $sentinel -Raw).Trim() -ceq 'Preserve existing data.') 'Destination sentinel changed.'
    foreach ($match in [regex]::Matches($logText, 'TEST temporary directory: ([^\r\n]+)')) {
        Assert-Check (-not (Test-Path -LiteralPath $match.Groups[1].Value)) 'Setup temporary directory remains.'
    }
    if ($Success) {
        foreach ($file in $metadata.msvcRuntime.files) {
            Assert-Check ((Get-FileHash -LiteralPath (Join-Path $target "bin/$($file.name)") -Algorithm SHA256).Hash -ieq $file.sha256) 'App-Local fixture hash mismatch.'
        }
    } else {
        Assert-Check (-not (Test-Path -LiteralPath (Join-Path $target 'bin/vcruntime140.dll'))) 'Failure installed partial CRT files.'
    }
    if ($Name -eq 'destination-conflict') {
        Assert-Check ((Get-Content -LiteralPath (Join-Path $target 'bin/msvcp140.dll') -Raw).Trim() -ceq 'Preserve existing runtime.') 'Existing runtime overwritten.'
    }
    if ($Name -eq 'reparse') {
        Assert-Check ((Get-Content -LiteralPath (Join-Path $directory 'outside/sentinel.txt') -Raw).Trim() -ceq 'Preserve outside.') 'Junction target changed.'
    }
    Record-Pass $Name ([ordered]@{exit=$process.ExitCode;assertions=([regex]::Matches($logText,'TEST PASS:')).Count;log=$log})
}

Invoke-Case 'policy' $true 'retry preserves reboot requirement'
Invoke-Case 'currentuser' $true 'current-user skips system prerequisite'
Invoke-Case 'system-present' $true 'host has compatible x64 registry and system files'
Invoke-Case 'destination-conflict' $false 'contains App-Local MSVC'
Invoke-Case 'reparse' $false 'symbolic link/junction'
Invoke-Case 'integrity' $true 'verified official x64 package version'
Invoke-Case 'corrupt' $false 'release files are invalid'

foreach ($negative in 'missing', 'tampered', 'wrong-x86') {
    $path = Join-Path $work "$negative.exe"
    if ($negative -eq 'tampered') {
        Copy-Item -LiteralPath $redist -Destination $path
        $bytes = [IO.File]::ReadAllBytes($path)
        $bytes[100] = $bytes[100] -bxor 1
        [IO.File]::WriteAllBytes($path, $bytes)
    } elseif ($negative -eq 'wrong-x86') {
        $cache = Get-Content -LiteralPath (Join-Path $BuildDirectory 'CMakeCache.txt') -Raw
        $instance = [regex]::Match($cache, '(?m)^CMAKE_GENERATOR_INSTANCE:INTERNAL=(.+?)\r?$').Groups[1].Value
        Copy-Item -LiteralPath (Join-Path $instance 'VC/Redist/MSVC/v143/vc_redist.x86.exe') -Destination $path
    }
    $negativeOutput = @(& pwsh -NoProfile -File (Join-Path $repository 'packaging/windows/Prepare-MsvcRuntime.ps1') -BuildDirectory $BuildDirectory -RedistributablePath $path -OutputDirectory (Join-Path $work "rejected-$negative") 2>&1)
    $negativeExit = $LASTEXITCODE
    $negativeOutput | Set-Content -LiteralPath (Join-Path $work "$negative.log")
    Assert-Check ($negativeExit -ne 0) "Accepted $negative runtime input."
    Assert-Check (-not (Test-Path -LiteralPath (Join-Path $work "rejected-$negative"))) 'Rejected input created a runtime bundle.'
    Record-Pass "reject-$negative" $negativeExit
}
if ($CheckOfficialDownload) {
    Invoke-Case 'download' $true 'native official download verified without execution'
    Invoke-Case 'download-cancel' $false 'setup was cancelled'
    Invoke-Case 'download-bad-hash' $false 'Verification error: File hash is incorrect\.'
}
Write-Output "PASS: $($results.Count) runtime checks. Evidence: $work"
