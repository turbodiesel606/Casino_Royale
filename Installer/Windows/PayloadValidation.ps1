#requires -Version 7.2
# Shared local Release validation for staging and offline Setup.
# No build, archive, installation, download, or output creation occurs here.
function Assert-NoReparsePoint([string]$Path) {
    $itemPath = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Path)
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
    $actualCounts = [Collections.Generic.Dictionary[string, int]]::new([StringComparer]::Ordinal)
    $expectedCounts = [Collections.Generic.Dictionary[string, int]]::new([StringComparer]::Ordinal)
    $diagnostics = [Collections.Generic.List[string]]::new()
    foreach ($side in @(
        @{ Name = 'actual'; Paths = $Actual; Counts = $actualCounts },
        @{ Name = 'expected'; Paths = $Expected; Counts = $expectedCounts }
    )) {
        $caseNames = [Collections.Generic.Dictionary[string, string]]::new([StringComparer]::OrdinalIgnoreCase)
        foreach ($path in $side.Paths) {
            if ($side.Counts.ContainsKey($path)) { $side.Counts[$path]++ }
            else { $side.Counts.Add($path, 1) }
            if (-not $caseNames.TryAdd($path, $path) -and $caseNames[$path] -cne $path) {
                $diagnostics.Add("Case-insensitive duplicate in $($side.Name): '$($caseNames[$path])' and '$path'.")
            }
        }
        foreach ($entry in $side.Counts.GetEnumerator()) {
            if ($entry.Value -gt 1) {
                $diagnostics.Add("Duplicate payload inventory entry in $($side.Name): '$($entry.Key)' ($($entry.Value) occurrences).")
            }
        }
    }

    $missing = @($expectedCounts.Keys | Where-Object { -not $actualCounts.ContainsKey($_) })
    $unexpected = @($actualCounts.Keys | Where-Object { -not $expectedCounts.ContainsKey($_) })
    foreach ($path in $missing) {
        $diagnostics.Add("Missing: '$path'.")
        foreach ($actualPath in $unexpected) {
            if ([StringComparer]::OrdinalIgnoreCase.Equals($path, $actualPath)) {
                $diagnostics.Add("Case difference: expected '$path', actual '$actualPath'.")
            }
        }
    }
    foreach ($path in $unexpected) { $diagnostics.Add("Unexpected: '$path'.") }

    # Preserve strict ordered comparison even when the path counts match.
    if (($Actual -join "`n") -cne ($Expected -join "`n") -and $diagnostics.Count -eq 0) {
        $diagnostics.Add('Path order differs; expected the ordinal inventory order.')
    }
    if ($diagnostics.Count) {
        throw ("Payload inventory mismatch:`n" + ($diagnostics -join "`n"))
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

function Get-VerifiedReleaseBuild {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$BuildDirectory)
    if (-not $IsWindows) { throw 'Windows Release validation requires Windows.' }
    $repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
    $build = [IO.Path]::TrimEndingDirectorySeparator($ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($BuildDirectory))
    Assert-NoReparsePoint $build
    $cache = Get-Content -LiteralPath (Join-Path $build 'CMakeCache.txt') -Raw
    if ($cache -notmatch '(?m)^CMAKE_GENERATOR:INTERNAL=Visual Studio 17 2022\r?$' -or
        $cache -notmatch '(?m)^CMAKE_GENERATOR_PLATFORM:INTERNAL=x64\r?$' -or
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
    $builtExe = Join-Path $build "Release/$executable"
    Assert-NoReparsePoint $builtExe
    $info = Assert-ReleaseBinary $builtExe
    if ($info.FileVersion -cne $version -or $info.ProductVersion -cne $version -or $info.ProductName -cne 'JobTracker') {
        throw 'Executable version/product mismatch.'
    }
    return [pscustomobject]@{
        BuildDirectory = $build
        Version = $version
        Executable = $executable
        QtVersion = $qtVersion.Groups[1].Value
    }
}

function Get-VerifiedReleasePayload {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$BuildDirectory,
          [Parameter(Mandatory)][string]$StageDirectory)
    $release = Get-VerifiedReleaseBuild -BuildDirectory $BuildDirectory
    $build = $release.BuildDirectory
    $version = $release.Version
    $executable = $release.Executable
    $stage = [IO.Path]::TrimEndingDirectorySeparator($ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($StageDirectory))
    Assert-NoReparsePoint $build
    Assert-NoReparsePoint $stage
    [string[]]$inventory = [IO.File]::ReadAllLines((Join-Path $PSScriptRoot 'payload-files.txt'))
    foreach ($path in $inventory) { Assert-RelativePath $path }
    [Array]::Sort($inventory, [StringComparer]::Ordinal)
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
                if ($info.ProductName -match 'Qt' -and $info.FileVersion -ne "$($release.QtVersion).0") {
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
