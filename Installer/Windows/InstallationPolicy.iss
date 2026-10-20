// Installer-owned identity, version and scope policy. Never opens app data.
[CustomMessages]
english.InstallStateInvalid=The installation path or existing JobTracker metadata cannot be verified. Choose a regular local folder. If the existing executable or its metadata is missing or damaged, uninstall that copy while preserving personal data before reinstalling.
russian.InstallStateInvalid=Не удалось проверить путь установки или данные существующей установки JobTracker. Выберите обычную локальную папку. Если программа или данные её установки отсутствуют либо повреждены, удалите эту копию с сохранением личных данных перед повторной установкой.
english.InstallDowngrade=A newer JobTracker version is already installed in this mode. Downgrades are blocked to protect your data.
russian.InstallDowngrade=В этом режиме уже установлена более новая версия JobTracker. Установка старой версии заблокирована для защиты данных.
english.InstallScopeConflict=JobTracker is already installed in the other installation mode. To change modes, first uninstall the other copy manually while preserving personal data, then run Setup again. If both modes are already installed, remove the other copy before repairing or upgrading this one. Setup does not switch modes automatically.
russian.InstallScopeConflict=JobTracker уже установлен в другом режиме. Чтобы сменить режим, сначала вручную удалите другую копию с сохранением личных данных, затем снова запустите установщик. Если уже установлены обе копии, удалите другую копию перед восстановлением или обновлением этой. Установщик не меняет режим автоматически.
english.InstallPathConflict=Installation folders must be separate and must not contain one another. Choose a different folder.
russian.InstallPathConflict=Папки установок должны быть раздельными и не должны находиться одна внутри другой. Выберите другую папку.
english.InstallMove=To change the folder of an existing installation, first uninstall that copy while preserving personal data, then install into the new folder. This prevents an old executable or uninstaller from remaining active.
russian.InstallMove=Чтобы изменить папку существующей установки, сначала удалите эту копию с сохранением личных данных, затем установите её в новую папку. Это предотвращает сохранение старой программы или деинсталлятора.
english.InstallFolderNotEmpty=A new installation requires an empty folder outside any existing JobTracker installation. Existing files will not be adopted or deleted.
russian.InstallFolderNotEmpty=Для новой установки нужна пустая папка вне существующей установки JobTracker. Существующие файлы не будут присвоены установщиком или удалены.
english.InstallProfileUnavailable=All-users installation cannot safely check another Windows profile. Sign in to that profile and keep its session open, then retry, or use current-user installation. No offline registry hive will be loaded or changed.
russian.InstallProfileUnavailable=Не удалось безопасно проверить другой профиль Windows для установки всем пользователям. Войдите в этот профиль и оставьте сеанс открытым, затем повторите попытку, либо установите только для текущего пользователя. Реестр неактивного профиля не будет загружен или изменён.
english.InstallBusy=Another JobTracker installer or uninstaller is active, or its installation lock is inaccessible. Finish it before retrying.
russian.InstallBusy=Другой установщик или деинсталлятор JobTracker уже работает либо блокировка установки недоступна. Завершите его работу и повторите попытку.
english.InstallShortcutConflict=A JobTracker shortcut belongs to another target or cannot be verified. Move that shortcut yourself before retrying.
russian.InstallShortcutConflict=Ярлык JobTracker указывает на другую программу либо не может быть проверен. Переместите этот ярлык вручную перед повторной попыткой.
english.InstallSharedDataInfo=To change installation modes, first uninstall the existing copy manually while preserving personal data, then reinstall in the desired mode. Setup preserves personal data and never opens or migrates the database.
russian.InstallSharedDataInfo=Чтобы сменить режим установки, сначала вручную удалите существующую копию с сохранением личных данных, затем установите программу в нужном режиме. Установщик сохраняет личные данные, не открывает и не обновляет базу данных.

[Code]
const
  InstallUninstallKey = 'Software\Microsoft\Windows\CurrentVersion\Uninstall\JobTracker.';
  InstallProfileKey = 'Software\Microsoft\Windows NT\CurrentVersion\ProfileList';
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
function PolicyRegOpen(Root: THandle; const Key: String; Options, Access: Cardinal;
  var Handle: THandle): Longint;
  external 'RegOpenKeyExW@advapi32.dll stdcall';
function PolicyRegClose(Handle: THandle): Longint;
  external 'RegCloseKey@advapi32.dll stdcall';

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
  for I := 4 to Length(Path) + 1 do begin
    if (I > Length(Path)) or (Copy(Path, I, 1) = '\') then begin
      Part := Copy(Path, Start, I - Start);
      if (Part = '') or (Part = '.') or (Part = '..') or
         (Trim(Part) <> Part) or (Copy(Part, Length(Part), 1) = '.') then
        RaiseException(CustomMessage('InstallStateInvalid'));
      Start := I + 1;
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
    Result := RemoveBackslashUnlessRoot(Copy(Buffer, 5, MaxInt)) + Suffix;
  finally
    PolicyCloseHandle(Handle);
  end;
end;

// Native open distinguishes an absent key from access denied. Treat unreadable
// registry state as a conflict, not as proof that no installation exists.
function InstallKeyExists(const Root: Integer; const Key: String): Boolean;
var
  NativeRoot: Integer;
  Handle: THandle;
  Access: Cardinal;
  Status: Longint;
begin
  Access := $20019;
  if (Root = HKLM64) or (Root = HKCU64) or (Root = HKU64) then
    Access := Access or $100
  else
    Access := Access or $200;
  if (Root = HKLM64) or (Root = HKLM32) then NativeRoot := HKLM
  else if (Root = HKCU64) or (Root = HKCU32) then NativeRoot := HKCU
  else NativeRoot := HKU;
  // Native HKEY outputs are pointer-sized. Predefined roots require sign extension.
  Status := PolicyRegOpen(THandle(Int64(NativeRoot)), Key, 0, Access, Handle);
  if Status = 0 then PolicyRegClose(Handle)
  else if (Status <> 2) and (Status <> 3) then
    RaiseException(CustomMessage('InstallStateInvalid') + #13#10 + Key);
  Result := Status = 0;
end;

function ReadInstallRecord(const Root: Integer; const Key, Scope: String;
  var Path: String; var Version: Int64): Boolean;
var
  Text, InnoPath, Marker, Exe: String;
  FileVersion: Int64;
begin
  Result := InstallKeyExists(Root, Key);
  if not Result then Exit;
  if not RegQueryStringValue(Root, Key, 'DisplayVersion', Text) or
     not ParseInstallVersion(Text, Version) or
     not RegQueryStringValue(Root, Key, 'InstallLocation', Path) or
     not RegQueryStringValue(Root, Key, 'Inno Setup: App Path', InnoPath) then
    RaiseException(CustomMessage('InstallStateInvalid'));
  Path := CanonicalInstallPath(Path);
  if CompareText(Path, CanonicalInstallPath(InnoPath)) <> 0 then
    RaiseException(CustomMessage('InstallStateInvalid'));
  Marker := AddBackslash(Path) + 'JobTracker.install.ini';
  Exe := AddBackslash(Path) + 'bin\{#JobTrackerExecutableName}';
  RequireSafePayloadPath(Marker);
  RequireSafePayloadPath(Exe);
  if (GetIniString('Installation', 'Scope', '', Marker) <> Scope) or
     not GetPackedVersion(Exe, FileVersion) then
    RaiseException(CustomMessage('InstallStateInvalid'));
  if CompareInstallVersion(Version, FileVersion) <> 0 then
    RaiseException(CustomMessage('InstallStateInvalid'));
  // Offline installations always write Version; missing metadata is not repairable identity.
  Text := GetIniString('Installation', 'Version', '', Marker);
  if not ParseInstallVersion(Text, FileVersion) then
    RaiseException(CustomMessage('InstallStateInvalid'));
  if CompareInstallVersion(Version, FileVersion) <> 0 then
    RaiseException(CustomMessage('InstallStateInvalid'));
end;

procedure CheckInstallRecord(const Root: Integer; const Key, Scope, Destination: String;
  const Incoming: Int64; const SameScope: Boolean; var Found: Boolean);
var
  Path: String;
  Version: Int64;
begin
  if not ReadInstallRecord(Root, Key, Scope, Path, Version) then Exit;
  Found := True;
  if SameScope then begin
    if CompareInstallVersion(Incoming, Version) < 0 then
      RaiseException(CustomMessage('InstallDowngrade'));
    if CompareText(Destination, Path) <> 0 then
      RaiseException(CustomMessage('InstallMove'));
  end else begin
    RaiseException(CustomMessage('InstallScopeConflict') + #13#10 + Path);
  end;
end;

function IsRegularInstallProfile(const Sid: String): Boolean;
begin
  Result := (Pos('S-', Sid) = 1) and (Pos('_', Sid) = 0) and
    (Sid <> 'S-1-5-18') and (Sid <> 'S-1-5-19') and (Sid <> 'S-1-5-20');
end;

procedure CheckProfileInstall(const Sid, Destination: String; const Incoming: Int64);
var
  Found: Boolean;
begin
  if not IsRegularInstallProfile(Sid) then Exit;
  if not InstallKeyExists(HKU64, Sid) then
    RaiseException(CustomMessage('InstallProfileUnavailable') + #13#10 + Sid);
  Found := False;
  CheckInstallRecord(HKU64, Sid + '\' + InstallUninstallKey + 'CurrentUser_is1',
    'CurrentUser', Destination, Incoming, False, Found);
  CheckInstallRecord(HKU32, Sid + '\' + InstallUninstallKey + 'CurrentUser_is1',
    'CurrentUser', Destination, Incoming, False, Found);
end;

procedure CheckAllUserProfiles(const Destination: String; const Incoming: Int64);
var
  Profiles, Loaded: TArrayOfString;
  I: Integer;
begin
  if not RegGetSubkeyNames(HKLM64, InstallProfileKey, Profiles) or
     not RegGetSubkeyNames(HKU64, '', Loaded) then
    RaiseException(CustomMessage('InstallProfileUnavailable'));
  for I := 0 to GetArrayLength(Profiles) - 1 do
    CheckProfileInstall(Profiles[I], Destination, Incoming);
  // Include loaded profiles not yet represented by ProfileList (e.g. temporary).
  for I := 0 to GetArrayLength(Loaded) - 1 do
    CheckProfileInstall(Loaded[I], Destination, Incoming);
end;

procedure RequireEmptyInstallDirectory(const Destination: String);
var
  Find: TFindRec;
begin
  if FileExists(Destination) then RaiseException(CustomMessage('InstallFolderNotEmpty'));
  if not DirExists(Destination) then Exit;
  if not FindFirst(AddBackslash(Destination) + '*', Find) then
    RaiseException(CustomMessage('InstallStateInvalid'));
  try
    repeat
      if (Find.Name <> '.') and (Find.Name <> '..') then
        RaiseException(CustomMessage('InstallFolderNotEmpty'));
    until not FindNext(Find);
  finally
    FindClose(Find);
  end;
end;

procedure CheckInstallationPolicy(const AllUsers: Boolean; const SelectedPath, Release: String);
var
  Destination, Parent, Scope: String;
  Incoming: Int64;
  Found, OtherFound: Boolean;
begin
  if not ParseInstallVersion(Release, Incoming) then
    RaiseException(CustomMessage('InstallStateInvalid'));
  Destination := CanonicalInstallPath(SelectedPath);
  if Length(Destination) <= 3 then RaiseException(CustomMessage('InstallFolderNotEmpty'));
  Found := False;
  OtherFound := False;
  if AllUsers then begin
    Scope := 'AllUsers';
    CheckInstallRecord(HKLM64, InstallUninstallKey + Scope + '_is1', Scope, Destination, Incoming, True, Found);
    CheckInstallRecord(HKLM32, InstallUninstallKey + Scope + '_is1', Scope, Destination, Incoming, True, Found);
    CheckAllUserProfiles(Destination, Incoming);
  end else begin
    Scope := 'CurrentUser';
    CheckInstallRecord(HKCU64, InstallUninstallKey + Scope + '_is1', Scope, Destination, Incoming, True, Found);
    CheckInstallRecord(HKCU32, InstallUninstallKey + Scope + '_is1', Scope, Destination, Incoming, True, Found);
    CheckInstallRecord(HKLM64, InstallUninstallKey + 'AllUsers_is1', 'AllUsers', Destination, Incoming, False, OtherFound);
    CheckInstallRecord(HKLM32, InstallUninstallKey + 'AllUsers_is1', 'AllUsers', Destination, Incoming, False, OtherFound);
  end;
  if not Found then RequireEmptyInstallDirectory(Destination);
  // Also reject nesting in an unregistered/older installation.
  Parent := ExtractFileDir(Destination);
  while Length(Parent) > 3 do begin
    if FileExists(AddBackslash(Parent) + 'JobTracker.install.ini') or
       FileExists(AddBackslash(Parent) + 'bin\{#JobTrackerExecutableName}') then
      RaiseException(CustomMessage('InstallPathConflict'));
    Parent := ExtractFileDir(Parent);
  end;
  Log('Installation policy passed: ' + Scope + '; version ' + Release + '; ' + Destination);
end;

function PolicyShortcutName(const AllUsers, Russian: Boolean): String;
begin
  if Russian then begin
    if AllUsers then Result := 'JobTracker (все пользователи)'
    else Result := 'JobTracker (текущий пользователь)';
  end else begin
    if AllUsers then Result := 'JobTracker (all users)'
    else Result := 'JobTracker (current user)';
  end;
end;

// Only inspect/delete the two known localized leaf names in this mode's shell
// folders, and only when their actual target is this installation's executable.
procedure CheckInstallShortcuts(const AllUsers, Cleanup: Boolean; const Destination: String);
var
  I, J: Integer;
  Folder, LinkPath, Target, ActiveName: String;
  Shell, Link: Variant;
begin
  ActiveName := PolicyShortcutName(AllUsers, ActiveLanguage = 'russian');
  for I := 0 to 1 do begin
    if I = 0 then Folder := ExpandConstant('{autoprograms}\JobTracker')
    else Folder := ExpandConstant('{autodesktop}');
    for J := 0 to 1 do begin
      LinkPath := AddBackslash(Folder) + PolicyShortcutName(AllUsers, J = 1) + '.lnk';
      RequireSafePayloadPath(LinkPath);
      if FileExists(LinkPath) then begin
        Shell := CreateOleObject('WScript.Shell');
        Link := Shell.CreateShortcut(LinkPath);
        Target := Link.TargetPath;
        if CompareText(CanonicalInstallPath(Target),
          CanonicalInstallPath(AddBackslash(Destination) + 'bin\{#JobTrackerExecutableName}')) <> 0 then
          RaiseException(CustomMessage('InstallShortcutConflict'));
        if Cleanup and ((PolicyShortcutName(AllUsers, J = 1) <> ActiveName) or
           ((I = 1) and not WizardIsTaskSelected('desktopicon'))) then
          if not DeleteFile(LinkPath) then Log('Could not remove obsolete owned shortcut: ' + LinkPath);
      end;
    end;
  end;
end;
