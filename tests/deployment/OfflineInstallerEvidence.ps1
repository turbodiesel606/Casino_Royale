#requires -Version 7.2
# Test-only reader for the private evidence produced by the offline Setup build.
# Never copy this JSON beside the end-user EXE or accept an archive manifest here.
. (Join-Path $PSScriptRoot '../../Installer/Windows/PayloadValidation.ps1')
function Get-OfflineInstallerEvidence {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$InstallerPath,
        [Parameter(Mandatory)][string]$MetadataPath
    )
    $evidence = Get-Content -LiteralPath $MetadataPath -Raw | ConvertFrom-Json
    if ($evidence.kind -cne 'offline' -or
        $evidence.sha256 -ine (Get-FileHash -LiteralPath $InstallerPath -Algorithm SHA256).Hash) {
        throw 'Rebuild offline Setup: evidence kind or installer hash mismatch.'
    }
    $item = Get-Item -LiteralPath $InstallerPath
    $approvedName = "JobTracker-Setup-$($evidence.version).exe"
    if ($evidence.PSObject.Properties.Name -contains 'installerFileName') { $approvedName = $evidence.installerFileName }
    $approvedPattern = '^JobTracker-Setup-' + [regex]::Escape($evidence.version) + '(?:-\d{8}T\d{6}Z-[0-9a-f]{12})?\.exe$'
    if ($approvedName -cnotmatch $approvedPattern -or $item.Name -cne $approvedName -or
        [IO.Path]::GetFileName($evidence.installer) -cne $approvedName -or $item.Length -ne $evidence.size) {
        throw 'Offline Setup filename/size does not match its private build evidence.'
    }
    $info = Assert-ReleaseBinary $InstallerPath
    if ($info.ProductName.Trim() -cne 'JobTracker' -or $info.ProductVersion.Trim() -cne $evidence.version -or
        $info.FileVersion.Trim() -cne "$($evidence.version).0") {
        throw 'Offline Setup version does not match its private build evidence.'
    }
    $installerSource = Join-Path $PSScriptRoot '../../Installer/Windows'
    foreach ($name in @('JobTracker.iss', 'Build-Installer.ps1', 'PayloadValidation.ps1',
            'PayloadSafety.iss', 'InstallationPolicy.iss', 'ManualClose.iss', 'PersonalData.iss',
            'MsvcRuntime.iss', 'Prepare-MsvcRuntime.ps1', 'MsvcRuntime.lock.json',
            'MSVC-NOTICE.txt', 'payload-files.txt')) {
        if ($evidence.sourceHashes.$name -ine
            (Get-FileHash -LiteralPath (Join-Path $installerSource $name) -Algorithm SHA256).Hash) {
            throw "Rebuild offline Setup: source hash mismatch for $name."
        }
    }
    return $evidence
}
