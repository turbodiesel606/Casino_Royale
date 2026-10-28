; TEST ONLY. Only dialog/silent/identity boundaries are replaced in a private
; include. The production cleanup core deletes disposable native fixtures.
#if Ver < EncodeVer(7, 0, 0)
  #error Inno Setup 7 or newer is required.
#endif
#define JobTrackerExecutableName "JobTrackerApp.exe"
#include "../../Installer/Windows/PayloadSafety.iss"
#include "../../Installer/Windows/InstallationPolicy.iss"

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
  TestCase, TestBase, TestRoot, TestFailure, TestLastAction: String;
  TestPrompts, TestIdentityReads, TestCleanups, TestNotifications: Integer;

procedure RequirePolicyTest(const Condition: Boolean; const Detail: String);
begin
  if not Condition then begin
    TestFailure := 'TEST FAILURE: ' + Detail;
    RaiseException(TestFailure);
  end;
  Log('TEST PASS: ' + Detail);
end;

function TestUninstallSilent: Boolean;
begin
  Result := TestCase = 'silent';
end;

function TestSuppressibleMsgBox(const Text: String; Kind: TMsgBoxType;
  Buttons, DefaultResult: Integer): Integer;
var IsIdentityPrompt: Boolean;
begin
  if Kind = mbConfirmation then begin
    TestPrompts := TestPrompts + 1;
    RequirePolicyTest((Buttons = (MB_YESNO or MB_DEFBUTTON2)) and (DefaultResult = IDNO),
      'consent defaults to No, including suppressed dialogs');
    RequirePolicyTest(Pos(TestRoot, Text) > 0, 'consent names exact synthetic Data path');
    RequirePolicyTest(Pos(TestBase + '\profile', Text) > 0, 'consent names exact synthetic profile');
    RequirePolicyTest(Pos('fixture-user', Text) > 0, 'consent names exact target account');
    IsIdentityPrompt := (Pos('fallback-', TestCase) = 1) and (TestPrompts = 1);
    if not IsIdentityPrompt then
      RequirePolicyTest(Pos('fixture-execution-account', Text) > 0, 'consent distinguishes execution account');
    Result := IDYES;
    if (TestCase = 'no') or (TestCase = 'fallback-no') or
       ((TestCase = 'fallback-keep') and not IsIdentityPrompt) then Result := IDNO;
    if (TestCase = 'close') or (TestCase = 'fallback-close') then Result := IDCANCEL;
    if not IsIdentityPrompt and (Result = IDYES) then TestLastAction := 'consent';
  end else begin
    RequirePolicyTest((Kind = mbInformation) and (Buttons = MB_OK) and (DefaultResult = IDOK),
      'information cannot authorize deletion');
    TestNotifications := TestNotifications + 1;
    if TestCase = 'locked-data' then
      RequirePolicyTest(Pos('Windows error 32', Text) > 0, 'actual native sharing failure reported');
    Log('Policy result notification: ' + Text);
    Result := IDOK;
  end;
end;

#include TestPersonalDataFile

[Code]
function PrepareToInstall(var NeedsRestart: Boolean): String;
var ShouldDelete, ShouldPrompt: Boolean; ExpectedPrompts: Integer;
begin
  Result := '';
  try
    TestCase := ExpandConstant('{param:Case}');
    RequirePolicyTest(Pos('|' + TestCase + '|',
      '|no|close|yes|silent|allusers|elevated|running-copy|remaining-copy|' +
      'incomplete-program|disconnected-copy|alternate-account|identity-error|' +
      'fallback-no|fallback-close|fallback-yes|fallback-keep|pre-uninstall|' +
      'repeat-callback|locked-data|absent|') > 0, 'known bounded scenario');
    RequirePolicyTest((TestCase <> '') and (Pos('|', TestCase) = 0), 'single scenario');
    TestBase := '{#TestOutput}\callback-' + ActiveLanguage + '-' + TestCase;
    TestRoot := TestBase + '\JobTracker\Data';
    RequirePolicyTest(DirExists(TestBase), 'synthetic fixture already prepared');
    if (TestCase = 'incomplete-program') or (TestCase = 'running-copy') or
       (TestCase = 'remaining-copy') then
      RequirePolicyTest(ForceDirectories(TestBase + '\remaining-program'), 'program remains independently');
    InitializePersonalDataPolicy;
    if TestCase = 'pre-uninstall' then PersonalDataAfterUninstall(usPostUninstall)
    else PersonalDataAfterUninstall(usUninstall);
    if TestCase = 'repeat-callback' then PersonalDataAfterUninstall(usUninstall);
    RequirePolicyTest(TestFailure = '', 'callback had no hidden assertion failure: ' + TestFailure);
    ShouldPrompt := (TestCase <> 'silent') and (TestCase <> 'identity-error') and (TestCase <> 'pre-uninstall');
    ExpectedPrompts := Ord(ShouldPrompt);
    if (TestCase = 'fallback-yes') or (TestCase = 'fallback-keep') then ExpectedPrompts := 2;
    RequirePolicyTest(TestPrompts = ExpectedPrompts, 'expected prompt count');
    RequirePolicyTest(TestIdentityReads = Ord((TestCase <> 'silent') and (TestCase <> 'pre-uninstall')),
      'identity is resolved only for interactive uninstall');
    ShouldDelete := ShouldPrompt and (TestCase <> 'no') and (TestCase <> 'close') and
      (TestCase <> 'fallback-no') and (TestCase <> 'fallback-close') and (TestCase <> 'fallback-keep');
    RequirePolicyTest(TestCleanups = Ord(ShouldDelete), 'exactly one cleanup immediately follows deletion consent');
    if TestCase = 'locked-data' then
      RequirePolicyTest(FileExists(TestRoot + '\database.sqlite') and not FileExists(TestRoot + '\database.sqlite-wal'),
        'locked file preserved and unlocked sibling removed')
    else if TestCase <> 'absent' then
      RequirePolicyTest(DirExists(TestRoot) <> ShouldDelete, 'only confirmed target data removed');
    RequirePolicyTest(FileExists(TestBase + '\JobTracker\settings.json'), 'JobTracker sibling preserved');
    RequirePolicyTest(FileExists(TestBase + '\outside.txt'), 'outside file preserved');
    RequirePolicyTest(SaveStringToFile(TestBase + '\callback.txt',
      Format('PASS prompts=%d identityReads=%d cleanups=%d notifications=%d', [TestPrompts, TestIdentityReads, TestCleanups, TestNotifications]), False), 'callback result');
  except
    Result := GetExceptionMessage;
  end;
end;
