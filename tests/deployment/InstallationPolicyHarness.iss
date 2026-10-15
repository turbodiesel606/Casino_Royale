; TEST ONLY: no application registration, shortcuts, data access or prerequisites.
#define JobTrackerExecutableName "JobTrackerApp.exe"
#include PipelineFile
#include PolicyFile

[Setup]
AppName=JobTracker installation policy TEST ONLY
AppVersion={#TestVersion}
VersionInfoVersion={#TestVersion}.0
DefaultDirName={tmp}\unused
OutputDir={#TestOutput}
OutputBaseFilename=policy-{#TestVersion}-test-only
PrivilegesRequired=lowest
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

procedure CheckVersion(const Text: String; const Expected: Boolean);
var
  Version: Int64;
begin
  RequireTest(ParseInstallVersion(Text, Version) = Expected, 'parse ' + Text);
end;

procedure DeinitializeSetup;
begin
  ReleaseInstallPolicyLock;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  CaseName, Expected, Path, Key, Scope, ErrorMessage: String;
  First, Second: Int64;
  Found: Boolean;
begin
  Result := '';
  CaseName := ExpandConstant('{param:Case}');
  Expected := ExpandConstant('{param:Expected}');
  try
    if CaseName = 'parse' then begin
      CheckVersion('0.0.0', True);
      CheckVersion('65535.65535.65535', True);
      CheckVersion('1.2', False);
      CheckVersion('1.2.3.0', False);
      CheckVersion('1.2.3.4.5', False);
      CheckVersion('01.2.3', False);
      CheckVersion('1.2.03', False);
      CheckVersion('1.2.65536', False);
      CheckVersion('9999999999999.0.0', False);
      CheckVersion('1.2.3-beta', False);
      CheckVersion('-1.2.3', False);
      CheckVersion(' 1.2.3', False);
      CheckVersion('1..3', False);
      RequireTest(ParseInstallVersion('1.10.0', First) and ParseInstallVersion('1.9.9', Second), 'parse numerical comparison');
      RequireTest(CompareInstallVersion(First, Second) > 0, '10 sorts above 9 numerically');
      RequireTest(ParseInstallVersion('65535.0.0', First) and ParseInstallVersion('32767.0.0', Second), 'parse unsigned comparison');
      RequireTest(CompareInstallVersion(First, Second) > 0, 'unsigned high major version');
      RequireTest(CompareInstallVersion(Second, First) < 0, 'unsigned reverse order');
      RequireTest(CompareInstallVersion(First, First) = 0, 'unsigned equality');
    end else if CaseName = 'path' then begin
      Path := CanonicalInstallPath(ExpandConstant('{param:Path}'));
      RequireTest(CompareText(Path, ExpandConstant('{param:Canonical}')) = 0, 'canonical path');
    end else if CaseName = 'shortpath' then begin
      Path := ExpandConstant('{param:Path}');
      RequireTest(CompareText(CanonicalInstallPath(GetShortName(Path)),
        CanonicalInstallPath(Path)) = 0, 'short-name path alias');
    end else if CaseName = 'overlap' then begin
      Path := CanonicalInstallPath(ExpandConstant('{param:Path}'));
      RequireTest(InstallPathsOverlap(Path, CanonicalInstallPath(ExpandConstant('{param:Other}'))) =
        (ExpandConstant('{param:Overlap}') = 'yes'), 'path overlap');
    end else if CaseName = 'record' then begin
      Key := ExpandConstant('{param:Key}');
      // A test fixture cannot name real application registration.
      RequireTest(Pos('Software\JobTrackerInstallerTests\', Key) = 1, 'isolated registry fixture');
      Scope := ExpandConstant('{param:Scope}');
      RequireTest(ParseInstallVersion(ExpandConstant('{param:Incoming}'), First), 'incoming version');
      Found := False;
      CheckInstallRecord(HKCU64, Key, Scope, CanonicalInstallPath(ExpandConstant('{param:Path}')),
        First, ExpandConstant('{param:SameScope}') = 'yes', Found);
      RequireTest(Found, 'installed record was found');
    end else if CaseName = 'unloaded-profile' then begin
      CheckProfileInstall('S-1-5-21-0-0-0-99999', ExpandConstant('{app}'), 0);
    end else if CaseName = 'loaded-profile' then begin
      RequireTest(ParseInstallVersion('0.1.0', First), 'profile version');
      CheckProfileInstall(ExpandConstant('{param:Sid}'), ExpandConstant('{app}'), First);
    end else if CaseName = 'allusers-audit' then begin
      CheckInstallationPolicy(True, ExpandConstant('{app}'), '0.1.0');
    end else if CaseName = 'empty' then begin
      RequireEmptyInstallDirectory(ExpandConstant('{param:Path}'));
    end else if CaseName = 'lock' then begin
      AcquireInstallPolicyLock;
      AcquireInstallPolicyLock;
    end else if CaseName = 'current-policy' then begin
      CheckInstallationPolicy(False, ExpandConstant('{app}'), '{#TestVersion}');
    end else RaiseException('TEST FAILURE: unknown case');
    if Expected <> '' then RaiseException('TEST FAILURE: expected rejection ' + Expected);
    Log('TEST PASS: ' + CaseName);
  except
    ErrorMessage := GetExceptionMessage;
    if (Expected <> '') and (Pos(CustomMessage(Expected), ErrorMessage) = 1) then
      Log('TEST PASS: rejected ' + Expected)
    else begin
      Result := ErrorMessage;
      Log(Result);
    end;
  end;
end;
