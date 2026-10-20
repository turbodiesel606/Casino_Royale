#requires -Version 7.2
<# Direct-stage boundary checks. Uses private copies; never installs or launches. #>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$BuildDirectory,
    [Parameter(Mandatory)][string]$StageDirectory,
    [Parameter(Mandatory)][string]$WorkDirectory
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
. (Join-Path $repository 'Installer/Windows/PayloadValidation.ps1')
$work = [IO.Path]::GetFullPath($WorkDirectory)
if (-not $work.StartsWith((Join-Path $repository 'build') + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Private payload tests must stay below this repository''s build/ directory.'
}
Assert-NoReparsePoint $work
if (Test-Path -LiteralPath $work) { throw 'Test output already exists; choose a fresh directory.' }
$original = Get-VerifiedReleasePayload -BuildDirectory $BuildDirectory -StageDirectory $StageDirectory
New-Item -ItemType Directory -Path $work | Out-Null
$results = [Collections.Generic.List[object]]::new()
function Record-Pass([string]$Name, [string]$Evidence) {
    $results.Add([ordered]@{name=$Name;result='PASS';evidence=$Evidence})
    $results | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $work 'results.json')
    Write-Output "PASS: $Name"
}
function New-PayloadCopy([string]$Name, [string]$Omit = '') {
    $root = Join-Path $work $Name
    foreach ($file in $original.Files) {
        if ($file.path -ceq $Omit) { continue }
        $target = Join-Path $root $file.path
        [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($target)) | Out-Null
        Copy-Item -LiteralPath (Join-Path $original.StageDirectory $file.path) -Destination $target
    }
    return $root
}
function Assert-Rejected([string]$Name, [scriptblock]$Action, [string]$Pattern) {
    $message = ''
    try { & $Action | Out-Null } catch { $message = $_.Exception.Message }
    if ($message -notmatch $Pattern) { throw "${Name}: expected $Pattern; got '$message'." }
    Record-Pass $Name $message
}
function New-BuildCopy([string]$Name) {
    $root = Join-Path $work $Name
    foreach ($relative in @('CMakeCache.txt','Installer/Release/JobTrackerIdentity.cmake','Release/JobTrackerApp.exe')) {
        $target = Join-Path $root $relative
        [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($target)) | Out-Null
        Copy-Item -LiteralPath (Join-Path $BuildDirectory $relative) -Destination $target
    }
    return $root
}

Record-Pass 'valid-direct-stage' "$($original.Files.Count) exact files, production identity $($original.Version)"
$caseStage = Join-Path $work 'missing-stage'
Assert-Rejected 'missing-stage' { Get-VerifiedReleasePayload $BuildDirectory $caseStage } 'does not exist|Cannot find'
$caseStage = New-PayloadCopy 'missing-sqlite' 'plugins/sqldrivers/qsqlite.dll'
Assert-Rejected 'missing-sqlite' { Get-VerifiedReleasePayload $BuildDirectory $caseStage } 'inventory mismatch'
$caseStage = New-PayloadCopy 'missing-platform' 'plugins/platforms/qwindows.dll'
Assert-Rejected 'missing-platform' { Get-VerifiedReleasePayload $BuildDirectory $caseStage } 'inventory mismatch'
$caseStage = New-PayloadCopy 'unexpected-file'
Set-Content -LiteralPath (Join-Path $caseStage 'bin/private.pdb') -Value 'not a customer artifact'
Assert-Rejected 'unexpected-file' { Get-VerifiedReleasePayload $BuildDirectory $caseStage } 'inventory mismatch'
foreach ($extra in @('source.cpp', 'header.hpp', 'build.cmake', 'Qt6Test.dll', 'JobTrackerCommonTests.exe', 'secret.env')) {
    $caseStage = New-PayloadCopy "extra-$extra"
    Set-Content -LiteralPath (Join-Path $caseStage "bin/$extra") -Value 'test-only contamination'
    Assert-Rejected "reject-$extra" { Get-VerifiedReleasePayload $BuildDirectory $caseStage } 'inventory mismatch'
}
# A Debug binary renamed to its Release name must still be rejected.
$cache = Get-Content -LiteralPath (Join-Path $BuildDirectory 'CMakeCache.txt') -Raw
$qtDir = [regex]::Match($cache, '(?m)^Qt6_DIR:PATH=(.+?)\r?$').Groups[1].Value
$caseStage = New-PayloadCopy 'renamed-debug-dll'
Copy-Item -LiteralPath (Join-Path $qtDir '../../../bin/Qt6Cored.dll') -Destination (Join-Path $caseStage 'bin/Qt6Core.dll')
Assert-Rejected 'renamed-debug-dll' { Get-VerifiedReleasePayload $BuildDirectory $caseStage } 'Debug binary'
$caseStage = New-PayloadCopy 'modified-executable'
[IO.File]::AppendAllText((Join-Path $caseStage 'bin/JobTrackerApp.exe'), 'changed')
Assert-Rejected 'modified-executable' { Get-VerifiedReleasePayload $BuildDirectory $caseStage } 'does not match'
$caseStage = New-PayloadCopy 'x86-dll'
$path = Join-Path $caseStage 'bin/Qt6Core.dll'
$bytes = [IO.File]::ReadAllBytes($path)
$peOffset = [BitConverter]::ToInt32($bytes, 0x3c)
$bytes[$peOffset + 4] = 0x4c
$bytes[$peOffset + 5] = 0x01
[IO.File]::WriteAllBytes($path, $bytes)
Assert-Rejected 'x86-dll' { Get-VerifiedReleasePayload $BuildDirectory $caseStage } 'Not a Windows x64'
$caseStage = New-PayloadCopy 'empty-file'
[IO.File]::WriteAllBytes((Join-Path $caseStage 'bin/Qt6Core.dll'), [byte[]]@())
Assert-Rejected 'empty-file' { Get-VerifiedReleasePayload $BuildDirectory $caseStage } 'Empty payload file'
$caseStage = New-PayloadCopy 'absolute-qt-prefix'
Set-Content -LiteralPath (Join-Path $caseStage 'bin/qt.conf') -Value "[Paths]`nPrefix = D:/private-sdk"
Assert-Rejected 'absolute-qt-prefix' { Get-VerifiedReleasePayload $BuildDirectory $caseStage } 'relative staging prefix'
$caseStage = New-PayloadCopy 'absolute-qml-path'
Set-Content -LiteralPath (Join-Path $caseStage 'qml/QtQuick/qmldir') -Value 'plugin D:/private-sdk/qtquick2plugin'
Assert-Rejected 'absolute-qml-path' { Get-VerifiedReleasePayload $BuildDirectory $caseStage } 'Absolute machine path'
$caseStage = New-PayloadCopy 'alternate-stream'
Set-Content -LiteralPath (Join-Path $caseStage 'bin/qt.conf') -Stream extra -Value 'hidden data'
Assert-Rejected 'alternate-stream' { Get-VerifiedReleasePayload $BuildDirectory $caseStage } 'Alternate data stream'
$caseStage = Join-Path $work 'junction'
New-Item -ItemType Junction -Path $caseStage -Target $original.StageDirectory | Out-Null
Assert-Rejected 'junction-root' { Get-VerifiedReleasePayload $BuildDirectory $caseStage } 'Reparse point'
$caseStage = New-PayloadCopy 'junction-child'
New-Item -ItemType Junction -Path (Join-Path $caseStage 'plugins/linked') -Target $original.StageDirectory | Out-Null
Assert-Rejected 'junction-child' { Get-VerifiedReleasePayload $BuildDirectory $caseStage } 'Reparse point'
$caseStage = New-PayloadCopy 'network-qml-path'
Set-Content -LiteralPath (Join-Path $caseStage 'qml/QtQuick/qmldir') -Value 'plugin \\server\share\qtquick2plugin'
Assert-Rejected 'network-qml-path' { Get-VerifiedReleasePayload $BuildDirectory $caseStage } 'Absolute machine path'

$caseBuild = New-BuildCopy 'test-enabled-build'
$path = Join-Path $caseBuild 'CMakeCache.txt'
$text = [IO.File]::ReadAllText($path).Replace('JOBTRACKER_BUILD_TESTS:BOOL=OFF','JOBTRACKER_BUILD_TESTS:BOOL=ON')
[IO.File]::WriteAllText($path, $text)
Assert-Rejected 'test-enabled-build' { Get-VerifiedReleasePayload $caseBuild $StageDirectory } 'tests OFF'
$caseBuild = New-BuildCopy 'stale-version'
$path = Join-Path $caseBuild 'Installer/Release/JobTrackerIdentity.cmake'
$text = [IO.File]::ReadAllText($path).Replace('"' + $original.Version + '"', '"999.999.999"')
[IO.File]::WriteAllText($path, $text)
Assert-Rejected 'stale-version' { Get-VerifiedReleasePayload $caseBuild $StageDirectory } 'stale Release identity'
$caseBuild = New-BuildCopy 'wrong-app-name'
$path = Join-Path $caseBuild 'Installer/Release/JobTrackerIdentity.cmake'
[IO.File]::WriteAllText($path, [IO.File]::ReadAllText($path).Replace('JobTrackerApp.exe', 'DifferentApp.exe'))
Assert-Rejected 'wrong-app-name' { Get-VerifiedReleasePayload $caseBuild $StageDirectory } 'stale Release identity'
$caseBuild = New-BuildCopy 'wrong-build-architecture'
$path = Join-Path $caseBuild 'CMakeCache.txt'
[IO.File]::WriteAllText($path, [IO.File]::ReadAllText($path).Replace('CMAKE_GENERATOR_PLATFORM:INTERNAL=x64', 'CMAKE_GENERATOR_PLATFORM:INTERNAL=Win32'))
Assert-Rejected 'wrong-build-architecture' { Get-VerifiedReleasePayload $caseBuild $StageDirectory } 'x64 production build'
$caseBuild = New-BuildCopy 'foreign-checkout'
$path = Join-Path $caseBuild 'CMakeCache.txt'
$text = [regex]::Replace([IO.File]::ReadAllText($path), '(?m)^CMAKE_HOME_DIRECTORY:INTERNAL=.+$', 'CMAKE_HOME_DIRECTORY:INTERNAL=D:/not-this-checkout')
[IO.File]::WriteAllText($path, $text)
Assert-Rejected 'foreign-checkout' { Get-VerifiedReleasePayload $caseBuild $StageDirectory } 'different source checkout'

# Change only equal-length UTF-16 resource text in private PE fixtures. Match the
# private build EXE too, so product/version rejection is tested after the hash gate.
function Set-PeTextFixture([string]$Path, [string]$Before, [string]$After) {
    if ($Before.Length -ne $After.Length) { throw 'PE fixture replacements must retain length.' }
    $bytes = [IO.File]::ReadAllBytes($Path)
    $needle = [Text.Encoding]::Unicode.GetBytes($Before + [char]0)
    $replacement = [Text.Encoding]::Unicode.GetBytes($After + [char]0)
    $count = 0
    for ($offset = 0; $offset -le $bytes.Length - $needle.Length; ++$offset) {
        if ($bytes[$offset] -ne $needle[0]) { continue }
        $matches = $true
        for ($index = 1; $index -lt $needle.Length; ++$index) {
            if ($bytes[$offset + $index] -ne $needle[$index]) { $matches = $false; break }
        }
        if ($matches) { [Array]::Copy($replacement, 0, $bytes, $offset, $replacement.Length); ++$count }
    }
    if (-not $count) { throw "PE fixture resource not found: $Before" }
    [IO.File]::WriteAllBytes($Path, $bytes)
}
foreach ($scenario in @('wrong-product', 'wrong-exe-version')) {
    $caseBuild = New-BuildCopy $scenario
    $caseStage = New-PayloadCopy "$scenario-stage"
    $path = Join-Path $caseStage 'bin/JobTrackerApp.exe'
    if ($scenario -eq 'wrong-product') {
        Set-PeTextFixture $path 'JobTracker' 'NotTracker'
    } else {
        $wrongVersion = '9' + $original.Version.Substring(1)
        if ($wrongVersion -ceq $original.Version) { $wrongVersion = '8' + $original.Version.Substring(1) }
        Set-PeTextFixture $path $original.Version $wrongVersion
    }
    Copy-Item -LiteralPath $path -Destination (Join-Path $caseBuild 'Release/JobTrackerApp.exe')
    Assert-Rejected $scenario { Get-VerifiedReleasePayload $caseBuild $caseStage } 'Executable version/product mismatch'
}

# Keep the copied validator's source/build context coherent before mutating its inventory.
$privateRepository = Join-Path $work 'inventory-checkout'
$privateValidator = Join-Path $privateRepository 'Installer/Windows'
New-Item -ItemType Directory -Path $privateValidator | Out-Null
Copy-Item -LiteralPath (Join-Path $repository 'Installer/Windows/PayloadValidation.ps1') -Destination $privateValidator
Copy-Item -LiteralPath (Join-Path $repository 'CMakeLists.txt') -Destination $privateRepository
$inventoryBuild = New-BuildCopy 'inventory-build'
$inventoryCache = Join-Path $inventoryBuild 'CMakeCache.txt'
$sourceSetting = 'CMAKE_HOME_DIRECTORY:INTERNAL=' + $privateRepository.Replace('\', '/')
$cacheText = [regex]::Replace([IO.File]::ReadAllText($inventoryCache),
    '(?m)^CMAKE_HOME_DIRECTORY:INTERNAL=[^\r\n]+', $sourceSetting)
[IO.File]::WriteAllText($inventoryCache, $cacheText)
$inventory = Get-Content -LiteralPath (Join-Path $repository 'Installer/Windows/payload-files.txt')
$inventory | Set-Content -LiteralPath (Join-Path $privateValidator 'payload-files.txt')
$privatePayload = & {
    . (Join-Path $privateValidator 'PayloadValidation.ps1')
    Get-VerifiedReleasePayload $inventoryBuild $StageDirectory
}
if (($original.Files | ConvertTo-Json -Compress) -cne ($privatePayload.Files | ConvertTo-Json -Compress)) {
    throw 'Private validator fixture differs from the original valid payload.'
}
Record-Pass 'valid-private-inventory-fixture' 'Copied source/build context validates the original payload before mutation.'
@($inventory) + @($inventory[0]) | Set-Content -LiteralPath (Join-Path $privateValidator 'payload-files.txt')
Assert-Rejected 'duplicate-inventory' {
    . (Join-Path $privateValidator 'PayloadValidation.ps1')
    Get-VerifiedReleasePayload $inventoryBuild $StageDirectory
} 'Duplicate payload inventory'
@($inventory) + @('bin/../outside.dll') | Set-Content -LiteralPath (Join-Path $privateValidator 'payload-files.txt')
Assert-Rejected 'inventory-traversal' {
    . (Join-Path $privateValidator 'PayloadValidation.ps1')
    Get-VerifiedReleasePayload $inventoryBuild $StageDirectory
} 'Unsafe payload path'
foreach ($unsafe in @('bin/../../escape.dll', 'bin/https://example.invalid/payload.dll', 'bin//server/share.dll')) {
    @($inventory) + @($unsafe) | Set-Content -LiteralPath (Join-Path $privateValidator 'payload-files.txt')
    Assert-Rejected "unsafe-inventory-$unsafe" {
        . (Join-Path $privateValidator 'PayloadValidation.ps1')
        Get-VerifiedReleasePayload $inventoryBuild $StageDirectory
    } 'Unsafe payload path'
}

$after = Get-VerifiedReleasePayload -BuildDirectory $BuildDirectory -StageDirectory $StageDirectory
if (($original.Files | ConvertTo-Json -Compress) -cne ($after.Files | ConvertTo-Json -Compress)) {
    throw 'Original stage changed during negative tests.'
}
Record-Pass 'input-preservation' 'All original file hashes and paths are unchanged.'
Write-Output "PASS: $($results.Count) direct-payload checks. Evidence: $work"
