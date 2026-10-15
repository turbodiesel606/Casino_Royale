# Windows Release Workflow

This document describes the implemented Windows workflow. Commands belong only
in [build.md](build.md); dated results and stage selection belong in
[STATUS.md](../Task-Report/windows-release/STATUS.md). The
[accepted requirements](windows-release-requirements.md) remain the product
contract. Developer entry points are [BUILDING](../../docs/BUILDING.md),
[WINDOWS-RELEASE](../../docs/WINDOWS-RELEASE.md) and
[WINDOWS-INSTALLER](../../docs/WINDOWS-INSTALLER.md).

**Publication HOLD:** the stage-14 audit completed with unresolved R1-R7 in
[REDISTRIBUTION.md](../../packaging/windows/REDISTRIBUTION.md). Existing candidates
must not be published. This is a maintainer hold; the packaging scripts can still
produce candidates. A DONE stage, valid hash or passing test is not release or
legal approval. Do not resolve these findings by silently changing dependencies,
source policy, notices or owner terms during a documentation stage.

## Toolchain And Lifecycle

Target Windows 10/11 x64 with C++20, Visual Studio 2022 / MSVC v143, CMake 3.21+
and exactly Qt 6.10.3. Packaging scripts require PowerShell 7.2+; Setup requires
Inno Setup 6.7.1+. The recorded candidate used CMake 4.0.2 and Inno 6.7.1. The
current CRT lock is 14.44.35211.0; a different/newer toolchain requires deliberate
compatibility and lock review. Linux also requires Qt 6.10.3; existing Ninja Debug
presets remain, with Linux execution and packaging outside this Windows evidence.

Shared presets use `QTDIR`; machine-specific SDK overrides belong in
`CMakeUserPresets.json`. That file is already tracked here: ignoring it does not
untrack it, and Git writes still need explicit authorization.

| Step | Owner / input | Result |
|---|---|---|
| Configure/build | Root CMake, matching production preset | Compiled/linked `JobTrackerApp`; no automatic install/archive/Setup. |
| Development runtime | Explicit `JobTrackerApp_dev_runtime` | Qt files beside the development EXE; not a release payload. |
| Release install | CMake Qt QML deployment, fresh absolute prefix | Common application/Qt/QML/SQLite runtime tree. |
| Archive | `Package-Release.ps1`, matching build/staging | Versioned 7z, checksum and per-file manifest under fresh `dist/`. |
| Setup compilation | `Build-Installer.ps1`, verified archive, generated identity, locked CRT | Online or Local Setup plus private build evidence. |
| Verification | Separate backend/standalone test consumers | Logs and retained fixtures; never dependencies of production. |
| Redistribution decision | Exact payload audit and owner decisions | Resolve the hold before any separately authorized publication. |

Run Windows commands in PowerShell from the project root, sequentially, waiting
for final exit codes. Every CMake build requires elevated tool access from its
first attempt. CurrentUser installation tests require a non-elevated Windows
token; host-tool permissions and OS administrator elevation are different.
Keep production/tests in separate trees and preserve existing outputs and failures.

## Qt, QML And SQLite Payload

The Windows Release install deploys `bin/`, `plugins/`, `qml/`, `translations/`
and relative `bin/qt.conf`. Platform and SQLite plugins are loaded dynamically;
QtSql alone is insufficient. QML imports determine further runtime dependencies.
Development/tooling/input plugins and non-SQLite SQL drivers are excluded.
Debug installs remain executable-only. Linux's explicit installation rule remains
separate from normal builds.

`NO_COMPILER_RUNTIME` leaves the common payload free of MSVC CRT DLLs. The
installer applies the scope-specific policy below. The current 148-file allowlist
is [payload-files.txt](../../packaging/windows/payload-files.txt); it contains
Lottie and other dependencies subject to the audit hold. Do not regenerate this
list blindly or call the common archive a standalone clean-machine distribution.
Missing DXC/DXIL deployment warnings still require graphics acceptance.

## Version, Icon And Archive Trust

`project(JobTracker VERSION ...)` in root CMake owns the release version, currently
`0.1.0`. Windows numeric metadata appends `.0`; QML module version `1.0` is an
independent import contract. `JobTrackerApp.exe` remains the binary filename.
Generated `.rc`, CMake and Inno identity files live under
`<build>/packaging/windows/<configuration>/`. Reconfigure/rebuild after a version
or icon change. Replace [JobTracker.ico](../../packaging/windows/JobTracker.ico)
for the final icon; the current original temporary asset has seven sizes from
16 to 256 pixels and supplies executable, Setup, shortcut and uninstall icons.
Publisher is blank and the application/Setup candidates are unsigned.

The archive tool requires production x64 Release inputs, an exact executable
hash match to the build, the reviewed inventory and safe relative layout. It
rejects Debug/test/developer extras, reparse points and unsafe paths. It writes:

- `JobTracker-<version>-windows-x64.7z`
- `JobTracker-<version>-windows-x64.7z.sha256`
- `JobTracker-<version>-windows-x64.7z.manifest.json`

The checksum is lowercase SHA-256, two spaces and the archive basename. The
schema-1 manifest records identity, archive size/hash and ordered per-file
paths/sizes/hashes. Local verification checks the sidecar, extracts a retained
copy and checks every file. Same bytes with the same archive writer were
deterministic; reproducible C++ compilation is not claimed.

The Online URL is fixed at build time:

```text
https://github.com/turbodiesel606/Releases/releases/download/v<version>/JobTracker-<version>-windows-x64.7z
```

For the current identity this is tag `v0.1.0` and asset
`JobTracker-0.1.0-windows-x64.7z`. There is no `latest`, endpoint override or
runtime replacement hash. Setup embeds the locally verified archive digest,
size and file checks. A downloaded checksum/manifest cannot authenticate itself
or replace that trust anchor. Inno performs native HTTPS download, hash checking
and extraction into its private temporary area; explicit validated file entries
reach the destination. EN/RU failures/cancellation allow safe retry or exit.

Online Setup is `JobTracker-<version>-windows-x64-online-setup.exe`. Local mode
embeds the common payload as `JobTracker-<version>-windows-x64-local-setup.exe`
for local integration. Local payload does not mean every AllUsers prerequisite
is offline: the machine CRT can still require Microsoft's download.
Both builds use the same installer policies. Neither build publishes anything.

## Scopes, Runtime And Maintenance

| Scope | Identity / default path | Runtime |
|---|---|---|
| CurrentUser | `JobTracker.CurrentUser`; HKCU; `%LocalAppData%/Programs/JobTracker` | Ten verified App-Local x64 Release CRT DLLs beside the EXE; no system prerequisite/elevation. |
| AllUsers | `JobTracker.AllUsers`; HKLM; `%ProgramFiles%/JobTracker` | Check system x64 CRT; download/verify/run pinned official Microsoft redistributable only when insufficient. |

The lock records exact runtime hashes, size, URL and version; build preparation
validates signatures and selected Visual Studio inputs. Reboot-required results
stop before application installation and require a restart/new Setup run.
The machine runtime is neither rolled back on later failure nor uninstalled
with JobTracker. `licenses/MSVC-NOTICE.txt` is delivered in both scopes; it is
not a complete Qt/third-party notice bundle or a grant of redistribution rights.

Both scopes offer editable new-install paths, scoped Start Menu shortcuts and
desktop opt-in unchecked by default. English and Russian are included; system
UI language can select Russian, and the language dialog permits a choice.
English is the fallback, not a forced selection on Russian Windows.

Upgrade and repair preserve the known path/data and require consistent registry,
PE and installer-marker identity. Downgrades, ambiguous paths, nesting and unsafe
relocation fail closed. Relocation requires uninstall with data preserved, then
reinstall. Opposite scopes may coexist only at exactly equal release versions.
AllUsers examines regular profiles/loaded hives; unloaded or unreadable profile
state blocks it. Setup never loads offline hives or migrates the database.

Both scopes used by one account resolve to the same
`QStandardPaths::AppLocalDataLocation/Data`: normally
`%LocalAppData%/JobTracker/Data`, with `database.sqlite` and `Resumes/`.
Separate AppIds do not isolate data. To upgrade two copies, remove one preserving
data, upgrade the remaining copy, then reinstall the other at the same version.
Equal versions do not prove concurrent application sessions are safe. Manually
copied/legacy binaries and already migrated data remain outside the installer
version gate.

Setup and Uninstall require manual closure of the exact installed EXE. They
never terminate or restart a process. Rechecks cover later boundaries and copies;
unsafe automatic-close/INF overrides are refused. Unattended Setup requires both
`/VERYSILENT` and `/SUPPRESSMSGBOXES`. The gate is not a transaction-wide file lock;
late failures do not guarantee full restoration of already overwritten files.

Uninstall removes registered program files/shortcuts and preserves personal data
by default. Only verified non-elevated CurrentUser removal offers an EN/RU
default-No prompt after complete program removal. Yes permanently deletes the
entire owned `%LocalAppData%/JobTracker` folder, not only the database. Silent,
elevated/AllUsers, shared, incomplete or unverifiable cases preserve data. Path,
SID, registration, reparse/hardlink and handle checks guard cleanup; no CLI delete
switch exists. Cleanup is irreversible and nontransactional; unknown copies
cannot be exhaustively discovered. Detailed implementation is in the
[packaging reference](../../packaging/windows/README.md).

## Verification And Release Hold

Use the [canonical suites](build.md#windows-release-integration-verification),
[testing policy](testing.md) and
[acceptance matrix](../../tests/deployment/Windows-Release-Checklist.md).
Stage 13 recorded 18/18 backend tests, staging 1/1, App-Local smoke, 19 archive,
54 policy twice, 21 manual-close, 19 data, 25 Online and 15 runtime checks, plus
Russian Unicode-path lifecycle checks. These are dated local evidence, not new
runs implied by this guide.

Still unresolved: application startup/graphics and interactive EN/RU dialogs,
AllUsers/UAC/dual-scope/multiple sessions, actual missing-runtime installation and
reboot, historical-release/database compatibility and clean Windows 10/11.
The stage-13 public asset returned 404; safe rejection is not successful public
installation. Stage-12 sharing error 32 did not recur in two unchanged stage-13
runs, but its cause/fix remains unknown. Retain the failures and manual-close gate.

The stage-14 R1-R7 hold covers three GPL-only/commercial Lottie files, missing
delivered notices/source/replacement material and unresolved SBOM correspondence,
Windows ICU/Mesa/D3D/Inno provenance, recoverable private QML, and owner/Visual
Studio entitlement. Use the file-level audit, not an older Qt audit or a generic
license list. Internal resource/audit/build metadata must remain private.

No CI/CD, Linux packaging, automatic updates or signing is implemented by this
workflow. Future signing must occur before the hashes it changes: application
before staging/archive generation, Setup before its final published digest.
Owner identity, certificates and authorization remain separate work. Do not
promise SmartScreen reputation. Publication is a separately authorized manual
action after the hold and acceptance issues are resolved.
