#requires -Version 7.2
<#
Compile the single offline Setup from a verified direct Release stage.
Never builds, installs, downloads, launches JobTracker, publishes or cleans.
Private snapshots/logs stay under build/, outside Installer/Windows/output.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$BuildDirectory,
    [Parameter(Mandatory)][string]$StageDirectory,
    [ValidateNotNullOrEmpty()][string]$IsccPath,
    [string]$RedistributablePath,
    [switch]$TimestampedOutput,
    [switch]$PassThru
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false
if (-not $IsWindows) { throw 'The Windows installer requires Windows.' }
. (Join-Path $PSScriptRoot 'PayloadValidation.ps1')
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$build = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($BuildDirectory)
$run = [Guid]::NewGuid().ToString('N')
$destination = Join-Path $PSScriptRoot 'output'
if (-not $TimestampedOutput) { $destination = Join-Path $destination $run }
Assert-NoReparsePoint $destination
if (-not $PSBoundParameters.ContainsKey('IsccPath')) {
    $IsccPath = (Get-Command ISCC.exe -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
}
Assert-NoReparsePoint $IsccPath
$compilerItem = Get-Item -LiteralPath $IsccPath -ErrorAction Stop
if ($compilerItem.PSIsContainer -or $compilerItem.Name -ine 'ISCC.exe') { throw 'IsccPath must identify an existing ISCC.exe.' }
$compiler = $compilerItem.FullName
$verified = Get-VerifiedReleasePayload -BuildDirectory $build -StageDirectory $StageDirectory
$version = $verified.Version
$identityFile = Join-Path $build 'Installer/Release/JobTrackerIdentity.iss'
Assert-NoReparsePoint $identityFile
$identity = Get-Content -LiteralPath $identityFile -Raw
$versionMatch = [regex]::Match($identity, '(?m)^#define JobTrackerReleaseVersion "(\d+\.\d+\.\d+)"\r?$')
if (-not $versionMatch.Success -or $versionMatch.Groups[1].Value -cne $version -or
    $identity -notmatch '(?m)^#define JobTrackerExecutableName "JobTrackerApp\.exe"\r?$' -or
    $identity -notmatch '(?m)^#define JobTrackerProductName "JobTracker"\r?$' -or
    $identity -notmatch ('(?m)^#define JobTrackerWindowsVersion "' + [regex]::Escape("$version.0") + '"\r?$')) {
    throw 'Missing or stale generated Release Inno identity; reconfigure and rebuild.'
}
$installerName = "JobTracker-Setup-$version.exe"
if ($TimestampedOutput) {
    $installerName = "JobTracker-Setup-$version-$([DateTime]::UtcNow.ToString('yyyyMMddTHHmmssZ'))-$($run.Substring(0, 12)).exe"
}
$installer = Join-Path $destination $installerName
$work = Join-Path $repository "build/installer/$run"
Assert-NoReparsePoint $work
foreach ($path in @($work, $destination, $identityFile)) {
    if ($path -match '["{}\r\n]') { throw "Path contains unsupported Inno metacharacters: $path" }
    if ($path.StartsWith($verified.StageDirectory + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase) -or
        $path -ieq $verified.StageDirectory) { throw 'Installer output/work must be outside the staging tree.' }
}
$newDirectories = @($work)
if (-not $TimestampedOutput) { $newDirectories += $destination }
foreach ($path in $newDirectories) {
    if (Test-Path -LiteralPath $path) { throw "Output already exists; choose a fresh run: $path" }
}
New-Item -ItemType Directory -Path $work | Out-Null
$null = [IO.Directory]::CreateDirectory($destination)
$compileDirectory = Join-Path $work 'compiler-output'
$null = [IO.Directory]::CreateDirectory($compileDirectory)
$compiledInstaller = Join-Path $compileDirectory "JobTracker-Setup-$version.exe"
Write-Host "Setup output: $installer"
Write-Host "Private build evidence: $work"
Write-Host "Compiler log: $(Join-Path $work 'compile.log')"
if (-not $RedistributablePath) {
    $cache = Get-Content -LiteralPath (Join-Path $build 'CMakeCache.txt') -Raw
    $instance = [regex]::Match($cache, '(?m)^CMAKE_GENERATOR_INSTANCE:INTERNAL=(.+?)\r?$')
    if (-not $instance.Success) { throw 'Visual Studio instance is missing from the build cache.' }
    $RedistributablePath = Join-Path $instance.Groups[1].Value 'VC/Redist/MSVC/v143/vc_redist.x64.exe'
}
$RedistributablePath = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($RedistributablePath)
$runtime = & (Join-Path $PSScriptRoot 'Prepare-MsvcRuntime.ps1') -BuildDirectory $build -RedistributablePath $RedistributablePath -OutputDirectory (Join-Path $work 'msvc-runtime') -PassThru

# Snapshot only the exact reviewed files. Hash: also binds ISCC's bytes to validation.
$snapshot = Join-Path $work 'payload'
$payload = [Collections.Generic.List[string]]::new()
$payload.Add('// Generated from verified direct local staging. Do not edit.')
$payload.Add('[Files]')
foreach ($file in $verified.Files) {
    $relative = $file.path.Replace('/', '\')
    $source = Join-Path $snapshot $relative
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($source)) | Out-Null
    Copy-Item -LiteralPath (Join-Path $verified.StageDirectory $relative) -Destination $source
    if ((Get-Sha256 $source) -cne $file.sha256) { throw "Staging changed during snapshot: $relative" }
    $parent = [IO.Path]::GetDirectoryName($relative)
    $payload.Add(('Source: "{0}"; DestDir: "{{app}}\{1}"; Hash: "{2}"; Flags: ignoreversion; Check: CanInstallWithManualClose(ExpandConstant(''{{app}}\{3}''))' -f $source, $parent, $file.sha256, $relative))
}
$payload.Add('[Code]')
$payload.Add('procedure ValidatePayloadDestination;')
$payload.Add('begin')
foreach ($file in $verified.Files) {
    $payload.Add(('  RequireSafePayloadPath(ExpandConstant(''{{app}}\{0}''));' -f $file.path.Replace('/', '\')))
}
$payload.Add('  RequireSafePayloadPath(ExpandConstant(''{app}\JobTracker.install.ini''));')
$payload.Add('  RequireSafePayloadPath(ExpandConstant(''{app}\licenses\MSVC-NOTICE.txt''));')
$payload.Add('end;')
$payloadFile = Join-Path $work 'JobTrackerPayload.iss'
$payload | Set-Content -LiteralPath $payloadFile -Encoding utf8
$compilerArguments = @(
    "/DJobTrackerIdentityFile=$identityFile"
    "/DJobTrackerOutputDirectory=$compileDirectory"
    "/DJobTrackerMsvcFile=$($runtime.IncludePath)"
    "/DJobTrackerPayloadFile=$payloadFile"
    (Join-Path $PSScriptRoot 'JobTracker.iss')
)
$compilerOutput = @(& $compiler @compilerArguments 2>&1)
$compilerExitCode = $LASTEXITCODE
$compilerOutput | Set-Content -LiteralPath (Join-Path $work 'compile.log') -Encoding utf8
Add-Content -LiteralPath (Join-Path $work 'compile.log') -Value "Exit code: $compilerExitCode"
if ($compilerExitCode -ne 0) {
    throw "Inno Setup compilation failed ($compilerExitCode). See $work/compile.log."
}
$installerItem = Get-Item -LiteralPath $compiledInstaller
$null = Assert-ReleaseBinary $compiledInstaller
$installerInfo = $installerItem.VersionInfo
if ($installerInfo.ProductName.Trim() -cne 'JobTracker' -or $installerInfo.ProductVersion.Trim() -cne $version -or
    $installerInfo.FileVersion.Trim() -cne "$version.0") { throw 'Compiled installer version/product mismatch.' }
$sourceHashes = [ordered]@{}
foreach ($name in @('JobTracker.iss','Build-Installer.ps1','PayloadValidation.ps1','PayloadSafety.iss',
        'MsvcRuntime.iss','Prepare-MsvcRuntime.ps1','MsvcRuntime.lock.json','MSVC-NOTICE.txt',
        'InstallationPolicy.iss','ManualClose.iss','PersonalData.iss','payload-files.txt')) {
    $sourceHashes[$name] = Get-Sha256 (Join-Path $PSScriptRoot $name)
}
[ordered]@{
    kind = 'offline'
    version = $version
    installer = $installer
    installerFileName = $installerName
    size = $installerItem.Length
    sha256 = Get-Sha256 $compiledInstaller
    payloadFileCount = $verified.Files.Count
    files = $verified.Files
    stageDirectory = $verified.StageDirectory
    payloadDirectory = $snapshot
    compiler = $compiler
    compilerSha256 = Get-Sha256 $compiler
    identitySha256 = Get-Sha256 $identityFile
    sourceHashes = $sourceHashes
    generatedPayloadSha256 = Get-Sha256 $payloadFile
    msvcRuntime = $runtime.Evidence
} | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $work 'installer-build.json') -Encoding utf8
# Atomic, non-overwriting publication of the already validated bytes. Inno never
# writes a partial/unvalidated executable into the shared final output directory.
[IO.File]::Move($compiledInstaller, $installer, $false)
Write-Host "PASS: $installer; $($verified.Files.Count) embedded application files; verified local MSVC inputs."
Write-Host "Private build evidence: $work"
if ($PassThru) {
    return [pscustomobject]@{
        InstallerPath = $installer
        EvidenceDirectory = $work
        MetadataPath = Join-Path $work 'installer-build.json'
        CompilerPath = $compiler
        Version = $version
    }
}
