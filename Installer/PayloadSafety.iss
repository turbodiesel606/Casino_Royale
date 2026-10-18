; Shared destination and local-file integrity checks. No transport code.
[CustomMessages]
english.PayloadInvalid=The installation files are invalid or the destination is unsafe. Installation has not started.
russian.PayloadInvalid=Файлы установки повреждены или папка назначения небезопасна. Установка ещё не началась.
english.PayloadUnsafePath=The destination contains an inaccessible path or a symbolic link/junction. Choose a regular folder.
russian.PayloadUnsafePath=Путь назначения недоступен либо содержит символическую ссылку. Выберите обычную папку.

[Code]
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

