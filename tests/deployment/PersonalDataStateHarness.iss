; TEST ONLY: read-only production account/coexistence/completion consumers.
#define JobTrackerExecutableName "JobTrackerApp.exe"
#include "../../packaging/windows/OnlinePayload.iss"
#include "../../packaging/windows/InstallationPolicy.iss"
#include "../../packaging/windows/PersonalData.iss"

[Setup]
AppName=JobTracker personal data state TEST ONLY
AppVersion=0.1.0
DefaultDirName={tmp}\unused
OutputDir={#TestOutput}
OutputBaseFilename=personal-data-state-test-only
PrivilegesRequired=lowest
ArchitecturesAllowed=x64os
ArchitecturesInstallIn64BitMode=x64os
MinVersion=10.0
Uninstallable=no
; Keep {app} meaningful for the production identity check. No files are copied.
CreateAppDir=yes
CreateUninstallRegKey=no
UsePreviousAppDir=no
DirExistsWarning=no
DisableDirPage=yes
DisableProgramGroupPage=yes
CloseApplications=no
RestartApplications=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Code]
function PrepareToInstall(var NeedsRestart: Boolean): String;
var Failure, Expected, Path: String; Version: Int64; Found: Boolean;
begin
  Result := '';
  try
    Found := ReadInstallRecord(HKCU64, InstallUninstallKey + 'CurrentUser_is1', 'CurrentUser', Path, Version);
    Log(Format('State probe: admin=%d; allusers=%d; found=%d; registered=%s; selected=%s', [Ord(IsAdmin), Ord(IsAdminInstallMode), Ord(Found), Path, ExpandConstant('{app}')]));
    InitializePersonalDataPolicy;
    Expected := ExpandConstant('{param:Verified}');
    if DataAccountVerified <> (Expected = 'yes') then
      RaiseException('TEST FAILURE: account identity');
    Log('TEST PASS: account identity ' + Expected);
    Failure := '';
    try RequireNoRemainingInstallation;
    except Failure := GetExceptionMessage; end;
    if (Failure <> '') <> (ExpandConstant('{param:Shared}') = 'yes') then
      RaiseException('TEST FAILURE: shared-data gate');
    Log('TEST PASS: shared-data gate: ' + Failure);
    // No data path is resolved or touched by either check.
  except
    Result := GetExceptionMessage;
  end;
end;
