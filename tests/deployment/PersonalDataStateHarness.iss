; TEST ONLY. Read-only Windows token/profile/known-folder resolution.
#if Ver < EncodeVer(7, 0, 0)
  #error Inno Setup 7 or newer is required.
#endif
#define JobTrackerExecutableName "JobTrackerApp.exe"
#include "../../Installer/Windows/PayloadSafety.iss"
#include "../../Installer/Windows/InstallationPolicy.iss"
#include "../../Installer/Windows/PersonalData.iss"

[Setup]
AppName=JobTracker personal data identity TEST ONLY
AppVersion=0.1.0
DefaultDirName={tmp}\unused
OutputDir={#TestOutput}
OutputBaseFilename=personal-data-state-test-only
PrivilegesRequired=lowest
SetupArchitecture=x64
ArchitecturesAllowed=x64os
ArchitecturesInstallIn64BitMode=x64os
MinVersion=10.0
Uninstallable=no
CreateAppDir=no
CreateUninstallRegKey=no
UsePreviousAppDir=no
DisableDirPage=yes
DisableProgramGroupPage=yes
CloseApplications=no
RestartApplications=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Code]
function PrepareToInstall(var NeedsRestart: Boolean): String;
var Identity: TPersonalDataIdentity;
begin
  Result := '';
  ClearPersonalDataIdentity(Identity);
  try
    try
      ResolveExecutionPersonalDataIdentity(Identity);
      if CompareText(Identity.LocalData, ExpandConstant('{param:ExpectedLocalData}')) <> 0 then
        RaiseException('TEST FAILURE: execution Local AppData mismatch: ' + Identity.LocalData);
      if CompareText(Identity.Profile, ExpandConstant('{param:ExpectedProfile}')) <> 0 then
        RaiseException('TEST FAILURE: execution profile mismatch: ' + Identity.Profile);
      if Identity.SidText <> ExpandConstant('{param:ExpectedSid}') then
        RaiseException('TEST FAILURE: execution SID mismatch: ' + Identity.SidText);
      if Pos(Identity.SidText, Identity.Account) = 0 then
        RaiseException('TEST FAILURE: account display lacks SID');
      ReleasePersonalDataIdentity(Identity);
      ResolvePersonalDataIdentity(Identity);
      if (Identity.LocalData = '') or (Identity.Profile = '') or
         (Identity.Account = '') or (Identity.ExecutionAccount = '') then
        RaiseException('TEST FAILURE: incomplete target identity');
      Log('TEST PASS: Windows token/profile/known-folder identity resolved without opening application data.');
      Log('Target account: ' + Identity.Account + '; execution account: ' + Identity.ExecutionAccount);
      if not SaveStringToFile('{#TestOutput}\identity.txt', 'PASS', False) then
        RaiseException('TEST FAILURE: identity result');
    except
      Result := GetExceptionMessage;
    end;
  finally
    ReleasePersonalDataIdentity(Identity);
  end;
end;
