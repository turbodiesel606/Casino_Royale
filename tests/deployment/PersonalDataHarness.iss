; TEST ONLY. Every deletion target is a fresh fixture below TestOutput.
#if Ver < EncodeVer(7, 0, 0)
  #error Inno Setup 7 or newer is required.
#endif
#define JobTrackerExecutableName "JobTrackerApp.exe"
#include "../../Installer/Windows/PayloadSafety.iss"
#include "../../Installer/Windows/InstallationPolicy.iss"
#include "../../Installer/Windows/PersonalData.iss"

[Setup]
AppName=JobTracker personal data TEST ONLY
AppVersion=0.1.0
DefaultDirName={#TestOutput}\program
OutputDir={#TestOutput}
OutputBaseFilename=personal-data-test-only
PrivilegesRequired=lowest
SetupArchitecture=x64
ArchitecturesAllowed=x64os
ArchitecturesInstallIn64BitMode=x64os
MinVersion=10.0
Uninstallable=yes
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

[Files]
Source: "{#TestOutput}\program.txt"; DestDir: "{app}\bin"; Flags: ignoreversion

[Code]
procedure RequireTest(const Condition: Boolean; const Detail: String);
begin
  if not Condition then RaiseException('TEST FAILURE: ' + Detail);
  Log('TEST PASS: ' + Detail);
end;

procedure TestCurrentSid(var Identity: TPersonalDataIdentity);
var Needed: Cardinal;
begin
  ClearPersonalDataIdentity(Identity);
  RequireTest(DataOpenProcessToken(THandle(-1), 8, Identity.Token) <> 0, 'query fixture account token');
  Needed := 256;
  Identity.SidBuffer := DataAllocate($40, Needed);
  RequireTest(Identity.SidBuffer <> 0, 'allocate fixture SID buffer');
  RequireTest(DataTokenInformation(Identity.Token, 1, Identity.SidBuffer, Needed, Needed) <> 0,
    'read fixture account SID');
  DataReadPointer(Identity.Sid, Identity.SidBuffer, SizeOf(Identity.Sid));
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  CaseName, LocalData, Root, Expected, Failure: String;
  Identity: TPersonalDataIdentity;
begin
  Result := '';
  try
    CaseName := ExpandConstant('{param:Case}');
    if CaseName = 'lifecycle' then Exit;
    LocalData := ExpandConstant('{param:LocalData}');
    Root := ExpandConstant('{param:Root}');
    Expected := ExpandConstant('{param:Expected}');
    RequireTest(Pos(AddBackslash('{#TestOutput}'), LocalData) = 1, 'isolated LocalData fixture');
    RequireTest(Pos(AddBackslash('{#TestOutput}'), Root) = 1, 'isolated root fixture');
    if CaseName = 'delete' then begin
      TestCurrentSid(Identity);
      try
        Failure := '';
        try DeleteConfirmedPersonalData(LocalData, Root, Identity.Sid);
        except Failure := GetExceptionMessage; end;
        if Expected = '' then RequireTest(Failure = '', 'native explicit cleanup: ' + Failure)
        else begin
          Log('Expected cleanup failure: ' + Failure);
          RequireTest(Pos(Expected, Failure) > 0, 'failure reason: ' + Expected + ' (' + Failure + ')');
        end;
      finally
        ReleasePersonalDataIdentity(Identity);
      end;
    end else if CaseName = 'invalid-account' then begin
      Failure := '';
      try DeleteConfirmedPersonalData(LocalData, Root, 0);
      except Failure := GetExceptionMessage; end;
      RequireTest(Pos('Invalid confirmed account SID', Failure) > 0,
        'missing confirmed account identity refused');
    end;
  except
    Result := GetExceptionMessage;
  end;
end;

procedure CurUninstallStepChanged(CurStep: TUninstallStep);
var Expected: String;
begin
  PersonalDataAfterUninstall(CurStep);
  if CurStep <> usDone then Exit;
  Expected := ExpandConstant('{param:Expected}');
  RequireTest(FileExists(ExpandConstant('{app}\bin\program.txt')) = (Expected = 'partial'),
    'actual native program-removal result');
  RequireTest(DataChoiceHandled, 'silent preservation policy executed independently of removal');
  RequireTest(SaveStringToFile(ExpandConstant('{param:Result}'), 'PASS', False), 'lifecycle result');
end;
