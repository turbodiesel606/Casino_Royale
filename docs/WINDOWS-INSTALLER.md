# Windows Installer Behavior

JobTracker has one native x64 offline installer:
`Installer/output/JobTracker-Setup-<version>.exe` for Windows 10/11 x64.
It embeds the Release application, Qt/QML/plugins/SQLite, scope-specific MSVC
inputs and runtime notice. Installation needs no Qt/Visual Studio/SDK, runtime
download, GitHub asset or application archive. The candidate remains under the
[publication hold](../packaging/windows/REDISTRIBUTION.md); disconnected
clean-machine acceptance is still pending.

Use the [command guide](../For-Agent/Docs/build.md),
[workflow reference](../For-Agent/Docs/windows-release.md) and
[installer implementation reference](../Installer/README.md).

## Installation Scope And Runtime

| Scope | Default and registration | MSVC runtime |
|---|---|---|
| CurrentUser | `%LocalAppData%/Programs/JobTracker`; HKCU; `JobTracker.CurrentUser` | Ten pinned App-Local x64 Release DLLs; no system prerequisite or admin rights. |
| AllUsers | `%ProgramFiles%/JobTracker`; HKLM; `JobTracker.AllUsers`; UAC | Check system runtime; extract and run the bundled verified official x64 redistributable only if needed. |

`Installer/MsvcRuntime.lock.json` pins version 14.44.35211.0 and exact inputs.
Cancellation/failure stops before application copying. Restart-required results
require reboot and a new Setup run. JobTracker uninstall and later installation
failure do not remove the shared machine runtime.

Fresh destinations are editable. Start Menu shortcuts are created; desktop
shortcuts default to unchecked. English and Russian are available with a
language-selection dialog; system UI language may select Russian, with English
as fallback. The application, Setup and shortcuts share the current temporary icon.

## Manual Updates And Closure

Run a newer offline Setup over the existing same-scope installation. Upgrade
and same-version repair retain the validated directory, stable uninstall
identity and personal data. Downgrade, missing/inconsistent registry/PE/marker
versions, unsafe paths and relocation are rejected.

An opposite-scope copy blocks installation at every version, including equal
versions. Switch scope by manually uninstalling with data preserved, then
reinstalling. Existing dual-scope layouts must remove the other copy first.
AllUsers rejects unavailable profile/registry state without loading offline
hives. Separate AppIds do not create independent data stores.

Close JobTracker manually before install/repair/upgrade/uninstall, and keep it
closed until completion. Setup never force-closes or restarts it. Retry rechecks
the selected file; unattended Setup needs both `/VERYSILENT` and
`/SUPPRESSMSGBOXES`. Unsafe automatic-close/INF overrides are refused.
A late failure can leave partial program-file changes; full rollback is not
guaranteed. The earlier sharing error 32 has no established root-cause fix.

## Personal Data

The usual per-account store is `%LocalAppData%/JobTracker/Data`, including
`database.sqlite` and `Resumes/`, independent of install scope/directory.
Setup never starts JobTracker, opens its database or runs migrations.

Uninstall preserves personal data by default. Only verified non-elevated
CurrentUser removal can offer an EN/RU default-No cleanup prompt after complete
program removal. Explicit Yes permanently deletes the entire verified owned
`%LocalAppData%/JobTracker` tree, including SQLite sidecars and CVs.
There is no command-line data-delete switch.

Silent/unattended, elevated, AllUsers, incomplete, shared, inaccessible or
ambiguous cases preserve data. AllUsers never deletes another profile's data.
Cleanup checks scope/registration, SID ownership, ancestors, reparse points,
hardlinks, access and handles. It is irreversible and nontransactional; a late
failure may leave partial deletion. Unknown manually copied applications cannot
be discovered exhaustively.

## Verification Limits

[Stage 08](../For-Agent/Task-Report/offline-installer/08.md) records native suites,
standalone Qt/QML/SQLite/App-Local smoke and actual owned CurrentUser
install/repair/uninstall with synthetic sentinels. Those tests never launched
JobTracker or inspected real application data.

Interactive EN/RU/Retry/icons, AllUsers/UAC, real prerequisite skip/run/error/
reboot, actual SQLite/CV/settings preservation, graphics and disconnected clean
Windows 10/11 remain separate acceptance work. Use disposable accounts/VMs and
the [checklist](../tests/deployment/Windows-Release-Checklist.md).
