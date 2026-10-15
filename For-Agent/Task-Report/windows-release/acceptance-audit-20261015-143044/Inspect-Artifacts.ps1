param([ValidateSet('before','after')][string]$Phase)
$ErrorActionPreference = 'Stop'
$root = 'D:\Project_CV\Root'
$manifestPath = Join-Path $root 'dist/stage13-package/JobTracker-0.1.0-windows-x64.7z.manifest.json'
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
$stage = Join-Path $root 'build/qt-6.10.3/windows-release-stage13'
$paths = [Collections.Generic.List[string]]::new()
foreach ($file in $manifest.files) { $paths.Add((Join-Path $stage $file.path)) }
foreach ($relative in @('dist/stage13-package/JobTracker-0.1.0-windows-x64.7z','dist/stage13-package/JobTracker-0.1.0-windows-x64.7z.sha256','dist/stage13-package/JobTracker-0.1.0-windows-x64.7z.manifest.json','dist/stage13-online/JobTracker-0.1.0-windows-x64-online-setup.exe','dist/stage13-local/JobTracker-0.1.0-windows-x64-local-setup.exe','dist/stage13-online/installer-build.json','dist/stage13-local/installer-build.json','dist/stage13-online/JobTrackerPayload.iss','dist/stage13-local/JobTrackerPayload.iss','build/qt-6.10.3/windows-release/Release/JobTrackerApp.exe','build/qt-6.10.3/windows-deployment-tests/Release/JobTrackerDeploymentSmoke.exe')) { $paths.Add((Join-Path $root $relative)) }
foreach ($item in Get-ChildItem -LiteralPath (Join-Path $root 'dist/stage13-online/msvc-runtime') -File) { $paths.Add($item.FullName) }
$rows = @(foreach ($path in $paths) {
  $item=Get-Item -LiteralPath $path
  if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Link rejected: $path" }
  [pscustomobject]@{Path=[IO.Path]::GetRelativePath($root,$path).Replace('\','/');Size=$item.Length;SHA256=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant();LastWriteUtc=$item.LastWriteTimeUtc.ToString('o');FileVersion=$item.VersionInfo.FileVersion;ProductVersion=$item.VersionInfo.ProductVersion;IsDebug=$item.VersionInfo.IsDebug}
})
$rows | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $PSScriptRoot "snapshots/artifacts-$Phase.json") -Encoding utf8
$issues=[Collections.Generic.List[string]]::new()
foreach($file in $manifest.files) {
  $row=$rows | Where-Object Path -CEQ ('build/qt-6.10.3/windows-release-stage13/'+$file.path)
  if($row.SHA256 -cne $file.sha256 -or $row.Size -ne $file.size) {$issues.Add($file.path)}
}
$metadata=Get-Content -LiteralPath (Join-Path $root 'dist/stage13-online/installer-build.json') -Raw | ConvertFrom-Json
$scriptProperties=@{ 'JobTracker.iss'='scriptSha256'; 'OnlinePayload.iss'='onlinePayloadScriptSha256'; 'MsvcRuntime.iss'='msvcScriptSha256'; 'InstallationPolicy.iss'='installationPolicySha256'; 'ManualClose.iss'='manualCloseSha256'; 'PersonalData.iss'='personalDataSha256' }
$binding=@(foreach($name in $scriptProperties.Keys) {
  $actual=(Get-FileHash -LiteralPath (Join-Path $root "packaging/windows/$name") -Algorithm SHA256).Hash.ToLowerInvariant()
  [pscustomobject]@{Path="packaging/windows/$name";Actual=$actual;Recorded=$metadata.($scriptProperties[$name]);Matches=($actual -ceq $metadata.($scriptProperties[$name]))}
})
$archiveRow=$rows | Where-Object Path -CEQ 'dist/stage13-package/JobTracker-0.1.0-windows-x64.7z'
$noticePaths=@($manifest.files | Where-Object path -match 'licen[sc]e|notice|copying|replacement|source-offer')
$result=[ordered]@{Phase=$Phase;Manifest=$manifestPath;Version=$manifest.version;Files=$manifest.files.Count;StagedMismatch=$issues;ArchiveHashMatches=($archiveRow.SHA256 -ceq $manifest.sha256);ArchiveBytes=$archiveRow.Size;ArchiveSHA256=$archiveRow.SHA256;SetupArchiveHashMatches=($metadata.archiveSha256 -ceq $manifest.sha256);FixedURL=$metadata.url;ScriptBinding=$binding;DeliveredNoticePaths=$noticePaths}
if($Phase -eq 'after') {
 $before=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'snapshots/artifacts-before.json') -Raw | ConvertFrom-Json -DateKind String
 $changes=@(Compare-Object ($before|ForEach-Object{$_|ConvertTo-Json -Compress}) ($rows|ForEach-Object{$_|ConvertTo-Json -Compress}))
 $result['ChangedExistingArtifactEntries']=$changes.Count
 $result['Changes']=$changes
}
$result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $PSScriptRoot "snapshots/artifact-verification-$Phase.json") -Encoding utf8
$result | ConvertTo-Json -Depth 8
