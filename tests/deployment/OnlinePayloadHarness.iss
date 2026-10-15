; TEST ONLY. No JobTracker registration, shortcuts, application launch or uninstall.
; Production never includes this file or accepts its runtime endpoint/hash inputs.
#include PipelineFile
#define JobTrackerExecutableName "JobTrackerApp.exe"
#include "..\..\packaging\windows\InstallationPolicy.iss"
#include "..\..\packaging\windows\ManualClose.iss"
#include PayloadFile

[Setup]
AppName=JobTracker Online Payload TEST ONLY
AppVersion=0.0.0
DefaultDirName={tmp}\unused-test-destination
OutputDir={#TestOutput}
OutputBaseFilename=online-payload-test-only
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
ArchiveExtraction=enhanced/nopassword

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Code]
function CancelDownload(const Url, FileName: String; const Progress, ProgressMax: Int64): Boolean;
begin
  Result := CheckPayloadDownloadSize(Url, FileName, Progress, ProgressMax);
  if Progress > 0 then
    PayloadDownloadPage.AbortButton.OnClick(PayloadDownloadPage.AbortButton);
end;

function CancelExtraction(const ArchiveName, FileName: String; const Progress, ProgressMax: Int64): Boolean;
begin
  Result := True;
  PayloadExtractionPage.AbortButton.OnClick(PayloadExtractionPage.AbortButton);
end;

procedure InitializeWizard;
begin
  InitializeOnlinePayload;
  if ExpandConstant('{param:Case}') = 'cancel-download' then
    PayloadDownloadPage := CreateDownloadPage('TEST download cancellation', '', @CancelDownload);
  if ExpandConstant('{param:Case}') = 'cancel-extract' then
    PayloadExtractionPage := CreateExtractionPage('TEST extraction cancellation', '', @CancelExtraction);
end;

procedure DeinitializeSetup;
begin
  CleanupOnlinePayload;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  Url, CaseName, ExpectedHash: String;
  ExpectedSize: Int64;
begin
  Result := '';
  CaseName := ExpandConstant('{param:Case}');
  Url := ExpandConstant('{param:Url}');
  ExpectedHash := ExpandConstant('{param:Hash|{#JobTrackerArchiveSha256}}');
  ExpectedSize := StrToInt64Def(ExpandConstant('{param:Size}'), {#JobTrackerArchiveSize});
  try
    RequireSafePayloadPath(ExpandConstant('{app}'));
    if CaseName = 'retry' then begin
      Result := PrepareOnlinePayload(Url + '/missing', 'payload.7z', ExpectedHash, ExpectedSize);
      if (Result = '') or FileExists(PayloadArchivePath) or DirExists(ExpandConstant('{tmp}\payload')) then
        RaiseException('Failed attempt did not clean up.');
      Log('TEST: failed attempt cleaned; retrying.');
      Url := Url + '/valid';
    end;
    Result := PrepareOnlinePayload(Url, 'payload.7z', ExpectedHash, ExpectedSize);
    if CaseName = 'traversal' then begin
      if FileExists(ExpandConstant('{tmp}\escape.txt')) then
        Log('TEST FAILURE: archive escaped the extraction directory.');
      if Result = '' then
        Log('TEST FAILURE: native extractor accepted traversal.');
      Log('TEST: checked traversal rejection and escape path before temporary cleanup.');
    end;
    if Result = '' then begin
      if CaseName = 'tampered-extracted' then
        SaveStringToFile(ExpandConstant('{tmp}\payload\bin\qt.conf'), 'tampered', False);
      ValidateOnlinePayload;
      Log('TEST: all files validated before installation.');
    end;
  except
    Result := CustomMessage('PayloadInvalid') + #13#10 + GetExceptionMessage;
  end;
  if Result <> '' then begin
    Log(Result);
    CleanupOnlinePayload;
    if FileExists(PayloadArchivePath) or DirExists(ExpandConstant('{tmp}\payload')) then
      Log('TEST FAILURE: temporary payload remains.');
  end;
end;
