# Windows Offline Release Workflow

This document describes the implemented workflow.
[build.md](build.md) owns executable commands;
[requirements](../../Tasks/Offline-Installer/00_REQUIREMENTS.md) are binding;
[STATUS](../../Tasks/Offline-Installer/STATUS.md) selects exactly one stage.
Reports under `For-Agent/Task-Report/offline-installer/` are dated evidence,
not automatic validation of current artifacts.

**Publication HOLD:** unresolved
[license issues](../Task-Report/offline-installer/LICENSE_ISSUES.md) and the
[redistribution hold](../../packaging/windows/REDISTRIBUTION.md) remain.
Stage completion and technical checks do not approve distribution.

## Toolchain And Operation Boundaries

Windows 10/11 x64 uses C++20, Visual Studio 2022/MSVC v143, CMake 3.21+ and exactly
Qt 6.10.3. Installer scripts need PowerShell 7.2+ and Inno Setup 7+; 7.1.0 is the
recorded native-x64 compiler. Inno 6's I386 launcher is rejected even when its
payload/install mode is x64. Linux has Ninja Debug/Release production/test
presets using Qt 6.10.3; Linux execution is still NOT RUN on this Windows host.

Shared presets use `QTDIR`; local overrides belong in `CMakeUserPresets.json`.
That file is already tracked: ignoring it does not untrack it. Preserve local
values; Git writes require separate authorization.

| Operation | Owner / result |
|---|---|
| Configure/build | Root CMake and matching production preset; compile/link only. |
| Development runtime | Explicit `JobTrackerApp_dev_runtime`; development-machine output only. |
| Release staging | Explicit CMake install to a fresh absolute prefix; Qt/QML/platform/SQLite payload. |
| Offline Setup | `Installer/Build-Installer.ps1` validates/snapshots the direct stage and local runtime, then compiles one native x64 EXE. |
| Backend and deployment tests | Separate consumers; never production dependencies. |
| Release decision | Exact-artifact technical evidence plus separate owner/license resolution. |

Run Windows commands sequentially in PowerShell from the repository root.
Every CMake build needs elevated tool access from its first attempt and a final
exit code before proceeding. CurrentUser installer tests require a non-elevated
Windows token; host-tool permission is different from OS elevation.
Do not delete or overwrite prior outputs, failures or personal files.

## Payload, Identity And Output

CMake installation supplies `bin/`, `plugins/`, `qml/`, `translations/` and relative
`bin/qt.conf`. The 148-file [reviewed inventory](../../Installer/payload-files.txt)
includes `qwindows.dll` and `qsqlite.dll`; development/tooling/input plugins and
non-SQLite SQL drivers are excluded. Windows Debug install is executable-only.
`NO_COMPILER_RUNTIME` leaves CRT delivery to the installer.

Root `project(JobTracker VERSION ...)` owns app/Setup versions; Windows numeric
metadata appends `.0`. The binary remains `JobTrackerApp.exe`, independent of the
QML import version. Generated RC/CMake/Inno identity is private under
`<build>/Installer/<configuration>/`. `Installer/JobTracker.ico` is the temporary
seven-size icon; publisher is blank.

The wrapper checks production Release identity/hash, exact file inventory,
x64 Release PEs, Qt versions, relative metadata, streams and reparse paths.
It snapshots verified bytes and generates explicit hashed `[Files]` entries.
The only end-user artifact is
`Installer/output/JobTracker-Setup-<version>.exe`; that directory is ignored.
Existing same-name output is rejected. Private snapshots/runtime/includes/logs
and `installer-build.json` remain in `<build>/Installer/work/<id>/`.

There is no application archive, GitHub release/download, remote checksum,
Online/Local mode or installer-time network requirement. The required inputs
are local. Stage 10 removed the retired archive/transport implementation and
old working tasks/reports. Necessary evidence and open findings remain in the
[cleanup handoff](../Task-Report/offline-installer/10.md) and license register;
Git preserves the older history.

Embedded QML resources are explicitly allowed. Loose C++ headers/sources,
CMake files, tests, PDBs and internal documents must not ship. The current payload
still contains Lottie and external DLLs covered by the hold; missing DXC/DXIL
warnings remain a graphics acceptance question.

## Runtime, Scopes And Maintenance

| Scope | Registration / default directory | Runtime |
|---|---|---|
| CurrentUser | `JobTracker.CurrentUser`, HKCU, `%LocalAppData%/Programs/JobTracker` | Ten signed/hash-pinned App-Local x64 Release DLLs. |
| AllUsers | `JobTracker.AllUsers`, HKLM, `%ProgramFiles%/JobTracker` | Read system-runtime identity; use the bundled authentic Microsoft package only if insufficient. |

The lock pins 14.44.35211.0. Build preparation validates local signatures, hashes,
sizes, versions and CRT architecture against the selected kit. Missing inputs
fail with actionable paths/version; no download or silent repinning occurs.
Actual prerequisite failure/cancellation stops app copying; reboot-required
results require restart and rerunning Setup. The shared runtime is not removed
with JobTracker or rolled back after later failure. `licenses/MSVC-NOTICE.txt`
is not a complete Qt/third-party notice bundle or proof of entitlement.

Fresh locations are editable. Both scopes have EN/RU UI, Start Menu shortcuts
and an unchecked desktop option. Same-scope repair/upgrade retains validated
location and uninstall identity. Downgrade, inconsistent/missing registry/PE/
marker versions, unsafe paths and relocation fail closed.

Opposite-scope installations reject every version, including equal versions.
Switch scope by manual uninstall preserving data, then reinstall. AllUsers
checks available profiles/hives and blocks unreadable state without loading
offline hives. Setup does not migrate or open the database. Both scopes used
by one account resolve to the same normal `%LocalAppData%/JobTracker/Data`.

Manual-close gates require closure of the exact installed executable, recheck
later boundaries, and never terminate/restart the app. Unattended Setup requires
`/VERYSILENT` plus `/SUPPRESSMSGBOXES`; unsafe close/INF overrides are refused.
The gate is not a transaction-wide file lock or a full rollback guarantee.

## Uninstall And Evidence

Default uninstall preserves personal data. Only verified non-elevated CurrentUser
removal can offer EN/RU default-No consent after complete program removal.
Explicit Yes deletes the owned `%LocalAppData%/JobTracker` tree. Silent, elevated/
AllUsers, shared, incomplete or unverifiable cases preserve data. AllUsers cleanup
does not enumerate profiles. Ancestor/ownership/link/handle checks protect the
operation; cleanup is irreversible and nontransactional.

The [implementation reference](../../Installer/README.md) maps these contracts.
Use [testing.md](testing.md), the
[acceptance checklist](../../tests/deployment/Windows-Release-Checklist.md) and
[acceptance matrix](../../Tasks/Offline-Installer/00_ACCEPTANCE_MATRIX.md).
Static source/artifact checks, native harnesses, actual owned installation,
real app/data, GUI and clean-machine results are distinct.

Stage 08 records 34 payload, 14 artifact, 12 runtime, 50 policy, 16 manual-close,
62 data checks, owned CurrentUser install/repair/uninstall, standalone CTest 1/1
and App-Local smoke. These are prior runs, not new documentation-stage results.
Earlier failed attempts and the unresolved sharing error 32 remain evidence.

Actual app/data, AllUsers/UAC/prerequisite/reboot, interactive cleanup,
disconnected clean Windows 10/11 and Linux acceptance remain NOT RUN.
Use disposable accounts/VMs, never personal data. Optional
[Setup signing](build.md#optional-installer-signing) records the final digest
after signature verification; unsigned builds remain supported and no
certificate/secret is stored by the workflow.
