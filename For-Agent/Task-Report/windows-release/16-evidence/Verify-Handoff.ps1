#requires -Version 7.2
<# Stage-16 read-only candidate/source correspondence and scope evidence.
   Writes one fresh JSON file; does not run builds, tests, installers or the app. #>
[CmdletBinding()]
param([Parameter(Mandatory)][string]$OutputPath)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../../..'))
if ((Get-Location).Path -ne $root) { throw 'Run from the JobTracker repository root.' }
$output = [IO.Path]::GetFullPath($OutputPath)
$allowedRoot = [IO.Path]::GetFullPath((Join-Path $root 'dist/stage16-evidence'))
if (-not $output.StartsWith($allowedRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw 'Output must stay under dist/stage16-evidence.' }
if (Test-Path -LiteralPath $output) { throw 'Preserve prior evidence; choose a fresh output file.' }
$checks = [Collections.Generic.List[object]]::new()
function Hash([string]$Path) { (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant() }
function Check([string]$Name, [bool]$Passed, $Evidence) {
    $checks.Add([pscustomobject]@{ name = $Name; result = $(if ($Passed) { 'PASS' } else { 'FAIL' }); evidence = $Evidence })
}

$prior = Get-Content 'dist/stage13-evidence/verification-summary.json' -Raw | ConvertFrom-Json
$artifacts = @($prior.artifacts | ForEach-Object {
    $actualHash = Hash $_.path
    $bytes = (Get-Item -LiteralPath $_.path).Length
    Check "Retained artifact: $($_.path)" ($actualHash -ceq $_.sha256 -and $bytes -eq $_.bytes) @{ bytes = $bytes; sha256 = $actualHash }
    [pscustomobject]@{ path = $_.path; bytes = $bytes; sha256 = $actualHash }
})
$sourceFields = [ordered]@{
    scriptSha256 = 'packaging/windows/JobTracker.iss'
    onlinePayloadScriptSha256 = 'packaging/windows/OnlinePayload.iss'
    msvcScriptSha256 = 'packaging/windows/MsvcRuntime.iss'
    installationPolicySha256 = 'packaging/windows/InstallationPolicy.iss'
    manualCloseSha256 = 'packaging/windows/ManualClose.iss'
    personalDataSha256 = 'packaging/windows/PersonalData.iss'
}
foreach ($mode in 'online', 'local') {
    $metadata = Get-Content "dist/stage13-$mode/installer-build.json" -Raw | ConvertFrom-Json
    foreach ($field in $sourceFields.Keys) {
        Check "$mode source binding: $field" ((Hash $sourceFields[$field]) -ceq $metadata.$field) $sourceFields[$field]
    }
    Check "$mode generated payload binding" ((Hash "dist/stage13-$mode/JobTrackerPayload.iss") -ceq $metadata.generatedPayloadSha256) $metadata.generatedPayloadSha256
    Check "$mode runtime lock binding" ((Hash 'packaging/windows/MsvcRuntime.lock.json') -ceq $metadata.msvcRuntime.sourceLockSha256) $metadata.msvcRuntime.sourceLockSha256
    Check "$mode compiler binding" ((Hash $metadata.compiler) -ceq $metadata.compilerSha256) $metadata.compilerSha256
    foreach ($file in $metadata.msvcRuntime.files) {
        $path = "dist/stage13-$mode/msvc-runtime/$($file.name)"
        Check "$mode retained CRT: $($file.name)" ((Hash $path) -ceq $file.sha256 -and (Get-Item -LiteralPath $path).Length -eq $file.size) $file.sha256
    }
}

$baseline = @(Get-Content 'dist/stage16-evidence/input-sha256.json' -Raw | ConvertFrom-Json)
$changed = @($baseline | Where-Object { (Hash $_.Path) -cne $_.SHA256 })
$allowed = @('docs/WINDOWS-RELEASE.md', 'For-Agent/Task-Report/windows-release/STATUS.md')
$unexpected = @($changed | Where-Object Path -CNotIn $allowed)
Check 'Preserved stage-16 inputs' ($unexpected.Count -eq 0) @{ checked = $baseline.Count; unchanged = $baseline.Count - $changed.Count; changed = @($changed | ForEach-Object Path); unexpected = @($unexpected | ForEach-Object Path) }

$manifest = Get-Content 'dist/stage13-package/JobTracker-0.1.0-windows-x64.7z.manifest.json' -Raw | ConvertFrom-Json
$failed = @($checks | Where-Object result -CEQ 'FAIL')
[pscustomobject]@{
    checkedAt = [DateTimeOffset]::Now.ToString('o')
    scope = 'Static hash/size correspondence and input preservation; no build, test suite, installer or application execution.'
    version = $manifest.version
    artifacts = $artifacts
    checks = @($checks)
    passed = $checks.Count - $failed.Count
    failed = $failed.Count
} | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $output -Encoding utf8
"$($checks.Count - $failed.Count) passed; $($failed.Count) failed. $output"
if ($failed.Count) { $failed | ConvertTo-Json -Depth 6; exit 1 }
