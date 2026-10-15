# Windows Installer Behavior

Both Online and Local Setup are implemented, unsigned candidates under the
[publication hold](../packaging/windows/REDISTRIBUTION.md). Use
[canonical build/test commands](../For-Agent/Docs/build.md) and the maintained
[release workflow](../For-Agent/Docs/windows-release.md). The
[packaging reference](../packaging/windows/README.md) documents detailed native
contracts; this guide summarizes what a maintainer or tester should expect.

## Payload And Installation Scope

Online is the default build and downloads one fixed versioned archive with an
embedded SHA-256. Local embeds that same payload for integration work. These
payload modes are independent of the installation scope: each offers both scopes.
Local payload does not eliminate a possible AllUsers MSVC prerequisite download.

| Scope | Defaults and registration | MSVC runtime |
|---|---|---|
| Current user | `%LocalAppData%/Programs/JobTracker`, HKCU, `JobTracker.CurrentUser` | Ten pinned App-Local x64 Release DLLs; no system installer/elevation. |
| All users | `%ProgramFiles%/JobTracker`, HKLM, `JobTracker.AllUsers`, explicit elevation | Validate system x64 runtime; download/verify/run the pinned official Microsoft package when needed. |

Runtime identity/hashes come from `MsvcRuntime.lock.json`, currently version
14.44.35211.0. Reboot-required results stop before app files change; restart and
rerun Setup. The shared runtime remains installed after JobTracker uninstall or
later installation failure.

New destinations are editable. Start Menu shortcuts are always created; desktop
shortcuts are unchecked by default. Each scope has distinct localized names.
English and Russian are available with a language-selection dialog; Russian can
be selected from the system UI language, with English as fallback. Executable,
Setup, shortcuts and uninstall use the shared temporary icon.

## Upgrades And Closure

Same-version repair and upgrades retain the validated previous directory and
personal data. Downgrades, inconsistent registration/EXE/marker identity, unsafe
paths and changing an existing scope's directory are rejected. Relocate by
uninstalling while preserving data, then reinstalling.

Opposite scopes can coexist only at exactly equal release versions. AllUsers
checks other profiles and blocks when their registration state cannot be read.
To upgrade two copies, uninstall one while preserving data, upgrade the other,
then install the matching new version in the first scope. Independent uninstall
records do not create independent data stores or prove concurrent app use safe.

Close the selected JobTracker copy manually before maintenance. Setup/Uninstall
never force-close or restart it. Interactive Retry rechecks the file; silent
runs must be rerun after closure. Unattended Setup needs `/VERYSILENT` and
`/SUPPRESSMSGBOXES`; unsafe automatic-close and INF replay overrides are refused.
Keep the copy closed until maintenance finishes. Late cancellation can briefly
show progress; partial upgrades do not promise restoration of every old file.

## Personal Data

Both copies used by one account share the normal Windows
`%LocalAppData%/JobTracker/Data` store, including `database.sqlite` and `Resumes/`.
Setup does not launch JobTracker, open SQLite or run migrations.

Uninstall preserves personal data by default. Only a verified, non-elevated
CurrentUser uninstall can offer cleanup after complete program removal. The
EN/RU prompt defaults to No and shows the exact folder. Yes permanently removes
the entire owned `%LocalAppData%/JobTracker` folder, including CVs and SQLite
sidecars. There is no command-line deletion switch. Silent, elevated/AllUsers,
shared, incomplete, inaccessible or unverifiable cases preserve data.

Cleanup validates scope, registration, SID ownership and paths/handles, rejecting
links and unsafe or locked trees. It is irreversible and nontransactional;
a late failure can leave a partially deleted tree. Unregistered copies cannot
be discovered exhaustively; keep data if another copy may use it.

## Evidence Limits

Stage 13 passed local native suites and real non-elevated CurrentUser lifecycle
checks, including a Russian Unicode path. It did not launch the application.
The earlier sharing error 32 did not recur, but no root-cause fix is established.
The public archive returned 404; safe rejection does not prove public installation.

Interactive EN/RU prompts/Retry/icons, AllUsers/UAC, simultaneous scopes/sessions,
actual missing-runtime/reboot paths, historical upgrades, graphics and clean
Windows 10/11 remain acceptance work in a disposable account/VM. Follow the
[matrix](../tests/deployment/Windows-Release-Checklist.md) and record new evidence
separately from prior reports. Do not use actual personal data as a fixture.
