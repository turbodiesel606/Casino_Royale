; Version-locked Online Installer; optional local mode preserves the stage-07 fixture.
; Build through Build-Installer.ps1, which supplies the generated Release identity.
#if Ver < EncodeVer(6, 7, 1)
  #error Inno Setup 6.7.1 or newer is required.
#endif
#ifndef JobTrackerIdentityFile
  #error Build through packaging/windows/Build-Installer.ps1.
#endif
#ifndef JobTrackerOutputDirectory
  #error An explicit installer output directory is required.
#endif
#include JobTrackerIdentityFile
#ifndef JobTrackerMsvcFile
  #error A verified MSVC runtime input is required.
#endif
; Shared native integrity/path checks also protect the runtime prerequisite.
#include "OnlinePayload.iss"
#include JobTrackerMsvcFile
#include "MsvcRuntime.iss"
#include "InstallationPolicy.iss"
#include "ManualClose.iss"
#include "PersonalData.iss"
#ifndef JobTrackerPayloadFile
  #error A verified generated payload include is required.
#endif
#include JobTrackerPayloadFile
#ifdef JobTrackerLocalPayload
  #ifndef JobTrackerPayloadDirectory
    #error A verified local payload directory is required.
  #endif
  #define JobTrackerInstallerKind "local"
#else
  #define JobTrackerInstallerKind "online"
  #define JobTrackerArchiveName "JobTracker-" + JobTrackerReleaseVersion + "-windows-x64.7z"
  // Deliberately fixed: no runtime or build-wrapper endpoint/tag override.
  #define JobTrackerReleaseUrl "https://github.com/turbodiesel606/Releases/releases/download/v" + JobTrackerReleaseVersion + "/" + JobTrackerArchiveName
#endif

[Setup]
; These IDs are independent of version, language and installation directory.
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
UsePreviousAppDir=yes
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
; Required with a code-based AppId; also keeps both modes explicitly selectable.
UsePreviousPrivileges=no
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
OutputBaseFilename={#JobTrackerProductName}-{#JobTrackerReleaseVersion}-windows-x64-{#JobTrackerInstallerKind}-setup
ArchiveExtraction=enhanced/nopassword
Compression=lzma2
SolidCompression=yes
Uninstallable=yes
; ManualClose.iss checks files; no Restart Manager shutdown/restart is permitted.
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
Source: "MSVC-NOTICE.txt"; DestDir: "{app}\licenses"; Flags: ignoreversion; Check: CanInstallWithManualClose(ExpandConstant('{app}\licenses\MSVC-NOTICE.txt'))

[Icons]
; Different leaf names prevent merged user/common shell folders hiding one copy.
Name: "{autoprograms}\{#JobTrackerProductName}\{code:GetShortcutName}"; Filename: "{app}\bin\{#JobTrackerExecutableName}"; WorkingDir: "{app}\bin"; IconFilename: "{#JobTrackerShortcutIconFilename}"; IconIndex: {#JobTrackerShortcutIconIndex}
Name: "{autodesktop}\{code:GetShortcutName}"; Filename: "{app}\bin\{#JobTrackerExecutableName}"; WorkingDir: "{app}\bin"; IconFilename: "{#JobTrackerShortcutIconFilename}"; IconIndex: {#JobTrackerShortcutIconIndex}; Tasks: desktopicon

[INI]
; Installer-owned metadata only; this is not the application's data/settings store.
Filename: "{app}\JobTracker.install.ini"; Section: "Installation"; Key: "Scope"; String: "{code:GetInstallScope}"
Filename: "{app}\JobTracker.install.ini"; Section: "Installation"; Key: "Version"; String: "{#JobTrackerReleaseVersion}"

[UninstallDelete]
Type: files; Name: "{app}\JobTracker.install.ini"

[Code]
function InitializeSetup: Boolean;
begin
  Result := ManualCloseOptionsAllowed;
end;

procedure InitializeWizard;
begin
  InitializeOnlinePayload;
  InitializeMsvcRuntime;
end;

procedure DeinitializeSetup;
begin
  CleanupOnlinePayload;
  CleanupMsvcRuntime;
  ReleaseInstallPolicyLock;
end;

function InitializeUninstall: Boolean;
begin
  Result := False;
  try
    AcquireInstallPolicyLock;
    RequireClosedJobTracker;
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
  ReleaseInstallPolicyLock;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
    CheckInstallShortcuts(IsAdminInstallMode, True, ExpandConstant('{app}'));
end;

function UpdateReadyMemo(Space, NewLine, MemoUserInfoInfo, MemoDirInfoInfo,
  MemoTypeInfo, MemoComponentsInfo, MemoGroupInfo, MemoTasksInfo: String): String;
begin
  Result := MemoDirInfoInfo + NewLine + NewLine + MemoTasksInfo + NewLine + NewLine;
  if IsAdminInstallMode then
    Result := Result + CustomMessage('MsvcAllUsersInfo')
  else
    Result := Result + CustomMessage('MsvcCurrentUserInfo');
  Result := Result + NewLine + NewLine + CustomMessage('InstallSharedDataInfo');
end;

function GetInstallScope(Param: String): String;
begin
  if IsAdminInstallMode then
    Result := 'AllUsers'
  else
    Result := 'CurrentUser';
end;

function GetInstallAppId(Param: String): String;
begin
  Result := 'JobTracker.' + GetInstallScope('');
end;

function GetScopeCaption(Param: String): String;
begin
  Result := CustomMessage(GetInstallScope('') + 'Scope');
end;

function GetShortcutName(Param: String): String;
begin
  Result := '{#JobTrackerProductName} (' + GetScopeCaption('') + ')';
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  Result := '';
  try
    AcquireInstallPolicyLock;
    CheckInstallationPolicy(IsAdminInstallMode, ExpandConstant('{app}'), '{#JobTrackerReleaseVersion}');
    CheckInstallShortcuts(IsAdminInstallMode, False, ExpandConstant('{app}'));
    RequireClosedJobTracker;
    ValidatePayloadDestination;
#ifndef JobTrackerLocalPayload
    Result := PrepareOnlinePayload('{#JobTrackerReleaseUrl}',
      '{#JobTrackerArchiveName}', '{#JobTrackerArchiveSha256}', {#JobTrackerArchiveSize});
    if Result = '' then
      ValidateOnlinePayload;
#endif
    if Result = '' then
      Result := PrepareMsvcRuntime(IsAdminInstallMode, NeedsRestart);
    // Downloads/prerequisites may take time. Recheck immediately before commit.
    if Result = '' then begin
      CheckInstallationPolicy(IsAdminInstallMode, ExpandConstant('{app}'), '{#JobTrackerReleaseVersion}');
      RequireClosedJobTracker;
      ValidatePayloadDestination;
    end;
  except
    Result := GetExceptionMessage;
  end;
  if Result <> '' then begin
    Log(Result);
    CleanupOnlinePayload;
  end;
end;
