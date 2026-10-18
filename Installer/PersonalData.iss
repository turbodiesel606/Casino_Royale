// Optional cleanup is separate from Inno's program-file uninstall log.
// No Setup path, silent switch, or Deinitialize event authorizes data deletion.
[CustomMessages]
english.DataKeep=JobTracker personal data has been preserved.
russian.DataKeep=Личные данные JobTracker сохранены.
english.DataAccount=Personal data is preserved because the original Windows account could not be verified. All-users and elevated uninstall always preserve data. No other account's folders are inspected.
russian.DataAccount=Личные данные сохранены: исходную учётную запись Windows не удалось подтвердить. Удаление для всех пользователей и удаление с повышенными правами всегда сохраняют данные. Папки других пользователей не проверяются.
english.DataShared=Personal data is preserved because another JobTracker installation remains, or installation registration could not be checked safely.
russian.DataShared=Личные данные сохранены: осталась другая установка JobTracker либо не удалось безопасно проверить сведения об установках.
english.DataIncomplete=Personal data is preserved because removal of this installation and its shortcuts could not be confirmed complete.
russian.DataIncomplete=Личные данные сохранены: полное удаление этой установки и её ярлыков не подтверждено.
english.DataConfirm=The program has been uninstalled. Keep your personal data by choosing No (recommended).%n%nPermanently delete ALL JobTracker personal data for this Windows account in:%n%1%n%nThis includes the SQLite database, imported CVs, and any other files in this folder. It cannot be undone. If you use any other manually copied or older JobTracker copy, choose No. Choose Yes only to delete this data.
russian.DataConfirm=Программа удалена. Чтобы сохранить личные данные, выберите «Нет» (рекомендуется).%n%nБезвозвратно удалить ВСЕ личные данные JobTracker этой учётной записи Windows из папки:%n%1%n%nБудут удалены база SQLite, импортированные резюме и все остальные файлы этой папки. Отменить это действие нельзя. Если вы используете другую скопированную вручную или старую копию JobTracker, выберите «Нет». Выберите «Да» только для удаления этих данных.
english.DataUnsafe=Personal-data cleanup was stopped because ownership, access, or path safety could not be verified. No links are followed and no deletion is scheduled for restart. Files already removed after your confirmation cannot be restored; any remaining data is preserved.%n%1
russian.DataUnsafe=Очистка личных данных остановлена: не удалось подтвердить владельца, доступ или безопасность пути. Переход по ссылкам и удаление после перезагрузки не выполняются. Файлы, уже удалённые после вашего подтверждения, восстановить нельзя; оставшиеся данные сохранены.%n%1
english.DataDeleted=The confirmed JobTracker personal-data folder was deleted.
russian.DataDeleted=Подтверждённая папка личных данных JobTracker удалена.

[Code]
type
  TDataFileInfo = record
    Attributes: Cardinal;
    CreationTime, AccessTime, WriteTime: TFileTime;
    Volume, SizeHigh, SizeLow, Links, IndexHigh, IndexLow: Cardinal;
  end;
  TDataFindInfo = record
    Attributes: Cardinal;
    CreationTime, AccessTime, WriteTime: TFileTime;
    SizeHigh, SizeLow, Reserved0, Reserved1: Cardinal;
    Name: array[0..259] of Char;
    AlternateName: array[0..13] of Char;
  end;
  TDataHandles = array of THandle;
var
  DataAccountVerified: Boolean;
  DataRestartCheckReached: Boolean;

function DataOpenProcessToken(Process: THandle; Access: Cardinal; var Token: THandle): Longint;
  external 'OpenProcessToken@advapi32.dll stdcall';
function DataTokenInformation(Token: THandle; Kind: Integer; Buffer: THandle;
  Size: Cardinal; var Needed: Cardinal): Longint;
  external 'GetTokenInformation@advapi32.dll stdcall';
function DataAllocate(Flags: Cardinal; Size: THandle): THandle;
  external 'LocalAlloc@kernel32.dll stdcall';
function DataFree(Memory: THandle): THandle;
  external 'LocalFree@kernel32.dll stdcall';
procedure DataReadPointer(var Destination: THandle; Source: THandle; Size: THandle);
  external 'RtlMoveMemory@kernel32.dll stdcall';
function DataSecurityInfo(Handle: THandle; Kind, Information: Cardinal;
  var Owner: THandle; Group, Dacl, Sacl: THandle; var Descriptor: THandle): Cardinal;
  external 'GetSecurityInfo@advapi32.dll stdcall';
function DataEqualSid(First, Second: THandle): Longint;
  external 'EqualSid@advapi32.dll stdcall';
function DataValidSid(Sid: THandle): Longint;
  external 'IsValidSid@advapi32.dll stdcall';
function DataFileInformation(Handle: THandle; var Info: TDataFileInfo): Longint;
  external 'GetFileInformationByHandle@kernel32.dll stdcall';
function DataSetDisposition(Handle: THandle; Kind: Integer; var Delete: Byte; Size: Cardinal): Longint;
  external 'SetFileInformationByHandle@kernel32.dll stdcall';
function DataFindFirst(const Pattern: String; var Info: TDataFindInfo): THandle;
  external 'FindFirstFileW@kernel32.dll stdcall';
function DataFindNext(Handle: THandle; var Info: TDataFindInfo): Longint;
  external 'FindNextFileW@kernel32.dll stdcall';
function DataFindClose(Handle: THandle): Longint;
  external 'FindClose@kernel32.dll stdcall';

procedure RequireDataCondition(const Condition: Boolean; const Detail: String);
begin
  if not Condition then RaiseException(Detail);
end;

procedure CloseDataHandles(var Handles: TDataHandles);
var I: Integer;
begin
  for I := GetArrayLength(Handles) - 1 downto 0 do
    if Handles[I] <> 0 then PolicyCloseHandle(Handles[I]);
  SetArrayLength(Handles, 0);
end;

procedure KeepDataHandle(var Handles: TDataHandles; const Handle: THandle);
var Count: Integer;
begin
  Count := GetArrayLength(Handles);
  SetArrayLength(Handles, Count + 1);
  Handles[Count] := Handle;
end;

procedure RequireDataOwner(const Handle: THandle; const UserSid: THandle);
var Owner, Descriptor: THandle;
begin
  Descriptor := 0;
  Owner := 0;
  try
    RequireDataCondition(DataSecurityInfo(Handle, 1, 1, Owner, 0, 0, 0, Descriptor) = 0,
      'Cannot read personal-data ownership.');
    RequireDataCondition((Owner <> 0) and (DataValidSid(Owner) <> 0), 'Invalid personal-data owner.');
    RequireDataCondition(DataEqualSid(Owner, UserSid) <> 0, 'Personal-data owner differs from the Windows token.');
  finally
    if Descriptor <> 0 then DataFree(Descriptor);
  end;
end;

// OPEN_REPARSE_POINT prevents leaf traversal. Retaining every ancestor handle
// without FILE_SHARE_DELETE prevents rename/redirection of the validated path.
// Files are exclusive; directories allow enumeration but not rename/deletion.
function OpenDataHandle(const Path: String; const Deleting, Directory: Boolean;
  const UserSid: THandle): THandle;
var Access, Sharing, Count: Cardinal; Info: TDataFileInfo; FinalPath: String;
begin
  RequireDataCondition((Length(Path) <= 240) and not PathHasInvalidCharacters(Path, True),
    'Unsupported personal-data path.');
  Access := $20080; // READ_CONTROL | FILE_READ_ATTRIBUTES
  if Deleting then Access := Access or $10000; // DELETE
  if Directory then Sharing := 3 else Sharing := 0;
  Result := PolicyCreateFile(Path, Access, Sharing, 0, 3, $02200000, 0);
  RequireDataCondition(Result <> THandle(-1), 'Cannot lock personal-data path: ' + Path);
  try
    RequireDataCondition(DataFileInformation(Result, Info) <> 0, 'Cannot inspect personal-data handle.');
    RequireDataCondition((Info.Attributes and $400) = 0, 'Reparse point in personal-data path.');
    RequireDataCondition(((Info.Attributes and $10) <> 0) = Directory, 'Personal-data path type changed.');
    if Deleting then begin
      RequireDataCondition((Info.Attributes and 1) = 0, 'Read-only personal data is preserved.');
      if not Directory then RequireDataCondition(Info.Links = 1, 'Hard-linked personal data is preserved.');
      RequireDataOwner(Result, UserSid);
    end;
    FinalPath := StringOfChar(#0, 32768);
    Count := PolicyFinalPath(Result, FinalPath, Length(FinalPath), 0);
    RequireDataCondition((Count > 0) and (Count < Cardinal(Length(FinalPath))), 'Cannot resolve personal-data handle.');
    SetLength(FinalPath, Count);
    RequireDataCondition(CompareText(FinalPath, '\\?\' + Path) = 0,
      'Personal-data path alias or redirection is not supported.');
  except
    PolicyCloseHandle(Result);
    RaiseException(GetExceptionMessage);
  end;
end;

procedure LockDataTree(const Path: String; const Directory: Boolean; const Depth: Integer;
  const UserSid: THandle; var Handles: TDataHandles);
var
  Find: TDataFindInfo;
  Search: THandle;
  More: Boolean;
  ErrorCode: Cardinal;
  Name: String;
  I: Integer;
begin
  RequireDataCondition((Depth <= 32) and (GetArrayLength(Handles) < 8192),
    'Personal-data tree exceeds the safe cleanup limit.');
  KeepDataHandle(Handles, OpenDataHandle(Path, True, Directory, UserSid));
  if not Directory then Exit;
  // Use native enumeration so DLLGetLastError distinguishes completion from
  // errors. Inno's built-in FindNext does not update DLLGetLastError.
  Search := DataFindFirst(AddBackslash(Path) + '*', Find);
  if Search = THandle(-1) then begin
    ErrorCode := DLLGetLastError;
    RequireDataCondition(ErrorCode = 2, 'Cannot enumerate personal-data directory.');
    Exit;
  end;
  try
    repeat
      Name := '';
      for I := 0 to 259 do begin
        if Find.Name[I] = #0 then Break;
        Name := Name + Find.Name[I];
      end;
      if (Name <> '.') and (Name <> '..') then begin
        RequireDataCondition((Name <> '') and (Trim(Name) = Name) and
          (Copy(Name, Length(Name), 1) <> '.'), 'Ambiguous personal-data filename.');
        LockDataTree(AddBackslash(Path) + Name, (Find.Attributes and $10) <> 0,
          Depth + 1, UserSid, Handles);
      end;
      More := DataFindNext(Search, Find) <> 0;
      if not More then begin
        ErrorCode := DLLGetLastError;
        RequireDataCondition(ErrorCode = 18, 'Personal-data enumeration did not finish.');
      end;
    until not More;
  finally
    DataFindClose(Search);
  end;
end;

// The caller supplies the resolved Windows Local AppData folder, never a path
// from a command-line deletion switch. Root must be its exact JobTracker child.
// All checks finish before any disposition is set. Only already-open handles
// are deleted, bottom-up; no recursive pathname delete or reboot cleanup exists.
procedure DeleteConfirmedPersonalData(const LocalData, Root: String);
var
  Guards, Handles: TDataHandles;
  Token: THandle;
  Buffer, UserSid: THandle;
  Needed: Cardinal;
  Parent: String;
  I: Integer;
  Delete: Byte;
begin
  Token := 0;
  Buffer := 0;
  try
    RequireDataCondition(not IsAdmin and not IsAdminInstallMode, 'Elevated/all-users cleanup is disabled.');
    RequireDataCondition((Length(LocalData) > 3) and
      (CompareText(Root, AddBackslash(LocalData) + 'JobTracker') = 0), 'Unexpected personal-data root.');
    RequireDataCondition(CompareText(CanonicalInstallPath(LocalData), LocalData) = 0,
      'Ambiguous Local AppData path.');
    RequireDataCondition(DataOpenProcessToken(THandle(-1), 8, Token) <> 0, 'Cannot read Windows account token.');
    Buffer := DataAllocate($40, 256);
    RequireDataCondition(Buffer <> 0, 'Cannot allocate token information.');
    RequireDataCondition(DataTokenInformation(Token, 1, Buffer, 256, Needed) <> 0, 'Cannot read Windows account SID.');
    DataReadPointer(UserSid, Buffer, SizeOf(UserSid));
    RequireDataCondition((UserSid <> 0) and (DataValidSid(UserSid) <> 0), 'Invalid Windows account SID.');
    // Pin top-down; never follow an ancestor before its parent is pinned.
    Parent := Copy(LocalData, 1, 3);
    KeepDataHandle(Guards, OpenDataHandle(Parent, False, True, UserSid));
    for I := 4 to Length(LocalData) + 1 do
      if (I > Length(LocalData)) or (Copy(LocalData, I, 1) = '\') then begin
        Parent := Copy(LocalData, 1, I - 1);
        KeepDataHandle(Guards, OpenDataHandle(Parent, False, True, UserSid));
      end;
    RequireDataOwner(Guards[GetArrayLength(Guards) - 1], UserSid);
    LockDataTree(Root, True, 0, UserSid, Handles);
    Delete := 1;
    for I := GetArrayLength(Handles) - 1 downto 0 do begin
      RequireDataCondition(DataSetDisposition(Handles[I], 4, Delete, 1) <> 0,
        'Windows refused personal-data deletion; remaining handles are released without deletion.');
      PolicyCloseHandle(Handles[I]);
      Handles[I] := 0;
    end;
  finally
    CloseDataHandles(Handles);
    CloseDataHandles(Guards);
    if Buffer <> 0 then DataFree(Buffer);
    if Token <> 0 then PolicyCloseHandle(Token);
  end;
end;

procedure InitializePersonalDataPolicy;
var Path: String; Version: Int64;
begin
  DataAccountVerified := False;
  if IsAdminInstallMode or IsAdmin then Exit;
  try
    DataAccountVerified := ReadInstallRecord(HKCU64,
      InstallUninstallKey + 'CurrentUser_is1', 'CurrentUser', Path, Version);
    DataAccountVerified := DataAccountVerified and
      (CompareText(Path, CanonicalInstallPath(ExpandConstant('{app}'))) = 0);
  except
    DataAccountVerified := False;
    Log('Personal-data account check: ' + GetExceptionMessage);
  end;
end;

procedure RequireNoRemainingInstallation;
begin
  // Presence alone blocks deletion, including malformed or older records.
  // Check only this account and machine scope, never enumerate other profiles.
  if InstallKeyExists(HKCU64, InstallUninstallKey + 'CurrentUser_is1') or
     InstallKeyExists(HKCU32, InstallUninstallKey + 'CurrentUser_is1') or
     InstallKeyExists(HKLM64, InstallUninstallKey + 'AllUsers_is1') or
     InstallKeyExists(HKLM32, InstallUninstallKey + 'AllUsers_is1') then
    RaiseException(CustomMessage('DataShared'));
end;

procedure RequireProgramRemovalComplete;
var I, J: Integer; Folder, Path: String;
begin
  // usDone also runs after incomplete native removal. Require the whole program
  // directory to be gone: even unknown leftovers conservatively preserve data.
  Path := ExpandConstant('{app}');
  RequireSafePayloadPath(Path);
  if DirExists(Path) or FileExists(Path) then RaiseException(CustomMessage('DataIncomplete'));
  for I := 0 to 1 do begin
    if I = 0 then Folder := ExpandConstant('{autoprograms}\JobTracker')
    else Folder := ExpandConstant('{autodesktop}');
    for J := 0 to 1 do begin
      Path := AddBackslash(Folder) + PolicyShortcutName(IsAdminInstallMode, J = 1) + '.lnk';
      RequireSafePayloadPath(Path);
      if FileExists(Path) or DirExists(Path) then RaiseException(CustomMessage('DataIncomplete'));
    end;
  end;
end;

// Inno calls this only when native removal has not already required a restart.
// It is an additional completion gate, not a replacement for residual checks.
<event('UninstallNeedRestart')>
function PersonalDataRestartCheck: Boolean;
begin
  DataRestartCheckReached := True;
  Result := False;
end;

<event('CurUninstallStepChanged')>
procedure PersonalDataAfterUninstall(CurStep: TUninstallStep);
var LocalData, Root, Message: String;
begin
  if CurStep <> usDone then Exit;
  Log(CustomMessage('DataKeep'));
  // Silence is never consent, including unrecognized /DELETE... switches.
  if UninstallSilent then Exit;
  try
    if not DataAccountVerified then RaiseException(CustomMessage('DataAccount'));
    if not DataRestartCheckReached then RaiseException(CustomMessage('DataIncomplete'));
    RequireProgramRemovalComplete;
    RequireNoRemainingInstallation;
    LocalData := RemoveBackslashUnlessRoot(ExpandConstant('{localappdata}'));
    Root := AddBackslash(LocalData) + 'JobTracker';
    RequireSafePayloadPath(Root);
    if not DirExists(Root) then Exit;
    if SuppressibleMsgBox(FmtMessage(CustomMessage('DataConfirm'), [Root]),
      mbConfirmation, MB_YESNO or MB_DEFBUTTON2, IDNO) <> IDYES then Exit;
    // Consent never bypasses a fresh coexistence/completion check.
    RequireProgramRemovalComplete;
    RequireNoRemainingInstallation;
    try
      DeleteConfirmedPersonalData(LocalData, Root);
    except
      RaiseException(FmtMessage(CustomMessage('DataUnsafe'), [GetExceptionMessage]));
    end;
    Log(CustomMessage('DataDeleted'));
  except
    Message := GetExceptionMessage;
    Log(Message);
    SuppressibleMsgBox(Message, mbInformation, MB_OK, IDOK);
  end;
end;
