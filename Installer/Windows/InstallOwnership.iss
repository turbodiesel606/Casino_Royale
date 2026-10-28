// Per-directory program ownership. Personal data is handled independently.
[CustomMessages]
english.InstallMoved=This uninstaller belongs to a different installation path. No program files were removed. Run Setup into this folder to repair its local ownership and uninstaller.
russian.InstallMoved=Этот деинсталлятор относится к другому пути установки. Файлы программы не удалены. Запустите установщик в этой папке для восстановления локальных сведений и деинсталлятора.
english.ProgramIncomplete=Program removal is incomplete. The following owned files or shortcuts could not be removed. Personal-data cleanup has its own separate result.%n%1
russian.ProgramIncomplete=Удаление программы не завершено. Не удалось удалить следующие принадлежащие этой копии файлы или ярлыки. Результат очистки личных данных сообщается отдельно.%n%1
english.ProgramComplete=Owned program files and shortcuts were removed. Unrelated files are preserved. Personal-data cleanup has its own separate result.
russian.ProgramComplete=Файлы и ярлыки этой копии удалены. Посторонние файлы сохранены. Результат очистки личных данных сообщается отдельно.
english.InstallWriteFailed=Installation bookkeeping or payload verification failed. This folder may contain a partial installation; run Setup here again to repair it. Personal data was not changed.
russian.InstallWriteFailed=Не удалось записать сведения об установке или проверить файлы. Установка может быть частичной; повторно запустите установщик в эту папку для восстановления. Личные данные не изменены.
english.InstallIncompleteHeading=Installation is incomplete
russian.InstallIncompleteHeading=Установка не завершена
english.InstallAutomaticClose=Automatic application closing or restarting is not supported. Remove the /CLOSEAPPLICATIONS, /FORCECLOSEAPPLICATIONS, /RESTARTAPPLICATIONS or /LOADINF option.
russian.InstallAutomaticClose=Автоматическое закрытие или перезапуск приложений не поддерживается. Уберите параметры /CLOSEAPPLICATIONS, /FORCECLOSEAPPLICATIONS, /RESTARTAPPLICATIONS или /LOADINF.

[Code]
type
  IOwnershipShellLinkW = interface(IUnknown)
    '{000214F9-0000-0000-C000-000000000046}'
    procedure GetPath(Buffer: String; Size: Integer; FindData: THandle; Flags: DWORD); safecall;
  end;
  IOwnershipPersistFile = interface(IUnknown)
    '{0000010B-0000-0000-C000-000000000046}'
    procedure DummyGetClassID;
    procedure DummyIsDirty;
    procedure Load(FileName: String; Mode: Longint); safecall;
  end;
  TOwnedCandidate = record
    Path, Hash: String;
  end;
  TOwnershipRename = record
    ReplaceExisting, Padding: Cardinal;
    RootDirectory: THandle;
    NameBytes: Cardinal;
    Name: array[0..32767] of Char;
  end;
var
  OwnedCandidates: array of TOwnedCandidate;
  OwnedFiles: TArrayOfString;
  OwnershipRoot, OwnershipId, OwnershipLedger: String;
  OwnedStartMenu, OwnedDesktop, ProgramFailures: String;
  OwnershipWriteFailed: Boolean;
  ProgramRootGuards: TDataHandles;

procedure RegisterCurrentPayload; forward;
function RemoveOwnedShortcut(const Path: String): Boolean; forward;

function OwnershipMoveFile(const Source, Destination: String): Boolean;
  external 'MoveFileW@kernel32.dll stdcall';
function OwnershipWrite(Handle: THandle; Buffer: String; Count: Cardinal;
  var Written: Cardinal; Overlapped: THandle): Boolean;
  external 'WriteFile@kernel32.dll stdcall';
function OwnershipFlush(Handle: THandle): Boolean;
  external 'FlushFileBuffers@kernel32.dll stdcall';
function OwnershipRename(Handle: THandle; Kind: Integer; var Info: TOwnershipRename;
  Size: Cardinal): Boolean;
  external 'SetFileInformationByHandle@kernel32.dll stdcall';

function OpenOwnedHandleShared(const Path: String; const Directory, Deleting: Boolean;
  const Sharing: Cardinal): THandle;
var Access, ErrorCode, Count: Cardinal; Info: TDataFileInfo; FinalPath: String;
begin
  Access := $80;
  if Deleting then Access := Access or $10000;
  // These are filesystem handles for the requested operation, never an
  // application/profile idle lock. Directories are pinned against redirection.
  Result := PolicyCreateFile(Path, Access, Sharing, 0, 3, $02200000, 0);
  if Result = THandle(-1) then begin
    ErrorCode := DLLGetLastError;
    RaiseException(DataWindowsError('Cannot open owned path', Path, ErrorCode));
  end;
  try
    if DataFileInformation(Result, Info) = 0 then
      RaiseException(DataWindowsError('Cannot inspect owned path', Path, DLLGetLastError));
    if ((Info.Attributes and $400) <> 0) or
       (((Info.Attributes and $10) <> 0) <> Directory) or
       (not Directory and (Info.Links <> 1)) then
      RaiseException('Unsafe or hard-linked owned path: ' + Path);
    FinalPath := StringOfChar(#0, 32768);
    Count := PolicyFinalPath(Result, FinalPath, Length(FinalPath), 0);
    if (Count = 0) or (Count >= Cardinal(Length(FinalPath))) then
      RaiseException('Cannot resolve owned handle: ' + Path);
    SetLength(FinalPath, Count);
    if CompareText(FinalPath, '\\?\' + Path) <> 0 then
      RaiseException('Owned path changed or is redirected: ' + Path);
  except
    PolicyCloseHandle(Result);
    RaiseException(GetExceptionMessage);
  end;
end;

function OpenOwnedHandle(const Path: String; const Directory, Deleting: Boolean): THandle;
var Sharing: Cardinal;
begin
  if Directory then Sharing := 3 else Sharing := 7;
  Result := OpenOwnedHandleShared(Path, Directory, Deleting, Sharing);
end;

procedure PinOwnedParents(const Path: String; var Handles: TDataHandles);
var Parent: String; I: Integer;
begin
  Parent := ExtractFileDir(Path);
  RequireSafePayloadPath(Parent);
  KeepDataHandle(Handles, OpenOwnedHandle(Copy(Parent, 1, 3), True, False));
  for I := 4 to Length(Parent) + 1 do
    if (I > Length(Parent)) or (Copy(Parent, I, 1) = '\') then
      KeepDataHandle(Handles, OpenOwnedHandle(Copy(Parent, 1, I - 1), True, False));
end;

procedure ReleaseProgramRoot;
begin
  CloseDataHandles(ProgramRootGuards);
end;

function InstallationId(const Path: String): String;
begin
  Result := Copy(GetSHA256OfUnicodeString(Lowercase(CanonicalInstallPath(Path))), 1, 16);
end;

function SafeOwnedRelative(const Path: String): Boolean;
var I, Start: Integer; Part: String;
begin
  Result := False;
  if (Path = '') or (Length(Path) > 220) or (Pos(':', Path) <> 0) or
     (Pos('/', Path) <> 0) or (Pos(#13, Path) <> 0) or (Pos(#10, Path) <> 0) or
     PathHasInvalidCharacters(Path, True) then Exit;
  Start := 1;
  for I := 1 to Length(Path) + 1 do
    if (I > Length(Path)) or (Copy(Path, I, 1) = '\') then begin
      Part := Copy(Path, Start, I - Start);
      if (Part = '') or (Part = '.') or (Part = '..') or (Trim(Part) <> Part) or
         (Copy(Part, Length(Part), 1) = '.') then Exit;
      Start := I + 1;
    end;
  Result := True;
end;

procedure RegisterOwnedCandidate(const Path, Hash: String);
var N: Integer;
begin
  if not SafeOwnedRelative(Path) then RaiseException(CustomMessage('InstallStateInvalid'));
  N := GetArrayLength(OwnedCandidates);
  SetArrayLength(OwnedCandidates, N + 1);
  OwnedCandidates[N].Path := Path;
  OwnedCandidates[N].Hash := Hash;
end;

function OwnedIndex(const Path: String): Integer;
var I: Integer;
begin
  Result := -1;
  for I := 0 to GetArrayLength(OwnedFiles) - 1 do
    if CompareText(OwnedFiles[I], Path) = 0 then begin
      Result := I;
      Exit;
    end;
end;

procedure LoadLocalOwnership(const Root: String);
var I, Count, N: Integer; Path, Ledger: String;
begin
  SetArrayLength(OwnedFiles, 0);
  Ledger := AddBackslash(Root) + 'JobTracker.ownership.ini';
  RequireSafePayloadPath(Ledger);
  if GetIniString('Identity', 'Product', '', Ledger) <> 'JobTracker-2' then Exit;
  Count := GetIniInt('Identity', 'Count', 0, 0, 20000, Ledger);
  for I := 0 to Count - 1 do begin
    Path := GetIniString('Files', IntToStr(I), '', Ledger);
    if SafeOwnedRelative(Path) and (OwnedIndex(Path) < 0) then begin
      N := GetArrayLength(OwnedFiles);
      SetArrayLength(OwnedFiles, N + 1);
      OwnedFiles[N] := Path;
    end else
      Log('Unverifiable ownership entry preserved without deletion: ' + Path);
  end;
end;

procedure LoadOwnershipLines(const FileName: String; var Lines: TArrayOfString);
var Raw: AnsiString; Decoded: String; I: Integer; List: TStringList;
begin
  if not LoadStringFromFile(FileName, Raw) or (Length(Raw) > 4194304) then
    RaiseException('Cannot read local metadata: ' + FileName);
  // Windows INI APIs use UTF-16 for Unicode files; LoadStringsFromFile only
  // decodes ANSI/UTF-8. Decode our UTF-16LE bytes before splitting the lines.
  if (Length(Raw) >= 2) and (Ord(Raw[1]) = 255) and (Ord(Raw[2]) = 254) then begin
    if (Length(Raw) mod 2) <> 0 then RaiseException('Truncated local metadata: ' + FileName);
    SetLength(Decoded, (Length(Raw) - 2) div 2);
    for I := 1 to Length(Decoded) do
      Decoded[I] := Chr(Ord(Raw[I * 2 + 1]) + Ord(Raw[I * 2 + 2]) * 256);
    List := TStringList.Create;
    try
      List.Text := Decoded;
      SetArrayLength(Lines, List.Count);
      for I := 0 to List.Count - 1 do Lines[I] := List[I];
    finally
      List.Free;
    end;
  end else if not LoadStringsFromFile(FileName, Lines) then
    RaiseException('Cannot decode local metadata: ' + FileName);
end;

function UpdatedOwnershipText(const Section, Key, Value, FileName: String): String;
var Lines: TArrayOfString; I, Separator: Integer; Line: String;
    InSection, FoundSection, Written: Boolean;
begin
  if FileExists(FileName) then LoadOwnershipLines(FileName, Lines);
  Result := #$FEFF;
  InSection := False;
  FoundSection := False;
  Written := False;
  for I := 0 to GetArrayLength(Lines) - 1 do begin
    Line := Lines[I];
    if (Copy(Line, 1, 1) = '[') then begin
      if InSection and not Written then begin
        Result := Result + Key + '=' + Value + #13#10;
        Written := True;
      end;
      InSection := CompareText(Trim(Line), '[' + Section + ']') = 0;
      FoundSection := FoundSection or InSection;
    end;
    Separator := Pos('=', Line);
    if InSection and (Separator > 0) and
       (CompareText(Trim(Copy(Line, 1, Separator - 1)), Key) = 0) then begin
      if not Written then Result := Result + Key + '=' + Value + #13#10;
      Written := True;
    end else
      Result := Result + Line + #13#10;
  end;
  if not FoundSection then Result := Result + '[' + Section + ']' + #13#10;
  if not Written then Result := Result + Key + '=' + Value + #13#10;
end;

procedure WriteOwnershipValue(const Section, Key, Value, FileName: String);
var Temporary, Contents: String; N, I: Integer; Guards: TDataHandles;
    Handle: THandle; Written, ErrorCode: Cardinal; Rename: TOwnershipRename;
begin
  Handle := THandle(-1);
  try
    try
      PinOwnedParents(FileName, Guards);
      RequireSafePayloadPath(FileName);
      Contents := UpdatedOwnershipText(Section, Key, Value, FileName);
      N := 0;
      repeat
        Temporary := FileName + '.write-' + IntToStr(N);
        N := N + 1;
        // CREATE_NEW reserves the exact object. All writes and the final atomic
        // rename use this exclusive handle, never a re-opened temporary path.
        Handle := PolicyCreateFile(Temporary, $40010000, 0, 0, 1, $80, 0);
        if Handle = THandle(-1) then begin
          ErrorCode := DLLGetLastError;
          if (ErrorCode <> 80) and (ErrorCode <> 183) then
            RaiseException(DataWindowsError('Cannot reserve local metadata', Temporary, ErrorCode));
        end;
      until Handle <> THandle(-1);
      if not OwnershipWrite(Handle, Contents, Length(Contents) * 2, Written, 0) or
         (Written <> Cardinal(Length(Contents) * 2)) or not OwnershipFlush(Handle) then
        RaiseException(DataWindowsError('Cannot write local metadata', Temporary, DLLGetLastError));
      if Length(FileName) > 32767 then RaiseException('Metadata path is too long.');
      Rename.ReplaceExisting := 1;
      Rename.Padding := 0;
      Rename.RootDirectory := 0;
      Rename.NameBytes := Length(FileName) * 2;
      for I := 1 to Length(FileName) do Rename.Name[I - 1] := FileName[I];
      if not OwnershipRename(Handle, 3, Rename, 20 + Rename.NameBytes) then
        RaiseException(DataWindowsError('Cannot replace local metadata', FileName, DLLGetLastError));
    except
      OwnershipWriteFailed := True;
      RaiseException(CustomMessage('InstallWriteFailed') + #13#10 + GetExceptionMessage);
    end;
  finally
    if Handle <> THandle(-1) then PolicyCloseHandle(Handle);
    CloseDataHandles(Guards);
  end;
end;

procedure RecordInstalledFile(const Path: String);
var N: Integer;
begin
  if not SafeOwnedRelative(Path) then RaiseException(CustomMessage('InstallStateInvalid'));
  if OwnedIndex(Path) >= 0 then Exit;
  N := GetArrayLength(OwnedFiles);
  WriteOwnershipValue('Files', IntToStr(N), Path, OwnershipLedger);
  WriteOwnershipValue('Identity', 'Count', IntToStr(N + 1), OwnershipLedger);
  SetArrayLength(OwnedFiles, N + 1);
  OwnedFiles[N] := Path;
end;

procedure PreserveUnknownFile(const Path: String);
var Backup: String; N: Integer; Guards: TDataHandles;
begin
  RequireSafePayloadPath(Path);
  if not FileExists(Path) and not DirExists(Path) then Exit;
  N := 1;
  repeat
    Backup := Path + '.preserved-' + IntToStr(N);
    N := N + 1;
  until not FileExists(Backup) and not DirExists(Backup);
  RequireSafePayloadPath(Backup);
  try
    PinOwnedParents(Path, Guards);
    if not OwnershipMoveFile(Path, Backup) then
      RaiseException('Windows could not preserve an existing file: ' + Path +
        ' (' + SysErrorMessage(DLLGetLastError) + ')');
    Log('Preserved existing unverified file: ' + Backup);
  finally
    CloseDataHandles(Guards);
  end;
end;

function ValidLocalHistory(const Root, Scope: String): String;
var Marker, Leaf, Hash: String;
begin
  Result := '';
  Marker := InstallMarker(Root);
  if (GetIniString('History', 'Path', '', Marker) <> Root) or
     (GetIniString('History', 'Scope', '', Marker) <> Scope) then Exit;
  Leaf := GetIniString('History', 'Log', '', Marker);
  Hash := GetIniString('History', 'SHA256', '', Marker);
  if (Length(Leaf) <> 12) or (Copy(Leaf, 1, 5) <> 'unins') or
     (Copy(Leaf, 9, 4) <> '.dat') or (Length(Hash) <> 64) then Exit;
  RequireSafePayloadPath(AddBackslash(Root) + Leaf);
  if FileExists(AddBackslash(Root) + Leaf) then
    if CompareText(GetSHA256OfFile(AddBackslash(Root) + Leaf), Hash) = 0 then Result := Leaf;
end;

procedure PreserveUnverifiedHistory(const Root, Scope: String);
var Find: TFindRec; Valid, Leaf, Path: String; Files: TArrayOfString; N, I: Integer;
begin
  Valid := ValidLocalHistory(Root, Scope);
  if FindFirst(AddBackslash(Root) + 'unins*', Find) then begin
    try
      repeat
        Leaf := Find.Name;
        if (Length(Leaf) = 12) and
           ((ExtractFileExt(Leaf) = '.dat') or (ExtractFileExt(Leaf) = '.exe') or
            (ExtractFileExt(Leaf) = '.msg')) and
           ((Valid = '') or (ChangeFileExt(Leaf, '.dat') <> Valid)) then begin
          N := GetArrayLength(Files);
          SetArrayLength(Files, N + 1);
          Files[N] := Leaf;
        end;
      until not FindNext(Find);
    finally
      FindClose(Find);
    end;
  end;
  for I := 0 to GetArrayLength(Files) - 1 do begin
    Path := AddBackslash(Root) + Files[I];
    PreserveUnknownFile(Path);
  end;
  if Valid <> '' then Log('Preserving valid local uninstall history for append: ' + Valid)
  else Log('Reconstructing local ownership; unavailable historical records are not recovered.');
end;

function GetOwnershipScope: String;
begin
  if IsAdminInstallMode then Result := 'AllUsers' else Result := 'CurrentUser';
end;

procedure PrepareInstallOwnership;
var I: Integer; Path, Marker, Scope: String;
begin
  OwnershipRoot := CanonicalInstallPath(ExpandConstant('{app}'));
  OwnershipId := InstallationId(OwnershipRoot);
  OwnershipLedger := AddBackslash(OwnershipRoot) + 'JobTracker.ownership.ini';
  Scope := GetOwnershipScope;
  LoadLocalOwnership(OwnershipRoot);
  if not ForceDirectories(OwnershipRoot) then
    RaiseException(CustomMessage('InstallWriteFailed') + #13#10 + OwnershipRoot);
  PinOwnedParents(AddBackslash(OwnershipRoot) + 'JobTracker.install.ini', ProgramRootGuards);
  PreserveUnverifiedHistory(OwnershipRoot, Scope);
  if FileExists(OwnershipLedger) and
     (GetIniString('Identity', 'Product', '', OwnershipLedger) <> 'JobTracker-2') then
    PreserveUnknownFile(OwnershipLedger);
  Marker := InstallMarker(OwnershipRoot);
  if FileExists(Marker) and
     ((GetIniString('Installation', 'Product', '', Marker) <> 'JobTracker') or
      (GetIniString('Installation', 'Format', '', Marker) <> '2')) then
    PreserveUnknownFile(Marker);
  // Missing history is reconstructed only from the relative ledger and exact
  // known payload hashes. Colliding unowned files are saved, never adopted.
  for I := 0 to GetArrayLength(OwnedCandidates) - 1 do begin
    Path := AddBackslash(OwnershipRoot) + OwnedCandidates[I].Path;
    RequireSafePayloadPath(Path);
    if (OwnedIndex(OwnedCandidates[I].Path) < 0) and FileExists(Path) then begin
      if CompareText(GetSHA256OfFile(Path), OwnedCandidates[I].Hash) = 0 then
        RecordInstalledFile(OwnedCandidates[I].Path)
      else
        PreserveUnknownFile(Path);
    end;
    if DirExists(Path) then RaiseException(CustomMessage('InstallStateInvalid') + #13#10 + Path);
  end;
  WriteOwnershipValue('Identity', 'Product', 'JobTracker-2', OwnershipLedger);
  WriteOwnershipValue('Identity', 'Path', OwnershipRoot, OwnershipLedger);
  Marker := InstallMarker(OwnershipRoot);
  // Retain shortcut locations only when they came from this exact directory.
  OwnedStartMenu := '';
  OwnedDesktop := '';
  if CompareText(GetIniString('Installation', 'Path', '', Marker), OwnershipRoot) = 0 then begin
    OwnedStartMenu := GetIniString('Shortcuts', 'StartMenu', '', Marker);
    OwnedDesktop := GetIniString('Shortcuts', 'Desktop', '', Marker);
  end;
  WriteOwnershipValue('Installation', 'Format', '2', Marker);
  WriteOwnershipValue('Installation', 'Product', 'JobTracker', Marker);
  WriteOwnershipValue('Installation', 'Path', OwnershipRoot, Marker);
  WriteOwnershipValue('Installation', 'Id', OwnershipId, Marker);
  WriteOwnershipValue('Installation', 'Scope', Scope, Marker);
end;

function ReadShortcutTargetsCopy(const Path, Root: String): Boolean;
var Obj: IUnknown; Link: IOwnershipShellLinkW; Persist: IOwnershipPersistFile;
    Target: String; Count: Integer;
begin
  Result := False;
  RequireSafePayloadPath(Path);
  Obj := CreateComObject(StringToGuid('{00021401-0000-0000-C000-000000000046}'));
  Persist := IOwnershipPersistFile(Obj);
  Persist.Load(Path, 0);
  Link := IOwnershipShellLinkW(Obj);
  Target := StringOfChar(#0, 32768);
  // Read the stored Unicode target without resolving or updating the link.
  Link.GetPath(Target, Length(Target), 0, 4);
  Count := Pos(#0, Target);
  if Count = 0 then RaiseException('Unterminated shortcut path.');
  SetLength(Target, Count - 1);
  // A valid link retargeted to a remote, missing-volume or unsupported path is
  // foreign to this copy. Reading the link itself must still succeed.
  try
    Result := CompareText(CanonicalInstallPath(Target),
      AddBackslash(Root) + 'bin\{#JobTrackerExecutableName}') = 0;
  except
    Result := False;
  end;
end;

function ShortcutTargetsCopy(const Path, Root: String): Boolean;
begin
  Result := False;
  if (Path = '') or not FileExists(Path) then Exit;
  try
    Result := ReadShortcutTargetsCopy(Path, Root);
  except
    Log('Shortcut could not be verified and is preserved: ' + Path + ': ' + GetExceptionMessage);
  end;
end;

function HasOwnedShortcutName(const Path: String): Boolean;
var Name, Base, Suffix: String; I: Integer;
begin
  Result := False;
  if CompareText(ExtractFileExt(Path), '.lnk') <> 0 then Exit;
  Name := ChangeFileExt(ExtractFileName(Path), '');
  Base := 'JobTracker (' + OwnershipId + ')';
  if CompareText(Name, Base) = 0 then begin
    Result := True;
    Exit;
  end;
  if CompareText(Copy(Name, 1, Length(Base) + 1), Base + '-') <> 0 then Exit;
  Suffix := Copy(Name, Length(Base) + 2, MaxInt);
  if Suffix = '' then Exit;
  for I := 1 to Length(Suffix) do
    if (Suffix[I] < '0') or (Suffix[I] > '9') then Exit;
  Result := True;
end;

function ShortcutInFolder(const Path, Folder: String): Boolean;
begin
  Result := False;
  if not HasOwnedShortcutName(Path) then Exit;
  try
    Result := CompareText(CanonicalInstallPath(ExtractFileDir(Path)),
      CanonicalInstallPath(Folder)) = 0;
  except
    Result := False;
  end;
end;

function MakeOwnedShortcut(const Key, Folder, Existing: String): String;
var Base, Path: String; N: Integer; Reuse: Boolean;
begin
  Reuse := False;
  if Existing <> '' then begin
    if not ShortcutInFolder(Existing, Folder) then
      RaiseException('Recorded shortcut location changed during reconciliation: ' + Existing);
    // A late sharing/access failure on a recorded link must not make us choose
    // a replacement and overwrite the only record of the old owned shortcut.
    Reuse := ReadShortcutTargetsCopy(Existing, OwnershipRoot);
    if not Reuse then Log('Preserving newly retargeted shortcut: ' + Existing);
  end;
  if Reuse then Path := Existing
  else begin
    Base := AddBackslash(Folder) + 'JobTracker (' + OwnershipId + ')';
    Path := Base + '.lnk';
    N := 1;
    while FileExists(Path) or DirExists(Path) do begin
      if ShortcutTargetsCopy(Path, OwnershipRoot) then begin
        Reuse := True;
        Break;
      end;
      Path := Base + '-' + IntToStr(N) + '.lnk';
      N := N + 1;
    end;
  end;
  RequireSafePayloadPath(Path);
  // Record the intended path before creation, so a partial creation or a later
  // metadata failure cannot lose the location needed for repair/uninstall.
  WriteOwnershipValue('Shortcuts', Key, Path, InstallMarker(OwnershipRoot));
  Result := Path;
  if Reuse then Exit;
  if not ForceDirectories(Folder) then RaiseException('Cannot create shortcut folder: ' + Folder);
  try
    CreateShellLink(Path, 'JobTracker - ' + OwnershipRoot,
      AddBackslash(OwnershipRoot) + 'bin\{#JobTrackerExecutableName}', '',
      AddBackslash(OwnershipRoot) + 'bin',
      AddBackslash(OwnershipRoot) + 'bin\{#JobTrackerExecutableName}', 0, SW_SHOWNORMAL);
  except
    RaiseException('Cannot create owned shortcut: ' + Path + #13#10 + GetExceptionMessage);
  end;
end;

procedure ReconcileOwnedShortcut(const Key, Folder: String; const Enabled: Boolean;
  var Existing: String);
begin
  if (Existing <> '') and
     (not Enabled or not ShortcutInFolder(Existing, Folder) or
      not ShortcutTargetsCopy(Existing, OwnershipRoot)) then begin
    // Keep the old record until removal succeeds or the link is established
    // as absent/foreign. A Windows failure must leave it available for retry.
    if not RemoveOwnedShortcut(Existing) then
      RaiseException('Cannot reconcile owned ' + Key + ' shortcut: ' +
        Existing + #13#10 + ProgramFailures);
    WriteOwnershipValue('Shortcuts', Key, '', InstallMarker(OwnershipRoot));
    Existing := '';
  end;
  if Enabled then Existing := MakeOwnedShortcut(Key, Folder, Existing)
  else WriteOwnershipValue('Shortcuts', Key, '', InstallMarker(OwnershipRoot));
end;

procedure FinishInstallOwnership;
var Marker, LogPath: String; I: Integer;
begin
  Marker := InstallMarker(OwnershipRoot);
  for I := 0 to GetArrayLength(OwnedCandidates) - 1 do begin
    RequireSafePayloadPath(AddBackslash(OwnershipRoot) + OwnedCandidates[I].Path);
    if not FileExists(AddBackslash(OwnershipRoot) + OwnedCandidates[I].Path) or
       (CompareText(GetSHA256OfFile(AddBackslash(OwnershipRoot) + OwnedCandidates[I].Path),
         OwnedCandidates[I].Hash) <> 0) then begin
      OwnershipWriteFailed := True;
      RaiseException(CustomMessage('InstallWriteFailed') + #13#10 + OwnedCandidates[I].Path);
    end;
  end;
  ReconcileOwnedShortcut('StartMenu', ExpandConstant('{autoprograms}\JobTracker'),
    True, OwnedStartMenu);
  ReconcileOwnedShortcut('Desktop', ExpandConstant('{autodesktop}'),
    WizardIsTaskSelected('desktopicon'), OwnedDesktop);
  WriteOwnershipValue('Installation', 'Version', '{#JobTrackerReleaseVersion}', Marker);
  WriteOwnershipValue('Installation', 'Uninstaller', ExpandConstant('{uninstallexe}'), Marker);
  LogPath := ChangeFileExt(ExpandConstant('{uninstallexe}'), '.dat');
  WriteOwnershipValue('History', 'Path', OwnershipRoot, Marker);
  WriteOwnershipValue('History', 'Scope', GetOwnershipScope, Marker);
  WriteOwnershipValue('History', 'Log', ExtractFileName(LogPath), Marker);
  WriteOwnershipValue('History', 'SHA256', GetSHA256OfFile(LogPath), Marker);
end;

procedure ValidateLocalUninstaller;
var Actual, Recorded: String;
begin
  Actual := CanonicalInstallPath(ExtractFileDir(ExpandConstant('{uninstallexe}')));
  Recorded := CanonicalInstallPath(ExpandConstant('{app}'));
  if CompareText(Actual, Recorded) <> 0 then RaiseException(CustomMessage('InstallMoved'));
  OwnershipRoot := Actual;
  OwnershipId := InstallationId(Actual);
  OwnershipLedger := AddBackslash(Actual) + 'JobTracker.ownership.ini';
  PinOwnedParents(InstallMarker(Actual), ProgramRootGuards);
  Recorded := GetIniString('Installation', 'Path', Actual, InstallMarker(Actual));
  if CompareText(Actual, Recorded) <> 0 then RaiseException(CustomMessage('InstallMoved'));
  try
    LoadLocalOwnership(Actual);
  except
    ProgramFailures := OwnershipLedger + ': ' + GetExceptionMessage + #13#10;
  end;
  RegisterCurrentPayload;
  OwnedStartMenu := GetIniString('Shortcuts', 'StartMenu', '', InstallMarker(Actual));
  OwnedDesktop := GetIniString('Shortcuts', 'Desktop', '', InstallMarker(Actual));
end;

procedure AddProgramFailure(const Path, Detail: String);
begin
  ProgramFailures := ProgramFailures + Path + ': ' + Detail + #13#10;
  Log('Program removal incomplete: ' + Path + ': ' + Detail);
end;

procedure ReconstructLocalOwnership;
var I, N: Integer; Path: String; Attributes, ErrorCode: Cardinal;
begin
  // Run after the data choice. A locked/unreadable program file cannot prevent
  // confirmed cleanup; uncertainty preserves the file and reports separately.
  for I := 0 to GetArrayLength(OwnedCandidates) - 1 do begin
    Path := AddBackslash(OwnershipRoot) + OwnedCandidates[I].Path;
    if OwnedIndex(OwnedCandidates[I].Path) < 0 then
      try
        Attributes := PayloadGetFileAttributes(Path);
        if Attributes = $FFFFFFFF then begin
          ErrorCode := DLLGetLastError;
          if (ErrorCode <> 2) and (ErrorCode <> 3) then
            AddProgramFailure(Path, SysErrorMessage(ErrorCode));
        end else begin
          RequireSafePayloadPath(Path);
          if CompareText(GetSHA256OfFile(Path), OwnedCandidates[I].Hash) = 0 then begin
            N := GetArrayLength(OwnedFiles);
            SetArrayLength(OwnedFiles, N + 1);
            OwnedFiles[N] := OwnedCandidates[I].Path;
          end else
            AddProgramFailure(Path, 'Ownership unavailable; preserved.');
        end;
      except
        AddProgramFailure(Path, GetExceptionMessage);
      end;
  end;
end;

procedure RemoveOwnedFile(const Path: String);
var ErrorCode, Attributes: Cardinal; Handle: THandle; Delete: Byte; Guards: TDataHandles;
begin
  Handle := 0;
  try
    try
      Attributes := PayloadGetFileAttributes(Path);
      if Attributes = $FFFFFFFF then begin
        ErrorCode := DLLGetLastError;
        if (ErrorCode = 2) or (ErrorCode = 3) then Exit;
        RaiseException(DataWindowsError('Cannot inspect owned path', Path, ErrorCode));
      end;
      PinOwnedParents(Path, Guards);
      Handle := OpenOwnedHandle(Path, False, True);
      Delete := 1;
      if DataSetDisposition(Handle, 4, Delete, 1) = 0 then begin
        ErrorCode := DLLGetLastError;
        AddProgramFailure(Path, IntToStr(ErrorCode) + ' ' + SysErrorMessage(ErrorCode));
      end;
    except
      AddProgramFailure(Path, GetExceptionMessage);
    end;
  finally
    if (Handle <> 0) and (Handle <> THandle(-1)) then PolicyCloseHandle(Handle);
    CloseDataHandles(Guards);
  end;
end;

function RemoveOwnedShortcut(const Path: String): Boolean;
var Guards: TDataHandles; ReadHandle, DeleteHandle: THandle;
    BeforeInfo, DeleteInfo: TDataFileInfo; Delete: Byte; Attributes, ErrorCode: Cardinal;
begin
  Result := True;
  if Path = '' then Exit;
  if not HasOwnedShortcutName(Path) then begin
    Log('Preserving shortcut without this copy''s identity: ' + Path);
    Exit;
  end;
  Result := False;
  ReadHandle := THandle(-1);
  DeleteHandle := THandle(-1);
  try
    try
      Attributes := PayloadGetFileAttributes(Path);
      if Attributes = $FFFFFFFF then begin
        ErrorCode := DLLGetLastError;
        if (ErrorCode = 2) or (ErrorCode = 3) then begin
          Result := True;
          Exit;
        end;
        RaiseException(DataWindowsError('Cannot inspect owned shortcut', Path, ErrorCode));
      end;
      PinOwnedParents(Path, Guards);
      // Deny writes while COM reads the target. The second handle must refer
      // to the same object and deny rename/delete before its disposition.
      ReadHandle := OpenOwnedHandleShared(Path, False, False, 5);
      if not ReadShortcutTargetsCopy(Path, OwnershipRoot) then begin
        Log('Preserving retargeted shortcut: ' + Path);
        Result := True;
        Exit;
      end;
      DeleteHandle := OpenOwnedHandleShared(Path, False, True, 1);
      if (DataFileInformation(ReadHandle, BeforeInfo) = 0) or
         (DataFileInformation(DeleteHandle, DeleteInfo) = 0) then
        RaiseException('Cannot compare shortcut identity.');
      if (BeforeInfo.Volume <> DeleteInfo.Volume) or
         (BeforeInfo.IndexHigh <> DeleteInfo.IndexHigh) or
         (BeforeInfo.IndexLow <> DeleteInfo.IndexLow) then begin
        Log('Preserving replaced shortcut: ' + Path);
        Result := True;
        Exit;
      end;
      Delete := 1;
      if DataSetDisposition(DeleteHandle, 4, Delete, 1) = 0 then
        RaiseException(DataWindowsError('Cannot remove owned shortcut', Path, DLLGetLastError));
      Result := True;
    except
      AddProgramFailure(Path, GetExceptionMessage);
    end;
  finally
    if DeleteHandle <> THandle(-1) then PolicyCloseHandle(DeleteHandle);
    if ReadHandle <> THandle(-1) then PolicyCloseHandle(ReadHandle);
    CloseDataHandles(Guards);
  end;
end;

procedure RemoveLocalProgram;
var I: Integer; Path, Parent: String;
begin
  ReconstructLocalOwnership;
  for I := 0 to GetArrayLength(OwnedFiles) - 1 do
    RemoveOwnedFile(AddBackslash(OwnershipRoot) + OwnedFiles[I]);
  RemoveOwnedShortcut(OwnedStartMenu);
  RemoveOwnedShortcut(OwnedDesktop);
  if ProgramFailures = '' then begin
    RemoveOwnedFile(InstallMarker(OwnershipRoot));
    RemoveOwnedFile(OwnershipLedger);
  end;
  // Only empty directories represented by owned paths, never recursive trees.
  for I := 0 to GetArrayLength(OwnedFiles) - 1 do begin
    Path := ExtractFileDir(AddBackslash(OwnershipRoot) + OwnedFiles[I]);
    while (CompareText(Path, OwnershipRoot) <> 0) and (Length(Path) > Length(OwnershipRoot)) do begin
      try
        RequireSafePayloadPath(Path);
        RemoveDir(Path);
      except
        Log('Preserving program directory: ' + Path);
      end;
      Parent := ExtractFileDir(Path);
      if Parent = Path then Break;
      Path := Parent;
    end;
  end;
end;

function InstallerOptionsAllowed: Boolean;
var I: Integer; Argument: String;
begin
  Result := False;
  for I := 1 to ParamCount do begin
    Argument := Uppercase(ParamStr(I));
    if (Argument = '/CLOSEAPPLICATIONS') or (Argument = '/FORCECLOSEAPPLICATIONS') or
       (Argument = '/RESTARTAPPLICATIONS') or (Pos('/LOADINF=', Argument) = 1) then begin
      Log(CustomMessage('InstallAutomaticClose'));
      if not WizardSilent then
        SuppressibleMsgBox(CustomMessage('InstallAutomaticClose'), mbError, MB_OK, IDOK);
      Exit;
    end;
  end;
  Result := True;
end;
