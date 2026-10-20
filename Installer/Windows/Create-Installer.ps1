#requires -Version 7.2
<# Configure/build the current Release application, then use the existing offline
staging and installer builders. Never installs, launches, downloads, or cleans.
#>
[CmdletBinding()]
param(
    [ValidateNotNullOrEmpty()][string]$ConfigurePreset,
    [ValidateNotNullOrEmpty()][string]$QtRoot,
    [ValidateNotNullOrEmpty()][string]$IsccPath,
    [ValidateNotNullOrEmpty()][string]$RedistributablePath,
    [ValidateRange(1, 1024)][int]$Parallel,
    [ValidateNotNullOrEmpty()][string]$ConfigFile,
    [switch]$PassThru,
    [switch]$Help
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false
if ($Help) {
    Write-Host @'
Create-Installer.ps1 [-ConfigurePreset name] [-QtRoot kit] [-IsccPath ISCC.exe]
  [-RedistributablePath vc_redist.x64.exe] [-Parallel jobs] [-ConfigFile file]
  [-PassThru] [-Help]
One-time optional setup: copy Create-Installer.example.psd1 to
Create-Installer.local.psd1 and fill only required overrides. CLI > local config
> existing preset/environment defaults. See Installer/README.md.
'@
    return
}
if (-not $IsWindows -or [Runtime.InteropServices.RuntimeInformation]::OSArchitecture -ne 'X64') {
    throw 'Installer creation requires native x64 Windows and PowerShell 7.2+.'
}
. (Join-Path $PSScriptRoot 'PayloadValidation.ps1')
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
if (-not $ConfigFile) { $ConfigFile = Join-Path $PSScriptRoot 'Create-Installer.local.psd1' }
$settings = @{}
if (Test-Path -LiteralPath $ConfigFile) {
    $settings = Import-PowerShellDataFile -LiteralPath $ConfigFile
    foreach ($key in $settings.Keys) {
        if ($key -notin 'ConfigurePreset', 'QtRoot', 'IsccPath', 'RedistributablePath', 'Parallel') {
            throw "Unknown local installer setting '$key': $ConfigFile"
        }
    }
} elseif ($PSBoundParameters.ContainsKey('ConfigFile')) {
    throw "Local configuration does not exist: $ConfigFile. Copy Installer/Windows/Create-Installer.example.psd1 and fill the needed overrides."
}
foreach ($key in @('ConfigurePreset', 'QtRoot', 'IsccPath', 'RedistributablePath', 'Parallel')) {
    if (-not $PSBoundParameters.ContainsKey($key) -and $settings.ContainsKey($key) -and $settings[$key]) {
        Set-Variable -Name $key -Value $settings[$key]
    }
}
if ($Parallel -and ($Parallel -lt 1 -or $Parallel -gt 1024)) { throw 'Parallel must be between 1 and 1024.' }
foreach ($key in @('QtRoot', 'IsccPath', 'RedistributablePath')) {
    $value = Get-Variable -Name $key -ValueOnly
    if ($value) { Set-Variable -Name $key -Value ([IO.Path]::GetFullPath($value, $repository)) }
}
$cmake = (Get-Command cmake -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
if (-not $IsccPath) {
    $command = Get-Command ISCC.exe -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $command) { throw 'ISCC.exe is missing. Add Inno Setup 7+ to PATH or set IsccPath once in Create-Installer.local.psd1.' }
    $IsccPath = $command.Source
}
Assert-NoReparsePoint $IsccPath
$compiler = Get-Item -LiteralPath $IsccPath
if ($compiler.PSIsContainer -or $compiler.Name -ine 'ISCC.exe') {
    throw "Select an existing Inno Setup 7+ ISCC.exe: $IsccPath"
}
if ($RedistributablePath -and -not (Test-Path -LiteralPath $RedistributablePath -PathType Leaf)) {
    throw "Local redistributable is missing: $RedistributablePath. Set RedistributablePath once in Create-Installer.local.psd1; nothing will be downloaded."
}
$run = Join-Path $repository ('build/installer-create/windows-' + [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssZ') + '-' + [Guid]::NewGuid().ToString('N'))
Assert-NoReparsePoint $run
$null = [IO.Directory]::CreateDirectory($run)
Write-Host "Launcher logs: $run"
$step = 'resolve preset and configuration'
$lock = $null
$savedEnvironment = @{}
foreach ($key in @('JOBTRACKER_CREATE_PRESET', 'JOBTRACKER_CREATE_QT', 'JOBTRACKER_CREATE_RESULT')) {
    $savedEnvironment[$key] = [Environment]::GetEnvironmentVariable($key, 'Process')
}
function Invoke-CreateStep([string]$Name, [string[]]$Arguments) {
    $log = Join-Path $run "$Name.log"
    Write-Host "[$Name] cmake $($Arguments -join ' ')"
    "Command: $cmake $($Arguments -join ' ')" | Set-Content -LiteralPath $log -Encoding utf8
    $output = @(& $cmake @Arguments 2>&1)
    $code = $LASTEXITCODE
    $output | Add-Content -LiteralPath $log -Encoding utf8
    Add-Content -LiteralPath $log -Value "Exit code: $code"
    $output | ForEach-Object { Write-Host $_ }
    if ($code -ne 0) { throw "Step '$Name' failed (exit $code). See $log" }
}
function Read-CreateResolution {
    $record = @{}
    foreach ($line in Get-Content -LiteralPath (Join-Path $run 'preset.txt')) {
        $parts = $line.Split('=', 2)
        if ($parts.Count -ne 2) { throw 'Invalid preset result record.' }
        $record[$parts[0]] = $parts[1]
    }
    return $record
}
Push-Location -LiteralPath $repository
try {
    $env:JOBTRACKER_CREATE_PRESET = $ConfigurePreset
    $env:JOBTRACKER_CREATE_QT = $QtRoot
    $env:JOBTRACKER_CREATE_RESULT = Join-Path $run 'preset.txt'
    $resolver = Join-Path $repository 'Installer/Resolve-CreatePreset.cmake'
    Invoke-CreateStep 'resolve' @('-P', $resolver)
    $resolved = Read-CreateResolution
    foreach ($key in $resolved.Keys) {
        if ($key -match '^(env|unset)\.JOBTRACKER_CREATE_') {
            throw 'Preset environment must not override reserved JOBTRACKER_CREATE_* launcher inputs.'
        }
    }
    Assert-NoReparsePoint $resolved.build
    $null = [IO.Directory]::CreateDirectory($resolved.build)
    $step = 'acquire build-tree lock'
    try {
        $lock = [IO.File]::Open((Join-Path $resolved.build '.jobtracker-installer.lock'), [IO.FileMode]::OpenOrCreate, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
    } catch { throw "Cannot acquire exclusive installer launcher lock for $($resolved.build). Another launcher may be running. $($_.Exception.Message)" }
    # Recheck cache compatibility after acquiring the build-tree lock.
    Invoke-CreateStep 'resolve-locked' @('-P', $resolver)
    $step = 'configure'
    Invoke-CreateStep 'configure' @('--preset', $resolved.preset, "-DCMAKE_PREFIX_PATH:PATH=$($resolved.qt)", '-DJOBTRACKER_BUILD_TESTS:BOOL=OFF')
    Invoke-CreateStep 'verify-configured-cache' @('-P', $resolver)
    $step = 'build'
    $arguments = @('--build', $resolved.build, '--config', 'Release', '--target', 'JobTrackerApp')
    if ($resolved.buildPreset) {
        $arguments = @('--build', '--preset', $resolved.buildPreset, '--config', 'Release', '--target', 'JobTrackerApp')
    } else {
        # Without a paired build preset, retain the configure preset's effective
        # environment for the exact-directory build.
        foreach ($key in $resolved.Keys) {
            if ($key -match '^(env|unset)\.(.+)$') {
                $name = $Matches[2]
                if (-not $savedEnvironment.ContainsKey($name)) { $savedEnvironment[$name] = [Environment]::GetEnvironmentVariable($name, 'Process') }
                $value = $resolved[$key]
                if ($key.StartsWith('unset.')) { $value = $null }
                [Environment]::SetEnvironmentVariable($name, $value, 'Process')
            }
        }
    }
    if ($Parallel) { $arguments += @('--parallel', "$Parallel") }
    Invoke-CreateStep 'build' $arguments
    if (-not $resolved.buildPreset) {
        foreach ($key in $resolved.Keys) {
            if ($key -match '^(env|unset)\.(.+)$') {
                $name = $Matches[2]
                [Environment]::SetEnvironmentVariable($name, $savedEnvironment[$name], 'Process')
            }
        }
    }
    $step = 'stage'
    Write-Host '[stage] Stage-Release.ps1'
    $stage = & (Join-Path $PSScriptRoot 'Stage-Release.ps1') -BuildDirectory $resolved.build -PassThru
    $step = 'package'
    Write-Host '[package] Build-Installer.ps1'
    $builder = @{BuildDirectory=$resolved.build; StageDirectory=$stage.StageDirectory; IsccPath=$IsccPath; TimestampedOutput=$true; PassThru=$true}
    if ($RedistributablePath) { $builder.RedistributablePath = $RedistributablePath }
    $result = & (Join-Path $PSScriptRoot 'Build-Installer.ps1') @builder
    $result | Add-Member -NotePropertyMembers @{
        BuildDirectory = $resolved.build
        StageDirectory = $stage.StageDirectory
        StagingEvidenceDirectory = $stage.EvidenceDirectory
        LauncherLogDirectory = $run
    }
    $result | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $run 'result.json') -Encoding utf8
    Write-Host "PASS: JobTracker $($result.Version)"
    Write-Host "Installer: $($result.InstallerPath)"
    Write-Host "Evidence: $($result.MetadataPath)"
    Write-Host "Logs/result: $run"
    if ($PassThru) { return $result }
} catch {
    "Failed step: $step`n$($_ | Out-String)" | Set-Content -LiteralPath (Join-Path $run 'failure.log') -Encoding utf8
    throw "Installer creation failed during '$step'. Logs: $run. $($_.Exception.Message)"
} finally {
    if ($lock) { $lock.Dispose() }
    foreach ($key in $savedEnvironment.Keys) { [Environment]::SetEnvironmentVariable($key, $savedEnvironment[$key], 'Process') }
    Pop-Location
}
