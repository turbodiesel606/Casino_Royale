#requires -Version 7.2
<#
Compile a version-locked Online Installer using the stage-06 verifier.
Local mode retains the stage-07 embedded-payload fixture.
No download, installation, application launch, publication or cleanup occurs.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$BuildDirectory,
    [Parameter(Mandatory)][string]$ManifestPath,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [Parameter(Mandatory)][string]$IsccPath,
    [string]$RedistributablePath,
    [ValidateSet('Online', 'Local')][string]$Mode = 'Online'
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if (-not $IsWindows) { throw 'The Windows installer requires Windows.' }
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$build = [IO.Path]::GetFullPath($BuildDirectory)
$destination = [IO.Path]::GetFullPath($OutputDirectory)
$distPrefix = (Join-Path $repository 'dist') + [IO.Path]::DirectorySeparatorChar
if (-not $destination.StartsWith($distPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Installer output must be below this repository''s dist/ directory.'
}
if (Test-Path -LiteralPath $destination) { throw 'Output directory already exists; choose a new directory.' }
$ancestor = $destination
while ($ancestor) {
    if ((Test-Path -LiteralPath $ancestor) -and
        ((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw "Reparse point is not permitted in the output path: $ancestor"
    }
    $ancestor = [IO.Path]::GetDirectoryName($ancestor)
}
$compiler = (Get-Item -LiteralPath $IsccPath -ErrorAction Stop).FullName
$identityFile = Join-Path $build 'packaging/windows/Release/JobTrackerIdentity.iss'
$identity = Get-Content -LiteralPath $identityFile -Raw
$versionMatch = [regex]::Match($identity, '(?m)^#define JobTrackerReleaseVersion "(\d+\.\d+\.\d+)"\r?$')
$project = Get-Content -LiteralPath (Join-Path $repository 'CMakeLists.txt') -Raw
$projectVersion = [regex]::Match($project, '(?s)project\(JobTracker\s+VERSION\s+(\d+\.\d+\.\d+)\s')
if (-not $versionMatch.Success -or -not $projectVersion.Success -or
    $versionMatch.Groups[1].Value -cne $projectVersion.Groups[1].Value) {
    throw 'Missing or stale generated Release identity; reconfigure and rebuild.'
}
$version = $versionMatch.Groups[1].Value
$verified = & (Join-Path $PSScriptRoot 'Package-Release.ps1') -ManifestPath $ManifestPath -ExpectedVersion $version -PassThru
$manifest = $verified.Manifest
$builtExecutable = Join-Path $build 'Release/JobTrackerApp.exe'
$exeRecord = @($manifest.files | Where-Object path -CEQ 'bin/JobTrackerApp.exe')
if ($exeRecord.Count -ne 1 -or
    (Get-FileHash -LiteralPath $builtExecutable -Algorithm SHA256).Hash.ToLowerInvariant() -cne $exeRecord[0].sha256) {
    throw 'Archive executable does not match the selected Release build.'
}
New-Item -ItemType Directory -Path $destination | Out-Null
if (-not $RedistributablePath) {
    $cache = Get-Content -LiteralPath (Join-Path $build 'CMakeCache.txt') -Raw
    $instance = [regex]::Match($cache, '(?m)^CMAKE_GENERATOR_INSTANCE:INTERNAL=(.+?)\r?$')
    if (-not $instance.Success) { throw 'Visual Studio instance is missing from the build cache.' }
    $RedistributablePath = Join-Path $instance.Groups[1].Value 'VC/Redist/MSVC/v143/vc_redist.x64.exe'
}
$runtime = & (Join-Path $PSScriptRoot 'Prepare-MsvcRuntime.ps1') -BuildDirectory $build `
    -RedistributablePath $RedistributablePath -OutputDirectory (Join-Path $destination 'msvc-runtime') -PassThru
# Only locally verified inputs enter the compiled trust anchor. There is no URL,
# digest or release-tag override and no downloaded manifest is ever trusted.
$archiveName = "JobTracker-$version-windows-x64.7z"
$releaseUrl = "https://github.com/turbodiesel606/Releases/releases/download/v$version/$archiveName"
$payloadFile = Join-Path $destination 'JobTrackerPayload.iss'
$payload = [Collections.Generic.List[string]]::new()
$payload.Add('// Generated from the verified local archive. Do not edit.')
$payload.Add('#define JobTrackerArchiveSha256 "' + $manifest.sha256 + '"')
$payload.Add('#define JobTrackerArchiveSize ' + $manifest.size)
$payload.Add('[Files]')
foreach ($file in $manifest.files) {
    $relative = $file.path.Replace('/', '\')
    $parent = [IO.Path]::GetDirectoryName($relative)
    if ($Mode -eq 'Local') {
        $source = Join-Path $verified.PayloadDirectory $relative
        $payload.Add(('Source: "{0}"; DestDir: "{{app}}\{1}"; Flags: ignoreversion; Check: CanInstallWithManualClose(ExpandConstant(''{{app}}\{2}''))' -f $source, $parent, $relative))
    } else {
        $payload.Add(('Source: "{{tmp}}\payload\{0}"; DestDir: "{{app}}\{1}"; ExternalSize: {2}; Hash: "{3}"; Flags: external ignoreversion; Check: CanInstallWithManualClose(ExpandConstant(''{{app}}\{0}''))' -f $relative, $parent, $file.size, $file.sha256))
    }
}
$payload.Add('[Code]')
$payload.Add('procedure ValidatePayloadDestination;')
$payload.Add('begin')
foreach ($file in $manifest.files) {
    $payload.Add(('  RequireSafePayloadPath(ExpandConstant(''{{app}}\{0}''));' -f $file.path.Replace('/', '\')))
}
$payload.Add('  RequireSafePayloadPath(ExpandConstant(''{app}\JobTracker.install.ini''));')
$payload.Add('end;')
$payload.Add('procedure ValidateOnlinePayload;')
$payload.Add('begin')
foreach ($file in $manifest.files) {
    $relative = $file.path.Replace('/', '\')
    $payload.Add(('  VerifyPayloadFile(ExpandConstant(''{{tmp}}\payload\{0}''), ''{1}'', {2});' -f $relative, $file.sha256, $file.size))
}
$payload.Add('  ValidatePayloadDestination;')
$payload.Add('end;')
$payload | Set-Content -LiteralPath $payloadFile -Encoding utf8
$compilerArguments = @(
    "/DJobTrackerIdentityFile=$identityFile"
    "/DJobTrackerOutputDirectory=$destination"
    "/DJobTrackerMsvcFile=$($runtime.IncludePath)"
    "/DJobTrackerPayloadFile=$payloadFile"
)
if ($Mode -eq 'Local') {
    $compilerArguments += '/DJobTrackerLocalPayload'
    $compilerArguments += "/DJobTrackerPayloadDirectory=$($verified.PayloadDirectory)"
}
$compilerArguments += (Join-Path $PSScriptRoot 'JobTracker.iss')
$compilerOutput = @(& $compiler @compilerArguments 2>&1)
$compilerExitCode = $LASTEXITCODE
$compilerOutput | Set-Content -LiteralPath (Join-Path $destination 'compile.log') -Encoding utf8
if ($compilerExitCode -ne 0) {
    throw "Inno Setup compilation failed ($compilerExitCode). See $destination/compile.log."
}
$installer = Join-Path $destination "JobTracker-$version-windows-x64-$($Mode.ToLowerInvariant())-setup.exe"
$installerItem = Get-Item -LiteralPath $installer
$installerInfo = $installerItem.VersionInfo
if ($installerInfo.ProductName.Trim() -cne 'JobTracker' -or $installerInfo.ProductVersion.Trim() -cne $version -or
    $installerInfo.FileVersion.Trim() -cne "$version.0") {
    throw 'Compiled installer version/product mismatch.'
}
[ordered]@{
    kind = if ($Mode -eq 'Online') { 'version-locked-online' } else { 'local-payload-fixture' }
    version = $version
    tag = "v$version"
    archive = $archiveName
    url = if ($Mode -eq 'Online') { $releaseUrl } else { $null }
    installer = $installerItem.Name
    size = $installerItem.Length
    sha256 = (Get-FileHash -LiteralPath $installer -Algorithm SHA256).Hash.ToLowerInvariant()
    archiveSha256 = $manifest.sha256
    payloadFileCount = $manifest.files.Count
    verifiedPayloadDirectory = $verified.PayloadDirectory
    compiler = $compiler
    compilerSha256 = (Get-FileHash -LiteralPath $compiler -Algorithm SHA256).Hash.ToLowerInvariant()
    scriptSha256 = (Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'JobTracker.iss') -Algorithm SHA256).Hash.ToLowerInvariant()
    onlinePayloadScriptSha256 = (Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'OnlinePayload.iss') -Algorithm SHA256).Hash.ToLowerInvariant()
    generatedPayloadSha256 = (Get-FileHash -LiteralPath $payloadFile -Algorithm SHA256).Hash.ToLowerInvariant()
    msvcRuntime = $runtime.Evidence
    msvcScriptSha256 = (Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'MsvcRuntime.iss') -Algorithm SHA256).Hash.ToLowerInvariant()
    installationPolicySha256 = (Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'InstallationPolicy.iss') -Algorithm SHA256).Hash.ToLowerInvariant()
    manualCloseSha256 = (Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'ManualClose.iss') -Algorithm SHA256).Hash.ToLowerInvariant()
    personalDataSha256 = (Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'PersonalData.iss') -Algorithm SHA256).Hash.ToLowerInvariant()
} | ConvertTo-Json -Depth 7 | Set-Content -LiteralPath (Join-Path $destination 'installer-build.json') -Encoding utf8
Write-Output "PASS: $installer; $Mode; $($manifest.files.Count) verified payload files; archive SHA-256 $($manifest.sha256)."
