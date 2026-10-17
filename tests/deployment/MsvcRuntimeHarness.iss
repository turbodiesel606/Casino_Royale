; TEST ONLY: consumes production functions, never executes the redistributable.
#if Ver < EncodeVer(7, 0, 0)
  #error Inno Setup 7 or newer is required to verify the native x64 installer ABI.
#endif
#include PipelineFile
#define JobTrackerExecutableName "JobTrackerApp.exe"
#include "..\..\Installer\InstallationPolicy.iss"
#include "..\..\Installer\ManualClose.iss"
#include RuntimeInput
#include RuntimeCode

[Setup]
AppName=JobTracker MSVC Runtime TEST ONLY
AppVersion=0.0.0
DefaultDirName={tmp}\unused
OutputDir={#TestOutput}
OutputBaseFilename=msvc-runtime-test-only
PrivilegesRequired=lowest
SetupArchitecture=x64
ArchitecturesAllowed=x64os
ArchitecturesInstallIn64BitMode=x64os
MinVersion=10.0
Uninstallable=no
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
procedure RequireTest(const Condition: Boolean; const Message: String);
begin
  if not Condition then RaiseException('TEST FAILURE: ' + Message);
  Log('TEST PASS: ' + Message);
end;

procedure DeinitializeSetup;
begin
  CleanupMsvcRuntime;
end;

procedure CheckResult(const Launched: Boolean; const Code: Integer;
  const Compatible: Boolean; const ExpectedMessage: String; const Restart: Boolean);
var
  NeedsRestart: Boolean;
  Message: String;
begin
  NeedsRestart := False;
  MsvcRestartPending := False;
  Message := EvaluateMsvcResult(Launched, Code, Compatible, NeedsRestart);
  if ExpectedMessage = '' then
    RequireTest(Message = '', 'accepted compatible result ' + IntToStr(Code))
  else
    RequireTest(Pos(CustomMessage(ExpectedMessage), Message) = 1,
      'rejected incomplete result ' + IntToStr(Code));
  RequireTest((NeedsRestart = Restart) and (MsvcRestartPending = Restart),
    'restart state for ' + IntToStr(Code));
  MsvcRestartPending := False;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  CaseName, PackagePath: String;
begin
  Result := '';
  CaseName := ExpandConstant('{param:Case}');
  Log('TEST temporary directory: ' + ExpandConstant('{tmp}'));
  try
    if CaseName = 'policy' then begin
      CheckResult(True, 0, True, '', False);
      CheckResult(True, 0, False, 'MsvcFailed', False);
      CheckResult(False, 2, True, 'MsvcFailed', False);
      CheckResult(True, 1602, False, 'MsvcCancelled', False);
      CheckResult(True, -2147023294, False, 'MsvcCancelled', False);
      CheckResult(True, 1638, True, '', False);
      CheckResult(True, 1638, False, 'MsvcFailed', False);
      CheckResult(True, -2147023258, True, '', False);
      CheckResult(True, -2147023258, False, 'MsvcFailed', False);
      CheckResult(True, 3010, True, 'MsvcRestart', True);
      CheckResult(True, 1641, False, 'MsvcRestart', True);
      CheckResult(True, 1603, True, 'MsvcFailed', False);
      RequireTest(not MsvcFileCompatible(ExpandConstant('{tmp}\missing.dll')), 'missing runtime file rejected');
      RequireTest(not MsvcFileCompatible(ExpandConstant('{srcexe}')), 'old version rejected');
      MsvcRestartPending := True;
      Result := PrepareMsvcRuntime(True, NeedsRestart);
      RequireTest(NeedsRestart and (Result = CustomMessage('MsvcRestart')), 'retry preserves reboot requirement');
      MsvcRestartPending := False;
      NeedsRestart := False;
      Result := '';
    end else if CaseName = 'currentuser' then begin
      Result := PrepareMsvcRuntime(False, NeedsRestart);
      RequireTest((Result = '') and not NeedsRestart, 'current-user skips system prerequisite');
    end else if CaseName = 'system-present' then begin
      // Read-only detection only. Calling the all-users orchestration could race
      // an external runtime uninstall and unexpectedly execute the prerequisite.
      RequireTest(MsvcSystemCompatible, 'host has compatible x64 registry and system files');
    end else if (CaseName = 'destination-conflict') or (CaseName = 'reparse') then begin
      CheckMsvcDestination;
      RaiseException('TEST FAILURE: unsafe destination was accepted');
    end else if (CaseName = 'integrity') or (CaseName = 'corrupt') then begin
      ExtractTemporaryFile('vc_redist.x64.exe');
      PackagePath := ExpandConstant('{tmp}\vc_redist.x64.exe');
      if CaseName = 'corrupt' then SaveStringToFile(PackagePath, 'corrupt', True);
      VerifyPayloadFile(PackagePath, '{#MsvcRedistHash}', {#MsvcRedistSize});
      RequireTest(MsvcFileCompatible(PackagePath), 'verified official x64 package version');
      if CaseName = 'corrupt' then RaiseException('TEST FAILURE: corrupt package accepted');
    end else RaiseException('TEST FAILURE: unknown case');
  except
    Result := GetExceptionMessage;
    Log(Result);
  end;
  CleanupMsvcRuntime;
  if FileExists(ExpandConstant('{tmp}\vc_redist.x64.exe')) then
    RaiseException('TEST FAILURE: extracted prerequisite remains');
end;
