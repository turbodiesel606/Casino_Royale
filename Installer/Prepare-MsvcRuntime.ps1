#requires -Version 7.2
<#
Validate the pinned Visual Studio Release CRT and official redistributable.
Produce a private, verified App-Local bundle and Inno input for both scopes.
No download, prerequisite execution, application launch or cleanup occurs.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$BuildDirectory,
    [Parameter(Mandatory)][string]$RedistributablePath,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [switch]$PassThru
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if (-not $IsWindows) { throw 'MSVC runtime preparation requires Windows.' }
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
. (Join-Path $PSScriptRoot 'PayloadValidation.ps1')
$destination = [IO.Path]::GetFullPath($OutputDirectory)
$runtimeRoots = @(((Join-Path $repository 'dist') + '\'), ((Join-Path $repository 'build') + '\'))
if (-not @($runtimeRoots | Where-Object { $destination.StartsWith($_, [StringComparison]::OrdinalIgnoreCase) }).Count) {
    throw 'Runtime output must be below this repository''s build/ or dist/ directory.'
}
if (Test-Path -LiteralPath $destination) { throw 'Runtime output already exists; choose a new directory.' }
$ancestor = $destination
while ($ancestor) {
    if ((Test-Path -LiteralPath $ancestor) -and
        ((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw "Reparse point in runtime output: $ancestor"
    }
    $ancestor = [IO.Path]::GetDirectoryName($ancestor)
}
$lock = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'MsvcRuntime.lock.json') -Raw | ConvertFrom-Json
if ($lock.schemaVersion -ne 1 -or $lock.architecture -cne 'x64' -or
    $lock.minimumVersion -cnotmatch '^14\.\d+\.\d+\.\d+$' -or
    $lock.visualStudioRedistDirectory -cnotmatch '^14\.\d+\.\d+$' -or
    $lock.crtDirectory -cne 'Microsoft.VC143.CRT' -or
    $lock.redistributable.url -cnotmatch '^https://download\.visualstudio\.microsoft\.com/download/pr/[0-9a-f-]+/[0-9A-F]+/VC_redist\.x64\.exe$' -or
    $lock.redistributable.version -cne $lock.minimumVersion -or
    @($lock.files | Sort-Object name -Unique).Count -ne $lock.files.Count -or $lock.files.Count -eq 0) {
    throw 'Invalid reviewed MSVC runtime lock.'
}
$cache = Get-Content -LiteralPath (Join-Path $BuildDirectory 'CMakeCache.txt') -Raw
if ($cache -notmatch '(?m)^CMAKE_GENERATOR_PLATFORM:INTERNAL=x64\r?$' -or
    $cache -notmatch '(?m)^JOBTRACKER_BUILD_TESTS:BOOL=OFF\r?$') {
    throw 'MSVC runtime preparation requires an x64 production build.'
}
$instance = [regex]::Match($cache, '(?m)^CMAKE_GENERATOR_INSTANCE:INTERNAL=(.+?)\r?$')
if (-not $instance.Success) { throw 'Visual Studio installation was not recorded by CMake.' }
$visualStudio = $instance.Groups[1].Value
$crt = Join-Path $visualStudio "VC/Redist/MSVC/$($lock.visualStudioRedistDirectory)/x64/$($lock.crtDirectory)"
$cmakeVersion = @('MAJOR', 'MINOR', 'PATCH') | ForEach-Object {
    $match = [regex]::Match($cache, "(?m)^CMAKE_CACHE_$($_)_VERSION:INTERNAL=(\d+)\r?$")
    if (-not $match.Success) { throw 'CMake cache version is missing.' }
    $match.Groups[1].Value
}
$compilerFile = Join-Path $BuildDirectory ('CMakeFiles/' + ($cmakeVersion -join '.') + '/CMakeCXXCompiler.cmake')
$compilerData = Get-Content -LiteralPath $compilerFile -Raw
$tools = [regex]::Match($compilerData, '(?m)^set\(CMAKE_CXX_COMPILER "[^"\r\n]+/VC/Tools/MSVC/(14\.\d+\.\d+)/bin/Hostx64/x64/cl\.exe"\)\r?$')
if (-not $tools.Success -or [version]($tools.Groups[1].Value + '.0') -gt [version]$lock.minimumVersion) {
    throw "Cannot verify an MSVC toolset compatible with pinned runtime $($lock.minimumVersion): $compilerFile. Select a compatible installed toolset or review the runtime lock explicitly; it will not be repinned automatically."
}

function Assert-MicrosoftFile([string]$Path, [object]$Record, [bool]$Dll) {
    Assert-NoReparsePoint $Path
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        if ($Dll) {
            throw "Required App-Local x64 Release CRT is missing: $Path. Expected Microsoft version $($lock.minimumVersion) from Visual Studio redist $($lock.visualStudioRedistDirectory). Restore the matching authorized local CRT inputs; nothing will be downloaded."
        }
        throw "Official local x64 redistributable is missing: $Path. Supply -RedistributablePath to an existing Microsoft vc_redist.x64.exe, version $($lock.minimumVersion), SHA-256 $($Record.sha256), matching Installer/MsvcRuntime.lock.json; nothing will be downloaded."
    }
    $item = Get-Item -LiteralPath $Path -ErrorAction Stop
    if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -or
        $Record.sha256 -cnotmatch '^[0-9a-f]{64}$' -or $Record.size -le 0 -or
        $item.Length -ne $Record.size -or
        (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant() -cne $Record.sha256) {
        throw "Pinned MSVC file integrity mismatch: $Path. Required Microsoft version $($lock.minimumVersion), $($Record.size) bytes, SHA-256 $($Record.sha256). Supply the reviewed local input; do not silently repin Installer/MsvcRuntime.lock.json."
    }
    $info = $item.VersionInfo
    if ($info.IsDebug -or $info.FileVersion.Trim() -cne $lock.minimumVersion) {
        throw "MSVC file version/debug mismatch: $Path. Expected Release version $($lock.minimumVersion); observed '$($info.FileVersion)'."
    }
    $signature = Get-AuthenticodeSignature -LiteralPath $Path
    if ($signature.Status -ne 'Valid' -or
        $signature.SignerCertificate.Subject -notmatch '(^|, )O=Microsoft Corporation(,|$)') {
        throw "A valid Microsoft Authenticode signature is required: $Path ($($signature.Status))"
    }
    if ($Dll) {
        $stream = [IO.File]::OpenRead($Path)
        $pe = [Reflection.PortableExecutable.PEReader]::new($stream)
        try {
            if ($pe.PEHeaders.CoffHeader.Machine -ne [Reflection.PortableExecutable.Machine]::Amd64 -or
                -not ($pe.PEHeaders.CoffHeader.Characteristics -band [Reflection.PortableExecutable.Characteristics]::Dll)) {
                throw "MSVC CRT must be a Windows x64 DLL: $Path"
            }
        } finally { $pe.Dispose(); $stream.Dispose() }
    } elseif ($info.FileDescription -notmatch '^Microsoft Visual C\+\+ .* Redistributable \(x64\) - ') {
        # Microsoft's x64 bundle has an x86 bootstrapper. Its signed bundle
        # identity, pinned official x64 URL and exact hash identify the payload.
        throw 'The signed Microsoft bundle is not the x64 redistributable.'
    }
}

Assert-MicrosoftFile ([IO.Path]::GetFullPath($RedistributablePath)) $lock.redistributable $false
foreach ($file in $lock.files) {
    if ($file.name -cnotmatch '^(concrt140|msvcp140(_1|_2|_atomic_wait|_codecvt_ids)?|vccorlib140|vcruntime140(_1|_threads)?)\.dll$') {
        throw 'Unreviewed or unsafe CRT filename.'
    }
    Assert-MicrosoftFile (Join-Path $crt $file.name) $file $true
}
New-Item -ItemType Directory -Path $destination | Out-Null
$redistSnapshot = Join-Path $destination 'vc_redist.x64.exe'
Copy-Item -LiteralPath $RedistributablePath -Destination $redistSnapshot
if ((Get-Sha256 $redistSnapshot) -cne $lock.redistributable.sha256) {
    throw 'Redistributable input changed during snapshot creation.'
}
foreach ($file in $lock.files) {
    $target = Join-Path $destination $file.name
    Copy-Item -LiteralPath (Join-Path $crt $file.name) -Destination $target
    if ((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash.ToLowerInvariant() -cne $file.sha256) {
        throw 'CRT input changed during snapshot creation.'
    }
}
$include = [Collections.Generic.List[string]]::new()
$include.Add('// Generated only after pinned Microsoft signature, hash, version and architecture checks.')
$include.Add('#define MsvcMinimumVersion "' + $lock.minimumVersion + '"')
$components = $lock.minimumVersion.Split('.') -join ', '
$include.Add('#define MsvcMinimumComponents "' + $components + '"')
$include.Add('#define MsvcRedistHash "' + $lock.redistributable.sha256 + '"')
$include.Add('#define MsvcRedistSize ' + $lock.redistributable.size)
$include.Add('[Files]')
$include.Add(('Source: "{0}"; DestName: "vc_redist.x64.exe"; Hash: "{1}"; Flags: dontcopy' -f $redistSnapshot, $lock.redistributable.sha256))
foreach ($file in $lock.files) {
    $include.Add(('Source: "{0}\{1}"; DestDir: "{{app}}\bin"; Hash: "{2}"; Flags: ignoreversion; Check: not IsAdminInstallMode and CanInstallWithManualClose(ExpandConstant(''{{app}}\bin\{1}''))' -f $destination, $file.name, $file.sha256))
}
$include.Add('[Code]')
$include.Add('function MsvcFileCompatible(const FileName: String): Boolean; forward;')
$include.Add('function MsvcSystemFilesCompatible: Boolean;')
$include.Add('begin')
$include.Add('  Result := True;')
foreach ($file in $lock.files) {
    $include.Add(('  Result := MsvcFileCompatible(ExpandConstant(''{{sys}}\{0}'')) and Result;' -f $file.name))
}
$include.Add('end;')
$includePath = Join-Path $destination 'JobTrackerMsvcRuntime.iss'
$include | Set-Content -LiteralPath $includePath -Encoding utf8
$evidence = [ordered]@{
    minimumVersion = $lock.minimumVersion
    toolset = $tools.Groups[1].Value
    architecture = 'x64'
    crtSource = $crt
    files = $lock.files
    redistributable = $lock.redistributable
    sourceLockSha256 = (Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'MsvcRuntime.lock.json') -Algorithm SHA256).Hash.ToLowerInvariant()
}
$evidence | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $destination 'runtime-build.json') -Encoding utf8
if ($PassThru) { return [pscustomobject]@{ IncludePath = $includePath; Evidence = $evidence } }
Write-Output "PASS: $($lock.files.Count) signed x64 CRT DLLs; official x64 prerequisite; minimum $($lock.minimumVersion)."
