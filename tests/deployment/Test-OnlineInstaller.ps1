#requires -Version 7.2
<#
Standalone consumer of the production native download/extraction pipeline.
Only a separate TEST ONLY harness accepts mock endpoints and negative hashes.
All outputs are retained below dist/. No real install registration or app launch.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$BuildDirectory,
    [Parameter(Mandatory)][string]$InstallerDirectory,
    [Parameter(Mandatory)][string]$ManifestPath,
    [Parameter(Mandatory)][string]$WorkDirectory,
    [Parameter(Mandatory)][string]$IsccPath,
    [switch]$CheckPublicDownload
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$work = [IO.Path]::GetFullPath($WorkDirectory)
if (-not $work.StartsWith((Join-Path $repository 'dist') + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Tests must stay below this checkout''s dist/ directory.'
}
if (Test-Path -LiteralPath $work) { throw 'Test output exists; choose a fresh directory.' }
$ancestor = $work
while ($ancestor) {
    if ((Test-Path -LiteralPath $ancestor) -and
        ((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw "Reparse point in test output: $ancestor"
    }
    $ancestor = [IO.Path]::GetDirectoryName($ancestor)
}
$installerDirectory = [IO.Path]::GetFullPath($InstallerDirectory)
$manifestFile = [IO.Path]::GetFullPath($ManifestPath)
$manifest = Get-Content -LiteralPath $manifestFile -Raw | ConvertFrom-Json
$metadata = Get-Content -LiteralPath (Join-Path $installerDirectory 'installer-build.json') -Raw | ConvertFrom-Json
$archive = Join-Path (Split-Path -Parent $manifestFile) $manifest.archive
$pipeline = Join-Path $repository 'packaging/windows/OnlinePayload.iss'
$payload = Join-Path $installerDirectory 'JobTrackerPayload.iss'
$compiler = (Get-Item -LiteralPath $IsccPath).FullName
$results = [Collections.Generic.List[object]]::new()

function Assert-Check([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}

function Record-Pass([string]$Name, [object]$Evidence) {
    $results.Add([ordered]@{name=$Name;result='PASS';evidence=$Evidence})
    $results | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $work 'results.json') -Encoding utf8
    Write-Output "PASS: $Name"
}

New-Item -ItemType Directory -Path $work | Out-Null
Assert-Check ($metadata.kind -ceq 'version-locked-online') 'Not an Online Installer build.'
$expectedUrl = "https://github.com/turbodiesel606/Releases/releases/download/v$($manifest.version)/$($manifest.archive)"
Assert-Check ($metadata.url -ceq $expectedUrl -and $metadata.tag -ceq "v$($manifest.version)") 'URL/tag is not version locked.'
Assert-Check ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash -ieq $metadata.archiveSha256) 'Archive trust anchor changed.'
Assert-Check ((Get-FileHash -LiteralPath (Join-Path $installerDirectory $metadata.installer) -Algorithm SHA256).Hash -ieq $metadata.sha256) 'Installer changed.'
Assert-Check ((Get-FileHash -LiteralPath $pipeline -Algorithm SHA256).Hash -ieq $metadata.onlinePayloadScriptSha256) 'Pipeline changed; rebuild the production installer first.'
Assert-Check ((Get-FileHash -LiteralPath $payload -Algorithm SHA256).Hash -ieq $metadata.generatedPayloadSha256) 'Generated payload include changed.'
Record-Pass 'production version/hash/input identity' $expectedUrl

function Assert-BuildRejected([string]$Name, [hashtable]$Arguments, [string]$Pattern) {
    $message = ''
    try { & (Join-Path $repository 'packaging/windows/Build-Installer.ps1') @Arguments | Out-Null }
    catch { $message = $_.Exception.Message }
    Assert-Check ($message -match $Pattern) "$Name was not rejected as expected: $message"
    Assert-Check (-not (Test-Path -LiteralPath $Arguments.OutputDirectory)) "$Name reached compilation."
    Record-Pass $Name $message
}

$buildArguments = @{BuildDirectory=$BuildDirectory;ManifestPath=$manifestFile;OutputDirectory=(Join-Path $work 'must-not-compile');IsccPath=$compiler}
$fakeEndpoint = $buildArguments.Clone()
$fakeEndpoint.ReleaseUrl = 'http://127.0.0.1/fake'
Assert-BuildRejected 'public endpoint override' $fakeEndpoint 'parameter.*ReleaseUrl'
$fakeDigest = $buildArguments.Clone()
$fakeDigest.ArchiveSha256 = '0' * 64
Assert-BuildRejected 'public checksum override' $fakeDigest 'parameter.*ArchiveSha256'
$invalidAssets = Join-Path $work 'invalid-assets'
New-Item -ItemType Directory -Path $invalidAssets | Out-Null
$manifestCopy = Join-Path $invalidAssets ([IO.Path]::GetFileName($manifestFile))
$invalidArguments = $buildArguments.Clone()
$invalidArguments.ManifestPath = $manifestCopy
$invalid = Get-Content -LiteralPath $manifestFile -Raw | ConvertFrom-Json
$invalid.version = '999.0.0'
$invalid | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $manifestCopy
Assert-BuildRejected 'manifest version mismatch' $invalidArguments 'identity/version mismatch'
$invalid.version = $manifest.version
$invalid | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $manifestCopy
Assert-BuildRejected 'missing archive' $invalidArguments 'does not exist|cannot find'
Copy-Item -LiteralPath $archive -Destination (Join-Path $invalidAssets $manifest.archive)
Copy-Item -LiteralPath "$archive.sha256" -Destination (Join-Path $invalidAssets "$($manifest.archive).sha256")
$invalid.sha256 = ''
$invalid | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $manifestCopy
Assert-BuildRejected 'missing embedded checksum' $invalidArguments 'checksum/size mismatch'
$invalid.sha256 = '0' * 64
$invalid | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $manifestCopy
Assert-BuildRejected 'mismatched embedded checksum' $invalidArguments 'checksum/size mismatch'

$arguments = @("/DPipelineFile=$pipeline", "/DPayloadFile=$payload", "/DTestOutput=$work", (Join-Path $PSScriptRoot 'OnlinePayloadHarness.iss'))
$compilerOutput = @(& $compiler @arguments 2>&1)
$compilerExit = $LASTEXITCODE
$compilerOutput | Set-Content -LiteralPath (Join-Path $work 'compile.log') -Encoding utf8
Assert-Check ($compilerExit -eq 0) "Harness compilation failed ($compilerExit); see $work/compile.log."
$harness = Join-Path $work 'online-payload-test-only.exe'
Record-Pass 'test-only native pipeline compilation' (Get-FileHash -LiteralPath $harness -Algorithm SHA256).Hash

# CMake/libarchive permits creating this deliberately hostile test member name.
# It never reaches a production build: that requires the reviewed inventory.
$archiveWork = Join-Path $work 'archive-input'
New-Item -ItemType Directory -Path $archiveWork | Out-Null
Set-Content -LiteralPath (Join-Path $work 'escape.txt') -Value 'Do not extract outside payload.'
$unsafeArchive = Join-Path $work 'unsafe.7z'
Push-Location -LiteralPath $archiveWork
try {
    $archiveOutput = @(& cmake -E tar cf $unsafeArchive --format=7zip -- ../escape.txt 2>&1)
    Assert-Check ($LASTEXITCODE -eq 0) "Hostile fixture creation failed: $archiveOutput"
} finally { Pop-Location }
$archiveNames = @(& cmake -E tar tf $unsafeArchive)
Assert-Check ($LASTEXITCODE -eq 0 -and $archiveNames -contains '../escape.txt') 'Hostile fixture did not preserve traversal.'

# The server is test-owned, loopback-only, and accepts no arbitrary file paths.
# HTTP is intentionally limited to this separate harness; production is fixed HTTPS.
$server = Start-Job -ArgumentList $archive, $unsafeArchive, $work -ScriptBlock {
    param($Archive, $UnsafeArchive, $Work)
    $ErrorActionPreference = 'Stop'
    $good = [IO.File]::ReadAllBytes($Archive)
    $corrupt = $good.Clone()
    $corrupt[100] = $corrupt[100] -bxor 1
    $unsafe = [IO.File]::ReadAllBytes($UnsafeArchive)
    $bad = [Text.Encoding]::ASCII.GetBytes('Not a 7z archive')
    $listener = [Net.Sockets.TcpListener]::new([Net.IPAddress]::Loopback, 0)
    $listener.Start()
    [IO.File]::WriteAllText((Join-Path $Work 'server-port.txt'), [string]$listener.LocalEndpoint.Port)
    try {
        while ($true) {
            # Keep Stop-Job interruptible; AcceptTcpClient alone can block shutdown.
            if (-not $listener.Pending()) { Start-Sleep -Milliseconds 25; continue }
            $client = $listener.AcceptTcpClient()
            try {
                $stream = $client.GetStream()
                $reader = [IO.StreamReader]::new($stream, [Text.Encoding]::ASCII, $false, 1024, $true)
                $request = $reader.ReadLine()
                while ($reader.ReadLine()) { }
                $route = $request.Split(' ')[1]
                [IO.File]::AppendAllText((Join-Path $Work 'requests.log'), "$route`r`n")
                $status = '200 OK'
                $extra = ''
                $body = $good
                switch ($route) {
                    '/missing' { $status = '404 Not Found'; $body = [byte[]]@() }
                    '/unavailable' { $status = '503 Service Unavailable'; $body = [byte[]]@() }
                    '/redirect' { $status = '302 Found'; $extra = "Location: /valid`r`n"; $body = [byte[]]@() }
                    '/corrupt' { $body = $corrupt }
                    '/unsafe' { $body = $unsafe }
                    '/bad-archive' { $body = $bad }
                }
                $length = if ($route -eq '/oversized') { $good.Length + 1 } else { $body.Length }
                $headers = [Text.Encoding]::ASCII.GetBytes("HTTP/1.1 $status`r`n${extra}Content-Length: $length`r`nConnection: close`r`n`r`n")
                $stream.Write($headers, 0, $headers.Length)
                if ($route -eq '/truncated') {
                    $stream.Write($body, 0, 4096)
                } elseif ($route -eq '/slow') {
                    for ($offset = 0; $offset -lt $body.Length; $offset += 65536) {
                        $stream.Write($body, $offset, [Math]::Min(65536, $body.Length - $offset))
                        Start-Sleep -Milliseconds 10
                    }
                } else {
                    $stream.Write($body, 0, $body.Length)
                }
                $stream.Flush()
            } catch {
                # Early client disconnect is expected for size rejection/cancellation.
                [IO.File]::AppendAllText((Join-Path $Work 'server.log'), $_.Exception.Message + "`r`n")
            } finally { $client.Dispose() }
        }
    } finally { $listener.Stop() }
}

function Invoke-Case([string]$Name, [string]$Route, [bool]$Success, [string]$Pattern,
                     [string]$Language = 'english', [string]$Hash = '', [long]$Size = 0) {
    $caseDirectory = Join-Path $work $Name
    $target = Join-Path $caseDirectory 'application'
    New-Item -ItemType Directory -Path $target -Force | Out-Null
    $sentinel = Join-Path $target 'sentinel.txt'
    Set-Content -LiteralPath $sentinel -Value 'Preserve existing files on failure.'
    $outside = Join-Path $caseDirectory 'outside'
    if ($Name -eq 'reparse-destination') {
        New-Item -ItemType Directory -Path $outside | Out-Null
        Set-Content -LiteralPath (Join-Path $outside 'qt.conf') -Value 'Preserve junction target.'
        New-Item -ItemType Junction -Path (Join-Path $target 'bin') -Target $outside | Out-Null
    }
    $url = if ($Route -eq 'offline') { "http://127.0.0.1:$offlinePort/valid" }
           elseif ($Route -eq 'public') { $metadata.url }
           else { "$baseUrl$Route" }
    $log = Join-Path $caseDirectory 'setup.log'
    $arguments = @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/SP-', "/LANG=$Language", "/DIR=`"$target`"", "/LOG=`"$log`"", "/Case=$Name", "/Url=$url")
    if ($Hash) { $arguments += "/Hash=$Hash" }
    if ($Size) { $arguments += "/Size=$Size" }
    $process = Start-Process -FilePath $harness -ArgumentList $arguments -WindowStyle Hidden -Wait -PassThru
    $logText = Get-Content -LiteralPath $log -Raw
    Assert-Check (($process.ExitCode -eq 0) -eq $Success) "$Name exit $($process.ExitCode); see $log."
    Assert-Check ($logText -match $Pattern) "$Name missing expected diagnostic '$Pattern'."
    Assert-Check ($logText -notmatch 'TEST FAILURE') "$Name failed temporary cleanup."
    Assert-Check ((Get-Content -LiteralPath $sentinel -Raw).Trim() -ceq 'Preserve existing files on failure.') "$Name overwrote existing data."
    if ($Success) {
        foreach ($file in $manifest.files) {
            Assert-Check ((Get-FileHash -LiteralPath (Join-Path $target $file.path) -Algorithm SHA256).Hash -ieq $file.sha256) "$Name incorrect file $($file.path)."
        }
    } else {
        Assert-Check (-not (Test-Path -LiteralPath (Join-Path $target 'bin/JobTrackerApp.exe'))) "$Name installed files on failure."
        Assert-Check (-not (Test-Path -LiteralPath (Join-Path $target 'qml'))) "$Name wrote partial payload."
    }
    if ($Name -eq 'reparse-destination') {
        Assert-Check ((Get-Content -LiteralPath (Join-Path $outside 'qt.conf') -Raw).Trim() -ceq 'Preserve junction target.') 'Junction target overwritten.'
    }
    foreach ($match in [regex]::Matches($logText, 'Payload temporary directory: ([^\r\n]+)')) {
        Assert-Check (-not (Test-Path -LiteralPath $match.Groups[1].Value)) "$Name left its temporary directory."
    }
    if ($Name -in 'corrupt', 'truncated', 'missing', 'unavailable', 'offline', 'oversized', 'cancel-download', 'missing-russian') {
        Assert-Check ($logText -notmatch 'Starting verified payload extraction') "$Name extracted unverified data."
    }
    if ($Name -eq 'traversal') {
        Assert-Check ($logText -match 'Extraction failed: Extract failed') 'Native extractor did not reject traversal.'
        Assert-Check ($logText -match 'TEST: checked traversal rejection and escape path before temporary cleanup') 'Missing escape-path check.'
    }
    Record-Pass $Name ([ordered]@{exit=$process.ExitCode;log=$log;payloadFiles=if($Success){$manifest.files.Count}else{0}})
}

try {
    $deadline = [DateTime]::UtcNow.AddSeconds(15)
    while (-not (Test-Path -LiteralPath (Join-Path $work 'server-port.txt'))) {
        if ([DateTime]::UtcNow -gt $deadline -or $server.State -eq 'Failed') { throw 'Mock server did not start.' }
        Start-Sleep -Milliseconds 50
    }
    $port = [int](Get-Content -LiteralPath (Join-Path $work 'server-port.txt') -Raw)
    $baseUrl = "http://127.0.0.1:$port"
    $unusedListener = [Net.Sockets.TcpListener]::new([Net.IPAddress]::Loopback, 0)
    $unusedListener.Start()
    $offlinePort = $unusedListener.LocalEndpoint.Port
    $unusedListener.Stop()
    Invoke-Case 'valid' '/valid' $true 'TEST: all files validated before installation'
    Invoke-Case 'redirect' '/redirect' $true 'TEST: all files validated before installation'
    Invoke-Case 'corrupt' '/corrupt' $false 'could not be downloaded or verified'
    Invoke-Case 'truncated' '/truncated' $false 'could not be downloaded or verified'
    Invoke-Case 'missing' '/missing' $false '404'
    Invoke-Case 'unavailable' '/unavailable' $false '503'
    Invoke-Case 'offline' 'offline' $false 'could not be downloaded or verified'
    Invoke-Case 'oversized' '/oversized' $false 'could not be downloaded or verified'
    Invoke-Case 'missing-russian' '/missing' $false 'Не удалось загрузить или проверить JobTracker' 'russian'
    Invoke-Case 'cancel-download' '/slow' $false 'Download or extraction cancelled'
    Invoke-Case 'cancel-extract' '/valid' $false 'Download or extraction cancelled'
    Invoke-Case 'tampered-extracted' '/valid' $false 'release files are invalid'
    Invoke-Case 'reparse-destination' '/valid' $false 'symbolic link/junction'
    $badArchiveBytes = [Text.Encoding]::ASCII.GetBytes('Not a 7z archive')
    $badHash = [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($badArchiveBytes)).ToLowerInvariant()
    Invoke-Case 'bad-archive' '/bad-archive' $false 'release files are invalid' 'english' $badHash $badArchiveBytes.Length
    Invoke-Case 'traversal' '/unsafe' $false 'release files are invalid' 'english' (Get-FileHash -LiteralPath $unsafeArchive -Algorithm SHA256).Hash (Get-Item -LiteralPath $unsafeArchive).Length
    Invoke-Case 'retry' '' $true 'TEST: failed attempt cleaned; retrying'
    if ($CheckPublicDownload) {
        $httpClient = [Net.Http.HttpClient]::new()
        $httpClient.Timeout = [TimeSpan]::FromSeconds(20)
        try {
            $request = [Net.Http.HttpRequestMessage]::new([Net.Http.HttpMethod]::Head, $metadata.url)
            $response = $httpClient.Send($request)
            $status = [int]$response.StatusCode
            Assert-Check ($status -in 200, 404) "Public endpoint status $status needs manual investigation."
            [ordered]@{url=$metadata.url;status=$status} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $work 'public-asset-head.json')
        } finally { $httpClient.Dispose() }
        $pattern = if ($status -eq 200) { 'TEST: all files validated before installation' } else { '404' }
        Invoke-Case 'public-asset' 'public' ($status -eq 200) $pattern
    }
    Write-Output "PASS: $($results.Count) online checks; retained evidence $work."
} finally {
    Stop-Job -Job $server
    Receive-Job -Job $server | Set-Content -LiteralPath (Join-Path $work 'server-output.log')
    Remove-Job -Job $server
}
