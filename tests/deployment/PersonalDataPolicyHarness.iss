; TEST ONLY: execute the production callback against synthetic process state,
; dialog answers, registration and known folders. No host profile is resolved.
; Test-PersonalData.ps1 creates the private include by substituting only these
; boundary calls. The production source and native deletion core are unchanged.
#if Ver < EncodeVer(7, 0, 0)
  #error Inno Setup 7 or newer is required for native x64 personal-data checks.
#endif
#define JobTrackerExecutableName "JobTrackerApp.exe"
#include "../../Installer/PayloadSafety.iss"
#include "../../Installer/InstallationPolicy.iss"

[Setup]
AppName=JobTracker personal data policy TEST ONLY
AppVersion=0.1.0
DefaultDirName={tmp}\unused
OutputDir={#TestOutput}
OutputBaseFilename=personal-data-policy-test-only
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
var
  TestCase, TestBase, TestRoot, TestApp, TestFailure: String;
  TestPrompts, TestDataResolutions, TestAccountReads, TestRegistryReads: Integer;

procedure RequirePolicyTest(const Condition: Boolean; const Detail: String);
begin
  if not Condition then begin
    TestFailure := 'TEST FAILURE: ' + Detail;
    RaiseException(TestFailure);
  end;
  Log('TEST PASS: ' + Detail);
end;

function TestIsAdmin: Boolean;
begin
  Result := (TestCase = 'elevated') or (TestCase = 'core-elevated');
end;

function TestIsAdminInstallMode: Boolean;
begin
  Result := (TestCase = 'allusers') or (TestCase = 'core-allusers');
end;

function TestUninstallSilent: Boolean;
begin
  Result := TestCase = 'silent';
end;

function TestExpandConstant(const Value: String): String;
begin
  if Value = '{localappdata}' then begin
    TestDataResolutions := TestDataResolutions + 1;
    Result := TestBase;
  end else if Value = '{app}' then Result := TestApp
  else if Value = '{autoprograms}\JobTracker' then Result := TestBase + '\programs\JobTracker'
  else if Value = '{autodesktop}' then Result := TestBase + '\desktop'
  else RequirePolicyTest(False, 'unexpected known-folder request: ' + Value);
end;

function TestReadInstallRecord(const Root: Integer; const Key, Scope: String;
  var Path: String; var Version: Int64): Boolean;
begin
  TestAccountReads := TestAccountReads + 1;
  RequirePolicyTest((Root = HKCU64) and (Key = InstallUninstallKey + 'CurrentUser_is1') and
    (Scope = 'CurrentUser'), 'account check is current-user only');
  if TestCase = 'account-invalid' then RaiseException('Synthetic account access denied.');
  Result := TestCase <> 'account-missing';
  Path := TestApp;
  if TestCase = 'account-mismatch' then Path := TestBase + '\another-installation';
  Version := 0;
end;

function TestInstallKeyExists(const Root: Integer; const Key: String): Boolean;
begin
  TestRegistryReads := TestRegistryReads + 1;
  RequirePolicyTest(((Root = HKCU64) or (Root = HKCU32)) and
    (Key = InstallUninstallKey + 'CurrentUser_is1') or
    ((Root = HKLM64) or (Root = HKLM32)) and
    (Key = InstallUninstallKey + 'AllUsers_is1'), 'no other profile registry request');
  if TestCase = 'registry-denied' then RaiseException('Synthetic registry access denied.');
  Result := ((TestCase = 'shared-cu64') and (Root = HKCU64)) or
    ((TestCase = 'shared-cu32') and (Root = HKCU32)) or
    ((TestCase = 'shared-lm64') and (Root = HKLM64)) or
    ((TestCase = 'shared-lm32') and (Root = HKLM32)) or
    ((TestCase = 'late-shared') and (TestPrompts > 0) and (Root = HKLM64));
end;

function TestSuppressibleMsgBox(const Text: String; Kind: TMsgBoxType;
  Buttons, DefaultResult: Integer): Integer;
begin
  if Kind = mbConfirmation then begin
    TestPrompts := TestPrompts + 1;
    RequirePolicyTest((Buttons = (MB_YESNO or MB_DEFBUTTON2)) and (DefaultResult = IDNO),
      'consent defaults to No, including suppressed dialogs');
    RequirePolicyTest(Pos(TestRoot, Text) > 0, 'consent names exact synthetic root');
    Result := IDNO;
    if TestCase = 'close' then Result := IDCANCEL;
    if (TestCase = 'yes') or (TestCase = 'late-shared') or (TestCase = 'late-program') then
      Result := IDYES;
    if TestCase = 'late-program' then
      RequirePolicyTest(ForceDirectories(TestApp), 'simulate incomplete removal after consent');
  end else begin
    RequirePolicyTest((Kind = mbInformation) and (Buttons = MB_OK) and (DefaultResult = IDOK),
      'preservation notification cannot authorize deletion');
    Log('Synthetic policy notification: ' + Text);
    Result := IDOK;
  end;
end;

#include TestPersonalDataFile

[Code]
function PrepareToInstall(var NeedsRestart: Boolean): String;
var PromptExpected: Boolean; Failure: String;
begin
  Result := '';
  try
    TestCase := ExpandConstant('{param:Case}');
    RequirePolicyTest(Pos('|' + TestCase + '|',
      '|no|close|yes|silent|allusers|elevated|core-allusers|core-elevated|' +
      'account-missing|account-invalid|account-mismatch|restart-required|incomplete|' +
      'shared-cu64|shared-cu32|shared-lm64|shared-lm32|registry-denied|' +
      'late-shared|late-program|pre-completion|') > 0, 'known bounded scenario');
    RequirePolicyTest((TestCase <> '') and (Pos('|', TestCase) = 0), 'single scenario');
    RequirePolicyTest((ActiveLanguage = 'english') or (ActiveLanguage = 'russian'), 'known language');
    TestBase := '{#TestOutput}\callback-' + ActiveLanguage + '-' + TestCase;
    TestRoot := TestBase + '\JobTracker';
    TestApp := TestBase + '\program';
    RequirePolicyTest(DirExists(TestRoot), 'synthetic data already prepared');
    if (TestCase = 'core-allusers') or (TestCase = 'core-elevated') then begin
      Failure := '';
      try DeleteConfirmedPersonalData(TestBase, TestRoot);
      except Failure := GetExceptionMessage; end;
      RequirePolicyTest(Pos('Elevated/all-users cleanup is disabled.', Failure) > 0,
        'native core refuses unsafe scope before opening data');
    end else begin
      InitializePersonalDataPolicy;
      if TestCase <> 'restart-required' then NeedsRestart := PersonalDataRestartCheck;
      if TestCase = 'incomplete' then
        RequirePolicyTest(ForceDirectories(TestApp), 'simulate incomplete program removal');
      if TestCase = 'pre-completion' then PersonalDataAfterUninstall(usUninstall)
      else PersonalDataAfterUninstall(usDone);
    end;
    RequirePolicyTest(TestFailure = '', 'callback completed without hidden assertions: ' + TestFailure);
    PromptExpected := (TestCase = 'no') or (TestCase = 'close') or (TestCase = 'yes') or
      (TestCase = 'late-shared') or (TestCase = 'late-program');
    RequirePolicyTest(TestPrompts = Ord(PromptExpected), 'expected consent prompt count');
    RequirePolicyTest(TestDataResolutions = Ord(PromptExpected), 'data path is resolved only for eligible cleanup');
    if TestIsAdmin or TestIsAdminInstallMode then
      RequirePolicyTest((TestAccountReads = 0) and (TestRegistryReads = 0),
        'AllUsers/elevated cleanup never inspects account registrations or profiles');
    RequirePolicyTest(DirExists(TestRoot) <> (TestCase = 'yes'), 'only explicit Yes removes synthetic data');
    RequirePolicyTest(SaveStringToFile(TestBase + '\callback.txt',
      Format('PASS prompts=%d dataResolutions=%d accountReads=%d registryReads=%d', [
        TestPrompts, TestDataResolutions, TestAccountReads, TestRegistryReads]), False), 'callback result');
  except
    Result := GetExceptionMessage;
  end;
end;
