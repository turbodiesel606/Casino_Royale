// Path-specific manual-close gate. Requires OnlinePayload and InstallationPolicy.
// Never enumerates, signals, terminates or restarts a process.
[CustomMessages]
english.ManualCloseBusy=JobTracker or another process is using this installation file:%n%1%n%nSave your work and close this copy of JobTracker manually, then choose Retry. If it is open in another Windows session, ask that user to close it. Cancel stops this operation. Do not reopen this copy until Setup or Uninstall has finished.
russian.ManualCloseBusy=JobTracker или другой процесс использует файл этой установки:%n%1%n%nСохраните работу и закройте эту копию JobTracker вручную, затем нажмите «Повторить». Если она открыта в другом сеансе Windows, попросите пользователя закрыть её. «Отмена» останавливает операцию. Не запускайте эту копию до завершения установки или удаления.
english.ManualCloseUnknown=Cannot safely check this installation file:%n%1%n%nWindows error: %2. Close this copy of JobTracker manually and check access to its folder, then choose Retry, or Cancel to stop. No process will be closed automatically.
russian.ManualCloseUnknown=Не удалось безопасно проверить файл установки:%n%1%n%nОшибка Windows: %2. Закройте эту копию JobTracker вручную и проверьте доступ к её папке, затем нажмите «Повторить» или «Отмена». Процессы не будут закрыты автоматически.
english.ManualCloseCancelled=Operation cancelled: the installation files could not be confirmed available. Close this copy of JobTracker manually before trying again. Personal data is preserved.
russian.ManualCloseCancelled=Операция отменена: доступность файлов установки не подтверждена. Закройте эту копию JobTracker вручную перед повторной попыткой. Личные данные сохраняются.
english.ManualCloseOptions=Automatic application closing/restarting, /NOCANCEL and /LOADINF are not supported by this installer. Remove those options and run Setup again. Close JobTracker manually when requested.
russian.ManualCloseOptions=Этот установщик не поддерживает автоматическое закрытие/перезапуск приложений, /NOCANCEL и /LOADINF. Уберите эти параметры и запустите установщик снова. При запросе закройте JobTracker вручную.

[Code]
var
  ManualCloseInstalling, ManualCloseCancelRequested: Boolean;
  ManualCloseCancelAcknowledged: Boolean;

// An OPEN_EXISTING write-access probe does not write or truncate any bytes.
// Windows refuses it for a mapped executable image, including another session.
// Sharing all access avoids treating ordinary readers as running applications.
// A file lock is evidence of use, not proof of a particular PID/user; other
// access failures fail closed. There is deliberately no filename-only match.
function InstallationFileBusyMessage(const Path: String): String;
var
  Handle: THandle;
  ErrorCode: Cardinal;
begin
  Result := '';
  RequireSafePayloadPath(Path);
  Handle := PolicyCreateFile(Path, $40000000, 7, 0, 3, $80, 0);
  if Handle <> THandle(-1) then begin
    PolicyCloseHandle(Handle);
    Exit;
  end;
  ErrorCode := DLLGetLastError;
  if (ErrorCode = 2) or (ErrorCode = 3) then Exit;
  Log(Format('Manual-close check blocked: %s (Windows error %d)', [Path, ErrorCode]));
  if (ErrorCode = 32) or (ErrorCode = 33) or (ErrorCode = 1224) then
    Result := FmtMessage(CustomMessage('ManualCloseBusy'), [Path])
  else
    Result := FmtMessage(CustomMessage('ManualCloseUnknown'), [Path, IntToStr(ErrorCode)]);
end;

procedure RequireIdleInstallationFile(const Path: String);
var
  Message: String;
begin
  repeat
    Message := InstallationFileBusyMessage(Path);
    if Message = '' then Exit;
    Log(Message);
    // Silent operations skip our Retry/Cancel dialog. Setup can still display
    // its native preparation-error summary; use /SUPPRESSMSGBOXES unattended.
    if IsUninstaller then begin
      if UninstallSilent then RaiseException(CustomMessage('ManualCloseCancelled'));
    end else if WizardSilent then
      RaiseException(CustomMessage('ManualCloseCancelled'));
    if SuppressibleMsgBox(Message, mbError, MB_RETRYCANCEL, IDCANCEL) <> IDRETRY then
      RaiseException(CustomMessage('ManualCloseCancelled'));
  until False;
end;

procedure RequireClosedJobTracker;
begin
  RequireIdleInstallationFile(ExpandConstant('{app}\bin\{#JobTrackerExecutableName}'));
end;

<event('CancelButtonClick')>
procedure ManualCloseCancel(CurPageID: Integer; var Cancel, Confirm: Boolean);
begin
  if ManualCloseCancelRequested then begin
    Cancel := True;
    Confirm := False;
    ManualCloseCancelAcknowledged := True;
    Log('Manual-close native cancellation acknowledged.');
  end;
end;

// Inno catches exceptions in BeforeInstall AND Check. Return False to prevent
// this copy, latch all later copies off, and request native cancellation so
// ProcessEvents enters rollback before icons/INI/registry are committed.
// Inno 6.7.1's cancellation handler requires a visible/enabled wizard/button,
// including for /VERYSILENT. Restore UI immediately; never close an app process.
function CanInstallWithManualClose(const Path: String): Boolean;
var
  WasVisible, WasEnabled, ButtonVisible, ButtonEnabled: Boolean;
begin
  Result := not ManualCloseCancelRequested;
  if not Result or not ManualCloseInstalling then Exit;
  try
    RequireClosedJobTracker;
    RequireIdleInstallationFile(Path);
  except
    Log(GetExceptionMessage);
    Result := False;
    ManualCloseCancelRequested := True;
    WasVisible := WizardForm.Visible;
    WasEnabled := WizardForm.Enabled;
    ButtonVisible := WizardForm.CancelButton.Visible;
    ButtonEnabled := WizardForm.CancelButton.Enabled;
    try
      WizardForm.Enabled := True;
      WizardForm.CancelButton.Visible := True;
      WizardForm.CancelButton.Enabled := True;
      WizardForm.Show;
      WizardForm.CancelButton.OnClick(WizardForm.CancelButton);
      if not ManualCloseCancelAcknowledged then
        RaiseException('Manual-close native cancellation was not acknowledged.');
    finally
      WizardForm.Visible := WasVisible;
      WizardForm.Enabled := WasEnabled;
      WizardForm.CancelButton.Visible := ButtonVisible;
      WizardForm.CancelButton.Enabled := ButtonEnabled;
    end;
  end;
end;

function ManualCloseOptionsAllowed: Boolean;
var
  I: Integer;
  Argument: String;
begin
  Result := False;
  for I := 1 to ParamCount do begin
    Argument := Uppercase(ParamStr(I));
    // /LOADINF can set the same internal flags before InitializeSetup. Reject
    // it rather than rereading mutable input and trusting a second snapshot.
    if (Argument = '/CLOSEAPPLICATIONS') or
       (Argument = '/FORCECLOSEAPPLICATIONS') or
       (Argument = '/RESTARTAPPLICATIONS') or
       (Argument = '/NOCANCEL') or
       (Pos('/LOADINF=', Argument) = 1) then begin
      Log(CustomMessage('ManualCloseOptions'));
      if not WizardSilent then
        SuppressibleMsgBox(CustomMessage('ManualCloseOptions'), mbError, MB_OK, IDOK);
      Exit;
    end;
  end;
  Result := True;
end;

// These events also run in the standalone test consumer. Abort is required:
// ordinary exceptions in some Inno notification events are logged and ignored.
<event('CurStepChanged')>
procedure ManualCloseBeforeInstall(CurStep: TSetupStep);
begin
  if CurStep <> ssInstall then Exit;
  try
    RequireClosedJobTracker;
    ManualCloseInstalling := True;
    Log('Manual-close final install check passed.');
  except
    Log(GetExceptionMessage);
    Abort;
  end;
end;

<event('CurUninstallStepChanged')>
procedure ManualCloseBeforeUninstall(CurStep: TUninstallStep);
begin
  if (CurStep <> usAppMutexCheck) and (CurStep <> usUninstall) then Exit;
  try
    RequireClosedJobTracker;
    Log('Manual-close final uninstall check passed.');
  except
    Log(GetExceptionMessage);
    Abort;
  end;
end;
