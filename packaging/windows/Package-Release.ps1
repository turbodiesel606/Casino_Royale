#requires -Version 7.2
<#
Build a local Release payload, or verify an existing locally trusted manifest.
Never runs the application, builds, installs, downloads, deletes or publishes.
The manifest is a build handoff, not an authentication mechanism for downloads.
#>
[CmdletBinding(DefaultParameterSetName = 'Create')]
param(
    [Parameter(Mandatory, ParameterSetName = 'Create')][string]$BuildDirectory,
    [Parameter(Mandatory, ParameterSetName = 'Create')][string]$StageDirectory,
    [Parameter(Mandatory, ParameterSetName = 'Create')][string]$OutputDirectory,
    [Parameter(Mandatory, ParameterSetName = 'Verify')][string]$ManifestPath,
    [Parameter(Mandatory, ParameterSetName = 'Verify')][string]$ExpectedVersion,
    [Parameter(ParameterSetName = 'Verify')][switch]$PassThru
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$dist = Join-Path $repository 'dist'
$cmake = (Get-Command cmake -CommandType Application -ErrorAction Stop).Source
$utf8 = [Text.UTF8Encoding]::new($false)

function Assert-NoReparsePoint([string]$Path) {
    $itemPath = [IO.Path]::GetFullPath($Path)
    while ($itemPath) {
        if (Test-Path -LiteralPath $itemPath) {
            if ((Get-Item -LiteralPath $itemPath -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) {
                throw "Reparse point is not permitted: $itemPath"
            }
        }
        $itemPath = [IO.Path]::GetDirectoryName($itemPath)
    }
}

function Assert-DistPath([string]$Path) {
    if (-not $Path.StartsWith($dist + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Output and verification paths must be below this repository''s dist/ directory.'
    }
    Assert-NoReparsePoint $Path
}

function Assert-RelativePath([string]$Path) {
    # Only the portable runtime paths supported by the release inventory.
    if ($Path -cnotmatch '^(bin|plugins|qml|translations)/[A-Za-z0-9_./-]+$' -or
        @($Path.Split('/') | Where-Object { $_ -in '', '.', '..' -or $_.EndsWith('.') }).Count) {
        throw "Unsafe payload path: $Path"
    }
}

function Get-PayloadPaths([string]$Root) {
    Assert-NoReparsePoint $Root
    $pending = [Collections.Generic.Stack[string]]::new()
    $pending.Push($Root)
    $paths = [Collections.Generic.List[string]]::new()
    while ($pending.Count) {
        foreach ($item in Get-ChildItem -LiteralPath $pending.Pop() -Force) {
            if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) {
                throw "Reparse point is not permitted: $($item.FullName)"
            }
            if ($item.PSIsContainer) { $pending.Push($item.FullName); continue }
            $relative = [IO.Path]::GetRelativePath($Root, $item.FullName).Replace('\', '/')
            Assert-RelativePath $relative
            $paths.Add($relative)
        }
    }
    $paths.Sort([StringComparer]::Ordinal)
    return ,$paths.ToArray()
}

function Assert-SamePaths([string[]]$Actual, [string[]]$Expected) {
    if (($Actual -join "`n") -cne ($Expected -join "`n")) {
        throw 'Payload inventory mismatch: missing, unexpected, duplicate or incorrectly cased files.'
    }
}

function Get-Sha256([string]$Path) {
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Assert-ReleaseBinary([string]$Path) {
    $reader = [IO.BinaryReader]::new([IO.File]::OpenRead($Path))
    try {
        if ($reader.ReadUInt16() -ne 0x5a4d) { throw "Not a PE binary: $Path" }
        $reader.BaseStream.Position = 0x3c
        $offset = $reader.ReadInt32()
        if ($offset -lt 0 -or $offset -gt $reader.BaseStream.Length - 6) { throw "Invalid PE header: $Path" }
        $reader.BaseStream.Position = $offset
        if ($reader.ReadUInt32() -ne 0x4550 -or $reader.ReadUInt16() -ne 0x8664) {
            throw "Not a Windows x64 binary: $Path"
        }
    } finally { $reader.Dispose() }
    $info = [Diagnostics.FileVersionInfo]::GetVersionInfo($Path)
    if ($info.IsDebug) { throw "Debug binary is not permitted: $Path" }
    return $info
}

function Invoke-ArchiveTool([string[]]$Arguments) {
    $output = & $cmake @Arguments 2>&1
    if ($LASTEXITCODE -ne 0) { throw "CMake archive operation failed ($LASTEXITCODE): $output" }
    return $output
}

function Test-ArchivePayload([string]$Archive, [object[]]$Files, [string]$WorkDirectory) {
    [string[]]$expected = @($Files | ForEach-Object { $_.path })
    [string[]]$listed = @(Invoke-ArchiveTool @('-E', 'tar', 'tf', $Archive))
    foreach ($path in $listed) { Assert-RelativePath $path }
    [Array]::Sort($listed, [StringComparer]::Ordinal)
    Assert-SamePaths $listed $expected
    New-Item -ItemType Directory -Path $WorkDirectory -ErrorAction Stop | Out-Null
    Push-Location -LiteralPath $WorkDirectory
    try { Invoke-ArchiveTool @('-E', 'tar', 'xf', $Archive) | Out-Null }
    finally { Pop-Location }
    Assert-SamePaths (Get-PayloadPaths $WorkDirectory) $expected
    foreach ($file in $Files) {
        $path = Join-Path $WorkDirectory $file.path
        if ((Get-Item -LiteralPath $path).Length -ne $file.size -or (Get-Sha256 $path) -cne $file.sha256) {
            throw "Extracted payload mismatch: $($file.path)"
        }
    }
}

if (-not $IsWindows) { throw 'Windows Release packaging requires Windows.' }
[string[]]$inventory = [IO.File]::ReadAllLines((Join-Path $PSScriptRoot 'payload-files.txt'))
foreach ($path in $inventory) { Assert-RelativePath $path }
[Array]::Sort($inventory, [StringComparer]::Ordinal)
if (@($inventory | Sort-Object -Unique).Count -ne $inventory.Count) { throw 'Duplicate payload inventory entry.' }

if ($PSCmdlet.ParameterSetName -eq 'Verify') {
    $manifestFile = [IO.Path]::GetFullPath($ManifestPath)
    Assert-DistPath $manifestFile
    $manifest = Get-Content -LiteralPath $manifestFile -Raw | ConvertFrom-Json
    if ($ExpectedVersion -cnotmatch '^\d+\.\d+\.\d+$' -or $manifest.schemaVersion -ne 1 -or
        $manifest.version -cne $ExpectedVersion -or $manifest.product -cne 'JobTracker' -or
        $manifest.platform -cne 'windows-x64' -or $manifest.configuration -cne 'Release' -or
        $manifest.archiveRoot -cne '.' -or $manifest.executable -cne 'bin/JobTrackerApp.exe' -or
        $manifest.archive -cne "JobTracker-$ExpectedVersion-windows-x64.7z") {
        throw 'Manifest identity/version mismatch.'
    }
    Assert-SamePaths @($manifest.files | ForEach-Object { $_.path }) $inventory
    foreach ($file in $manifest.files) {
        if ($file.sha256 -cnotmatch '^[0-9a-f]{64}$' -or $file.size -le 0) { throw 'Invalid file digest/size.' }
    }
    $directory = Split-Path -Parent $manifestFile
    $archive = Join-Path $directory $manifest.archive
    Assert-NoReparsePoint $archive
    Assert-NoReparsePoint "$archive.sha256"
    if ($manifest.sha256 -cnotmatch '^[0-9a-f]{64}$' -or
        (Get-Item -LiteralPath $archive).Length -ne $manifest.size -or
        (Get-Sha256 $archive) -cne $manifest.sha256 -or
        (Get-Content -LiteralPath "$archive.sha256" -Raw).TrimEnd("`r", "`n") -cne "$($manifest.sha256)  $($manifest.archive)") {
        throw 'Archive checksum/size mismatch.'
    }
    $work = Join-Path $directory ('verify-' + [Guid]::NewGuid().ToString('N'))
    Test-ArchivePayload $archive $manifest.files $work
    if ($PassThru) {
        # Packaging consumers reuse this verified extraction without parsing console text.
        return [pscustomobject]@{ Manifest = $manifest; PayloadDirectory = $work }
    }
    Write-Output "PASS: $($manifest.archive); $($manifest.files.Count) extracted files; SHA-256 $($manifest.sha256)"
    Write-Output "Retained extraction: $work"
    return
}

$build = [IO.Path]::GetFullPath($BuildDirectory)
$stage = [IO.Path]::TrimEndingDirectorySeparator([IO.Path]::GetFullPath($StageDirectory))
$destination = [IO.Path]::GetFullPath($OutputDirectory)
Assert-NoReparsePoint $build
Assert-NoReparsePoint $stage
Assert-DistPath $destination
if (Test-Path -LiteralPath $destination) { throw 'Output directory already exists; choose a new directory. No files were replaced.' }
if ($destination.StartsWith($stage + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Output must be outside the staging tree.'
}
$cache = Get-Content -LiteralPath (Join-Path $build 'CMakeCache.txt') -Raw
if ($cache -notmatch '(?m)^CMAKE_GENERATOR_PLATFORM:INTERNAL=x64\r?$' -or
    $cache -notmatch '(?m)^JOBTRACKER_BUILD_TESTS:BOOL=OFF\r?$') {
    throw 'Packaging requires an x64 production build with tests OFF.'
}
$homeMatch = [regex]::Match($cache, '(?m)^CMAKE_HOME_DIRECTORY:INTERNAL=(.+?)\r?$')
if (-not $homeMatch.Success -or [IO.Path]::GetFullPath($homeMatch.Groups[1].Value) -ine $repository) {
    throw 'Build directory belongs to a different source checkout.'
}
# Read only literal generated fields; do not execute a supplied CMake program.
$identityPath = Join-Path $build 'packaging/windows/Release/JobTrackerIdentity.cmake'
Assert-NoReparsePoint $identityPath
$identity = Get-Content -LiteralPath $identityPath -Raw
$versionMatch = [regex]::Match($identity, '(?m)^set\(JOBTRACKER_RELEASE_VERSION "(\d+\.\d+\.\d+)"\)\r?$')
$exeMatch = [regex]::Match($identity, '(?m)^set\(JOBTRACKER_EXECUTABLE_NAME "(JobTrackerApp\.exe)"\)\r?$')
$project = Get-Content -LiteralPath (Join-Path $repository 'CMakeLists.txt') -Raw
$projectVersion = [regex]::Match($project, '(?s)project\(JobTracker\s+VERSION\s+(\d+\.\d+\.\d+)\s')
$qtVersion = [regex]::Match($project, 'set\(JOBTRACKER_QT_VERSION (\d+\.\d+\.\d+)\)')
if (-not $versionMatch.Success -or -not $exeMatch.Success -or -not $projectVersion.Success -or
    -not $qtVersion.Success -or $versionMatch.Groups[1].Value -cne $projectVersion.Groups[1].Value) {
    throw 'Missing or stale Release identity; reconfigure and rebuild.'
}
$version = $versionMatch.Groups[1].Value
$executable = $exeMatch.Groups[1].Value
Assert-SamePaths (Get-PayloadPaths $stage) $inventory
$builtExe = Join-Path $build "Release/$executable"
Assert-NoReparsePoint $builtExe
if ((Get-Sha256 (Join-Path $stage "bin/$executable")) -cne (Get-Sha256 $builtExe)) {
    throw 'Staged executable does not match the production Release build.'
}
$fileRecords = @(
    foreach ($relative in $inventory) {
        $path = Join-Path $stage $relative
        $item = Get-Item -LiteralPath $path
        if (@(Get-Item -LiteralPath $path -Stream * | Where-Object Stream -ne ':$DATA').Count) {
            throw "Alternate data stream is not permitted: $relative"
        }
        if ($item.Extension -in '.exe', '.dll') {
            $info = Assert-ReleaseBinary $path
            if ($info.ProductName -match 'Qt' -and $info.FileVersion -ne "$($qtVersion.Groups[1].Value).0") {
                throw "Qt version mismatch: $relative"
            }
            if ($relative -ceq "bin/$executable" -and
                ($info.FileVersion -cne $version -or $info.ProductVersion -cne $version -or $info.ProductName -cne 'JobTracker')) {
                throw 'Executable version/product mismatch.'
            }
        }
        if ($item.Name -eq 'qmldir' -and (Get-Content -LiteralPath $path -Raw) -match '([A-Za-z]:[\\/]|\\\\)') {
            throw "Absolute machine path in QML metadata: $relative"
        }
        [ordered]@{ path = $relative; size = $item.Length; sha256 = Get-Sha256 $path }
    }
)
if ((Get-Content -LiteralPath (Join-Path $stage 'bin/qt.conf') -Raw).Trim() -notmatch '^\[Paths\]\r?\nPrefix = \.\.$') {
    throw 'qt.conf must use the relative staging prefix.'
}

# Preserve a private snapshot and all failure evidence. Publish the manifest last.
$snapshot = Join-Path $destination 'work/payload'
New-Item -ItemType Directory -Path $snapshot | Out-Null
foreach ($file in $fileRecords) {
    $target = Join-Path $snapshot $file.path
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($target)) | Out-Null
    Copy-Item -LiteralPath (Join-Path $stage $file.path) -Destination $target
    if ((Get-Sha256 $target) -cne $file.sha256) { throw "Staging changed during packaging: $($file.path)" }
    # Normalize metadata that archive writers may include, independently of source timestamps.
    [IO.File]::SetAttributes($target, [IO.FileAttributes]::Normal)
    [IO.File]::SetCreationTimeUtc($target, [datetime]'2000-01-01T00:00:00Z')
    [IO.File]::SetLastWriteTimeUtc($target, [datetime]'2000-01-01T00:00:00Z')
    [IO.File]::SetLastAccessTimeUtc($target, [datetime]'2000-01-01T00:00:00Z')
}
$fileList = Join-Path $destination 'work/archive-files.txt'
[IO.File]::WriteAllLines($fileList, $inventory, $utf8)
$archiveName = "JobTracker-$version-windows-x64.7z"
$archive = Join-Path $destination $archiveName
Push-Location -LiteralPath $snapshot
try {
    Invoke-ArchiveTool @('-E', 'tar', 'cf', $archive, '--format=7zip', '--mtime=2000-01-01 00:00:00 UTC', "--files-from=$fileList") | Out-Null
} finally { Pop-Location }
Test-ArchivePayload $archive $fileRecords (Join-Path $destination 'work/extracted')
$hash = Get-Sha256 $archive
$manifest = [ordered]@{
    schemaVersion = 1
    product = 'JobTracker'
    version = $version
    platform = 'windows-x64'
    configuration = 'Release'
    archive = $archiveName
    archiveRoot = '.'
    executable = "bin/$executable"
    size = (Get-Item -LiteralPath $archive).Length
    sha256 = $hash
    files = $fileRecords
}
[IO.File]::WriteAllText("$archive.sha256", "$hash  $archiveName`n", $utf8)
[IO.File]::WriteAllText((Join-Path $destination "$archiveName.manifest.json"), ($manifest | ConvertTo-Json -Depth 5) + "`n", $utf8)
Write-Output "PASS: $archive; $($fileRecords.Count) extracted files; SHA-256 $hash"
Write-Output 'Manifest written last. Retained work/ is verification evidence, not a release asset.'
