#requires -Version 7.2
param(
    [Parameter(Mandatory)][string]$BuildDirectory,
    [Parameter(Mandatory)][string]$StageDirectory,
    [Parameter(Mandatory)][string]$ManifestPath
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$script = Join-Path $repository 'packaging/windows/Package-Release.ps1'
$stage = [IO.Path]::GetFullPath($StageDirectory)
$manifestFile = [IO.Path]::GetFullPath($ManifestPath)
$original = Get-Content -LiteralPath $manifestFile -Raw | ConvertFrom-Json
$run = Join-Path $repository ('dist/package-tests-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $run | Out-Null
$results = [Collections.Generic.List[object]]::new()

function Assert-Rejected([string]$Name, [scriptblock]$Action, [string]$Message) {
    $caught = $null
    try { & $Action | Out-Null } catch { $caught = $_.Exception.Message }
    if (-not $caught -or $caught -notmatch $Message) {
        throw "FAIL: $Name; expected '$Message'; got '$caught'"
    }
    $results.Add([ordered]@{name=$Name; result='PASS'; rejection=$caught})
    Write-Output "PASS: rejects $Name"
}

$create = @{ BuildDirectory=$BuildDirectory; StageDirectory=$stage; OutputDirectory=(Join-Path $run 'invalid-output') }
$outside = $create.Clone()
$outside.OutputDirectory = Join-Path $repository 'unsafe-output'
Assert-Rejected 'output path guard' { & $script @outside } 'must be below'
$existing = $create.Clone()
$existing.OutputDirectory = $run
Assert-Rejected 'existing output' { & $script @existing } 'already exists'

$missing = Join-Path $run 'missing-stage'
New-Item -ItemType Directory -Path $missing | Out-Null
$fixtureArgs = $create.Clone()
$fixtureArgs.StageDirectory = $missing
Assert-Rejected 'missing staged assets' { & $script @fixtureArgs } 'inventory mismatch'

$fixture = Join-Path $run 'stage'
Copy-Item -LiteralPath $stage -Destination $fixture -Recurse
$fixtureArgs.StageDirectory = $fixture
$nested = $fixtureArgs.Clone()
$nested.StageDirectory = $fixture + '/'
$nested.OutputDirectory = Join-Path $fixture 'archive'
Assert-Rejected 'output inside staging with trailing separator' { & $script @nested } 'outside the staging tree'
$fakeBuild = Join-Path $run 'build'
$identityRelative = 'packaging/windows/Release/JobTrackerIdentity.cmake'
[IO.Directory]::CreateDirectory((Join-Path $fakeBuild 'packaging/windows/Release')) | Out-Null
Copy-Item -LiteralPath (Join-Path $BuildDirectory 'CMakeCache.txt') -Destination (Join-Path $fakeBuild 'CMakeCache.txt')
$identity = Get-Content -LiteralPath (Join-Path $BuildDirectory $identityRelative) -Raw
$identity.Replace('set(JOBTRACKER_RELEASE_VERSION "' + $original.version + '")', 'set(JOBTRACKER_RELEASE_VERSION "999.0.0")') | Set-Content -LiteralPath (Join-Path $fakeBuild $identityRelative)
$fakeArgs = $fixtureArgs.Clone()
$fakeArgs.BuildDirectory = $fakeBuild
Assert-Rejected 'stale generated identity' { & $script @fakeArgs } 'stale Release identity'
$fakeCache = Get-Content -LiteralPath (Join-Path $fakeBuild 'CMakeCache.txt') -Raw
$fakeCache.Replace('JOBTRACKER_BUILD_TESTS:BOOL=OFF', 'JOBTRACKER_BUILD_TESTS:BOOL=ON') | Set-Content -LiteralPath (Join-Path $fakeBuild 'CMakeCache.txt')
Assert-Rejected 'test-enabled build directory' { & $script @fakeArgs } 'tests OFF'
$qtCore = Join-Path $fixture 'bin/Qt6Core.dll'
$sourceCore = Join-Path $stage 'bin/Qt6Core.dll'
$cache = Get-Content -LiteralPath (Join-Path $BuildDirectory 'CMakeCache.txt') -Raw
$qtDir = [regex]::Match($cache, '(?m)^Qt6_DIR:PATH=(.+?)\r?$').Groups[1].Value
$debugCore = [IO.Path]::GetFullPath((Join-Path $qtDir '../../../bin/Qt6Cored.dll'))
Copy-Item -LiteralPath $debugCore -Destination $qtCore
Assert-Rejected 'Debug DLL under Release filename' { & $script @fixtureArgs } 'Debug binary'
Copy-Item -LiteralPath $sourceCore -Destination $qtCore
$stream = [IO.File]::Open($qtCore, [IO.FileMode]::Open, [IO.FileAccess]::ReadWrite)
try {
    $reader = [IO.BinaryReader]::new($stream, [Text.Encoding]::UTF8, $true)
    $stream.Position = 0x3c
    $offset = $reader.ReadInt32()
    $stream.Position = $offset + 4
    $stream.WriteByte(0x4c)
    $stream.WriteByte(0x01)
    $reader.Dispose()
} finally { $stream.Dispose() }
Assert-Rejected 'x86 DLL' { & $script @fixtureArgs } 'Not a Windows x64 binary'
Copy-Item -LiteralPath $sourceCore -Destination $qtCore
$exe = Join-Path $fixture 'bin/JobTrackerApp.exe'
[IO.File]::AppendAllText($exe, 'tampered')
Assert-Rejected 'stale staged executable' { & $script @fixtureArgs } 'does not match'
Copy-Item -LiteralPath (Join-Path $stage 'bin/JobTrackerApp.exe') -Destination $exe
$config = Join-Path $fixture 'bin/qt.conf'
[IO.File]::WriteAllText($config, "[Paths]`nPrefix = D:/private/Qt")
Assert-Rejected 'absolute Qt prefix' { & $script @fixtureArgs } 'relative staging prefix'
Copy-Item -LiteralPath (Join-Path $stage 'bin/qt.conf') -Destination $config
[IO.File]::WriteAllText((Join-Path $fixture 'bin/secret.env'), 'fixture-only')
Assert-Rejected 'unexpected secret/development file' { & $script @fixtureArgs } 'inventory mismatch'
New-Item -ItemType Junction -Path (Join-Path $fixture 'linked') -Target $stage | Out-Null
Assert-Rejected 'junction in staging' { & $script @fixtureArgs } 'Reparse point'

$assets = Join-Path $run 'assets'
New-Item -ItemType Directory -Path $assets | Out-Null
$manifestCopy = Join-Path $assets ([IO.Path]::GetFileName($manifestFile))
Copy-Item -LiteralPath $manifestFile -Destination $manifestCopy
$verify = @{ManifestPath=$manifestCopy; ExpectedVersion=$original.version}
$wrongVersion = $verify.Clone()
$wrongVersion.ExpectedVersion = '999.0.0'
Assert-Rejected 'wrong requested version' { & $script @wrongVersion } 'identity/version mismatch'
Assert-Rejected 'missing archive' { & $script @verify } 'does not exist|Cannot find'
$sourceArchive = Join-Path (Split-Path -Parent $manifestFile) $original.archive
$archive = Join-Path $assets $original.archive
Copy-Item -LiteralPath $sourceArchive -Destination $archive
Copy-Item -LiteralPath "$sourceArchive.sha256" -Destination "$archive.sha256"
$stream = [IO.File]::OpenWrite($archive)
try { $stream.WriteByte(0) } finally { $stream.Dispose() }
Assert-Rejected 'corrupt archive with unchanged size' { & $script @verify } 'checksum/size mismatch'
Copy-Item -LiteralPath $sourceArchive -Destination $archive
[IO.File]::WriteAllText("$archive.sha256", 'wrong checksum')
Assert-Rejected 'wrong checksum sidecar' { & $script @verify } 'checksum/size mismatch'
Copy-Item -LiteralPath "$sourceArchive.sha256" -Destination "$archive.sha256"
$modified = Get-Content -LiteralPath $manifestFile -Raw | ConvertFrom-Json
$modified.files[0].path = '../escape.dll'
$modified | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $manifestCopy
Assert-Rejected 'manifest traversal path' { & $script @verify } 'inventory mismatch'
$modified = Get-Content -LiteralPath $manifestFile -Raw | ConvertFrom-Json
$modified.files[0].sha256 = '0' * 64
$modified | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $manifestCopy
Assert-Rejected 'incorrect extracted-file hash' { & $script @verify } 'Extracted payload mismatch'
Copy-Item -LiteralPath $manifestFile -Destination $manifestCopy
& $script @verify
$results.Add([ordered]@{name='valid archive verification';result='PASS'})
if (Test-Path -LiteralPath $create.OutputDirectory) { throw 'Rejected creation unexpectedly wrote output.' }
$results | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $run 'results.json')
Write-Output "PASS: $($results.Count) package checks; retained $run"
