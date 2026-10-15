$ErrorActionPreference = 'Stop'
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class Stage05Resources {
    [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
    public static extern IntPtr LoadLibraryEx(string file, IntPtr reserved, uint flags);
    [DllImport("kernel32.dll", SetLastError=true)]
    public static extern IntPtr FindResource(IntPtr module, IntPtr name, IntPtr type);
    [DllImport("kernel32.dll", SetLastError=true)]
    public static extern IntPtr LoadResource(IntPtr module, IntPtr resource);
    [DllImport("kernel32.dll")]
    public static extern IntPtr LockResource(IntPtr resource);
    [DllImport("kernel32.dll")]
    public static extern uint SizeofResource(IntPtr module, IntPtr resource);
    [DllImport("kernel32.dll")]
    public static extern bool FreeLibrary(IntPtr module);
    public static byte[] Read(IntPtr module, int type, int id) {
        IntPtr resource = FindResource(module, (IntPtr)id, (IntPtr)type);
        if (resource == IntPtr.Zero) throw new Exception("Missing resource " + type + ":" + id);
        byte[] bytes = new byte[SizeofResource(module, resource)];
        Marshal.Copy(LockResource(LoadResource(module, resource)), bytes, 0, bytes.Length);
        return bytes;
    }
}
'@
$root = 'D:/Project_CV/Root'
$cmake = Get-Content "$root/CMakeLists.txt" -Raw
$version = [regex]::Match($cmake, 'project\(JobTracker\s+VERSION\s+(\d+\.\d+\.\d+)').Groups[1].Value
if (!$version) { throw 'No project version' }
$ico = [IO.File]::ReadAllBytes("$root/packaging/windows/JobTracker.ico")
$count = [BitConverter]::ToUInt16($ico,4)
if ($count -ne 7) { throw 'Unexpected icon frame count' }
$rows = @()
foreach ($item in @(
    @{File='C:/Users/elmar/AppData/Local/Temp/JobTrackerAcceptance-20261015-143044/staging/bin/JobTrackerApp.exe'; Debug=$false; Identity='C:/Users/elmar/AppData/Local/Temp/JobTrackerAcceptance-20261015-143044/production/packaging/windows/Release'},
    @{File='D:/Project_CV/Root/build/qt-6.10.3/windows-release-stage13/bin/JobTrackerApp.exe'; Debug=$false; Identity='D:/Project_CV/Root/build/qt-6.10.3/windows-release/packaging/windows/Release'}
)) {
    $path = $item.File
    $info = [Diagnostics.FileVersionInfo]::GetVersionInfo($path)
    $numericFile = "$($info.FileMajorPart).$($info.FileMinorPart).$($info.FileBuildPart).$($info.FilePrivatePart)"
    $numericProduct = "$($info.ProductMajorPart).$($info.ProductMinorPart).$($info.ProductBuildPart).$($info.ProductPrivatePart)"
    if ($info.FileVersion -ne $version -or $info.ProductVersion -ne $version -or
        $numericFile -ne "$version.0" -or $numericProduct -ne "$version.0" -or
        $info.ProductName -ne 'JobTracker' -or $info.OriginalFilename -ne 'JobTrackerApp.exe' -or
        $info.CompanyName -ne '' -or $info.LegalCopyright -ne '' -or $info.IsDebug -ne $item.Debug) {
        throw "Incorrect VERSIONINFO: $path"
    }
    $module = [Stage05Resources]::LoadLibraryEx($path,[IntPtr]::Zero,2)
    if ($module -eq [IntPtr]::Zero) { throw "Cannot read PE resources: $path" }
    try {
        $group = [Stage05Resources]::Read($module,14,1)
        if ([BitConverter]::ToUInt16($group,4) -ne $count) { throw 'Icon group count mismatch' }
        for ($i=0; $i -lt $count; $i++) {
            $sourceEntry = 6 + 16 * $i
            $groupEntry = 6 + 14 * $i
            for ($j=0; $j -lt 12; $j++) {
                if ($ico[$sourceEntry+$j] -ne $group[$groupEntry+$j]) { throw 'Icon entry mismatch' }
            }
            $id = [BitConverter]::ToUInt16($group,$groupEntry+12)
            $actual = [Stage05Resources]::Read($module,3,$id)
            $offset = [BitConverter]::ToUInt32($ico,$sourceEntry+12)
            $length = [BitConverter]::ToUInt32($ico,$sourceEntry+8)
            $expected = [byte[]]$ico[$offset..($offset+$length-1)]
            if ([Convert]::ToBase64String($actual) -cne [Convert]::ToBase64String($expected)) { throw 'Icon bytes differ' }
        }
    } finally { [void][Stage05Resources]::FreeLibrary($module) }
    $identity = $item.Identity
    $cmakeIdentity = Get-Content "$identity/JobTrackerIdentity.cmake" -Raw
    $issIdentity = Get-Content "$identity/JobTrackerIdentity.iss" -Raw
    if (!$cmakeIdentity.Contains("set(JOBTRACKER_RELEASE_VERSION `"$version`")") -or
        !$cmakeIdentity.Contains("set(JOBTRACKER_WINDOWS_VERSION `"$version.0`")") -or
        !$issIdentity.Contains("#define JobTrackerReleaseVersion `"$version`"") -or
        !$issIdentity.Contains("#define JobTrackerWindowsVersion `"$version.0`"") -or
        !$issIdentity.Contains('#define JobTrackerShortcutIconFilename "{app}\bin\JobTrackerApp.exe"') -or
        !$issIdentity.Contains('#define JobTrackerUninstallDisplayIcon "{app}\bin\JobTrackerApp.exe,0"') -or
        !$issIdentity.Contains("#define JobTrackerSetupIconFile `"$root/packaging/windows/JobTracker.ico`"")) {
        throw 'Generated packaging identity mismatch'
    }
    $rows += [pscustomobject]@{File=$item.File; StringVersion=$version; NumericVersion=$numericFile; Debug=$info.IsDebug; IconFrames=$count; IconBytesMatch=$true; PackagingIdentityMatches=$true}
}
$rows | ConvertTo-Json | Set-Content "$PSScriptRoot/logs/17-identity-results.json"
$rows | Format-Table
