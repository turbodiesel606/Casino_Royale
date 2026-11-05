; TEST ONLY: no JobTracker registration/shortcuts, prerequisites or data access.
#if Ver < EncodeVer(7, 0, 0)
  #error Inno Setup 7 or newer is required to verify native x64 manual closure.
#endif
#define JobTrackerExecutableName "JobTrackerApp.exe"
#include "..\..\Installer\Windows\PayloadSafety.iss"
#include "..\..\Installer\Windows\InstallationPolicy.iss"
#include "..\..\Installer\Windows\ManualClose.iss"

[Setup]
AppName=JobTracker manual close TEST ONLY
AppVersion=0.0.0
DefaultDirName={tmp}\unused
OutputDir={#TestOutput}
OutputBaseFilename=manual-close-test-only
PrivilegesRequired=lowest
SetupArchitecture=x64
ArchitecturesAllowed=x64os
ArchitecturesInstallIn64BitMode=x64os
Uninstallable=no
CreateUninstallRegKey=no
UsePreviousAppDir=no
DirExistsWarning=no
DisableDirPage=yes
DisableProgramGroupPage=yes
CloseApplications=no
RestartApplications=no
AllowCancelDuringInstall=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Files]
Source: "{#TestPayload}"; DestDir: "{app}"; DestName: "payload.txt"; Flags: ignoreversion; Check: CanInstallWithManualClose(ExpandConstant('{app}\payload.txt'))
Source: "{#TestPayload}"; DestDir: "{app}"; DestName: "later-payload.txt"; Flags: ignoreversion; Check: CanInstallWithManualClose(ExpandConstant('{app}\later-payload.txt'))

[Code]
procedure RequireTest(Condition: Boolean; const Message: String);
begin
  if not Condition then RaiseException('TEST FAILURE: ' + Message);
  Log('TEST PASS: ' + Message);
end;

// A test-only boundary wait lets the driver start its process after preflight.
procedure WaitAtBoundary;
var
  I: Integer;
begin
  RequireTest(SaveStringToFile(ExpandConstant('{param:Ready}'), 'ready', False), 'boundary ready');
  for I := 1 to 300 do begin
    if FileExists(ExpandConstant('{param:Continue}')) then Exit;
    Sleep(100);
  end;
  RaiseException('TEST FAILURE: boundary timed out');
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if (CurStep = ssInstall) and (ExpandConstant('{param:Case}') = 'late-file') then
    WaitAtBoundary;
end;

function InitializeSetup: Boolean;
begin
  Result := ManualCloseOptionsAllowed;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  CaseName, Message: String;
begin
  Result := '';
  try
    CaseName := ExpandConstant('{param:Case}');
    if CaseName = 'probe' then begin
      Message := InstallationFileBusyMessage(ExpandConstant('{param:Path}'));
      RequireTest((Message <> '') = (ExpandConstant('{param:Busy}') = 'yes'), 'file availability');
      if Message <> '' then Log(Message);
    end else if CaseName = 'wait' then
      RequireIdleInstallationFile(ExpandConstant('{param:Path}'))
    else if CaseName = 'late-install' then begin
      RequireClosedJobTracker;
      WaitAtBoundary;
    end else if CaseName = 'late-uninstall' then begin
      RequireClosedJobTracker;
      WaitAtBoundary;
      ManualCloseBeforeUninstall(usUninstall);
      RequireTest(False, 'late uninstall must not reach file deletion');
    end;
  except
    Result := GetExceptionMessage;
    if Result = '' then Result := 'TEST: boundary aborted';
  end;
end;
