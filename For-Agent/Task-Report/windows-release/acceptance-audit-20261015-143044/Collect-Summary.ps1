$ErrorActionPreference='Stop'
$root='D:/Project_CV/Root'
$fresh='dist/acceptance-audit-20261015-143044'
$old=Get-Content -LiteralPath "$root/dist/stage13-package/JobTracker-0.1.0-windows-x64.7z.manifest.json" -Raw|ConvertFrom-Json
$new=Get-Content -LiteralPath "$root/$fresh/package/JobTracker-0.1.0-windows-x64.7z.manifest.json" -Raw|ConvertFrom-Json
$changed=@(foreach($record in $new.files){$previous=$old.files|Where-Object path -CEQ $record.path;if($previous.sha256 -cne $record.sha256){[pscustomobject]@{Path=$record.path;Previous=$previous.sha256;Fresh=$record.sha256}}})
$artifactPaths=@(
 'dist/stage13-package/JobTracker-0.1.0-windows-x64.7z',
 'dist/stage13-online/JobTracker-0.1.0-windows-x64-online-setup.exe',
 'dist/stage13-local/JobTracker-0.1.0-windows-x64-local-setup.exe',
 "$fresh/package/JobTracker-0.1.0-windows-x64.7z",
 "$fresh/online/JobTracker-0.1.0-windows-x64-online-setup.exe",
 'C:/Users/elmar/AppData/Local/Temp/JobTrackerAcceptance-20261015-143044/production/Release/JobTrackerApp.exe'
)
$artifactRows=@(foreach($path in $artifactPaths){$item=Get-Item -LiteralPath $path;[pscustomobject]@{Path=$path;Bytes=$item.Length;SHA256=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant();FileVersion=$item.VersionInfo.FileVersion;ProductVersion=$item.VersionInfo.ProductVersion}})
$source=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'snapshots/src-before.json') -Raw|ConvertFrom-Json
$preserved=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'snapshots/project-before.json') -Raw|ConvertFrom-Json
$initialStatus=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'snapshots/git-status-before.txt')
$existingStatus=@($initialStatus|Where-Object{$_ -notmatch 'acceptance-audit-20261015-143044/'})
$result=[ordered]@{
 DateBaku=[DateTime]::UtcNow.AddHours(4).ToString('yyyy-MM-dd HH:mm:ss');
 Head=(Get-Content -LiteralPath (Join-Path $PSScriptRoot 'snapshots/head-before.txt') -First 1);
 ChecklistSHA256=(Get-FileHash -LiteralPath "$root/Tasks/Windows-Release/JobTracker_Windows_Release_16_Tasks_Acceptance.md").Hash.ToLowerInvariant();
 AuditPromptSHA256=(Get-FileHash -LiteralPath "$root/Tasks/Windows-Release/JobTracker_Windows_Release_Audit_Prompt.md").Hash.ToLowerInvariant();
 PowerShell=$PSVersionTable.PSVersion.ToString();
 CMake=(Get-Command cmake).Version.ToString();
 InnoFileProductMetadata=(Get-Item -LiteralPath 'D:/Inno Setup 6/ISCC.exe').VersionInfo.ProductVersion;
 InnoCompilerEngine=([regex]::Match((Get-Content -LiteralPath "$root/$fresh/online/compile.log" -Raw),'Compiler engine version: Inno Setup ([0-9.]+)')).Groups[1].Value;
 OS=[Environment]::OSVersion.VersionString;
 QtCoreVersion=(Get-Item -LiteralPath 'D:/Qt_install/6.10.3/msvc2022_64/bin/Qt6Core.dll').VersionInfo.FileVersion;
 SourceFiles=@($source|Where-Object Kind -eq 'File').Count;
 SourceDirectories=@($source|Where-Object Kind -eq 'Directory').Count;
 SourceLinks=@($source|Where-Object Kind -eq 'Link').Count;
 ProtectedEntries=$preserved.Count;
 ProtectedFiles=@($preserved|Where-Object Kind -eq 'File').Count;
 InitialTrackedModified=@($existingStatus|Where-Object{$_ -notmatch '^\?\?'}).Count;
 InitialUntracked=@($existingStatus|Where-Object{$_ -match '^\?\?'}).Count;
 Artifacts=$artifactRows;
 FreshChangedPayloadFiles=$changed
}
$result|ConvertTo-Json -Depth 8|Set-Content -LiteralPath (Join-Path $PSScriptRoot 'snapshots/audit-summary.json') -Encoding utf8
Copy-Item -LiteralPath "$root/$fresh/package/JobTracker-0.1.0-windows-x64.7z.manifest.json" -Destination (Join-Path $PSScriptRoot 'snapshots/fresh-package-manifest.json')
Copy-Item -LiteralPath "$root/$fresh/online/installer-build.json" -Destination (Join-Path $PSScriptRoot 'snapshots/fresh-installer-build.json')
Copy-Item -LiteralPath "$root/$fresh/online/compile.log" -Destination (Join-Path $PSScriptRoot 'logs/inno-compile-details.log')
Copy-Item -LiteralPath "$root/$fresh/online-tests/results.json" -Destination (Join-Path $PSScriptRoot 'logs/online-results.json')
Copy-Item -LiteralPath "$root/dist/package-tests-3080de2ed9a944138bb5c674a17695ad/results.json" -Destination (Join-Path $PSScriptRoot 'logs/package-results.json')
$result|ConvertTo-Json -Depth 8
