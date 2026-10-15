#requires -Version 7.2
<#
Read-only reproduction of the stage-14 private-QML finding. Reads the generated
resource and the actual archived application; prints only metadata and hashes.
It does not execute the application or print/save recovered source contents.
This is specific to the inspected stage-13 resource layout, not a general PE parser.
#>
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$generatedPath = 'build/qt-6.10.3/windows-release/.qt/rcc/qrc_JobTrackerApp_raw_qml_0.cpp'
$generated = Get-Content -LiteralPath $generatedPath -Raw
$arrayMatch = [regex]::Match($generated, '(?s)qt_resource_data\[\] = \{(.*?)\};')
if (-not $arrayMatch.Success -or $arrayMatch.Groups[1].Value -notmatch '^\s*// Main\.qml\r?\n') {
    throw 'Generated resource layout changed; inspect it before interpreting record zero.'
}
[byte[]]$resourceData = @([regex]::Matches($arrayMatch.Groups[1].Value, '0x([0-9a-fA-F]+)') |
    ForEach-Object { [Convert]::ToByte($_.Groups[1].Value, 16) })
$recordSize = ($resourceData[0] * 16777216) + ($resourceData[1] * 65536) +
    ($resourceData[2] * 256) + $resourceData[3]
if ($recordSize -lt 5 -or ($recordSize + 4) -gt $resourceData.Length) { throw 'Invalid first resource length.' }
[byte[]]$compressed = $resourceData[8..(3 + $recordSize)]
$inputStream = [IO.MemoryStream]::new($compressed, $false)
$decoder = [IO.Compression.ZLibStream]::new($inputStream, [IO.Compression.CompressionMode]::Decompress)
$outputStream = [IO.MemoryStream]::new()
try {
    $decoder.CopyTo($outputStream)
    $recoveredHash = [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($outputStream.ToArray())).ToLowerInvariant()
    $sourceHash = (Get-FileHash -LiteralPath qml/Main.qml -Algorithm SHA256).Hash.ToLowerInvariant()
    $exePath = 'dist/stage13-package/work/extracted/bin/JobTrackerApp.exe'
    $exe = [IO.File]::ReadAllBytes((Join-Path (Get-Location) $exePath))
    $latin = [Text.Encoding]::Latin1
    $offset = $latin.GetString($exe).IndexOf($latin.GetString($resourceData), [StringComparison]::Ordinal)
    [ordered]@{
        executable=$exePath
        executableSha256=(Get-FileHash -LiteralPath $exePath -Algorithm SHA256).Hash.ToLowerInvariant()
        resourceArrayBytes=$resourceData.Length;embeddedInArchiveExe=($offset -ge 0);exeOffset=$offset
        mainQmlCompressedRecordBytes=$recordSize;recoveredMainQmlBytes=$outputStream.Length
        recoveredSourceSha256=$recoveredHash;matchesCurrentMainQml=($recoveredHash -ceq $sourceHash)
        qrcFileCount=([xml](Get-Content -LiteralPath 'build/qt-6.10.3/windows-release/.qt/rcc/JobTrackerApp_raw_qml_0.qrc' -Raw)).RCC.qresource.file.Count
        privateSourceCriterion=$(if ($offset -ge 0 -and $recoveredHash -ceq $sourceHash) { 'FAIL: recoverable Main.qml is embedded' } else { 'NOT_ESTABLISHED' })
    } | ConvertTo-Json -Depth 3
} finally {
    $decoder.Dispose()
    $inputStream.Dispose()
    $outputStream.Dispose()
}
