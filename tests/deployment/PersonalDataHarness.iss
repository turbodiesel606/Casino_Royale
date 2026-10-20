; TEST ONLY. All deletion fixtures are confined to TestOutput, never LocalAppData.
#if Ver < EncodeVer(7, 0, 0)
  #error Inno Setup 7 or newer is required to verify native x64 personal-data handles.
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
function TestSid(const Text: String; var Sid: THandle): Longint;
  external 'ConvertStringSidToSidW@advapi32.dll stdcall';

procedure RequireTest(const Condition: Boolean; const Detail: String);
begin
  if not Condition then RaiseException('TEST FAILURE: ' + Detail);
  Log('TEST PASS: ' + Detail);
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  CaseName, LocalData, Root, Expected, Failure: String;
  Sid, Handle: THandle;
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
      Failure := '';
      try DeleteConfirmedPersonalData(LocalData, Root);
      except Failure := GetExceptionMessage; end;
      if Expected = '' then RequireTest(Failure = '', 'native explicit cleanup: ' + Failure)
      else begin
        Log('Expected cleanup refusal: ' + Failure);
        RequireTest(Pos(Expected, Failure) > 0, 'refusal reason: ' + Expected);
      end;
    end else if CaseName = 'owner-mismatch' then begin
      Sid := 0;
      Handle := THandle(-1);
      try
        RequireTest(TestSid('S-1-5-18', Sid) <> 0, 'valid alternate account SID');
        Handle := PolicyCreateFile(Root + '\settings.json', $20080, 7, 0, 3, $200000, 0);
        RequireTest(Handle <> THandle(-1), 'open synthetic owned file');
        Failure := '';
        try RequireDataOwner(Handle, Sid);
        except Failure := GetExceptionMessage; end;
        RequireTest(Pos('owner differs', Failure) > 0, 'native SID mismatch refused');
      finally
        if Handle <> THandle(-1) then PolicyCloseHandle(Handle);
        if Sid <> 0 then DataFree(Sid);
      end;
    end else if CaseName = 'silent' then begin
      // Calling the production callback in Setup cannot authorize a cleanup:
      // it sees UninstallSilent=false, but account verification remains false.
      // Real silent lifecycle is exercised by the generated uninstaller below.
      PersonalDataAfterUninstall(usUninstall);
      RequireTest(DirExists(Root), 'pre-completion callback preserved data');
    end;
  except
    Result := GetExceptionMessage;
  end;
end;

procedure CurUninstallStepChanged(CurStep: TUninstallStep);
var Failure, Expected: String;
begin
  if CurStep <> usDone then Exit;
  Expected := ExpandConstant('{param:Expected}');
  Failure := '';
  try RequireProgramRemovalComplete;
  except Failure := GetExceptionMessage; end;
  if Expected = 'complete' then begin
    RequireTest(DataRestartCheckReached, 'native no-restart event observed');
    RequireTest(Failure = '', 'native program removal complete: ' + Failure);
  end else
    RequireTest(Failure <> '', 'native partial removal refused');
  RequireTest(SaveStringToFile(ExpandConstant('{param:Result}'), 'PASS', False), 'lifecycle result');
end;
