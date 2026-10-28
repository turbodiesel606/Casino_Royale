// Personal-data cleanup is independent of program removal and never runs in Setup.
[CustomMessages]
english.DataKeep=JobTracker personal data has been preserved.
russian.DataKeep=Личные данные JobTracker сохранены.
english.DataAccount=Personal data is preserved because the target Windows account or its Local AppData folder could not be resolved safely.%n%1
russian.DataAccount=Личные данные сохранены: не удалось безопасно определить целевую учётную запись Windows или её папку Local AppData.%n%1
english.DataIdentityFallback=The original desktop account could not be resolved. This uninstaller is running as:%n%1%n%nUse this execution account as the data-cleanup target?%nProfile: %2%nData: %3%n%nChoose No to keep personal data. Choosing Yes only selects this account; a separate deletion confirmation follows.
russian.DataIdentityFallback=Не удалось определить исходную учётную запись рабочего стола. Деинсталлятор запущен от имени:%n%1%n%nВыбрать эту учётную запись для очистки данных?%nПрофиль: %2%nДанные: %3%n%nВыберите «Нет», чтобы сохранить личные данные. «Да» только выбирает учётную запись; удаление потребует отдельного подтверждения.
english.DataConfirm=Keep personal data by choosing No (recommended).%n%nPermanently delete the shared JobTracker Data directory?%nTarget account: %1%nProfile: %2%nData: %3%nUninstaller account: %4%n%nAll JobTracker copies used by the target Windows user share this directory. Deletion affects every such copy, including running copies. Cleanup begins immediately after Yes, regardless of program-removal results. Windows may refuse locked or inaccessible files; files already deleted cannot be restored. Original CV files outside Data are not deleted.
russian.DataConfirm=Чтобы сохранить личные данные, выберите «Нет» (рекомендуется).%n%nБезвозвратно удалить общую папку Data программы JobTracker?%nЦелевая учётная запись: %1%nПрофиль: %2%nДанные: %3%nУчётная запись деинсталлятора: %4%n%nВсе копии JobTracker, используемые целевым пользователем Windows, используют эту папку. Удаление затронет каждую такую копию, включая работающие. Очистка начнётся сразу после «Да», независимо от результата удаления программы. Windows может запретить удаление занятых или недоступных файлов; уже удалённые файлы восстановить нельзя. Исходные файлы резюме вне Data не удаляются.
english.DataUnsafe=Personal-data cleanup is incomplete. Remaining failures are listed below. Files already deleted cannot be restored. No deletion is scheduled for restart.%n%1
russian.DataUnsafe=Очистка личных данных завершена не полностью. Ниже перечислены оставшиеся ошибки. Уже удалённые файлы восстановить нельзя. Удаление после перезагрузки не запланировано.%n%1
english.DataDeleted=The confirmed JobTracker Data directory is absent. Cleanup is complete at this time; a running application may create new files later.
russian.DataDeleted=Подтверждённая папка Data программы JobTracker отсутствует. На данный момент очистка завершена; работающая программа может позже создать новые файлы.

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
  TDataGuid = record
    Data1: Cardinal;
    Data2, Data3: Word;
    Data4: array[0..7] of Byte;
  end;
  TDataHandles = array of THandle;
  TPersonalDataIdentity = record
    Token, SidBuffer, Sid: THandle;
    Account, SidText, Profile, LocalData, ExecutionAccount: String;
    FallbackToExecutionAccount: Boolean;
  end;
var
  DataChoiceHandled: Boolean;

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
procedure DataReadWideString(Destination: String; Source: THandle; Size: THandle);
  external 'RtlMoveMemory@kernel32.dll stdcall';
function DataStringLength(Text: THandle): Integer;
  external 'lstrlenW@kernel32.dll stdcall';
function DataSidString(Sid: THandle; var Text: THandle): Longint;
  external 'ConvertSidToStringSidW@advapi32.dll stdcall';
function DataAccountName(SystemName: THandle; Sid: THandle; Name: String;
  var NameSize: Cardinal; Domain: String; var DomainSize: Cardinal; var Kind: Integer): Longint;
  external 'LookupAccountSidW@advapi32.dll stdcall';
function DataUserProfile(Token: THandle; Path: String; var Size: Cardinal): Longint;
  external 'GetUserProfileDirectoryW@userenv.dll stdcall';
function DataKnownFolder(var Folder: TDataGuid; Flags: Cardinal; Token: THandle;
  var Path: THandle): Longint;
  external 'SHGetKnownFolderPath@shell32.dll stdcall';
procedure DataFreeKnownFolder(Path: THandle);
  external 'CoTaskMemFree@ole32.dll stdcall';
function DataShellWindow: THandle;
  external 'GetShellWindow@user32.dll stdcall';
function DataWindowProcess(Window: THandle; var ProcessId: Cardinal): Cardinal;
  external 'GetWindowThreadProcessId@user32.dll stdcall';
function DataOpenProcess(Access: Cardinal; Inherit: Boolean; ProcessId: Cardinal): THandle;
  external 'OpenProcess@kernel32.dll stdcall';
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
function DataAttributes(const Path: String): Cardinal;
  external 'GetFileAttributesW@kernel32.dll stdcall';

procedure RequireDataCondition(const Condition: Boolean; const Detail: String);
begin
  if not Condition then RaiseException(Detail);
end;

function DataWindowsError(const Operation, Path: String; const ErrorCode: Cardinal): String;
begin
  Result := Format('%s: %s (Windows error %d: %s)', [Operation, Path, ErrorCode, SysErrorMessage(ErrorCode)]);
end;

function DataStringFromPointer(const Pointer: THandle): String;
var Count: Integer;
begin
  RequireDataCondition(Pointer <> 0, 'Missing Windows string.');
  Count := DataStringLength(Pointer);
  RequireDataCondition((Count > 0) and (Count < 32768), 'Invalid Windows string length.');
  Result := StringOfChar(#0, Count);
  DataReadWideString(Result, Pointer, Count * 2);
end;

procedure ReleasePersonalDataIdentity(var Identity: TPersonalDataIdentity);
begin
  if Identity.SidBuffer <> 0 then DataFree(Identity.SidBuffer);
  if Identity.Token <> 0 then PolicyCloseHandle(Identity.Token);
  Identity.SidBuffer := 0;
  Identity.Sid := 0;
  Identity.Token := 0;
end;

procedure ClearPersonalDataIdentity(var Identity: TPersonalDataIdentity);
begin
  Identity.Token := 0;
  Identity.SidBuffer := 0;
  Identity.Sid := 0;
  Identity.Account := '';
  Identity.SidText := '';
  Identity.Profile := '';
  Identity.LocalData := '';
  Identity.ExecutionAccount := '';
  Identity.FallbackToExecutionAccount := False;
end;

procedure ReadPersonalDataIdentity(var Identity: TPersonalDataIdentity);
var
  Needed, NameSize, DomainSize: Cardinal;
  Name, Domain, Buffer: String;
  Text, FolderPath: THandle;
  Kind, Status: Integer;
  Folder: TDataGuid;
begin
  Needed := 0;
  DataTokenInformation(Identity.Token, 1, 0, 0, Needed);
  RequireDataCondition((Needed > 0) and (Needed <= 65536), 'Cannot size Windows account SID.');
  Identity.SidBuffer := DataAllocate($40, Needed);
  RequireDataCondition(Identity.SidBuffer <> 0, 'Cannot allocate token information.');
  if DataTokenInformation(Identity.Token, 1, Identity.SidBuffer, Needed, Needed) = 0 then
    RaiseException(DataWindowsError('Cannot read Windows account SID', '', DLLGetLastError));
  DataReadPointer(Identity.Sid, Identity.SidBuffer, SizeOf(Identity.Sid));
  RequireDataCondition((Identity.Sid <> 0) and (DataValidSid(Identity.Sid) <> 0), 'Invalid Windows account SID.');
  Text := 0;
  try
    RequireDataCondition(DataSidString(Identity.Sid, Text) <> 0, 'Cannot format Windows account SID.');
    Identity.SidText := DataStringFromPointer(Text);
  finally
    if Text <> 0 then DataFree(Text);
  end;
  NameSize := 1024;
  DomainSize := 1024;
  Name := StringOfChar(#0, NameSize);
  Domain := StringOfChar(#0, DomainSize);
  if DataAccountName(0, Identity.Sid, Name, NameSize, Domain, DomainSize, Kind) = 0 then
    RaiseException(DataWindowsError('Cannot resolve Windows account name', Identity.SidText, DLLGetLastError));
  SetLength(Name, NameSize);
  SetLength(Domain, DomainSize);
  Identity.Account := Name + ' [' + Identity.SidText + ']';
  if Domain <> '' then Identity.Account := Domain + '\' + Identity.Account;
  Needed := 32768;
  Buffer := StringOfChar(#0, Needed);
  if DataUserProfile(Identity.Token, Buffer, Needed) = 0 then
    RaiseException(DataWindowsError('Cannot resolve Windows profile', Identity.Account, DLLGetLastError));
  RequireDataCondition((Needed > 1) and (Needed <= 32768), 'Invalid Windows profile path.');
  SetLength(Buffer, Needed - 1);
  Identity.Profile := RemoveBackslashUnlessRoot(Buffer);
  // FOLDERID_LocalAppData; request the token's known folder without creating it.
  Folder.Data1 := $F1B32785; Folder.Data2 := $6FBA; Folder.Data3 := $4FCF;
  Folder.Data4[0] := $9D; Folder.Data4[1] := $55; Folder.Data4[2] := $7B; Folder.Data4[3] := $8E;
  Folder.Data4[4] := $7F; Folder.Data4[5] := $15; Folder.Data4[6] := $70; Folder.Data4[7] := $91;
  FolderPath := 0;
  try
    Status := DataKnownFolder(Folder, $4000, Identity.Token, FolderPath);
    RequireDataCondition(Status = 0, Format('Cannot resolve Local AppData for %s (HRESULT %x).', [Identity.Account, Status]));
    Identity.LocalData := RemoveBackslashUnlessRoot(DataStringFromPointer(FolderPath));
    RequireDataCondition(CompareText(CanonicalInstallPath(Identity.LocalData), Identity.LocalData) = 0,
      'Ambiguous Local AppData path.');
    RequireDataCondition(CompareText(CanonicalInstallPath(Identity.Profile), Identity.Profile) = 0,
      'Ambiguous Windows profile path.');
  finally
    if FolderPath <> 0 then DataFreeKnownFolder(FolderPath);
  end;
end;

procedure ResolveExecutionPersonalDataIdentity(var Identity: TPersonalDataIdentity);
begin
  ClearPersonalDataIdentity(Identity);
  try
    if DataOpenProcessToken(THandle(-1), $E, Identity.Token) = 0 then
      RaiseException(DataWindowsError('Cannot read execution account token', '', DLLGetLastError));
    ReadPersonalDataIdentity(Identity);
    Identity.ExecutionAccount := Identity.Account;
  except
    ReleasePersonalDataIdentity(Identity);
    RaiseException(GetExceptionMessage);
  end;
end;

procedure ResolvePersonalDataIdentity(var Identity: TPersonalDataIdentity);
var
  Execution: TPersonalDataIdentity;
  Window, Process: THandle;
  ProcessId: Cardinal;
  ExecutionAccount: String;
begin
  ClearPersonalDataIdentity(Identity);
  ResolveExecutionPersonalDataIdentity(Execution);
  ExecutionAccount := Execution.Account;
  ReleasePersonalDataIdentity(Execution);
  if IsAdmin then begin
    Process := 0;
    try
      try
        // The shell token identifies this desktop's user across UAC credentials.
        // This is a single identity lookup, never an application-process check.
        Window := DataShellWindow;
        RequireDataCondition(Window <> 0, 'No desktop shell is available.');
        RequireDataCondition(DataWindowProcess(Window, ProcessId) <> 0, 'Cannot identify desktop shell.');
        Process := DataOpenProcess($1000, False, ProcessId);
        if Process = 0 then RaiseException(DataWindowsError('Cannot query desktop shell identity', '', DLLGetLastError));
        if DataOpenProcessToken(Process, $E, Identity.Token) = 0 then
          RaiseException(DataWindowsError('Cannot read desktop account token', '', DLLGetLastError));
        ReadPersonalDataIdentity(Identity);
      except
        Log('Desktop account resolution: ' + GetExceptionMessage);
        ReleasePersonalDataIdentity(Identity);
        ResolveExecutionPersonalDataIdentity(Identity);
        Identity.FallbackToExecutionAccount := True;
      end;
    finally
      if Process <> 0 then PolicyCloseHandle(Process);
    end;
  end else
    ResolveExecutionPersonalDataIdentity(Identity);
  Identity.ExecutionAccount := ExecutionAccount;
end;

procedure ResolvePersonalDataLocations(var ExecutionLocalData, TargetLocalData: String);
var Identity: TPersonalDataIdentity;
begin
  ResolveExecutionPersonalDataIdentity(Identity);
  ExecutionLocalData := Identity.LocalData;
  ReleasePersonalDataIdentity(Identity);
  ResolvePersonalDataIdentity(Identity);
  TargetLocalData := Identity.LocalData;
  ReleasePersonalDataIdentity(Identity);
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

// Pin directories against rename/redirection; leaf files use normal Windows
// delete sharing. These are operation handles, not profile/application locks.
function OpenDataHandle(const Path: String; const Deleting, Directory: Boolean): THandle;
var Access, Sharing, Count: Cardinal; Info: TDataFileInfo; FinalPath: String;
begin
  RequireDataCondition((Length(Path) <= 240) and not PathHasInvalidCharacters(Path, True),
    'Unsupported personal-data path: ' + Path);
  Access := $80;
  if Deleting then Access := Access or $10000;
  if Directory then Sharing := 3 else Sharing := 7;
  Result := PolicyCreateFile(Path, Access, Sharing, 0, 3, $02200000, 0);
  if Result = THandle(-1) then
    RaiseException(DataWindowsError('Cannot open personal-data path for the requested operation', Path, DLLGetLastError));
  try
    if DataFileInformation(Result, Info) = 0 then
      RaiseException(DataWindowsError('Cannot inspect personal-data handle', Path, DLLGetLastError));
    RequireDataCondition((Info.Attributes and $400) = 0, 'Reparse point in personal-data path: ' + Path);
    RequireDataCondition(((Info.Attributes and $10) <> 0) = Directory, 'Personal-data path type changed: ' + Path);
    if Deleting and not Directory then
      RequireDataCondition(Info.Links = 1, 'Hard-linked personal data is preserved: ' + Path);
    FinalPath := StringOfChar(#0, 32768);
    Count := PolicyFinalPath(Result, FinalPath, Length(FinalPath), 0);
    RequireDataCondition((Count > 0) and (Count < Cardinal(Length(FinalPath))), 'Cannot resolve personal-data handle: ' + Path);
    SetLength(FinalPath, Count);
    RequireDataCondition(CompareText(FinalPath, '\\?\' + Path) = 0,
      'Personal-data path alias or redirection is not supported: ' + Path);
  except
    PolicyCloseHandle(Result);
    RaiseException(GetExceptionMessage);
  end;
end;

procedure DeleteDataTree(const Path: String; const Directory: Boolean; const Depth: Integer;
  const Failures: TStringList; var Visited, Deleted: Integer);
var
  Handle, Search: THandle;
  Find: TDataFindInfo;
  More: Boolean;
  ErrorCode: Cardinal;
  Name: String;
  I: Integer;
  Delete: Byte;
begin
  Handle := THandle(-1);
  try
    try
      RequireDataCondition((Depth <= 32) and (Visited < 65536), 'Personal-data tree exceeds the safe cleanup limit: ' + Path);
      Visited := Visited + 1;
      Handle := OpenDataHandle(Path, True, Directory);
      if Directory then begin
        Search := DataFindFirst(AddBackslash(Path) + '*', Find);
        if Search = THandle(-1) then begin
          ErrorCode := DLLGetLastError;
          if ErrorCode <> 2 then RaiseException(DataWindowsError('Cannot enumerate personal-data directory', Path, ErrorCode));
        end else begin
          try
            repeat
              Name := '';
              for I := 0 to 259 do begin
                if Find.Name[I] = #0 then Break;
                Name := Name + Find.Name[I];
              end;
              if (Name <> '.') and (Name <> '..') then begin
                RequireDataCondition((Name <> '') and (Trim(Name) = Name) and
                  (Copy(Name, Length(Name), 1) <> '.'), 'Ambiguous personal-data filename: ' + Path + '\' + Name);
                DeleteDataTree(AddBackslash(Path) + Name, (Find.Attributes and $10) <> 0,
                  Depth + 1, Failures, Visited, Deleted);
              end;
              More := DataFindNext(Search, Find) <> 0;
              if not More then begin
                ErrorCode := DLLGetLastError;
                if ErrorCode <> 18 then RaiseException(DataWindowsError('Personal-data enumeration did not finish', Path, ErrorCode));
              end;
            until not More;
          finally
            DataFindClose(Search);
          end;
        end;
      end;
      Delete := 1;
      if DataSetDisposition(Handle, 4, Delete, 1) = 0 then
        RaiseException(DataWindowsError('Windows refused personal-data deletion', Path, DLLGetLastError));
      Deleted := Deleted + 1;
    except
      Name := GetExceptionMessage;
      if Pos(Path, Name) = 0 then Name := Path + ': ' + Name;
      Failures.Add(Name);
      Log('Personal-data failure: ' + Name);
    end;
  finally
    if Handle <> THandle(-1) then PolicyCloseHandle(Handle);
  end;
end;

function PersonalDataPathAbsent(const Path: String): Boolean;
var Attributes, ErrorCode: Cardinal;
begin
  Attributes := DataAttributes(Path);
  Result := Attributes = $FFFFFFFF;
  if Result then begin
    ErrorCode := DLLGetLastError;
    if (ErrorCode <> 2) and (ErrorCode <> 3) then
      RaiseException(DataWindowsError('Cannot inspect personal-data path', Path, ErrorCode));
  end;
end;

procedure DeleteConfirmedPersonalData(const LocalData, Root: String; const UserSid: THandle);
var
  Guards: TDataHandles;
  Failures: TStringList;
  Parent: String;
  I, Visited, Deleted: Integer;
begin
  // LocalData and UserSid come from the explicitly selected token's known
  // folder resolution. Filesystem ownership is not user identity: restored or
  // elevated-created data may legitimately be owned by Administrators.
  // The exact root/handle checks define the boundary; Windows DELETE access
  // and disposition decide whether each operation is permitted.
  RequireDataCondition((Length(LocalData) > 3) and
    (CompareText(Root, AddBackslash(LocalData) + 'JobTracker\Data') = 0), 'Unexpected personal-data root.');
  RequireDataCondition((UserSid <> 0) and (DataValidSid(UserSid) <> 0), 'Invalid confirmed account SID.');
  RequireDataCondition(CompareText(CanonicalInstallPath(LocalData), LocalData) = 0,
    'Ambiguous Local AppData path.');
  if PersonalDataPathAbsent(Root) then begin
    Log('Personal-data cleanup: Data is already absent.');
    Exit;
  end;
  Failures := TStringList.Create;
  Visited := 0;
  Deleted := 0;
  try
    // Pin only ancestors of the exact target. Never delete the JobTracker parent.
    Parent := Copy(LocalData, 1, 3);
    KeepDataHandle(Guards, OpenDataHandle(Parent, False, True));
    for I := 4 to Length(LocalData) + 1 do
      if (I > Length(LocalData)) or (Copy(LocalData, I, 1) = '\') then begin
        Parent := Copy(LocalData, 1, I - 1);
        KeepDataHandle(Guards, OpenDataHandle(Parent, False, True));
      end;
    KeepDataHandle(Guards, OpenDataHandle(AddBackslash(LocalData) + 'JobTracker', False, True));
    DeleteDataTree(Root, True, 0, Failures, Visited, Deleted);
    if not PersonalDataPathAbsent(Root) then
      Failures.Add('The confirmed Data path still exists (files may be locked, inaccessible, or newly created): ' + Root);
    Log(Format('Personal-data cleanup: deletion accepted for %d entries; %d failures.', [Deleted, Failures.Count]));
    if Failures.Count > 0 then RaiseException(Failures.Text);
  finally
    CloseDataHandles(Guards);
    Failures.Free;
  end;
end;

procedure InitializePersonalDataPolicy;
begin
  DataChoiceHandled := False;
end;

procedure PersonalDataAfterUninstall(CurStep: TUninstallStep);
var Identity: TPersonalDataIdentity; Root, Message: String;
begin
  if (CurStep <> usUninstall) or DataChoiceHandled then Exit;
  DataChoiceHandled := True;
  // Silence and command-line flags never constitute deletion consent.
  if UninstallSilent then begin
    Log(CustomMessage('DataKeep'));
    Exit;
  end;
  ClearPersonalDataIdentity(Identity);
  try
    try
      try
        ResolvePersonalDataIdentity(Identity);
      except
        RaiseException(FmtMessage(CustomMessage('DataAccount'), [GetExceptionMessage]));
      end;
      Root := AddBackslash(Identity.LocalData) + 'JobTracker\Data';
      if Identity.FallbackToExecutionAccount then
        if SuppressibleMsgBox(FmtMessage(CustomMessage('DataIdentityFallback'), [Identity.Account, Identity.Profile, Root]), mbConfirmation,
          MB_YESNO or MB_DEFBUTTON2, IDNO) <> IDYES then begin
          Log(CustomMessage('DataKeep'));
          Exit;
        end;
      if SuppressibleMsgBox(FmtMessage(CustomMessage('DataConfirm'), [Identity.Account, Identity.Profile, Root, Identity.ExecutionAccount]),
        mbConfirmation, MB_YESNO or MB_DEFBUTTON2, IDNO) <> IDYES then begin
        Log(CustomMessage('DataKeep'));
        Exit;
      end;
      // No application, installation, profile-idle, or program-removal gate follows consent.
      try
        DeleteConfirmedPersonalData(Identity.LocalData, Root, Identity.Sid);
        Message := CustomMessage('DataDeleted');
      except
        Message := FmtMessage(CustomMessage('DataUnsafe'), [GetExceptionMessage]);
      end;
      Log(Message);
      SuppressibleMsgBox(Message, mbInformation, MB_OK, IDOK);
    except
      Message := GetExceptionMessage;
      Log(Message);
      SuppressibleMsgBox(Message, mbInformation, MB_OK, IDOK);
    end;
  finally
    ReleasePersonalDataIdentity(Identity);
  end;
end;
