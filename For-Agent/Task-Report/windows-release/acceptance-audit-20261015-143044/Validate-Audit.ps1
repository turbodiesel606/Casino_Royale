$ErrorActionPreference='Stop'
$root='D:/Project_CV/Root'
$requirements=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'requirements.json') -Raw|ConvertFrom-Json
$judgments=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'judgments.json') -Raw|ConvertFrom-Json -AsHashtable
$coverage=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'COVERAGE.md') -Raw
$remaining=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'REMAINING_WORK.md') -Raw
$main=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'AUDIT.md') -Raw
$checklist=Get-Content -LiteralPath "$root/Tasks/Windows-Release/JobTracker_Windows_Release_16_Tasks_Acceptance.md"
$matched=[regex]::Matches($coverage,'(?m)^\| (T\d{2}-(?:\d{2}|AC)|GLOBAL-\d{2}) / \[L(\d+)\]')
$ids=@($matched|ForEach-Object{$_.Groups[1].Value})
if($matched.Count -ne 205 -or ($ids|Sort-Object -Unique).Count -ne 205){throw 'Coverage count/duplicate failure'}
$missing=@($requirements|Where-Object{$_.ID -notin $ids -or -not $judgments.ContainsKey($_.ID)})
if($missing.Count){throw 'Missing requirement judgment'}
$checkboxRows=@($requirements|Where-Object{$checklist[$_.Line-1] -match '^- \[ \] '})
if($checkboxRows.Count -ne 166 -or @($checklist|Where-Object{$_ -match '^- \[ \] '}).Count -ne 166){throw 'Checkbox coverage failure'}
$lineValues=@($matched|ForEach-Object{[int]$_.Groups[2].Value})
for($i=1;$i -lt $lineValues.Count;$i++){if($lineValues[$i] -lt $lineValues[$i-1]){throw 'Source order failure'}}
$workIds=@([regex]::Matches($remaining,'(?m)^### (R\d{2}) —')|ForEach-Object{$_.Groups[1].Value})
if($workIds.Count -ne 20){throw 'Work item count failure'}
foreach($entry in $judgments.Values){
 if($entry.implementation -notin 'IMPLEMENTED','PARTIAL','NOT_IMPLEMENTED','UNDETERMINED','N/A'){throw 'Invalid implementation status'}
 if($entry.verification -notin 'PASS','FAIL','NOT_RUN'){throw 'Invalid verification status'}
 foreach($id in [regex]::Matches($entry.work,'R\d{2}').Value){if($id -notin $workIds){throw "Unknown remaining ID $id"}}
}
$srcFlags=[regex]::Matches($remaining,'(?m)^- \*\*src/ required: (NO|YES|UNKNOWN)\.\*\*')
if($srcFlags.Count -ne 20){throw 'Missing src decision'}
$taskRows=[regex]::Matches($main,'(?m)^\| (\d{2}) \|')
if($taskRows.Count -ne 16){throw 'Missing task summary'}
$before=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'snapshots/project-before.json') -Raw|ConvertFrom-Json -DateKind String
$after=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'snapshots/project-after.json') -Raw|ConvertFrom-Json -DateKind String
$changed=@(Compare-Object $before $after -Property Path,Kind,Length,SHA256,Attributes,LastWriteUtc,Target)
if($changed.Count){throw 'Protected input changes'}
$beforeStatus=@(Get-Content -LiteralPath (Join-Path $PSScriptRoot 'snapshots/git-status-before.txt')|Where-Object{$_ -notmatch 'acceptance-audit-20261015-143044/'})
$afterStatus=@(Get-Content -LiteralPath (Join-Path $PSScriptRoot 'snapshots/git-status-after.txt')|Where-Object{$_ -notmatch 'acceptance-audit-20261015-143044/'})
$statusChange=@(Compare-Object $beforeStatus $afterStatus)
if($statusChange.Count){throw 'Unexpected Git state change'}
$diffs=@(foreach($name in 'working-diff','head'){
 $extension=if($name -eq 'head'){'txt'}else{'patch'}
 $left=Join-Path $PSScriptRoot "snapshots/$name-before.$extension"
 $right=Join-Path $PSScriptRoot "snapshots/$name-after.$extension"
 [pscustomobject]@{Record=$name;Equal=((Get-FileHash -LiteralPath $left).Hash -eq (Get-FileHash -LiteralPath $right).Hash)}
})
if(@($diffs|Where-Object{-not $_.Equal}).Count){throw 'Git diff/HEAD mismatch'}
$stagedBefore=@($beforeStatus|Where-Object{$_ -match '^[^ ?]'})
$stagedAfter=@($afterStatus|Where-Object{$_ -match '^[^ ?]'})
if($stagedBefore.Count -or $stagedAfter.Count){throw 'Expected empty staged state requires manual review'}
$diffs+=[pscustomobject]@{Record='staged state from porcelain snapshots';Equal=$true;BeforeEntries=$stagedBefore.Count;AfterEntries=$stagedAfter.Count}
$source=@($after|Where-Object{ $_.Kind -eq 'File' -and $_.Path.StartsWith('src/') })
$reportHashes=@(foreach($name in 'AUDIT.md','COVERAGE.md','REMAINING_WORK.md','COMMANDS.md'){
 $path=Join-Path $PSScriptRoot $name
 [pscustomobject]@{File=$name;Bytes=(Get-Item -LiteralPath $path).Length;SHA256=(Get-FileHash -LiteralPath $path).Hash.ToLowerInvariant()}
})
$result=[ordered]@{TimeUtc=[DateTime]::UtcNow.ToString('o');Result='PASS';CoverageRows=$matched.Count;Checkboxes=$checkboxRows.Count;TaskSummaries=$taskRows.Count;RemainingItems=$workIds.Count;SrcFlags=$srcFlags.Count;SourceFiles=$source.Count;ProtectedChangedEntries=$changed.Count;UnexpectedGitStateChanges=$statusChange.Count;GitRecords=$diffs;ReportHashes=$reportHashes}
$result|ConvertTo-Json -Depth 7|Set-Content -LiteralPath (Join-Path $PSScriptRoot 'logs/23-report-validation.json') -Encoding utf8
$result|ConvertTo-Json -Depth 7
