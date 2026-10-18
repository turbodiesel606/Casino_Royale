#requires -Version 7.2
# Shared local Release validation for direct Setup and the retained legacy archive tool.
# No build, archive, installation, download, or output creation occurs here.
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
        $reader.BaseStream.Position = $offset + 24
        if ($reader.ReadUInt16() -ne 0x20b) { throw "Not a PE32+ binary: $Path" }
    } finally { $reader.Dispose() }
    $info = [Diagnostics.FileVersionInfo]::GetVersionInfo($Path)
    if ($info.IsDebug) { throw "Debug binary is not permitted: $Path" }
    return $info
}

function Get-VerifiedReleasePayload {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$BuildDirectory,
          [Parameter(Mandatory)][string]$StageDirectory)
    if (-not $IsWindows) { throw 'Windows Release validation requires Windows.' }
    $repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
    $build = [IO.Path]::GetFullPath($BuildDirectory)
    $stage = [IO.Path]::TrimEndingDirectorySeparator([IO.Path]::GetFullPath($StageDirectory))
    Assert-NoReparsePoint $build
    Assert-NoReparsePoint $stage
    [string[]]$inventory = [IO.File]::ReadAllLines((Join-Path $PSScriptRoot 'payload-files.txt'))
    foreach ($path in $inventory) { Assert-RelativePath $path }
    [Array]::Sort($inventory, [StringComparer]::Ordinal)
    if (@($inventory | Sort-Object -Unique).Count -ne $inventory.Count) { throw 'Duplicate payload inventory entry.' }

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
    $identityPath = Join-Path $build 'Installer/Release/JobTrackerIdentity.cmake'
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
            if ($item.Length -le 0) { throw "Empty payload file: $relative" }
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

    return [pscustomobject]@{
        Version = $version
        Executable = $executable
        StageDirectory = $stage
        Files = $fileRecords
    }
}
