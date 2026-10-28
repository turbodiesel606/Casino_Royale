; Single offline installer embedding a verified direct Release stage.
; Build through Build-Installer.ps1, which supplies the generated Release identity.
#if Ver < EncodeVer(7, 0, 0)
  #error Inno Setup 7 or newer is required for the native x64 installer. Inno Setup 6 produces a forbidden 32-bit launcher.
#endif
#ifndef JobTrackerIdentityFile
  #error Build through Installer/Windows/Build-Installer.ps1.
#endif
#ifndef JobTrackerOutputDirectory
  #error An explicit installer output directory is required.
#endif
#include JobTrackerIdentityFile
#ifndef JobTrackerMsvcFile
  #error A verified MSVC runtime input is required.
#endif
; Shared native integrity/path checks also protect the runtime prerequisite.
#include "PayloadSafety.iss"
#include JobTrackerMsvcFile
#include "MsvcRuntime.iss"
#include "InstallationPolicy.iss"

#include "PersonalData.iss"
#include "InstallOwnership.iss"
#ifndef JobTrackerPayloadFile
  #error A verified generated payload include is required.
#endif
#include JobTrackerPayloadFile

[Setup]
; The local directory determines identity; no uninstall registry is used.
AppId={code:GetInstallAppId}
AppName={#JobTrackerProductName}
AppVersion={#JobTrackerReleaseVersion}
AppPublisher={#JobTrackerPublisher}
VersionInfoVersion={#JobTrackerWindowsVersion}
VersionInfoProductName={#JobTrackerProductName}
VersionInfoProductVersion={#JobTrackerReleaseVersion}
SetupIconFile={#JobTrackerSetupIconFile}
UninstallDisplayIcon={#JobTrackerUninstallDisplayIcon}
UninstallDisplayName={#JobTrackerProductName} ({code:GetScopeCaption})
DefaultDirName={autopf}\{#JobTrackerProductName}
DisableDirPage=no
AppendDefaultDirName=no
UsePreviousAppDir=no
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
; Required with a code-based AppId; also keeps both modes explicitly selectable.
UsePreviousPrivileges=no
SetupArchitecture=x64
ArchitecturesAllowed=x64os
ArchitecturesInstallIn64BitMode=x64os
MinVersion=10.0
WizardStyle=modern
LanguageDetectionMethod=uilanguage
ShowLanguageDialog=yes
UsePreviousLanguage=no
UsePreviousTasks=no
DisableProgramGroupPage=yes
AllowNoIcons=no
OutputDir={#JobTrackerOutputDirectory}
OutputBaseFilename={#JobTrackerProductName}-Setup-{#JobTrackerReleaseVersion}
Compression=lzma2
SolidCompression=yes
Uninstallable=yes
CreateUninstallRegKey=no
UninstallLogMode=append
; No preliminary application-idle checks or Restart Manager shutdown/restart.
CloseApplications=no
RestartApplications=no
AllowCancelDuringInstall=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[CustomMessages]
english.CurrentUserScope=current user
russian.CurrentUserScope=текущий пользователь
english.AllUsersScope=all users
russian.AllUsersScope=все пользователи
english.DesktopShortcut=Create a &desktop shortcut
russian.DesktopShortcut=Создать &ярлык на рабочем столе

[Tasks]
Name: "desktopicon"; Description: "{cm:DesktopShortcut}"; Flags: unchecked

[Files]
Source: "MSVC-NOTICE.txt"; DestDir: "{app}\licenses"; Flags: ignoreversion uninsneveruninstall; AfterInstall: RecordInstalledFile('licenses\MSVC-NOTICE.txt')

[Code]
function GetInstallScope(Param: String): String;
begin
  Result := GetOwnershipScope;
end;

function GetInstallAppId(Param: String): String;
begin
  // AppId can be evaluated before the directory page is available.
  // A common product id is safe: history is selected only in this directory.
  Result := 'JobTracker.LocalCopy';
end;

function GetScopeCaption(Param: String): String;
begin
  Result := CustomMessage(GetInstallScope('') + 'Scope');
end;

function InitializeSetup: Boolean;
begin
  Result := InstallerOptionsAllowed;
  if Result then RegisterCurrentPayload;
end;

procedure DeinitializeSetup;
begin
  CleanupMsvcRuntime;
  ReleaseProgramRoot;
  ReleaseInstallPolicyLock;
end;

function InitializeUninstall: Boolean;
begin
  Result := False;
  try
    AcquireInstallPolicyLock;
    ValidateLocalUninstaller;
    InitializePersonalDataPolicy;
    Result := True;
  except
    Log(GetExceptionMessage);
    if not UninstallSilent then
      SuppressibleMsgBox(GetExceptionMessage, mbError, MB_OK, IDOK);
  end;
end;

procedure DeinitializeUninstall;
begin
  ReleaseProgramRoot;
  ReleaseInstallPolicyLock;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssInstall then begin
    try
      PrepareInstallOwnership;
    except
      OwnershipWriteFailed := True;
      Log(CustomMessage('InstallWriteFailed') + #13#10 + GetExceptionMessage);
      if not WizardSilent then
        SuppressibleMsgBox(CustomMessage('InstallWriteFailed') + #13#10 + GetExceptionMessage, mbError, MB_OK, IDOK);
      Abort;
    end;
  end;
  if CurStep = ssPostInstall then begin
    try
      FinishInstallOwnership;
    except
      OwnershipWriteFailed := True;
      Log(CustomMessage('InstallWriteFailed') + #13#10 + GetExceptionMessage);
      if not WizardSilent then
        SuppressibleMsgBox(CustomMessage('InstallWriteFailed') + #13#10 + GetExceptionMessage, mbError, MB_OK, IDOK);
    end;
  end;
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  if (CurPageID = wpFinished) and OwnershipWriteFailed then begin
    WizardForm.FinishedHeadingLabel.Caption := CustomMessage('InstallIncompleteHeading');
    WizardForm.FinishedLabel.Caption := CustomMessage('InstallWriteFailed');
  end;
end;

function GetCustomSetupExitCode: Integer;
begin
  if OwnershipWriteFailed then Result := 1 else Result := 0;
end;

procedure CurUninstallStepChanged(CurStep: TUninstallStep);
begin
  if CurStep = usUninstall then begin
    PersonalDataAfterUninstall(CurStep);
    RemoveLocalProgram;
    if ProgramFailures <> '' then begin
      Log(FmtMessage(CustomMessage('ProgramIncomplete'), [ProgramFailures]));
      if not UninstallSilent then
        SuppressibleMsgBox(FmtMessage(CustomMessage('ProgramIncomplete'), [ProgramFailures]),
          mbError, MB_OK, IDOK);
      // This event is fatal on Abort. Keep the local uninstaller and ledger for
      // repair/retry, and do not let native success mask an actual failure.
      Abort;
    end;
    Log(CustomMessage('ProgramComplete'));
  end;
  if CurStep = usDone then begin
    ReleaseProgramRoot;
    RemoveDir(OwnershipRoot);
  end;
end;

function UpdateReadyMemo(Space, NewLine, MemoUserInfoInfo, MemoDirInfoInfo,
  MemoTypeInfo, MemoComponentsInfo, MemoGroupInfo, MemoTasksInfo: String): String;
begin
  Result := MemoDirInfoInfo + NewLine + NewLine + MemoTasksInfo + NewLine + NewLine;
  if IsAdminInstallMode then Result := Result + CustomMessage('MsvcAllUsersInfo')
  else Result := Result + CustomMessage('MsvcCurrentUserInfo');
  Result := Result + NewLine + NewLine + CustomMessage('InstallSharedDataInfo');
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var ExecutionData, TargetData, Destination: String;
begin
  Result := '';
  try
    AcquireInstallPolicyLock;
    Destination := CanonicalInstallPath(ExpandConstant('{app}'));
    CheckInstallationPolicy(IsAdminInstallMode, Destination, '{#JobTrackerReleaseVersion}');
    ResolvePersonalDataLocations(ExecutionData, TargetData);
    if PathsOverlap(Destination, CanonicalInstallPath(AddBackslash(ExecutionData) + 'JobTracker\Data')) or
       PathsOverlap(Destination, CanonicalInstallPath(AddBackslash(TargetData) + 'JobTracker\Data')) then
      RaiseException(CustomMessage('InstallPathConflict'));
    ValidatePayloadDestination;
    Result := PrepareMsvcRuntime(IsAdminInstallMode, NeedsRestart);
  except
    Result := GetExceptionMessage;
  end;
  if Result <> '' then Log(Result);
end;
