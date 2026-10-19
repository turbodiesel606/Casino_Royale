#requires -Version 7.2
<# Stage the existing production Release build through CMake's install rules.
No application build, MSVC preparation, installation into the system, or cleanup.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$BuildDirectory,
    [ValidateNotNullOrEmpty()][string]$StageDirectory,
    [switch]$PassThru
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false
if (-not $IsWindows) { throw 'Windows Release staging requires Windows.' }
. (Join-Path $PSScriptRoot 'PayloadValidation.ps1')
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$release = Get-VerifiedReleaseBuild -BuildDirectory $BuildDirectory
$cmake = (Get-Command cmake -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
$work = Join-Path $repository ('build/installer-stage-' + [Guid]::NewGuid().ToString('N'))
if (-not $PSBoundParameters.ContainsKey('StageDirectory')) { $StageDirectory = Join-Path $work 'stage' }
$stage = [IO.Path]::TrimEndingDirectorySeparator($ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($StageDirectory))
if (-not $stage.StartsWith((Join-Path $repository 'build') + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Stage directory must be below this repository''s build/ directory.'
}
Assert-NoReparsePoint $stage
Assert-NoReparsePoint $work
if (Test-Path -LiteralPath $stage) { throw "Stage already exists; choose a fresh path: $stage" }
if (Test-Path -LiteralPath $work) { throw "Staging work already exists: $work" }
New-Item -ItemType Directory -Path $work | Out-Null
New-Item -ItemType Directory -Path $stage | Out-Null
$log = Join-Path $work 'install.log'
Write-Host "Stage: $stage"
Write-Host "Staging evidence: $work"
Write-Host "Install log: $log"
$arguments = @('--install', $release.BuildDirectory, '--config', 'Release', '--prefix', $stage)
$output = @(& $cmake @arguments 2>&1)
$installExitCode = $LASTEXITCODE
$output | Set-Content -LiteralPath $log -Encoding utf8
Add-Content -LiteralPath $log -Value "Exit code: $installExitCode"
if ($installExitCode -ne 0) { throw "CMake staging failed ($installExitCode). See $log." }
$verified = Get-VerifiedReleasePayload -BuildDirectory $release.BuildDirectory -StageDirectory $stage
$result = [pscustomobject]@{
    BuildDirectory = $release.BuildDirectory
    StageDirectory = $stage
    EvidenceDirectory = $work
    LogPath = $log
    Version = $verified.Version
    Files = $verified.Files
}
$result | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $work 'staging.json') -Encoding utf8
Write-Host "PASS: $stage; $($verified.Files.Count) verified Release files."
if ($PassThru) { return $result }
