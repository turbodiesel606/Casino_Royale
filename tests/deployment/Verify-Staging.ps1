#requires -Version 7.2
param(
    [Parameter(Mandatory)][string]$StageDir,
    [Parameter(Mandatory)][string]$Probe,
    [Parameter(Mandatory)][string]$WorkRoot,
    [string]$MsvcRuntimeDirectory
)
$ErrorActionPreference = 'Stop'
$stage = (Resolve-Path -LiteralPath $StageDir).Path
$required = @(
    'bin/JobTrackerApp.exe', 'bin/qt.conf', 'bin/Qt6Core.dll', 'bin/Qt6Quick.dll',
    'bin/Qt6Sql.dll', 'plugins/platforms/qwindows.dll', 'plugins/sqldrivers/qsqlite.dll',
    'qml/QtQuick/Controls/qmldir', 'qml/QtQuick/Dialogs/qmldir',
    'qml/QtQuick/Layouts/qmldir', 'qml/QtQuick/Shapes/qmldir'
)
foreach ($relative in $required) {
    if (-not (Test-Path -LiteralPath (Join-Path $stage $relative) -PathType Leaf)) {
        throw "Missing runtime file: $relative"
    }
}
$files = @(Get-ChildItem -LiteralPath $stage -Recurse -File)
foreach ($file in $files) {
    if ($file.Extension -in '.h','.hpp','.lib','.a','.pdb','.cmake','.prl' -or
        $file.Name -match '^(msvcp|vcruntime|concrt|vcomp|ucrtbase|vc_redist)' -or
        $file.Name -match '^Qt6.*d\.dll$' -or
        $file.FullName -match '[\\/](include|mkspecs|qmltooling)[\\/]') {
        throw "Unexpected development/debug/compiler-runtime artifact: $($file.FullName)"
    }
    if ($file.Extension -eq '.dll' -and $file.VersionInfo.ProductName -match 'Qt') {
        if ($file.VersionInfo.FileVersion -ne '6.10.3.0' -or $file.VersionInfo.IsDebug) {
            throw "Unexpected Qt version/debug binary: $($file.FullName)"
        }
    }
}

# Use a retained private copy; the install payload never acquires a test executable.
$run = Join-Path ([IO.Path]::GetFullPath($WorkRoot)) ([Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $run | Out-Null
Copy-Item -LiteralPath $stage -Destination (Join-Path $run 'payload') -Recurse
$payload = Join-Path $run 'payload'
$executable = Join-Path $payload 'bin/JobTrackerDeploymentSmoke.exe'
Copy-Item -LiteralPath $Probe -Destination $executable
if ($MsvcRuntimeDirectory) {
    $runtime = Get-Content -LiteralPath (Join-Path $MsvcRuntimeDirectory 'runtime-build.json') -Raw | ConvertFrom-Json
    foreach ($file in $runtime.files) {
        if ($file.name -notmatch '^[a-z0-9_]+\.dll$') { throw 'Unsafe test runtime filename.' }
        $source = Join-Path $MsvcRuntimeDirectory $file.name
        if ((Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash -ine $file.sha256) { throw 'Test runtime hash mismatch.' }
        Copy-Item -LiteralPath $source -Destination (Join-Path $payload "bin/$($file.name)")
    }
}

$start = [Diagnostics.ProcessStartInfo]::new($executable)
$start.UseShellExecute = $false
$start.CreateNoWindow = $true
$start.WorkingDirectory = $run
$start.RedirectStandardOutput = $true
$start.RedirectStandardError = $true
if ($MsvcRuntimeDirectory) { $start.ArgumentList.Add('--expect-app-local-msvc') }
foreach ($key in @($start.Environment.Keys)) {
    if ($key -match '^(QT|QML)' -or $key -in 'QTDIR','CMAKE_PREFIX_PATH') {
        $start.Environment.Remove($key) | Out-Null
    }
}
$start.Environment['PATH'] = "$env:SystemRoot\System32;$env:SystemRoot"
$start.Environment['QT_QPA_PLATFORM'] = 'windows'
$start.Environment['QML_DISABLE_DISK_CACHE'] = '1'
$process = [Diagnostics.Process]::Start($start)
$stdout = $process.StandardOutput.ReadToEndAsync()
$stderr = $process.StandardError.ReadToEndAsync()
if (-not $process.WaitForExit(20000)) {
    $process.Kill() # Only this disposable test helper, never JobTrackerApp.
    throw "Deployment probe timed out; retained $run"
}
$output = $stdout.Result + $stderr.Result
$output | Set-Content -LiteralPath (Join-Path $run 'probe.log')
Write-Output $output
Write-Output "Retained verification copy: $run"
if ($process.ExitCode -ne 0) { throw "Deployment probe failed: $($process.ExitCode)" }
Write-Output "PASS: $($files.Count) common staged files; no SDK/debug/compiler-runtime contamination"
