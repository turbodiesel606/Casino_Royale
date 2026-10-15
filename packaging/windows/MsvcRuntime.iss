// Hybrid MSVC prerequisite policy. Uses the existing payload integrity helpers.
// Generated runtime input supplies only reviewed Microsoft bytes and versions.
[CustomMessages]
english.MsvcDownload=Downloading Microsoft Visual C++ Runtime
russian.MsvcDownload=Загрузка Microsoft Visual C++ Runtime
english.MsvcDownloadDescription=Downloading the official x64 package and verifying its pinned SHA-256 checksum.
russian.MsvcDownloadDescription=Загрузка официального пакета x64 и проверка его контрольной суммы SHA-256.
english.MsvcFailed=Microsoft Visual C++ Runtime could not be installed or verified. JobTracker installation has not started. You can retry or exit Setup.
russian.MsvcFailed=Не удалось установить или проверить Microsoft Visual C++ Runtime. Установка JobTracker ещё не началась. Можно повторить попытку или выйти.
english.MsvcCancelled=Microsoft Visual C++ Runtime setup was cancelled. JobTracker installation has not started.
russian.MsvcCancelled=Установка Microsoft Visual C++ Runtime отменена. Установка JobTracker ещё не началась.
english.MsvcRestart=Microsoft Visual C++ Runtime requires a Windows restart. Restart Windows and run this JobTracker installer again. JobTracker files have not been changed.
russian.MsvcRestart=Для Microsoft Visual C++ Runtime требуется перезагрузка Windows. Перезагрузите Windows и снова запустите установщик JobTracker. Файлы JobTracker не изменены.
english.MsvcLocalConflict=This all-users destination contains App-Local MSVC runtime files. Choose a clean folder or remove the previous installation using its uninstaller.
russian.MsvcLocalConflict=В папке для всех пользователей уже есть локальные файлы MSVC Runtime. Выберите чистую папку или удалите предыдущую установку её деинсталлятором.
english.MsvcInstalling=Installing Microsoft Visual C++ Runtime...
russian.MsvcInstalling=Установка Microsoft Visual C++ Runtime...
english.MsvcCurrentUserInfo=MSVC runtime: bundled App-Local files; no system runtime installation.
russian.MsvcCurrentUserInfo=MSVC Runtime: локальные файлы в папке программы; без системной установки.
english.MsvcAllUsersInfo=MSVC runtime: a compatible system runtime is required. If missing, Setup downloads and installs the official Microsoft x64 package. Microsoft license terms: https://visualstudio.microsoft.com/license-terms/
russian.MsvcAllUsersInfo=MSVC Runtime: требуется совместимая системная версия. При её отсутствии будет загружен и установлен официальный пакет Microsoft x64. Лицензия Microsoft: https://visualstudio.microsoft.com/license-terms/

[Code]
var
  MsvcDownloadPage: TDownloadWizardPage;
  MsvcRestartPending: Boolean;

function MsvcFileCompatible(const FileName: String): Boolean;
var
  Version: Int64;
begin
  Result := GetPackedVersion(FileName, Version);
  if Result then
    Result := ComparePackedVersion(Version, PackVersionComponents({#MsvcMinimumComponents})) >= 0;
end;

function MsvcRegistryCompatible(const Root: Integer): Boolean;
var
  Installed, Major, Minor, Build, Revision: Cardinal;
  Key: String;
begin
  Key := 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64';
  Result := RegQueryDWordValue(Root, Key, 'Installed', Installed) and
    RegQueryDWordValue(Root, Key, 'Major', Major) and
    RegQueryDWordValue(Root, Key, 'Minor', Minor) and
    RegQueryDWordValue(Root, Key, 'Bld', Build) and
    RegQueryDWordValue(Root, Key, 'Rbld', Revision);
  if Result then
    Result := (Installed = 1) and (Major = 14) and
      (Minor <= 65535) and (Build <= 65535) and (Revision <= 65535);
  if Result then
    Result := ComparePackedVersion(PackVersionComponents(Major, Minor, Build, Revision),
      PackVersionComponents({#MsvcMinimumComponents})) >= 0;
end;

function MsvcSystemCompatible: Boolean;
begin
  // Query only the x64 runtime identity. A 32-bit runtime does not satisfy it.
  Result := IsWin64 and (MsvcRegistryCompatible(HKLM64) or MsvcRegistryCompatible(HKLM32));
  if Result then
    Result := MsvcSystemFilesCompatible;
end;

procedure CheckMsvcDestination;
var
  Pattern: String;
  Files: TFindRec;
  Index: Integer;
begin
  RequireSafePayloadPath(ExpandConstant('{app}\bin'));
  // Do not silently allow an old local DLL to shadow the all-users prerequisite.
  // Safe migration/removal of previous installations belongs to the upgrade stage.
  for Index := 0 to 5 do begin
    case Index of
      0: Pattern := 'msvcp*.dll';
      1: Pattern := 'vcruntime*.dll';
      2: Pattern := 'concrt*.dll';
      3: Pattern := 'vccorlib*.dll';
      4: Pattern := 'vcomp*.dll';
      5: Pattern := 'ucrtbase*.dll';
    end;
    if FindFirst(ExpandConstant('{app}\bin\') + Pattern, Files) then begin
      FindClose(Files);
      RaiseException(CustomMessage('MsvcLocalConflict'));
    end;
  end;
end;

function MsvcDownloadProgress(const Url, FileName: String; const Progress, ProgressMax: Int64): Boolean;
begin
  Result := (Progress <= {#MsvcRedistSize}) and (ProgressMax <= {#MsvcRedistSize});
end;

procedure InitializeMsvcRuntime;
begin
  MsvcDownloadPage := CreateDownloadPage(CustomMessage('MsvcDownload'),
    CustomMessage('MsvcDownloadDescription'), @MsvcDownloadProgress);
  MsvcDownloadPage.ShowBaseNameInsteadOfUrl := True;
end;

procedure CleanupMsvcRuntime;
begin
  // Only the fixed file in this Setup invocation's private temporary directory.
  if FileExists(ExpandConstant('{tmp}\vc_redist.x64.exe')) then
    if not DeleteFile(ExpandConstant('{tmp}\vc_redist.x64.exe')) then
      Log('MSVC download cleanup deferred to Setup temporary-directory cleanup.');
end;

function EvaluateMsvcResult(const Launched: Boolean; const ExitCode: Integer;
  const CompatibleAfter: Boolean; var NeedsRestart: Boolean): String;
var
  Code: Integer;
begin
  Code := ExitCode;
  // Burn can return HRESULT_FROM_WIN32 rather than the underlying MSI code.
  if Code = -2147023294 then Code := 1602; // $80070642: user cancelled
  if Code = -2147023258 then Code := 1638; // $80070666: another version exists
  Result := '';
  if Launched and ((Code = 3010) or (Code = 1641)) then begin
    MsvcRestartPending := True;
    NeedsRestart := True;
    Result := CustomMessage('MsvcRestart');
  end else if Launched and (Code = 1602) then
    Result := CustomMessage('MsvcCancelled')
  else if not (Launched and ((Code = 0) or (Code = 1638)) and CompatibleAfter) then
    Result := CustomMessage('MsvcFailed') + #13#10 + 'Code: ' + IntToStr(ExitCode);
end;

function PrepareMsvcRuntime(const AllUsers: Boolean; var NeedsRestart: Boolean): String;
var
  PackagePath, Arguments, OldStatus: String;
  ExitCode: Integer;
  Launched: Boolean;
begin
  Result := '';
  if not AllUsers then begin
    Log('MSVC policy: current-user App-Local runtime; no prerequisite execution.');
    Exit;
  end;
  try
    CheckMsvcDestination;
    if MsvcRestartPending then begin
      NeedsRestart := True;
      Result := CustomMessage('MsvcRestart');
      Exit;
    end;
    if MsvcSystemCompatible then begin
      Log('MSVC policy: compatible machine-wide x64 runtime; prerequisite skipped.');
      Exit;
    end;
    CleanupMsvcRuntime;
    PackagePath := ExpandConstant('{tmp}\vc_redist.x64.exe');
    MsvcDownloadPage.Clear;
    MsvcDownloadPage.Add('{#MsvcRedistUrl}', 'vc_redist.x64.exe', '{#MsvcRedistHash}');
    MsvcDownloadPage.Show;
    try
      MsvcDownloadPage.Download;
      VerifyPayloadFile(PackagePath, '{#MsvcRedistHash}', {#MsvcRedistSize});
    finally
      MsvcDownloadPage.Hide;
    end;
    // Hashes bind to the Microsoft-signed x64 bundle verified during compilation.
    // No mutable latest URL or downloaded checksum can authorize execution.
    if not MsvcFileCompatible(PackagePath) then
      RaiseException(CustomMessage('MsvcFailed'));
    Log('MSVC official x64 prerequisite verified before execution.');
    Arguments := '/install /passive /norestart';
    if WizardSilent then
      Arguments := '/install /quiet /norestart';
    Arguments := Arguments + ' /log "' + ExpandConstant('{tmp}\msvc-runtime.log') + '"';
    OldStatus := WizardForm.StatusLabel.Caption;
    WizardForm.StatusLabel.Caption := CustomMessage('MsvcInstalling');
    try
      Launched := Exec(PackagePath, Arguments, ExpandConstant('{tmp}'), SW_SHOWNORMAL,
        ewWaitUntilTerminated, ExitCode);
    finally
      WizardForm.StatusLabel.Caption := OldStatus;
    end;
    Log(Format('MSVC prerequisite returned: launched=%d, code=%d', [Ord(Launched), ExitCode]));
    Result := EvaluateMsvcResult(Launched, ExitCode, MsvcSystemCompatible, NeedsRestart);
  except
    if MsvcDownloadPage.AbortedByUser then
      Result := CustomMessage('MsvcCancelled')
    else
      Result := CustomMessage('MsvcFailed') + #13#10 + GetExceptionMessage;
  end;
  CleanupMsvcRuntime;
  if Result <> '' then
    Log(Result);
end;
