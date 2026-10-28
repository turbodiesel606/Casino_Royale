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
            'PayloadSafety.iss', 'InstallationPolicy.iss', 'InstallOwnership.iss', 'PersonalData.iss',
            'MsvcRuntime.iss', 'Prepare-MsvcRuntime.ps1', 'MsvcRuntime.lock.json',
            'MSVC-NOTICE.txt', 'payload-files.txt')) {
        if ($evidence.sourceHashes.$name -ine
            (Get-FileHash -LiteralPath (Join-Path $installerSource $name) -Algorithm SHA256).Hash) {
            throw "Rebuild offline Setup: source hash mismatch for $name."
        }
    }
    return $evidence
}

# WScript.Shell uses an ANSI shortcut interface on some Windows versions. Use
# IShellLinkW for independent verification of arbitrary Unicode installation paths.
function Get-InstallerShortcutInfo([string]$Path) {
    if (-not ('JobTrackerInstallerTests.ShortcutReader' -as [type])) {
        Add-Type -TypeDefinition @'
using System;
using System.Text;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;
namespace JobTrackerInstallerTests {
    [ComImport, Guid("00021401-0000-0000-C000-000000000046")]
    internal class ShellLink { }
    [ComImport, Guid("000214F9-0000-0000-C000-000000000046"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface IShellLinkW {
        [PreserveSig] int GetPath([Out, MarshalAs(UnmanagedType.LPWStr)] StringBuilder path, int size, IntPtr findData, uint flags);
        [PreserveSig] int GetIDList(out IntPtr value);
        [PreserveSig] int SetIDList(IntPtr value);
        [PreserveSig] int GetDescription([Out, MarshalAs(UnmanagedType.LPWStr)] StringBuilder value, int size);
        [PreserveSig] int SetDescription([MarshalAs(UnmanagedType.LPWStr)] string value);
        [PreserveSig] int GetWorkingDirectory([Out, MarshalAs(UnmanagedType.LPWStr)] StringBuilder value, int size);
        [PreserveSig] int SetWorkingDirectory([MarshalAs(UnmanagedType.LPWStr)] string value);
        [PreserveSig] int GetArguments([Out, MarshalAs(UnmanagedType.LPWStr)] StringBuilder value, int size);
        [PreserveSig] int SetArguments([MarshalAs(UnmanagedType.LPWStr)] string value);
        [PreserveSig] int GetHotkey(out short value);
        [PreserveSig] int SetHotkey(short value);
        [PreserveSig] int GetShowCmd(out int value);
        [PreserveSig] int SetShowCmd(int value);
        [PreserveSig] int GetIconLocation([Out, MarshalAs(UnmanagedType.LPWStr)] StringBuilder path, int size, out int index);
    }
    public sealed class ShortcutInfo {
        public string Target { get; internal set; }
        public string WorkingDirectory { get; internal set; }
        public string IconFilename { get; internal set; }
        public int IconIndex { get; internal set; }
    }
    public static class ShortcutReader {
        public static ShortcutInfo Read(string path) {
            object instance = new ShellLink();
            try {
                ((IPersistFile)instance).Load(path, 0);
                var link = (IShellLinkW)instance;
                var target = new StringBuilder(32768);
                var directory = new StringBuilder(32768);
                var icon = new StringBuilder(32768);
                int index;
                Marshal.ThrowExceptionForHR(link.GetPath(target, target.Capacity, IntPtr.Zero, 4));
                Marshal.ThrowExceptionForHR(link.GetWorkingDirectory(directory, directory.Capacity));
                Marshal.ThrowExceptionForHR(link.GetIconLocation(icon, icon.Capacity, out index));
                return new ShortcutInfo { Target=target.ToString(), WorkingDirectory=directory.ToString(), IconFilename=icon.ToString(), IconIndex=index };
            } finally { Marshal.FinalReleaseComObject(instance); }
        }
    }
}
'@
    }
    return [JobTrackerInstallerTests.ShortcutReader]::Read($Path)
}
