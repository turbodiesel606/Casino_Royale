#requires -Version 7.2
<# Stage-15 static documentation evidence only. Does not run documented builds,
tests, installers or the application. Writes only a new JSON result file. #>
[CmdletBinding()]
param([Parameter(Mandatory)][string]$OutputPath)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../../..'))
if ((Get-Location).Path -ne $root) { throw 'Run from the JobTracker repository root.' }
$output = [IO.Path]::GetFullPath($OutputPath)
if (-not $output.StartsWith((Join-Path $root 'dist/stage15-evidence') + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw 'Output must stay under dist/stage15-evidence.' }
if (Test-Path -LiteralPath $output) { throw 'Preserve prior results; choose a new output file.' }
$checks = [Collections.Generic.List[object]]::new()
function Check([string]$Name, [bool]$Passed, $Evidence) {
    $checks.Add([pscustomobject]@{ name = $Name; result = $(if ($Passed) { 'PASS' } else { 'FAIL' }); evidence = $Evidence })
}
function Hash([string]$Path) { (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant() }

$docs = @('AGENTS.md','README.md','BuildCommands.txt','qt_bins.txt','For-Agent/Docs/build.md','For-Agent/Docs/testing.md','For-Agent/Docs/windows-release.md','For-Agent/Docs/windows-release-requirements.md','docs/BUILDING.md','docs/WINDOWS-RELEASE.md','docs/WINDOWS-INSTALLER.md','packaging/windows/README.md','.agents/skills/cmake-build-debug/SKILL.md','.agents/skills/test-and-review/SKILL.md','.agents/skills/windows-release-packaging/SKILL.md')
$linkCount = 0
$linkErrors = [Collections.Generic.List[string]]::new()
foreach ($file in $docs) {
    $body = Get-Content -LiteralPath $file -Raw
    Check "No stale Qt requirement: $file" ($body -notmatch '6\.9\.[0-9]+') 'Maintained documentation only; historical reports excluded.'
    foreach ($link in [regex]::Matches($body, '\[[^\]\r\n]*\]\(([^)\r\n]+)\)')) {
        $target = $link.Groups[1].Value
        if ($target -match '^[a-zA-Z][a-zA-Z0-9+.-]*:') { continue }
        $linkCount++
        $parts = $target.Split('#', 2)
        $path = if ($parts[0]) { [IO.Path]::GetFullPath((Join-Path (Split-Path -Parent ([IO.Path]::GetFullPath($file))) $parts[0])) } else { [IO.Path]::GetFullPath($file) }
        if (-not (Test-Path -LiteralPath $path)) { $linkErrors.Add("${file}: $target missing"); continue }
        if ($parts.Count -eq 2 -and $parts[1]) {
            $headings = [regex]::Matches((Get-Content -LiteralPath $path -Raw), '(?m)^#{1,6}\s+(.+?)\r?$')
            $anchors = @($headings | ForEach-Object { ($_.Groups[1].Value.ToLowerInvariant() -replace '[^\p{L}\p{N}_\-\s]', '') -replace '\s', '-' })
            if ($parts[1] -cnotin $anchors) { $linkErrors.Add("${file}: $target anchor missing") }
        }
    }
}
Check 'Local Markdown links and anchors' ($linkErrors.Count -eq 0) @{ links = $linkCount; errors = @($linkErrors) }

$shared = Get-Content CMakePresets.json -Raw | ConvertFrom-Json
$local = Get-Content CMakeUserPresets.json -Raw | ConvertFrom-Json
$cmakeSource = Get-Content CMakeLists.txt -Raw
$buildDoc = Get-Content For-Agent/Docs/build.md -Raw
$commandRecords = [Collections.Generic.List[object]]::new()
foreach ($block in [regex]::Matches($buildDoc, '(?s)```powershell\r?\n(.*?)\r?\n```')) {
    $command = $block.Groups[1].Value.Trim()
    $tokens = $null
    $errors = $null
    $ast = [Management.Automation.Language.Parser]::ParseInput($command, [ref]$tokens, [ref]$errors)
    $calls = @($ast.FindAll({ param($node) $node -is [Management.Automation.Language.CommandAst] }, $true))
    Check 'Single parseable documented command' ($errors.Count -eq 0 -and $calls.Count -eq 1) $command
    if ($errors.Count -or $calls.Count -ne 1) { continue }
    $elements = @($calls[0].CommandElements | ForEach-Object { $_.Extent.Text.Trim('"', "'") })
    if ($elements[0] -in 'cmake','ctest') {
        $presetIndex = [Array]::IndexOf($elements, '--preset')
        if ($presetIndex -ge 0) {
            $kind = if ($elements[0] -eq 'ctest') { 'testPresets' } elseif ('--build' -in $elements) { 'buildPresets' } else { 'configurePresets' }
            $names = @($shared.$kind.name) + @($local.$kind.name)
            Check "Declared $kind preset" ($elements[$presetIndex + 1] -cin $names) $command
        }
        $targetIndex = [Array]::IndexOf($elements, '--target')
        if ($targetIndex -ge 0) {
            $target = $elements[$targetIndex + 1]
            Check 'Declared CMake target' ($cmakeSource.Contains($target)) $target
        }
    } elseif ($elements[0] -eq 'pwsh') {
        $fileIndex = [Array]::IndexOf($elements, '-File')
        $script = $elements[$fileIndex + 1]
        Check 'Documented script exists' (Test-Path -LiteralPath $script) $script
        $scriptAst = [Management.Automation.Language.Parser]::ParseFile([IO.Path]::GetFullPath($script), [ref]$tokens, [ref]$errors)
        Check 'Documented script parses' ($errors.Count -eq 0) $script
        $parameters = if ($null -ne $scriptAst.ParamBlock) { @($scriptAst.ParamBlock.Parameters.Name.VariablePath.UserPath) } else { @() }
        for ($i = $fileIndex + 2; $i -lt $elements.Count; $i++) {
            if ($elements[$i] -match '^-(\w+)$') {
                Check 'Declared script parameter' ($Matches[1] -in $parameters) "$script $($elements[$i])"
            }
        }
    }
    $commandRecords.Add([pscustomobject]@{ command = $command; executedByThisCheck = $false })
}
Check 'Qt and product identity source' ($cmakeSource -match 'VERSION 0\.1\.0\s' -and $cmakeSource -match 'set\(JOBTRACKER_QT_VERSION 6\.10\.3\)') 'CMakeLists.txt'

$manifestPath = 'dist/stage13-package/JobTracker-0.1.0-windows-x64.7z.manifest.json'
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
$archive = Join-Path (Split-Path -Parent $manifestPath) $manifest.archive
Check 'Retained archive size and digest' ((Hash $archive) -ceq $manifest.sha256 -and (Get-Item $archive).Length -eq $manifest.size) @{ path = $archive; sha256 = $manifest.sha256 }
Check 'Checksum sidecar' ((Get-Content "$archive.sha256" -Raw).TrimEnd() -ceq "$($manifest.sha256)  $($manifest.archive)") "$archive.sha256"
$inventory = @(Get-Content packaging/windows/payload-files.txt | Where-Object { $_ -and -not $_.StartsWith('#') })
Check 'Exact payload inventory' (($inventory -join "`n") -ceq ($manifest.files.path -join "`n")) @{ files = $manifest.files.Count }
$payloadFailures = @($manifest.files | Where-Object { (Hash (Join-Path 'dist/stage13-package/work/extracted' $_.path)) -cne $_.sha256 -or (Get-Item (Join-Path 'dist/stage13-package/work/extracted' $_.path)).Length -ne $_.size })
Check 'Retained extracted file bytes' ($payloadFailures.Count -eq 0) @{ checked = $manifest.files.Count; failures = @($payloadFailures | ForEach-Object path) }
Check 'Built executable matches archive' ((Hash 'build/qt-6.10.3/windows-release/Release/JobTrackerApp.exe') -ceq ($manifest.files | Where-Object path -CEQ 'bin/JobTrackerApp.exe').sha256) 'No application execution.'
foreach ($mode in 'online','local') {
    $metadata = Get-Content "dist/stage13-$mode/installer-build.json" -Raw | ConvertFrom-Json
    $setup = "dist/stage13-$mode/$($metadata.installer)"
    Check "$mode Setup identity and digest" ($metadata.version -ceq $manifest.version -and $metadata.archiveSha256 -ceq $manifest.sha256 -and (Hash $setup) -ceq $metadata.sha256) @{ path = $setup; sha256 = $metadata.sha256 }
    if ($mode -eq 'online') { Check 'Fixed release URL' ($metadata.url -ceq "https://github.com/turbodiesel606/Releases/releases/download/v$($manifest.version)/$($manifest.archive)") $metadata.url }
}
$runtime = Get-Content packaging/windows/MsvcRuntime.lock.json -Raw | ConvertFrom-Json
foreach ($file in $runtime.files) {
    Check "Retained CRT: $($file.name)" ((Hash (Join-Path 'dist/stage13-online/msvc-runtime' $file.name)) -ceq $file.sha256) $file.sha256
}

$summary = Get-Content dist/stage13-evidence/verification-summary.json -Raw | ConvertFrom-Json
foreach ($suite in $summary.suites) {
    $results = @(Get-Content -LiteralPath $suite.path -Raw | ConvertFrom-Json)
    Check "Prior result evidence: $($suite.suite)" ($results.Count -eq $suite.checks -and @($results | Where-Object result -CNE 'PASS').Count -eq 0) @{ path = $suite.path; checks = $results.Count; historical = $true }
}
foreach ($entry in @(@{path='dist/stage13-evidence/release-ctest.log';count=18}, @{path='dist/stage13-evidence/staging-ctest.log';count=1})) {
    $log = Get-Content -LiteralPath $entry.path -Raw
    $passed = [regex]::Matches($log, 'Test Passed\.').Count
    Check 'Prior CTest evidence' ($passed -eq $entry.count -and $log -notmatch 'Test Failed\.') @{ path = $entry.path; passed = $passed; historical = $true }
}
$security = @(Get-Content dist/stage14-online-tests/results.json -Raw | ConvertFrom-Json)
Check 'Prior stage-14 security evidence' ($security.Count -eq 24 -and @($security | Where-Object result -CNE 'PASS').Count -eq 0) @{ checks = $security.Count; historical = $true }

$allowed = @('AGENTS.md','.gitignore','README.md','BuildCommands.txt','qt_bins.txt','For-Agent/Docs/build.md','For-Agent/Docs/testing.md','packaging/windows/README.md','.agents/skills/cmake-build-debug/SKILL.md','.agents/skills/test-and-review/SKILL.md','For-Agent/Task-Report/windows-release/STATUS.md')
$baseline = @(Get-Content dist/stage15-evidence/input-sha256.json -Raw | ConvertFrom-Json)
$changed = @($baseline | Where-Object { (Hash $_.Path) -cne $_.SHA256 })
$unexpected = @($changed | Where-Object Path -CNotIn $allowed)
Check 'Preserve original source, tasks, artifacts, configuration and unrelated work' ($unexpected.Count -eq 0) @{ inputs = $baseline.Count; unchanged = $baseline.Count - $changed.Count; changed = @($changed | ForEach-Object Path); unexpected = @($unexpected | ForEach-Object Path) }
$failed = @($checks | Where-Object result -CEQ 'FAIL')
[pscustomobject]@{ checks = @($checks); commands = @($commandRecords); passed = $checks.Count - $failed.Count; failed = $failed.Count; scope = 'Static documentation and retained-file evidence; no build, native suite, installer, network or application executed.' } | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $output -Encoding utf8
"$($checks.Count - $failed.Count) passed; $($failed.Count) failed. $output"
if ($failed.Count) { $failed | ConvertTo-Json -Depth 6; exit 1 }
