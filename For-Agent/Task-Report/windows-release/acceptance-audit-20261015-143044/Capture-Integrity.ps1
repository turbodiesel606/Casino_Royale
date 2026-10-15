param([ValidateSet('before','after')][string]$Phase)
$ErrorActionPreference = 'Stop'
$root = 'D:\Project_CV\Root'
$audit = $PSScriptRoot
$excluded = @('.git','.vs','build','dist')
function Collect-Tree([string]$Directory) {
  foreach ($item in Get-ChildItem -LiteralPath $Directory -Force) {
    if ($item.FullName -eq $audit) { continue }
    if ($Directory -eq $root -and $excluded -contains $item.Name) { continue }
    $relative = [IO.Path]::GetRelativePath($root, $item.FullName).Replace('\','/')
    if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) {
      [pscustomobject]@{ Path=$relative; Kind='Link'; Length=$null; SHA256=$null; Attributes=[string]$item.Attributes; LastWriteUtc=$item.LastWriteTimeUtc.ToString('o'); Target=[string]$item.LinkTarget }
    } elseif ($item.PSIsContainer) {
      [pscustomobject]@{ Path=$relative; Kind='Directory'; Length=$null; SHA256=$null; Attributes=[string]$item.Attributes; LastWriteUtc=$null; Target=$null }
      Collect-Tree $item.FullName
    } else {
      [pscustomobject]@{ Path=$relative; Kind='File'; Length=$item.Length; SHA256=(Get-FileHash -LiteralPath $item.FullName -Algorithm SHA256).Hash; Attributes=[string]$item.Attributes; LastWriteUtc=$item.LastWriteTimeUtc.ToString('o'); Target=$null }
    }
  }
}
$rows = @(Collect-Tree $root | Sort-Object Path)
$rows | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $audit "snapshots/project-$Phase.json") -Encoding utf8
@($rows | Where-Object { $_.Path -eq 'src' -or $_.Path.StartsWith('src/') }) | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $audit "snapshots/src-$Phase.json") -Encoding utf8
& git -c core.fsmonitor=false -c core.untrackedCache=false status --porcelain=v1 --untracked-files=all | Set-Content -LiteralPath (Join-Path $audit "snapshots/git-status-$Phase.txt") -Encoding utf8
& git diff --binary | Set-Content -LiteralPath (Join-Path $audit "snapshots/working-diff-$Phase.patch") -Encoding utf8
& git diff --cached --binary | Set-Content -LiteralPath (Join-Path $audit "snapshots/staged-diff-$Phase.patch") -Encoding utf8
& git log -1 --format='%H%n%aI%n%s' | Set-Content -LiteralPath (Join-Path $audit "snapshots/head-$Phase.txt") -Encoding utf8
if ($Phase -eq 'after') {
  $before = Get-Content -LiteralPath (Join-Path $audit 'snapshots/project-before.json') -Raw | ConvertFrom-Json -DateKind String
  $changes = @(Compare-Object ($before | ForEach-Object { $_ | ConvertTo-Json -Compress }) ($rows | ForEach-Object { $_ | ConvertTo-Json -Compress }))
  $srcChanges = @($changes | Where-Object { ($_.InputObject | ConvertFrom-Json).Path -match '^src(?:/|$)' })
  [pscustomobject]@{ TimeUtc=[DateTime]::UtcNow.ToString('o'); ProtectedEntries=$rows.Count; ChangedEntries=$changes.Count; SrcChangedEntries=$srcChanges.Count; Changes=$changes; ExcludedRootDirectories=$excluded; ExcludedAuditDirectory=$audit } | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $audit 'snapshots/integrity-result.json') -Encoding utf8
}
[pscustomobject]@{ Phase=$Phase; Entries=$rows.Count; SrcEntries=@($rows | Where-Object { $_.Path -eq 'src' -or $_.Path.StartsWith('src/') }).Count; Excluded=$excluded } | ConvertTo-Json
