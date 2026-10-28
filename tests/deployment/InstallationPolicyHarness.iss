; TEST ONLY: consumes production policy without application registration/data access.
#if Ver < EncodeVer(7, 0, 0)
  #error Inno Setup 7 or newer is required to verify the native x64 policy ABI.
#endif
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

procedure CheckVersion(const Text: String; const Expected: Boolean);
var Version: Int64;
begin
  RequireTest(ParseInstallVersion(Text, Version) = Expected, 'parse ' + Text);
end;

procedure DeinitializeSetup;
begin
  ReleaseInstallPolicyLock;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  CaseName, Expected, Path, ErrorMessage: String;
  First, Second: Int64;
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
      RequireTest(CompareText(CanonicalInstallPath(Path), Path) = 0, 'canonical path remains stable');
    end else if CaseName = 'shortpath' then begin
      Path := ExpandConstant('{param:Path}');
      RequireTest(CompareText(CanonicalInstallPath(GetShortName(Path)),
        CanonicalInstallPath(Path)) = 0, 'short-name path alias');
    end else if CaseName = 'policy' then begin
      Path := CanonicalInstallPath(ExpandConstant('{param:Path}'));
      CheckInstallationPolicy(ExpandConstant('{param:AllUsers}') = 'yes',
        Path, ExpandConstant('{param:Incoming}'));
    end else if CaseName = 'identity' then begin
      RequireTest(HasLocalInstallIdentity(ExpandConstant('{param:Path}')) =
        (ExpandConstant('{param:Identified}') = 'yes'), 'local identity evidence');
    end else if CaseName = 'overlap' then begin
      RequireTest(PathsOverlap(CanonicalInstallPath(ExpandConstant('{param:Path}')),
        CanonicalInstallPath(ExpandConstant('{param:Other}'))) =
        (ExpandConstant('{param:Overlap}') = 'yes'), 'directory overlap contract');
    end else if CaseName = 'empty' then begin
      RequireEmptyInstallDirectory(ExpandConstant('{param:Path}'));
    end else if CaseName = 'lock' then begin
      AcquireInstallPolicyLock;
      AcquireInstallPolicyLock;
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
