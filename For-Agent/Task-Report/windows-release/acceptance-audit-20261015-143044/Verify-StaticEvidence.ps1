$ErrorActionPreference='Stop'
$doc=Get-Content -LiteralPath 'For-Agent/Docs/build.md' -Raw
$blocks=[regex]::Matches($doc,'(?s)```powershell\s*(.*?)\s*```')
$parseProblems=@()
foreach($block in $blocks){$tokens=$null;$errors=$null;$null=[Management.Automation.Language.Parser]::ParseInput($block.Groups[1].Value,[ref]$tokens,[ref]$errors);foreach($error in $errors){$parseProblems+=$error.Message}}
$shared=Get-Content -LiteralPath 'CMakePresets.json' -Raw|ConvertFrom-Json
$local=Get-Content -LiteralPath 'CMakeUserPresets.json' -Raw|ConvertFrom-Json
$allNames=@($shared.configurePresets.name)+@($shared.buildPresets.name)+@($shared.testPresets.name)+@($local.configurePresets.name)+@($local.buildPresets.name)+@($local.testPresets.name)
$named=@([regex]::Matches($doc,'--preset\s+([A-Za-z0-9_-]+)')|ForEach-Object{$_.Groups[1].Value}|Sort-Object -Unique)
$scripts=@([regex]::Matches($doc,'-File\s+([^\s]+\.ps1)')|ForEach-Object{$_.Groups[1].Value}|Sort-Object -Unique)
$baseline=Get-Content -LiteralPath 'dist/stage15-evidence/input-sha256.json' -Raw|ConvertFrom-Json
$preserved=@($baseline|Where-Object{$_.Path -match '^\.codex/|^For-Agent/(Review/|Task-Report/(?!windows-release/STATUS))|^\.agents/skills/(cpp-|qml-|qt-qml)'})
$differences=@(foreach($file in $preserved){if(-not(Test-Path -LiteralPath $file.Path) -or (Get-FileHash -LiteralPath $file.Path).Hash -ine $file.SHA256){$file.Path}})
$audit=Get-Content -LiteralPath 'For-Agent/Task-Report/windows-release/14-evidence/payload-audit.json' -Raw|ConvertFrom-Json
$manifest=Get-Content -LiteralPath 'dist/stage13-package/JobTracker-0.1.0-windows-x64.7z.manifest.json' -Raw|ConvertFrom-Json
$runtime=@(foreach($file in $audit.appLocalRuntime){
 $inputPath=Join-Path 'dist/stage13-online/msvc-runtime' ([IO.Path]::GetFileName($file.path))
 [pscustomobject]@{Path=$file.path;InputMatches=((Get-FileHash -LiteralPath $inputPath).Hash -ieq $file.sha256);VendorMatches=((Get-FileHash -LiteralPath $file.source).Hash -ieq $file.sha256)}
})
$sboms=@(foreach($file in $audit.sbomInputs){[pscustomobject]@{Path=$file.path;SHA256=(Get-FileHash -LiteralPath $file.path).Hash.ToLowerInvariant();Matches=((Get-FileHash -LiteralPath $file.path).Hash -ieq $file.sha256)}})
$lottie=@(foreach($file in $audit.payload|Where-Object{$_.path -match 'Lottie'}){
 $actual=Join-Path 'dist/stage13-package/work/extracted' $file.path
 $sdk=Join-Path 'D:/Qt_install/6.10.3/msvc2022_64' $file.path
 [pscustomobject]@{Path=$file.path;SHA256=(Get-FileHash -LiteralPath $actual).Hash.ToLowerInvariant();KitMatches=((Get-FileHash -LiteralPath $actual).Hash -eq (Get-FileHash -LiteralPath $sdk).Hash);InventoryMatches=((Get-FileHash -LiteralPath $actual).Hash -ieq $file.sha256)}
})
$spdx=Get-Content -LiteralPath 'D:/Qt_install/6.10.3/msvc2022_64/sbom/qtlottie-6.10.3.spdx.json' -Raw|ConvertFrom-Json
$result=[ordered]@{
 DateUtc=[DateTime]::UtcNow.ToString('o');
 Documentation=[ordered]@{CommandBlocks=$blocks.Count;ParseErrors=$parseProblems;MissingPresets=@($named|Where-Object{$_ -notin $allNames});MissingScripts=@($scripts|Where-Object{-not(Test-Path -LiteralPath $_)});PresetCount=$named.Count;ScriptCount=$scripts.Count};
 PreservedHistoricalInputs=[ordered]@{Count=$preserved.Count;Differences=$differences};
 Payload=[ordered]@{ManifestCount=$manifest.files.Count;InventoryCount=$audit.payload.Count;PathDifferences=@(Compare-Object ($manifest.files.path|Sort-Object) ($audit.payload.path|Sort-Object));DeliveredLegalPaths=@($manifest.files.path|Where-Object{$_ -match 'licen[cs]e|notice|source-offer|replacement'})};
 Runtime=$runtime;Sboms=$sboms;Lottie=$lottie;
 LottieLicenseRecords=@($spdx.files|Where-Object{$_.fileName -in './bin/Qt6Lottie.dll','./bin/Qt6LottieVectorImageGenerator.dll','./plugins/vectorimageformats/qlottievectorimage.dll'}|Select-Object fileName,licenseConcluded);
 Icu=[ordered]@{PayloadSHA256=(Get-FileHash -LiteralPath 'dist/stage13-package/work/extracted/bin/icuuc.dll').Hash.ToLowerInvariant();WindowsSHA256=(Get-FileHash -LiteralPath 'C:/Windows/System32/icuuc.dll').Hash.ToLowerInvariant();InQtKit=(Test-Path -LiteralPath 'D:/Qt_install/6.10.3/msvc2022_64/bin/icuuc.dll')};
 InnoCompilerBanner=@(Get-Content -LiteralPath (Join-Path $PSScriptRoot 'logs/inno-compile-details.log') -TotalCount 8);
 InnoLocalLicenseSHA256=(Get-FileHash -LiteralPath 'D:/Inno Setup 6/license.txt').Hash.ToLowerInvariant()
}
$result|ConvertTo-Json -Depth 10|Set-Content -LiteralPath (Join-Path $PSScriptRoot 'logs/19-static-evidence.json') -Encoding utf8
$result|ConvertTo-Json -Depth 10
