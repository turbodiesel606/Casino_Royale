// Local copy identity and path policy. No JobTracker registry discovery.
[CustomMessages]
english.InstallStateInvalid=The local installation path or metadata is unsafe or inaccessible. Choose a regular local folder and check its permissions.
russian.InstallStateInvalid=Локальный путь или сведения об установке небезопасны либо недоступны. Выберите обычную локальную папку и проверьте права доступа.
english.InstallDowngrade=This folder contains a reliably identified newer JobTracker version. Choose a separate folder for the older version.
russian.InstallDowngrade=В этой папке обнаружена более новая версия JobTracker. Для старой версии выберите отдельную папку.
english.InstallPathConflict=Installation folders must not contain one another or overlap the personal-data directory.
russian.InstallPathConflict=Папки установок не должны находиться одна внутри другой или пересекаться с папкой личных данных.
english.InstallFolderNotEmpty=This nonempty folder cannot be identified as JobTracker. Choose an empty folder. Existing unknown files will not be adopted.
russian.InstallFolderNotEmpty=Эта непустая папка не распознана как установка JobTracker. Выберите пустую папку. Неизвестные файлы не будут присвоены установщику.
english.InstallBusy=Another JobTracker installer or uninstaller is active, or its installation mutex is inaccessible.
russian.InstallBusy=Другой установщик или деинсталлятор JobTracker уже работает либо его мьютекс недоступен.
english.InstallSharedDataInfo=Copies in separate folders share this Windows user's existing personal data. Setup does not open, reset, restore or migrate that data. Windows may reject replacement of locked or inaccessible files.
russian.InstallSharedDataInfo=Копии в разных папках используют общие личные данные пользователя Windows. Установщик не открывает, не сбрасывает, не восстанавливает и не переносит эти данные. Windows может отказать в замене занятых или недоступных файлов.

[Code]
var
  InstallPolicyLock: THandle;

function PolicyCreateFile(const Name: String; Access, Sharing: Cardinal;
  Security: THandle; Disposition, Flags: Cardinal; Template: THandle): THandle;
  external 'CreateFileW@kernel32.dll stdcall';
function PolicyFinalPath(Handle: THandle; Buffer: String; Size, Flags: Cardinal): Cardinal;
  external 'GetFinalPathNameByHandleW@kernel32.dll stdcall';
function PolicyCloseHandle(Handle: THandle): Boolean;
  external 'CloseHandle@kernel32.dll stdcall';
function PolicyCreateMutex(Security: THandle; Owned: Boolean; const Name: String): THandle;
  external 'CreateMutexW@kernel32.dll stdcall';

procedure AcquireInstallPolicyLock;
var
  ErrorCode: Cardinal;
begin
  if InstallPolicyLock <> 0 then Exit;
  InstallPolicyLock := PolicyCreateMutex(0, False, 'Global\JobTracker.InstallationState');
  ErrorCode := DLLGetLastError;
  if InstallPolicyLock = 0 then RaiseException(CustomMessage('InstallBusy'));
  if ErrorCode = 183 then begin
    PolicyCloseHandle(InstallPolicyLock);
    InstallPolicyLock := 0;
    RaiseException(CustomMessage('InstallBusy'));
  end;
end;

function PolicyVersionSize(const FileName: String; var Ignored: Cardinal): Cardinal;
  external 'GetFileVersionInfoSizeW@version.dll stdcall';
function PolicyVersionRead(const FileName: String; Ignored, Size: Cardinal; Buffer: String): Boolean;
  external 'GetFileVersionInfoW@version.dll stdcall';
function PolicyVersionQuery(Buffer: String; const Name: String; var Value: THandle;
  var Size: Cardinal): Boolean;
  external 'VerQueryValueW@version.dll stdcall';
procedure PolicyCopyText(Destination: String; Source: THandle; Size: THandle);
  external 'RtlMoveMemory@kernel32.dll stdcall';

function LocalProductExecutable(const Path: String; var Version: Int64): Boolean;
var
  Size, Ignored, Count: Cardinal;
  Buffer, Product, Original: String;
  Value: THandle;
begin
  Result := False;
  RequireSafePayloadPath(Path);
  Size := PolicyVersionSize(Path, Ignored);
  if (Size = 0) or (Size > 1048576) then Exit;
  Buffer := StringOfChar(#0, (Size + 1) div 2);
  if not PolicyVersionRead(Path, 0, Size, Buffer) then Exit;
  if not PolicyVersionQuery(Buffer, '\StringFileInfo\040904B0\ProductName', Value, Count) or
     (Count < 2) or (Count > 128) then Exit;
  Product := StringOfChar(#0, Count - 1);
  PolicyCopyText(Product, Value, (Count - 1) * 2);
  if not PolicyVersionQuery(Buffer, '\StringFileInfo\040904B0\OriginalFilename', Value, Count) or
     (Count < 2) or (Count > 128) then Exit;
  Original := StringOfChar(#0, Count - 1);
  PolicyCopyText(Original, Value, (Count - 1) * 2);
  Result := (Product = 'JobTracker') and (Original = '{#JobTrackerExecutableName}') and
    GetPackedVersion(Path, Version);
end;

procedure ReleaseInstallPolicyLock;
begin
  if InstallPolicyLock <> 0 then PolicyCloseHandle(InstallPolicyLock);
  InstallPolicyLock := 0;
end;

// StrToVersion accepts truncated and extra fields. Require the exact CMake
// three-component release format before using unsigned packed comparisons.
function ParseInstallVersion(const Text: String; var Version: Int64): Boolean;
var
  I, Components, Digits, Value: Integer;
begin
  Result := False;
  if (Length(Text) < 5) or (Length(Text) > 17) then Exit;
  Components := 0;
  Digits := 0;
  Value := 0;
  for I := 1 to Length(Text) + 1 do begin
    if (I > Length(Text)) or (Copy(Text, I, 1) = '.') then begin
      if Digits = 0 then Exit;
      Components := Components + 1;
      Digits := 0;
      Value := 0;
    end else begin
      if (Text[I] < '0') or (Text[I] > '9') then Exit;
      if (Digits > 0) and (Value = 0) then Exit;
      Value := Value * 10 + Ord(Text[I]) - Ord('0');
      Digits := Digits + 1;
      if (Digits > 5) or (Value > 65535) then Exit;
    end;
  end;
  if Components = 3 then Result := StrToVersion(Text, Version);
end;

function CompareInstallVersion(const First, Second: Int64): Integer;
begin
  // Inno 6.7.1 compares signed Int64 values. Windows version words are unsigned.
  if (First < 0) and (Second >= 0) then Result := 1
  else if (First >= 0) and (Second < 0) then Result := -1
  else Result := ComparePackedVersion(First, Second);
end;

// Resolve the nearest existing ancestor through a handle: short names and
// SUBST drive aliases then compare by their real path. Reject redirection,
// remote/device/relative paths and ambiguous Windows filename spelling.
function CanonicalInstallPath(Path: String): String;
var
  Existing, Parent, Suffix, Part, Buffer: String;
  I, Start: Integer;
  Handle: THandle;
  Count: Cardinal;
begin
  if (Length(Path) < 3) or (Path[2] <> ':') or (Path[3] <> '\') or
     (Pos(':', Copy(Path, 3, MaxInt)) <> 0) or (Pos('/', Path) <> 0) then
    RaiseException(CustomMessage('InstallStateInvalid'));
  Path := RemoveBackslashUnlessRoot(Path);
  Start := 4;
  if Length(Path) > 3 then begin
    for I := 4 to Length(Path) + 1 do begin
      if (I > Length(Path)) or (Copy(Path, I, 1) = '\') then begin
        Part := Copy(Path, Start, I - Start);
        if (Part = '') or (Part = '.') or (Part = '..') or
           (Trim(Part) <> Part) or (Copy(Part, Length(Part), 1) = '.') then
          RaiseException(CustomMessage('InstallStateInvalid'));
        Start := I + 1;
      end;
    end;
  end;
  RequireSafePayloadPath(Path);
  Existing := Path;
  Suffix := '';
  while not DirExists(Existing) and not FileExists(Existing) do begin
    Parent := ExtractFileDir(Existing);
    if (Parent = '') or (Parent = Existing) then
      RaiseException(CustomMessage('InstallStateInvalid'));
    Suffix := '\' + ExtractFileName(Existing) + Suffix;
    Existing := Parent;
  end;
  Handle := PolicyCreateFile(Existing, $80, 7, 0, 3, $02000000, 0);
  if Handle = THandle(-1) then RaiseException(CustomMessage('InstallStateInvalid'));
  try
    Buffer := StringOfChar(#0, 32768);
    Count := PolicyFinalPath(Handle, Buffer, Length(Buffer), 0);
    if (Count = 0) or (Count >= Cardinal(Length(Buffer))) then
      RaiseException(CustomMessage('InstallStateInvalid'));
    SetLength(Buffer, Count);
    if (Copy(Buffer, 1, 4) <> '\\?\') or (Copy(Buffer, 6, 2) <> ':\') then
      RaiseException(CustomMessage('InstallStateInvalid'));
    Result := RemoveBackslashUnlessRoot(Copy(Buffer, 5, MaxInt));
    if Suffix <> '' then
      Result := AddBackslash(Result) + Copy(Suffix, 2, MaxInt);
  finally
    PolicyCloseHandle(Handle);
  end;
end;

function PathsOverlap(const First, Second: String): Boolean;
begin
  Result := (CompareText(First, Second) = 0) or
    (Pos(Lowercase(AddBackslash(First)), Lowercase(AddBackslash(Second))) = 1) or
    (Pos(Lowercase(AddBackslash(Second)), Lowercase(AddBackslash(First))) = 1);
end;

function InstallMarker(const Path: String): String;
begin
  Result := AddBackslash(Path) + 'JobTracker.install.ini';
end;

function HasLocalInstallIdentity(const Path: String): Boolean;
var Marker: String; Version: Int64;
begin
  Marker := InstallMarker(Path);
  RequireSafePayloadPath(Marker);
  Result := ((GetIniString('Installation', 'Product', '', Marker) = 'JobTracker') and
    (GetIniString('Installation', 'Format', '', Marker) = '2')) or
    LocalProductExecutable(AddBackslash(Path) + 'bin\{#JobTrackerExecutableName}', Version);
  // An interrupted install can retain its versioned relative ownership ledger
  // even when the final marker or executable has not yet been written.
  if not Result then
    Result := GetIniString('Identity', 'Product', '',
      AddBackslash(Path) + 'JobTracker.ownership.ini') = 'JobTracker-2';
end;

type
  TPolicyFindInfo = record
    Attributes: Cardinal;
    CreationTime, AccessTime, WriteTime: TFileTime;
    SizeHigh, SizeLow, Reserved0, Reserved1: Cardinal;
    Name: array[0..259] of Char;
    AlternateName: array[0..13] of Char;
  end;
function PolicyFindFirst(const Pattern: String; var Info: TPolicyFindInfo): THandle;
  external 'FindFirstFileW@kernel32.dll stdcall';
function PolicyFindNext(Handle: THandle; var Info: TPolicyFindInfo): Boolean;
  external 'FindNextFileW@kernel32.dll stdcall';
function PolicyFindClose(Handle: THandle): Boolean;
  external 'FindClose@kernel32.dll stdcall';

function PolicyFindName(const Info: TPolicyFindInfo): String;
var I: Integer;
begin
  Result := '';
  for I := 0 to 259 do begin
    if Info.Name[I] = #0 then Exit;
    Result := Result + Info.Name[I];
  end;
end;

procedure RequirePolicyEnumerationEnd(const ErrorCode: Cardinal);
begin
  if ErrorCode <> 18 then
    RaiseException(CustomMessage('InstallStateInvalid') + #13#10 +
      'Directory enumeration: ' + IntToStr(ErrorCode) + ' ' + SysErrorMessage(ErrorCode));
end;

function HasLocalInstallBoundary(const Path: String): Boolean;
var Find: TPolicyFindInfo; Search: THandle; ErrorCode: Cardinal;
begin
  Result := FileExists(InstallMarker(Path)) or
    FileExists(AddBackslash(Path) + 'JobTracker.ownership.ini') or
    FileExists(AddBackslash(Path) + 'bin\{#JobTrackerExecutableName}');
  if Result or not DirExists(Path) then Exit;
  Search := PolicyFindFirst(AddBackslash(Path) + 'unins*.dat', Find);
  if Search <> THandle(-1) then begin
    PolicyFindClose(Search);
    Result := True;
  end else begin
    ErrorCode := DLLGetLastError;
    if ErrorCode <> 2 then RequirePolicyEnumerationEnd(ErrorCode);
  end;
end;

procedure RequireEmptyInstallDirectory(const Destination: String);
var Find: TPolicyFindInfo; Search: THandle; Name: String; More: Boolean; ErrorCode: Cardinal;
begin
  if FileExists(Destination) then RaiseException(CustomMessage('InstallFolderNotEmpty'));
  if not DirExists(Destination) then Exit;
  Search := PolicyFindFirst(AddBackslash(Destination) + '*', Find);
  if Search = THandle(-1) then begin
    ErrorCode := DLLGetLastError;
    if ErrorCode <> 2 then RequirePolicyEnumerationEnd(ErrorCode);
    Exit;
  end;
  try
    repeat
      Name := PolicyFindName(Find);
      if (Name <> '.') and (Name <> '..') then
        RaiseException(CustomMessage('InstallFolderNotEmpty'));
      More := PolicyFindNext(Search, Find);
      if not More then RequirePolicyEnumerationEnd(DLLGetLastError);
    until not More;
  finally
    PolicyFindClose(Search);
  end;
end;

procedure CheckNestedInstallations(const Path: String; Depth: Integer; var Count: Integer);
var Find: TPolicyFindInfo; Search: THandle; Child, Name: String;
    More: Boolean; ErrorCode: Cardinal;
begin
  if not DirExists(Path) then Exit;
  if Depth > 64 then RaiseException(CustomMessage('InstallStateInvalid'));
  Search := PolicyFindFirst(AddBackslash(Path) + '*', Find);
  if Search = THandle(-1) then begin
    ErrorCode := DLLGetLastError;
    if ErrorCode <> 2 then RequirePolicyEnumerationEnd(ErrorCode);
    Exit;
  end;
  try
    repeat
      Name := PolicyFindName(Find);
      if (Name <> '.') and (Name <> '..') and ((Find.Attributes and $10) <> 0) then begin
        Child := AddBackslash(Path) + Name;
        RequireSafePayloadPath(Child);
        Count := Count + 1;
        if Count > 20000 then RaiseException(CustomMessage('InstallStateInvalid'));
        if HasLocalInstallBoundary(Child) then
          RaiseException(CustomMessage('InstallPathConflict') + #13#10 + Child);
        CheckNestedInstallations(Child, Depth + 1, Count);
      end;
      More := PolicyFindNext(Search, Find);
      if not More then RequirePolicyEnumerationEnd(DLLGetLastError);
    until not More;
  finally
    PolicyFindClose(Search);
  end;
end;

procedure CheckInstallationPolicy(const AllUsers: Boolean; const SelectedPath, Release: String);
var
  Destination, Parent, Marker: String;
  Incoming, Existing: Int64;
  Count: Integer;
begin
  if not ParseInstallVersion(Release, Incoming) then
    RaiseException(CustomMessage('InstallStateInvalid'));
  Destination := CanonicalInstallPath(SelectedPath);
  if Length(Destination) <= 3 then RaiseException(CustomMessage('InstallFolderNotEmpty'));
  if not HasLocalInstallIdentity(Destination) then RequireEmptyInstallDirectory(Destination);
  Parent := ExtractFileDir(Destination);
  while Length(Parent) > 3 do begin
    if HasLocalInstallBoundary(Parent) then
      RaiseException(CustomMessage('InstallPathConflict'));
    Parent := ExtractFileDir(Parent);
  end;
  Count := 0;
  CheckNestedInstallations(Destination, 0, Count);
  Marker := InstallMarker(Destination);
  if (GetIniString('Installation', 'Product', '', Marker) = 'JobTracker') and
     ParseInstallVersion(GetIniString('Installation', 'Version', '', Marker), Existing) then
    if CompareInstallVersion(Incoming, Existing) < 0 then
      RaiseException(CustomMessage('InstallDowngrade'));
  if LocalProductExecutable(AddBackslash(Destination) + 'bin\{#JobTrackerExecutableName}', Existing) then
    if CompareInstallVersion(Incoming, Existing) < 0 then
      RaiseException(CustomMessage('InstallDowngrade'));
  Log('Local installation policy passed: ' + Destination + '; incoming version ' + Release);
end;
