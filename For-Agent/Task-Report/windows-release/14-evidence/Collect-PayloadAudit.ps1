#requires -Version 7.2
<#
Stage-14 evidence collector for the existing stage-13 candidates, not a release
approval tool. Run from the repository root after Package-Release verification.
Reads local files only; never starts the app, installs, downloads or deletes.
The JSON is INTERNAL evidence (contains build/source paths), not a ship asset.
#>
[CmdletBinding()]
param([Parameter(Mandatory)][string]$OutputDirectory)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = (Get-Location).Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
if (-not $output.StartsWith((Join-Path $root 'dist') + [IO.Path]::DirectorySeparatorChar,
        [StringComparison]::OrdinalIgnoreCase)) { throw 'Output must be below repository dist/.' }
if (Test-Path -LiteralPath $output) { throw 'Choose a new output directory; prior evidence is preserved.' }
$ancestor = $output
while ($ancestor) {
    if ((Test-Path -LiteralPath $ancestor) -and
        ((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw 'Output path must not contain reparse points.'
    }
    $ancestor = [IO.Path]::GetDirectoryName($ancestor)
}
$kit = 'D:/Qt_install/6.10.3/msvc2022_64'
$stage = Join-Path $root 'dist/stage13-package/work/extracted'
$manifestPath = Join-Path $root 'dist/stage13-package/JobTracker-0.1.0-windows-x64.7z.manifest.json'
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
$lock = Get-Content -LiteralPath packaging/windows/MsvcRuntime.lock.json -Raw | ConvertFrom-Json
$records = [Collections.Generic.List[object]]::new()
$sbomInputs = [Collections.Generic.List[object]]::new()
$sbomFiles = @{}
$packages = @{}
$owners = @{}
$dependencies = @{}
$extracts = @{}
function Get-Digest([string]$Path, [string]$Algorithm = 'SHA256') {
    (Get-FileHash -LiteralPath $Path -Algorithm $Algorithm).Hash.ToLowerInvariant()
}
function Get-Value($Object, [string]$Name) {
    if ($Object.PSObject.Properties.Name -contains $Name) { return $Object.$Name }
    return $null
}
foreach ($module in 'qtbase','qtdeclarative','qtimageformats','qtsvg','qtlottie','qttranslations') {
    $path = Join-Path $kit "sbom/$module-6.10.3.spdx.json"
    $bom = Get-Content -LiteralPath $path -Raw | ConvertFrom-Json
    $moduleRoot = @($bom.packages | Where-Object name -CEQ $module)[0]
    $sbomInputs.Add([ordered]@{
        path=$path;sha256=(Get-Digest $path);namespace=$bom.documentNamespace
        source=$moduleRoot.downloadLocation;buildDescription=(Get-Value $moduleRoot 'comment')
    })
    foreach ($file in $bom.files) {
        $relative = $file.fileName -creplace '^\./',''
        if (-not $sbomFiles.ContainsKey($relative)) { $sbomFiles[$relative] = @() }
        $sbomFiles[$relative] += [pscustomobject]@{module=$module;file=$file}
    }
    foreach ($package in $bom.packages) {
        $packages[$package.SPDXID] = [pscustomobject]@{module=$module;package=$package}
    }
    foreach ($rel in $bom.relationships) {
        if ($rel.relationshipType -eq 'CONTAINS' -and $rel.relatedSpdxElement -like 'SPDXRef-PackagedFile-*') {
            if (-not $owners.ContainsKey($rel.relatedSpdxElement)) { $owners[$rel.relatedSpdxElement] = @() }
            $owners[$rel.relatedSpdxElement] += $rel.spdxElementId
        }
        if ($rel.relationshipType -eq 'DEPENDS_ON') {
            if (-not $dependencies.ContainsKey($rel.spdxElementId)) { $dependencies[$rel.spdxElementId] = @() }
            # Cross-document IDs retain a globally unique package suffix in this
            # installed kit. Resolve that suffix against the loaded package map.
            $dependencies[$rel.spdxElementId] += ($rel.relatedSpdxElement -creplace '^DocumentRef-[^:]+:','')
        }
    }
    foreach ($license in @(Get-Value $bom 'hasExtractedLicensingInfos')) {
        if ($license) { $extracts[$license.licenseId] = $license.extractedText }
    }
}
$seedIds = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
$actualPaths = @(Get-ChildItem -LiteralPath $stage -Recurse -File -Force | ForEach-Object {
    [IO.Path]::GetRelativePath($stage, $_.FullName).Replace('\','/')
} | Sort-Object)
if (Compare-Object $actualPaths @($manifest.files.path | Sort-Object)) { throw 'Stage/manifest path mismatch.' }
foreach ($entry in $manifest.files) {
    $path = Join-Path $stage $entry.path
    $sha = Get-Digest $path
    if ($sha -cne $entry.sha256 -or (Get-Item -LiteralPath $path).Length -ne $entry.size) {
        throw "Stage/manifest bytes differ: $($entry.path)"
    }
    $mappings = @()
    $mappingPath = $entry.path
    $mappingKind = 'Direct SBOM file path; raw digest comparison retained separately'
    if (-not $sbomFiles.ContainsKey($mappingPath) -and $entry.path.EndsWith('/qmldir')) {
        $parent = [IO.Path]::GetDirectoryName($entry.path).Replace('\','/')
        $sibling = @($manifest.files | Where-Object {
            $_.path.EndsWith('.dll') -and [IO.Path]::GetDirectoryName($_.path).Replace('\','/') -ceq $parent
        })
        if ($sibling.Count -eq 1 -and $sbomFiles.ContainsKey($sibling[0].path)) {
            $mappingPath = $sibling[0].path
            $mappingKind = 'QML metadata module attribution through sibling plugin; not a direct SBOM file record'
        }
    }
    if ($sbomFiles.ContainsKey($mappingPath)) {
        foreach ($match in $sbomFiles[$mappingPath]) {
            $file = $match.file
            foreach ($owner in @($owners[$file.SPDXID])) {
                if ($owner) { [void]$seedIds.Add($owner) }
            }
            $expectedSha1 = @($file.checksums | Where-Object algorithm -EQ 'SHA1' | ForEach-Object checksumValue)
            $mappings += [ordered]@{
                module=$match.module; spdxId=$file.SPDXID; license=$file.licenseConcluded
                evidence=$mappingKind;sbomFile=$file.fileName
                copyright=$file.copyrightText; sbomSha1=$expectedSha1
                sbomSha1Matches=$(if ($mappingPath -ceq $entry.path) { (Get-Digest $path 'SHA1') -cin $expectedSha1 } else { $null })
                owners=@($owners[$file.SPDXID] | Sort-Object -Unique)
            }
        }
    }
    $sdkPath = Join-Path $kit $entry.path
    $sdkMatches = if (Test-Path -LiteralPath $sdkPath -PathType Leaf) { (Get-Digest $sdkPath) -ceq $sha } else { $null }
    $binary = $entry.path -match '\.(dll|exe)$'
    $info = if ($binary) { [Diagnostics.FileVersionInfo]::GetVersionInfo($path) } else { $null }
    $signature = if ($binary) { Get-AuthenticodeSignature -LiteralPath $path } else { $null }
    $category = if ($mappings.Count -and $mappingPath -cne $entry.path) { 'Qt QML metadata (module attribution)' }
        elseif ($mappings.Count) { 'Qt SBOM file-path mapped' }
        elseif ($entry.path -ceq 'bin/JobTrackerApp.exe') { 'JobTracker executable' }
        elseif ($entry.path -ceq 'bin/qt.conf') { 'Generated deployment configuration' }
        else { 'External binary: license/provenance review required' }
    $records.Add([ordered]@{
        path=$entry.path; size=$entry.size; sha256=$sha; category=$category
        sdkSha256Matches=$sdkMatches; sbom=$mappings
        signatureStatus=$(if ($signature) { "$($signature.Status)" } else { $null })
        signer=$(if ($signature -and $signature.SignerCertificate) { $signature.SignerCertificate.Subject } else { $null })
        fileVersion=$(if ($info) { $info.FileVersion } else { $null })
        productVersion=$(if ($info) { $info.ProductVersion } else { $null })
        company=$(if ($info) { $info.CompanyName } else { $null })
        binaryCopyright=$(if ($info) { $info.LegalCopyright } else { $null })
    })
}
# Only DEPENDS_ON closure of packages owning shipped files. Do not call every
# SDK tool, source file, debug library or entire module root a runtime dependency.
$queue = [Collections.Generic.Queue[string]]::new()
foreach ($id in $seedIds) { $queue.Enqueue($id) }
$seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
$closure = [Collections.Generic.List[object]]::new()
while ($queue.Count) {
    $id = $queue.Dequeue()
    if (-not $seen.Add($id)) { continue }
    if ($packages.ContainsKey($id)) {
        $item = $packages[$id]
        $p = $item.package
        $closure.Add([ordered]@{
            spdxId=$id;module=$item.module;name=$p.name;version=(Get-Value $p 'versionInfo')
            license=$p.licenseConcluded;copyright=(Get-Value $p 'copyrightText')
            source=(Get-Value $p 'downloadLocation');homepage=(Get-Value $p 'homepage')
            shippedFileOwner=$seedIds.Contains($id); dependencies=@($dependencies[$id])
        })
        foreach ($dep in @($dependencies[$id])) { if ($dep) { $queue.Enqueue($dep) } }
    } else {
        $closure.Add([ordered]@{spdxId=$id;unresolvedExternalReference=$true})
    }
}
$runtime = [Collections.Generic.List[object]]::new()
$installerEvidence = Get-Content -LiteralPath dist/stage13-online/installer-build.json -Raw | ConvertFrom-Json
foreach ($file in $lock.files) {
    $path = Join-Path $root "dist/stage13-online/msvc-runtime/$($file.name)"
    $sha = Get-Digest $path
    $local = Join-Path $root "dist/stage13-local/msvc-runtime/$($file.name)"
    $source = Join-Path $installerEvidence.msvcRuntime.crtSource $file.name
    if ($sha -cne $file.sha256 -or (Get-Digest $local) -cne $sha -or
        (Get-Digest $source) -cne $sha) { throw 'Runtime candidate/lock/VS REDIST source mismatch.' }
    $sig = Get-AuthenticodeSignature -LiteralPath $path
    $runtime.Add([ordered]@{
        path="bin/$($file.name)";scope='CurrentUser (embedded in both installers)'
        size=(Get-Item -LiteralPath $path).Length;sha256=$sha;source=$source;sourceSha256Matches=$true
        version=[Diagnostics.FileVersionInfo]::GetVersionInfo($path).FileVersion
        signatureStatus="$($sig.Status)";signer=$(if ($sig.SignerCertificate) { $sig.SignerCertificate.Subject } else { $null })
        license='Microsoft Visual Studio distributable-code terms; owner entitlement required'
    })
}
$artifacts = [Collections.Generic.List[object]]::new()
foreach ($path in @($manifestPath, ($manifestPath -replace '\.manifest\.json$',''),
    ($manifestPath -replace '\.manifest\.json$','.sha256'),
    (Join-Path $root 'dist/stage13-online/JobTracker-0.1.0-windows-x64-online-setup.exe'),
    (Join-Path $root 'dist/stage13-local/JobTracker-0.1.0-windows-x64-local-setup.exe'),
    (Join-Path $root 'build/qt-6.10.3/stage09-evidence/vc_redist.x64.exe'))) {
    $sig = if ($path.EndsWith('.exe')) { Get-AuthenticodeSignature -LiteralPath $path } else { $null }
    $artifacts.Add([ordered]@{
        path=[IO.Path]::GetRelativePath($root,$path).Replace('\','/');size=(Get-Item -LiteralPath $path).Length
        sha256=(Get-Digest $path);signatureStatus=$(if ($sig) { "$($sig.Status)" } else { $null })
        signer=$(if ($sig -and $sig.SignerCertificate) { $sig.SignerCertificate.Subject } else { $null })
    })
}
$redistArtifact = @($artifacts | Where-Object path -Like '*vc_redist.x64.exe')[0]
if ($redistArtifact.sha256 -cne $lock.redistributable.sha256 -or
    $redistArtifact.size -ne $lock.redistributable.size) { throw 'Official runtime differs from lock.' }
# Bounded raw ASCII/UTF-16 credential-pattern scan. Values never enter evidence.
# Compressed/encrypted data, arbitrary secrets and reverse engineering are outside
# this check. A zero hit count is NOT an exhaustive absence-of-secrets assertion.
$patterns = [ordered]@{
    privateKey='-----BEGIN (?:RSA |EC |OPENSSH |DSA )?PRIVATE KEY-----'
    githubToken='(?:gh[pousr]_[A-Za-z0-9]{36,}|github_pat_[A-Za-z0-9_]{50,})'
    awsAccessId='(?:AKIA|ASIA)[A-Z0-9]{16}'
    openaiProjectKey='sk-proj-[A-Za-z0-9_-]{40,}'
}
$scan = [Collections.Generic.List[object]]::new()
$scanPaths = @($manifest.files | ForEach-Object { Join-Path $stage $_.path })
$scanPaths += @($lock.files | ForEach-Object { Join-Path $root "dist/stage13-online/msvc-runtime/$($_.name)" })
$scanPaths += @($artifacts | ForEach-Object { Join-Path $root $_.path })
foreach ($path in $scanPaths) {
    $bytes = [IO.File]::ReadAllBytes($path)
    $ascii = [Text.Encoding]::ASCII.GetString($bytes)
    $wide = [Text.Encoding]::Unicode.GetString($bytes)
    foreach ($key in $patterns.Keys) {
        $count = [regex]::Matches($ascii,$patterns[$key]).Count + [regex]::Matches($wide,$patterns[$key]).Count
        if ($count) {
            $completePem = '-----BEGIN (?:RSA |EC |OPENSSH |DSA )?PRIVATE KEY-----\r?\n[A-Za-z0-9+/=\r\n ]{32,}-----END (?:RSA |EC |OPENSSH |DSA )?PRIVATE KEY-----'
            $scan.Add([ordered]@{
                path=[IO.Path]::GetRelativePath($root,$path);kind=$key;count=$count
                completePemBlocks=([regex]::Matches($ascii,$completePem).Count + [regex]::Matches($wide,$completePem).Count)
            })
        }
    }
}
$result = [ordered]@{
    schemaVersion=1;scope='Stage-13 bytes; internal audit evidence, NOT release approval'
    collectedUtc=[DateTime]::UtcNow.ToString('o');manifestSha256=(Get-Digest $manifestPath)
    archiveSha256=$manifest.sha256;payload=$records.ToArray();sbomInputs=$sbomInputs.ToArray()
    packageDependencyClosure=$closure.ToArray();customLicenseTexts=$extracts
    appLocalRuntime=$runtime.ToArray();officialRedistributable=$lock.redistributable
    windowsIcuSource=[ordered]@{
        path='C:/Windows/System32/icuuc.dll';sha256=(Get-Digest 'C:/Windows/System32/icuuc.dll')
        stagedSha256Matches=((Get-Digest 'C:/Windows/System32/icuuc.dll') -ceq (Get-Digest (Join-Path $stage 'bin/icuuc.dll')))
        licenseStatus='No redistribution grant established for this Windows system binary'
    }
    artifacts=$artifacts.ToArray();credentialPatternScan=[ordered]@{
        filesScanned=$scanPaths.Count;patterns=@($patterns.Keys);findings=$scan.ToArray()
        limitation='Raw ASCII/UTF-16 known-pattern scan only; compressed data and arbitrary credentials not covered.'
    }
}
New-Item -ItemType Directory -Path $output | Out-Null
$result | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath (Join-Path $output 'payload-audit.json') -Encoding utf8
Write-Output "Collected $($records.Count) payload files, $($runtime.Count) App-Local DLLs, $($closure.Count) dependency records."
Write-Output "Credential-pattern findings: $($scan.Count). This is evidence collection, not license/security approval."
Write-Output "Internal output: $output/payload-audit.json"
