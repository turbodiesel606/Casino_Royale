#requires -Version 7.2
<# Inspect one compiled offline Setup and its private inputs. No installation,
network access, prerequisite execution or application launch occurs here. #>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$BuildDirectory,
    [Parameter(Mandatory)][string]$InstallerPath,
    [Parameter(Mandatory)][string]$MetadataPath,
    [Parameter(Mandatory)][string]$WorkDirectory
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'OfflineInstallerEvidence.ps1')
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$work = [IO.Path]::GetFullPath($WorkDirectory)
if (-not $work.StartsWith((Join-Path $repository 'build') + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Offline artifact checks must use a fresh directory below repository build/.'
}
Assert-NoReparsePoint $work
if (Test-Path -LiteralPath $work) { throw 'Test output exists; choose a fresh directory.' }
$metadata = Get-OfflineInstallerEvidence $InstallerPath $MetadataPath
$private = Split-Path -Parent ([IO.Path]::GetFullPath($MetadataPath))
New-Item -ItemType Directory -Path $work | Out-Null
$results = [Collections.Generic.List[object]]::new()
function Assert-Check([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}
function Record-Pass([string]$Name, [object]$Evidence) {
    $results.Add([ordered]@{name=$Name;result='PASS';evidence=$Evidence})
    $results | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $work 'results.json')
    Write-Output "PASS: $Name"
}
function Assert-Rejected([string]$Name, [scriptblock]$Action, [string]$Pattern) {
    $message = ''
    try { & $Action | Out-Null } catch { $message = $_.Exception.Message }
    Assert-Check ($message -match $Pattern) "${Name}: expected $Pattern; got '$message'."
    Record-Pass $Name $message
}
Record-Pass 'native-x64-setup-name-version-hash-sources' $metadata.sha256
$verified = Get-VerifiedReleasePayload $BuildDirectory $metadata.payloadDirectory
Assert-Check ($verified.Version -ceq $metadata.version -and $verified.Files.Count -eq $metadata.payloadFileCount) 'Setup payload identity/count mismatch.'
Assert-Check (($verified.Files | ConvertTo-Json -Compress) -ceq ($metadata.files | ConvertTo-Json -Compress)) 'Setup payload snapshot differs from build evidence.'
Record-Pass 'exact-release-snapshot' $verified.Files.Count

$identityFile = Join-Path $BuildDirectory 'Installer/Release/JobTrackerIdentity.iss'
Assert-Check ((Get-Sha256 $identityFile) -ceq $metadata.identitySha256) 'Generated Inno identity changed since compilation.'
Assert-Check ((Get-Sha256 $metadata.compiler) -ceq $metadata.compilerSha256) 'Compiler changed since compilation.'
Record-Pass 'compiler-and-generated-identity' $metadata.compiler
$payloadFile = Join-Path $private 'JobTrackerPayload.iss'
Assert-Check ((Get-Sha256 $payloadFile) -ceq $metadata.generatedPayloadSha256) 'Generated payload include changed since compilation.'
$payloadText = Get-Content -LiteralPath $payloadFile -Raw
$payloadEntries = [regex]::Matches($payloadText, '(?m)^Source: "([^"]+)"; DestDir: "([^"]+)"; Hash: "([a-f0-9]{64})"; Flags: ([^;\r\n]+);')
Assert-Check ($payloadEntries.Count -eq $verified.Files.Count) 'Generated direct payload entry count mismatch.'
for ($index = 0; $index -lt $verified.Files.Count; ++$index) {
    $file = $verified.Files[$index]
    $entry = $payloadEntries[$index].Groups
    $relative = $file.path.Replace('/', '\')
    Assert-Check ($entry[1].Value -ceq (Join-Path $metadata.payloadDirectory $relative) -and
        $entry[2].Value -ceq ('{app}\' + [IO.Path]::GetDirectoryName($relative)) -and
        $entry[3].Value -ceq $file.sha256 -and $entry[4].Value -ceq 'ignoreversion') 'Generated payload entry must embed its exact local file/hash/destination.'
}
Record-Pass 'direct-file-embedding' $payloadEntries.Count

$runtimeDirectory = Join-Path $private 'msvc-runtime'
$runtimeText = Get-Content -LiteralPath (Join-Path $runtimeDirectory 'JobTrackerMsvcRuntime.iss') -Raw
$lock = Get-Content -LiteralPath (Join-Path $repository 'Installer/MsvcRuntime.lock.json') -Raw | ConvertFrom-Json
Assert-Check ((Get-Sha256 (Join-Path $repository 'Installer/MsvcRuntime.lock.json')) -ceq $metadata.msvcRuntime.sourceLockSha256) 'Runtime lock changed since compilation.'
Assert-Check (($metadata.msvcRuntime.files | ConvertTo-Json -Compress) -ceq ($lock.files | ConvertTo-Json -Compress)) 'Runtime evidence differs from pinned DLL inventory.'
Assert-Check (($metadata.msvcRuntime.redistributable | ConvertTo-Json -Compress) -ceq ($lock.redistributable | ConvertTo-Json -Compress)) 'Runtime evidence differs from the pinned redistributable.'
$runtimeEntries = [regex]::Matches($runtimeText, '(?m)^Source: "([^"]+)"; [^\r\n]+\r?$')
Assert-Check ($runtimeEntries.Count -eq ($metadata.msvcRuntime.files.Count + 1)) 'Expected exactly the local redist and App-Local runtime inputs.'
$runtimeFiles = @($metadata.msvcRuntime.files) + @([pscustomobject]@{
    name='vc_redist.x64.exe';size=$metadata.msvcRuntime.redistributable.size;sha256=$metadata.msvcRuntime.redistributable.sha256
})
foreach ($file in $runtimeFiles) {
    $path = Join-Path $runtimeDirectory $file.name
    Assert-NoReparsePoint $path
    Assert-Check ((Get-Item -LiteralPath $path).Length -eq $file.size -and (Get-Sha256 $path) -ceq $file.sha256) "Runtime input mismatch: $($file.name)"
    $entries = @($runtimeEntries | Where-Object { $_.Groups[1].Value -ceq $path })
    Assert-Check ($entries.Count -eq 1 -and $entries[0].Value.Contains('Hash: "' + $file.sha256 + '"')) "Runtime embedding mismatch: $($file.name)"
}
Record-Pass 'pinned-local-runtime-embedding' $runtimeFiles.Count

# Static guard over active Inno sources and generated file entries. The lock's
# official URL is provenance only and is deliberately outside this executable scan.
$activeSources = @(Get-ChildItem -LiteralPath (Join-Path $repository 'Installer') -Filter '*.iss' -File)
$sourceLines = $activeSources | ForEach-Object { Get-Content -LiteralPath $_.FullName }
# Localized explanatory license links are text, not transport. Keep every code,
# directive and Files entry in the scan, including external-file flags.
$scan = (($sourceLines | Where-Object { $_ -notmatch '^\s*(;|//|english\.|russian\.)' }) -join "`n") + "`n" + $payloadText + "`n" + $runtimeText
Assert-Check ($scan -notmatch '(?i)https?://|ftp://|\.7z\b|DownloadTemporaryFile|Extract7ZipArchive|CreateDownloadPage|WinHttp|WinInet|URLDownloadToFile|OnlinePayload|Flags:[^;\r\n]*\bexternal\b') 'Active Setup contains transport/archive/external-file behavior.'
foreach ($entry in @($payloadEntries) + @($runtimeEntries)) {
    Assert-Check ($entry.Groups[1].Value -match '^[A-Za-z]:[\\/][^*?]+$') 'Setup inputs must be concrete local drive files, never network paths or wildcards.'
}
Record-Pass 'no-network-or-external-payload' 'Static executable-source and generated-entry check; disconnected VM remains separate.'

foreach ($scenario in @('kind', 'hash', 'size', 'version', 'source')) {
    $changed = Get-Content -LiteralPath $MetadataPath -Raw | ConvertFrom-Json
    $pattern = ''
    switch ($scenario) {
        'kind' { $changed.kind = 'retired'; $pattern = 'kind or installer hash' }
        'hash' { $changed.sha256 = '0' * 64; $pattern = 'kind or installer hash' }
        'size' { $changed.size += 1; $pattern = 'filename/size' }
        'version' { $changed.version = '999.0.0'; $pattern = 'filename/size|version' }
        'source' { $changed.sourceHashes.'MsvcRuntime.lock.json' = '0' * 64; $pattern = 'source hash mismatch' }
    }
    $path = Join-Path $work "$scenario.json"
    $changed | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $path
    Assert-Rejected "reject-stale-$scenario-evidence" { Get-OfflineInstallerEvidence $InstallerPath $path } $pattern
}
Assert-Rejected 'reject-missing-setup' { Get-OfflineInstallerEvidence (Join-Path $work 'missing.exe') $MetadataPath } 'does not exist|Cannot find|Could not find'
$renamed = Join-Path $work 'incorrect-name.exe'
Copy-Item -LiteralPath $InstallerPath -Destination $renamed
Assert-Rejected 'reject-renamed-setup' { Get-OfflineInstallerEvidence $renamed $MetadataPath } 'filename/size'
$null = Get-OfflineInstallerEvidence $InstallerPath $MetadataPath
Record-Pass 'original-setup-preserved' $metadata.sha256
Write-Output "PASS: $($results.Count) offline artifact checks. Evidence: $work"
