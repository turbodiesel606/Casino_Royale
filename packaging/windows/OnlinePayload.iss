; Native download/extraction pipeline. The consumer supplies a compiled trust anchor.
; No command-line URL, hash, manifest or fixture override is read here.
[CustomMessages]
english.PayloadDownload=Downloading JobTracker
russian.PayloadDownload=Загрузка JobTracker
english.PayloadDownloadDescription=Downloading the exact release and verifying its SHA-256 checksum.
russian.PayloadDownloadDescription=Загрузка выбранной версии и проверка контрольной суммы SHA-256.
english.PayloadExtract=Preparing JobTracker files
russian.PayloadExtract=Подготовка файлов JobTracker
english.PayloadExtractDescription=Extracting the verified archive into a private temporary folder.
russian.PayloadExtractDescription=Распаковка проверенного архива во временную папку установщика.
english.PayloadDownloadFailed=JobTracker could not be downloaded or verified. Check your connection and try again. The release asset may be unavailable. Installation has not started.
russian.PayloadDownloadFailed=Не удалось загрузить или проверить JobTracker. Проверьте подключение и повторите попытку. Файл выпуска может быть недоступен. Установка ещё не началась.
english.PayloadCancelled=Download or extraction cancelled. Installation has not started. You can retry or exit Setup.
russian.PayloadCancelled=Загрузка или распаковка отменена. Установка ещё не началась. Можно повторить попытку или выйти.
english.PayloadInvalid=The release files are invalid or the destination is unsafe. Installation has not started.
russian.PayloadInvalid=Файлы выпуска повреждены или папка назначения небезопасна. Установка ещё не началась.
english.PayloadUnsafePath=The destination contains an inaccessible path or a symbolic link/junction. Choose a regular folder.
russian.PayloadUnsafePath=Путь назначения недоступен либо содержит символическую ссылку. Выберите обычную папку.

[Code]
var
  PayloadDownloadPage: TDownloadWizardPage;
  PayloadExtractionPage: TExtractionWizardPage;
  PayloadExpectedSize: Int64;
  PayloadArchivePath: String;

function PayloadGetFileAttributes(const FileName: String): Cardinal;
  external 'GetFileAttributesW@kernel32.dll stdcall';

// Reject pre-existing redirection at every destination ancestor and leaf.
// Inno's native extraction also validates archive member paths.
procedure RequireSafePayloadPath(Path: String);
var
  Parent: String;
  Attributes, ErrorCode: Cardinal;
begin
  Path := RemoveBackslashUnlessRoot(ExpandFileName(Path));
  repeat
    Attributes := PayloadGetFileAttributes(Path);
    if Attributes = $FFFFFFFF then begin
      ErrorCode := DLLGetLastError;
      if (ErrorCode <> 2) and (ErrorCode <> 3) then
        RaiseException(CustomMessage('PayloadUnsafePath'));
    end else if (Attributes and $400) <> 0 then
      RaiseException(CustomMessage('PayloadUnsafePath'));
    Parent := ExtractFileDir(Path);
    if Parent = Path then
      Break;
    Path := Parent;
  until Path = '';
end;

procedure VerifyPayloadFile(const Path, ExpectedHash: String; const ExpectedSize: Int64);
var
  ActualSize: Int64;
begin
  RequireSafePayloadPath(Path);
  if not FileSize64(Path, ActualSize) then
    RaiseException(CustomMessage('PayloadInvalid'));
  if (ActualSize <> ExpectedSize) or
     (CompareText(GetSHA256OfFile(Path), ExpectedHash) <> 0) then
    RaiseException(CustomMessage('PayloadInvalid'));
end;

function CheckPayloadDownloadSize(const Url, FileName: String;
  const Progress, ProgressMax: Int64): Boolean;
begin
  Result := (Progress <= PayloadExpectedSize) and (ProgressMax <= PayloadExpectedSize);
end;

procedure InitializeOnlinePayload;
begin
  PayloadDownloadPage := CreateDownloadPage(CustomMessage('PayloadDownload'),
    CustomMessage('PayloadDownloadDescription'), @CheckPayloadDownloadSize);
  PayloadDownloadPage.ShowBaseNameInsteadOfUrl := True;
  PayloadExtractionPage := CreateExtractionPage(CustomMessage('PayloadExtract'),
    CustomMessage('PayloadExtractDescription'), nil);
end;

procedure CleanupOnlinePayload;
begin
  // These paths belong only to this Setup invocation's private {tmp} directory.
  // Never use {app} or a caller-supplied path for recursive cleanup.
  if (PayloadArchivePath <> '') and FileExists(PayloadArchivePath) then
    if not DeleteFile(PayloadArchivePath) then
      Log('Payload archive cleanup deferred to Setup temporary-directory cleanup.');
  if DirExists(ExpandConstant('{tmp}\payload')) then
    if not DelTree(ExpandConstant('{tmp}\payload'), True, True, True) then
      Log('Payload cleanup deferred to Setup temporary-directory cleanup.');
end;

function PrepareOnlinePayload(const Url, ArchiveName, ExpectedHash: String;
  const ExpectedSize: Int64): String;
begin
  Result := '';
  CleanupOnlinePayload;
  // A basename only; reject separators and special paths even for another consumer.
  if (ArchiveName = '') or (Pos('\', ArchiveName) <> 0) or
     (Pos('/', ArchiveName) <> 0) or (Pos(':', ArchiveName) <> 0) or
     (Pos('..', ArchiveName) <> 0) or (Length(ExpectedHash) <> 64) or
     (ExpectedSize <= 0) then begin
    Result := CustomMessage('PayloadInvalid');
    Exit;
  end;
  PayloadExpectedSize := ExpectedSize;
  PayloadArchivePath := ExpandConstant('{tmp}\') + ArchiveName;
  Log('Payload temporary directory: ' + ExpandConstant('{tmp}'));
  PayloadDownloadPage.Clear;
  PayloadDownloadPage.Add(Url, ArchiveName, ExpectedHash);
  PayloadDownloadPage.Show;
  try
    try
      PayloadDownloadPage.Download;
      VerifyPayloadFile(PayloadArchivePath, ExpectedHash, ExpectedSize);
      Log('Payload archive verified before extraction.');
    except
      if PayloadDownloadPage.AbortedByUser then
        Result := CustomMessage('PayloadCancelled')
      else
        Result := CustomMessage('PayloadDownloadFailed') + #13#10 + GetExceptionMessage;
    end;
  finally
    PayloadDownloadPage.Hide;
  end;
  if Result = '' then begin
    PayloadExtractionPage.Clear;
    PayloadExtractionPage.Add(PayloadArchivePath, ExpandConstant('{tmp}\payload'), True);
    PayloadExtractionPage.Show;
    try
      try
        Log('Starting verified payload extraction.');
        PayloadExtractionPage.Extract;
      except
        if PayloadExtractionPage.AbortedByUser then
          Result := CustomMessage('PayloadCancelled')
        else
          Result := CustomMessage('PayloadInvalid') + #13#10 + GetExceptionMessage;
      end;
    finally
      PayloadExtractionPage.Hide;
    end;
  end;
  if Result <> '' then begin
    Log(Result);
    CleanupOnlinePayload;
  end;
end;
