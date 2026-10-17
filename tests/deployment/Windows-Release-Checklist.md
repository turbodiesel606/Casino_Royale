# Windows release acceptance checklist

Use this checklist with the single `Installer/output/JobTracker-Setup-<version>.exe`
and private `installer-build.json` hashes recorded in the selected offline-stage
report. Results belong in `For-Agent/Task-Report/offline-installer/` and the
persisted `Tasks/Offline-Installer/00_ACCEPTANCE_MATRIX.md`. This file is reusable
procedure, not a claim of success. Commands are owned by `For-Agent/Docs/build.md`.

## Evidence rules

- Record Windows edition/build, x64 architecture, account/elevation, installed
  MSVC runtime, artifact hashes, language, destination, exit code and log path.
- Use `PASS`, `FAIL` or `NOT_RUN` per case. Name the evidence level: source check,
  test harness, real installer integration, interactive UI, or clean VM.
- Keep each build/test process sequential. Use fresh retained `build/`/`dist/`
  paths. Never delete earlier evidence or replace a failed result with a retry.
- Automated CurrentUser suites refuse existing registrations and own only their
  test installation. Verify/remove that fixture before another real suite.
- Never disable manual-close checks, security software, or data safeguards to
  make a case pass. Retain the exact native error and affected path on failure.
- Use disposable accounts/VMs for application startup and actual data. Changing
  environment variables is not proof that Qt's Windows known-folder storage has
  been redirected. Do not launch the app against the developer's real profile.

## Automated local pass

1. Configure/build Release production and the separate test-enabled tree; run
   the full Release CTest preset. Record test isolation and the selected Qt kit.
2. Install to a fresh absolute staging prefix. Run `Test-OfflinePayload.ps1`
   against the direct stage; retain all negative-case copies and results.
3. Compile one offline Setup with authentic, pinned local MSVC inputs. Run
   `Test-OfflineInstaller.ps1` against that EXE and its private build JSON;
   verify native x64 identity, exact embedding and absence of transport paths.
4. Configure/build/run the independent deployment CTest against staging. Run its
   probe against the direct stage plus the installer's App-Local CRT. Confirm
   confined QML/plugin paths and loaded runtime paths with sanitized PATH.
5. Run MSVC, installation-policy, manual-close and personal-data suites. Default
   runs use synthetic fixtures. Real CurrentUser cases are a separate opt-in,
   without OS elevation; they require no existing real JobTracker registration.
6. Verify actual installation/app startup with networking disconnected on the
   disposable clean hosts below. There is no release URL or archive prerequisite.
7. Record fixture cleanup and check that production source, QML, storage,
   migrations and unrelated work were preserved.

The standalone probe exercises Qt/QML/platform/image/SQLite dependencies. It does
not execute `JobTrackerApp`, verify its window, or certify a clean machine.

## Installation and lifecycle matrix

Repeat applicable cases on clean Windows 10 x64 and Windows 11 x64 without Qt or
Visual Studio, with networking disconnected, using snapshots and synthetic
SQLite/CV data. Keep database, CV and settings hashes before/after each action.

| Case | Expected behavior and evidence |
|---|---|
| Fresh CurrentUser, EN | No UAC; account default directory; Start Menu shortcut; desktop unchecked. Launch, exit normally, reopen and verify synthetic data. |
| Fresh AllUsers, EN | Explicit UAC; machine default directory; machine registration and shortcuts; launch as a standard user. |
| Custom destination, EN/RU | Editable valid path, including spaces/non-ASCII; shortcuts, metadata and uninstall path resolve to the selected installation. |
| RU UI and desktop opt-in | Read every page/error/prompt; opt-in creates the scoped desktop link. Silent localized tests do not verify visual UI. |
| Opposite scope, including equal version | Reject before mutation; preserve both program/data trees. Scope switching requires manual uninstall with data preserved, then reinstall. |
| Upgrade CurrentUser alone | Real supported older release upgrades in place, retains path/data, and starts successfully. Synthetic version metadata tests cover policy only. |
| Upgrade AllUsers alone | Same checks with machine scope and standard-user data; preserve other accounts. |
| Upgrade with opposite scope present | Reject every opposite-scope version before mutation. Verify registration, program bytes and shared data unchanged. Remove the other scope with data preserved first. |
| Same-version repair, each scope | Keep a valid executable identity, damage only an owned ancillary fixture file, reinstall; verify every payload hash, path, registration and data. |
| Downgrade, each scope | Reject before mutation; preserve payload, registrations, SQLite and CV hashes. |
| Existing dual-scope conflict | Reject maintenance until manual removal of the conflicting scope. Removing one registered copy must preserve the other's files and all shared data. |
| Default/silent uninstall | Preserve SQLite/CVs/settings and unknown program files. Verify native logs and actual data hashes. |
| Explicit optional cleanup | Interactive verified non-elevated CurrentUser only: default No, No/window-close preserve; Yes after complete uninstall deletes only synthetic owned data. |
| Unsafe cleanup contexts | Elevated/AllUsers/alternate-account UAC, remaining registration, partial uninstall, ACL denial, reparse/hardlink/locked paths: preserve data and explain where applicable. |

## Failure matrix

| Case | Expected behavior and evidence |
|---|---|
| No network connection | Both scopes install from the single EXE without downloading; app launches from the installed location on the clean VM. |
| Missing/mismatched stage or runtime input | Build fails before usable Setup output; retain diagnostic and unchanged input hashes. |
| Debug/dev/test/source artifact or wrong PE identity | Direct-stage validation rejects; embedded QML resources remain allowed. |
| Corrupt embedded prerequisite | Native extraction/integrity check rejects without executing the machine prerequisite. |
| Traversal inventory / junction destination | Reject unsafe local paths; outside sentinel and existing installation remain unchanged. |
| CurrentUser with no machine CRT | Load verified App-Local CRT; no system redistributable execution/UAC. A dev-host probe is not this clean-VM case. |
| AllUsers runtime present | Validate sufficient installed x64 runtime; skip prerequisite. |
| AllUsers runtime missing/old | Extract bundled official signed package, verify integrity, install and recheck with networking disconnected. |
| Runtime failure/cancellation/reboot | Exercise real prerequisite failure and cancellation on snapshots; preserve app state; correctly handle success/reboot exit codes. Harness return-code assertions alone are insufficient. |
| Running selected app during update/uninstall | Ask for manual closure, Retry/Cancel; never terminate it. Include another Windows session. Verify safe cancellation and retained files. |
| Unrelated same-name process | Do not block solely by basename. Verify exact installation identity. |
| App starts after preflight / late file lock | Recheck before mutation, stop native copy path, retain logs and protected files. Do not claim full restoration of already overwritten upgrade files. |
| Repeated Windows sharing error | Record timestamp/path and exact failing operation; obtain a narrow file-I/O trace to identify access/share mode and process before attributing the lock. |

## Mixed-version SQLite safety

The same account resolves one `JobTracker/Data` tree regardless of install scope.
Registration isolation does not isolate SQLite/CVs. The current installer rejects
every opposite-scope version and unverifiable profile state. Unloaded/unreadable
profiles block AllUsers changes conservatively. No automatic scope migration or
compatibility with the retired online installer is required.

Run the migration suite for supported forward upgrades, rollback and rejection
of newer schema versions, then separately test genuine historical application
binaries on a disposable data snapshot. Synthetic PE/registry version fixtures
do not prove database compatibility. Unregistered copies are not exhaustively
discoverable, and equal version strings do not prove identical binary content.
Do not infer multi-process CV/filesystem safety from SQLite transactions alone.

## Completion record

The stage report must include the concrete matrix, exact commands/hashes, failed
attempts, unresolved defects, and unavailable environments. External VM/UI/Linux
checks may remain `NOT_RUN` when explicitly recorded; a genuine unresolved local
implementation/acceptance failure must remain a blocker. Update `STATUS.md` only
after the report, and do not start the next numbered stage automatically.
