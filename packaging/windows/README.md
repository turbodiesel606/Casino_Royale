# Windows packaging implementation

Start with the maintained [release workflow](../../For-Agent/Docs/windows-release.md)
and [canonical command catalog](../../For-Agent/Docs/build.md). Developer entry
points are [release preparation](../../docs/WINDOWS-RELEASE.md) and
[installer behavior](../../docs/WINDOWS-INSTALLER.md). This file documents the
implementation contracts; stage numbers below identify their origin, not pending
implementation. Dated results belong in the
[status index](../../For-Agent/Task-Report/windows-release/STATUS.md).

**Release hold (stage 14):** the stage-13 candidates are not approved for
publication. The [redistribution checklist](REDISTRIBUTION.md) records GPL-only
Lottie binaries, missing legal/source materials, binary-provenance gaps and
recoverable private QML. Passing builds, archive hashes and installer tests do
not clear these findings. Internal build/audit evidence must not be published.

`JobTracker.ico` is an original, temporary briefcase/checkmark asset created for
this project. Replace that file with the final multi-resolution ICO and rebuild;
the executable and generated installer input share this one asset path. It has
32-bit images at 16, 24, 32, 48, 64, 128 and 256 pixels. No third-party artwork,
font, publisher identity or signing claim is included.

`project(JobTracker VERSION ...)` in the root CMake file owns the release version.
Windows numeric versions append a zero revision; string versions retain the
three-component project version. The QML module's `VERSION 1.0` is its import
contract, not a release version. Debug resources carry `VS_FF_DEBUG`.

Windows configuration generates these files under
`<build>/packaging/windows/<configuration>/`:

- `JobTracker.rc`: executable icon and VERSIONINFO, compiled into JobTrackerApp.
- `JobTrackerIdentity.cmake`: version, executable filename, icon path and empty
  publisher for packaging consumers.
- `JobTrackerIdentity.iss`: Inno Setup preprocessor definitions consumed by
  `JobTracker.iss`.

The installer includes the generated ISS file and uses its definitions
for `AppName`, `AppVersion`, `VersionInfoVersion`, `AppPublisher`, `SetupIconFile`,
both shortcuts' `IconFilename`/`IconIndex`, and `UninstallDisplayIcon`. Publisher
and legal ownership are intentionally empty until the owner supplies them.
Shortcut and uninstall hooks reference icon index 0 of the installed executable;
they require no separate installed ICO. The install layout remains
`{app}\bin\JobTrackerApp.exe`.

These are identity inputs only: they do not perform packaging or installation.
Generated metadata and the source ICO need not be
installed with the application. Regenerate in the current checkout before using
the absolute build-time icon path. Canonical checks are in
`For-Agent/Docs/build.md`; stage evidence is in the stage-05 report.

## Release payload archive

`Package-Release.ps1` is an explicit Windows/PowerShell 7.2+ workflow. It consumes
the generated Release identity, checks it against the current root project
version, checks the production x64 build cache, and verifies that the staged
executable is byte-identical to that build's Release executable. It does not
reconfigure, compile or install; prepare a current build and clean staging first.

`payload-files.txt` is the reviewed, exact relative file inventory of the current
Qt 6.10.3 common payload. Packaging fails on missing or extra files, including
development output, test helpers and secrets with unexpected names. All PE
files must be x64 and lack the Debug flag; Qt binary versions must match the
root SDK pin. Junctions/symlinks, alternate streams, absolute paths in `qmldir`
and nonrelative `qt.conf` prefixes are rejected. No automated inventory can
establish that arbitrary content inside an approved filename is safe; use only
trusted build/SDK inputs and review the final dependency set in stage 14.

Review this inventory when runtime deployment changes; do not regenerate it
blindly from a potentially contaminated directory. The common payload excludes
MSVC runtime files deliberately: stage 09 supplies them through mode-specific
installer entries, described below. The archive alone is not a self-contained
clean-machine distribution. Future license additions require inventory review.

The tool creates `JobTracker-<version>-windows-x64.7z` below ignored `dist/`.
It uses CMake's built-in 7zip writer, ordinal path order and fixed UTC timestamps
on a private snapshot. The root has `bin/`, `plugins/`, `qml/`, `translations/`
directly, with no machine path or enclosing version directory. Identical input
bytes with the same CMake archive implementation produce identical archives;
this does not promise reproducible C++ compilation or identical compression
across CMake/libarchive versions. Verified with CMake 4.0.2 on this host.

The choice of `.7z` matches Inno Setup's native non-password-protected 7z
extraction. Stage 08 verifies that runtime path separately from archive creation
using the real Inno decoder. References:
[CMake archive CLI](https://cmake.org/cmake/help/v3.21/manual/cmake.1.html#run-a-command-line-tool)
and [Inno Setup archive extraction](https://jrsoftware.org/ishelp/topic_setup_archiveextraction.htm).

## Installer build handoff

Three sibling release assets are produced:

- `JobTracker-<version>-windows-x64.7z`: exact common application/Qt payload.
- `JobTracker-<version>-windows-x64.7z.sha256`: lowercase SHA-256, two spaces,
  archive basename, LF terminator (sha256sum-compatible).
- `JobTracker-<version>-windows-x64.7z.manifest.json`: UTF-8 schema version 1.

Manifest fields are `schemaVersion`, `product`, `version`, `platform`,
`configuration`, `archive` (basename only), `archiveRoot` (`.`), `executable`
(`bin/JobTrackerApp.exe`), `size` (archive bytes), `sha256` (exact archive), and
`files` (ordinal path order, each with `path`, `size`, `sha256`). No local paths
or wall-clock creation time enter these three assets. The manifest is written
only after archive extraction/layout and all file hashes pass. Failed runs and
private snapshots remain for diagnosis; consumers must require the manifest
and invoke verification successfully, never use a leftover archive alone.

Verify mode requires an explicit expected version from the installer build's
generated identity; it validates identity, inventory, archive size/digest,
checksum sidecar and every extracted byte. It refuses missing/mismatched assets.
It is a **local trusted build verification tool**, not a hostile-download
sandbox. The manifest is not signed and cannot authenticate itself. The Online
Installer embeds the verified digest/version at build time and never trusts a
downloaded replacement checksum or manifest as its trust anchor.

Use a new output directory for every packaging attempt; existing directories
are refused. `work/` and `verify-*` contain retained private verification copies
and must not be published. No GitHub upload, signing, automatic update or
personal-data access is performed. Exact commands live in `For-Agent/Docs/build.md`.

## Local-payload installer foundation (stage 07)

`Build-Installer.ps1 -Mode Local` requires Windows, PowerShell 7.2+, CMake and an explicitly
selected Inno Setup 6.7.1+ compiler. It consumes the generated Release identity
and its matching release manifest. The existing verifier's optional `-PassThru` returns
`Manifest` and `PayloadDirectory` only after all verification succeeds; without
that option its original console output is unchanged. The installer wrapper
checks the current project version and selected Release executable hash, then
embeds the verified extracted files through `JobTracker.iss`.

Outputs are a versioned `JobTracker-<version>-windows-x64-local-setup.exe`,
`compile.log`, and local `installer-build.json` evidence under a new `dist/`
directory. The JSON includes source archive/installer/compiler/script hashes and
the retained verified extraction path. It contains local paths and is not a
public release manifest. Inno pads version-resource strings; the wrapper trims
that padding before checking product and version. Normal CMake builds do not
invoke Inno Setup or tests.

The one installer offers these independent, stable identities:

| Mode | AppId / uninstall-key suffix | Default destination | Registration and shortcuts |
|---|---|---|---|
| Current user | `JobTracker.CurrentUser` / `JobTracker.CurrentUser_is1` | `%LocalAppData%\Programs\JobTracker` | HKCU; current-user Start Menu/desktop |
| All users | `JobTracker.AllUsers` / `JobTracker.AllUsers_is1` | `%ProgramFiles%\JobTracker` | HKLM; common Start Menu/desktop |

`PrivilegesRequired=lowest` with the built-in override dialog defaults to current
user; all-users mode requests administrator rights. `UsePreviousPrivileges=no`
keeps mode selection available with the code-based AppId. Native previous-path
lookup uses the selected identity. `DisableDirPage=no` keeps the path editable.
These choices follow Inno's [privilege overrides](https://jrsoftware.org/ishelp/topic_setup_privilegesrequiredoverridesallowed.htm),
[AppId](https://jrsoftware.org/ishelp/topic_setup_appid.htm),
[previous-privilege rules](https://jrsoftware.org/ishelp/topic_setup_usepreviousprivileges.htm)
and [mode-specific directory/registry behavior](https://jrsoftware.org/ishelp/topic_admininstallmode.htm).

Both identities use 64-bit install mode on Windows 10/11 x64. Start Menu shortcuts
are unconditional, with distinct localized scope suffixes to avoid one copy
hiding the other in merged shell folders. Desktop creation is optional and
unchecked on each run. English is listed first for unsupported UI languages;
Russian is included, system UI language detection is enabled, and the language
dialog allows manual selection on every run. This follows Inno's
[language selection order](https://jrsoftware.org/ishelp/topic_languagessection.htm).

`JobTracker.install.ini` is installer-owned scope metadata in the installation
folder. A mismatching or unreadable existing scope marker blocks reuse of that
exact destination. Uninstall removes installed files, registered shortcuts and
that marker; it does not recursively delete the installation tree. Stage 12 adds
the separate, explicit personal-data policy below. Stage 10 adds the validation and coexistence rules below,
including owned-shortcut cleanup when changing language. Setup neither launches
JobTracker nor enables automatic application closure.

Stage-07 evidence includes Inno compilation and current-user installs in English
and Russian, both desktop choices, custom-path reuse, scope rejection, uninstall,
payload hashes and preserved synthetic sentinels. All-users/UAC, simultaneous
coexistence, default-directory UI, automatic language selection, visual icons
and clean-VM behavior remain manual acceptance checks. `Test-LocalInstaller.ps1`
is a standalone test consumer under `tests/deployment/`; it refuses existing
registrations/shortcuts and records owner/path/hash checks before uninstalling
only its own fixture. Commands and verification limits are in `build.md` and
the stage-07 report.

This local fixture remains available alongside the default Online Installer
described below. Both now use the stage-09 hybrid runtime and stage-10 version
policy, stage-11 manual-close workflow and stage-12 optional personal-data policy
below. Stage 14 completed its audit with the publication hold above; licensing
acceptance remains blocked. Both modes retain the unchanged application
storage identity and can share SQLite/CV data for one account; separate AppIds
do not create separate application data stores.

## Version-locked Online Installer (stage 08)

`Build-Installer.ps1` defaults to `-Mode Online`. After verifying the matching
release archive and Release executable, it generates `JobTrackerPayload.iss`
and compiles `JobTracker-<version>-windows-x64-online-setup.exe`. The generated
include contains the archive SHA-256/size, exact file destinations and individual
file sizes/hashes. It is a local build input; do not publish it or the build JSON.
The wrapper retains compiler output, installer/source hashes and this fixed URL:

```text
https://github.com/turbodiesel606/Releases/releases/download/v<version>/JobTracker-<version>-windows-x64.7z
```

The production script constructs the URL from its CMake-generated version.
There is no wrapper or runtime URL, tag, checksum or manifest override, no
`latest` lookup and no authentication credential. Every new release requires a
new installer. Missing/mismatched local assets fail before compilation.

`OnlinePayload.iss` owns native progress pages, download, verification, temporary
extraction and cleanup. [Inno's download API](https://jrsoftware.org/ishelp/topic_isxfunc_downloadtemporaryfile.htm)
checks the compiled SHA-256; the pipeline also verifies exact size/hash before
calling the native extractor. The download progress callback bounds received
bytes by the expected archive size. HTTPS certificate validation and GitHub
redirects use Inno's network implementation; no external downloader is shipped.

Archive paths are checked against the reviewed inventory at packaging time,
and the downloaded bytes must match that exact archive. Inno's native
[archive path protection](https://jrsoftware.org/is6help/topic_securitymeasures.htm)
then applies during extraction into `{tmp}\payload`. The selected
`enhanced/nopassword` decoder is Inno's bundled `is7zxr.dll` (the runtime log
reports 7-Zip 25.01); the stage-14 [audit](REDISTRIBUTION.md) includes this
dependency and leaves its exact embedded provenance/terms unresolved.
All expected extracted files are size/hash checked before installation starts.
Only explicit generated `[Files]` entries can reach `{app}`, with another native
hash check during copying. Existing destination ancestors and file leaves are
checked for reparse points both before installation and before each copy.

The two scope identities and UI remain shared with Local mode. Download and
extraction failures return from `PrepareToInstall` before application files or
registration change. English/Russian messages explain failure or cancellation;
native details distinguish HTTP, connection, integrity and extraction errors.
Users can retry or exit. Cleanup removes only this Setup invocation's archive
and `{tmp}\payload`; Inno removes its private temporary directory on exit.
Unexpected process termination, disk-full behavior and concurrent path mutation
still require separate acceptance checks. Stage 10 adds the common
custom-path/coexistence rules below.

`Test-OnlineInstaller.ps1` compiles `OnlinePayloadHarness.iss` separately under
`dist/`. This TEST ONLY consumer exercises the production pipeline and generated
file entries against a loopback HTTP server, without modifying the public script,
creating JobTracker registrations/shortcuts or launching the app. Only the test
binary accepts mock URL/hash inputs. Production packaging never references tests.
The suite verifies 148-file success, redirects, corrupt/truncated/oversized
downloads, HTTP 404/503, connection failure, both cancellation paths, retry,
post-extraction tampering, destination junctions, invalid archives and native
traversal rejection before cleanup. It also checks build-time version/digest
failures and refusal of public endpoint/checksum overrides.

All 25 checks passed, including an optional HTTPS request and native download
attempt against the fixed public URL: that asset returned 404 and failed safely.
A successful public asset download, interactive EN/RU progress/retry, elevated
AllUsers installation, clean-machine behavior and final distribution approval
remain unverified. Commands are in `For-Agent/Docs/build.md`; exact evidence and
remaining stage prerequisites are in the stage-08 report.

## Hybrid MSVC runtime (stage 09)

Both installer builds consume `MsvcRuntime.lock.json` through
`Prepare-MsvcRuntime.ps1`. The reviewed inputs are Visual Studio 2022 x64 Release
CRT 14.44.35211.0 and the exact official Microsoft x64 redistributable of that
version. The lock contains the versioned Microsoft CDN URL, size, SHA-256 and
ten CRT file hashes. Refresh this lock deliberately when changing the toolset,
Qt payload or responding to runtime security updates. There is no mutable
runtime `latest` URL, downloaded checksum or production endpoint override.

The preparation script finds the selected Visual Studio instance and toolset
from the production CMake build. It refuses a toolset newer than the lock, then
checks every input's exact hash/size, version and valid Microsoft Authenticode
signature. DLLs must be Release AMD64 PEs. The official x64 redistributable is
an x86 bootstrapper; its signed x64 bundle identity and pinned official x64
URL/hash identify the runtime payload. Builds can use the SDK's package or an
explicit local `-RedistributablePath`; neither path changes the trust anchor.

| Installation scope | Runtime deployment |
|---|---|
| Current user | Ten verified CRT DLLs embedded in Setup and copied beside `bin/JobTrackerApp.exe`; no runtime download, system installer or elevation. |
| All users | No App-Local CRT copied. Check the x64 registry identity and system DLL versions; if insufficient, download the pinned official x64 package, verify size/hash/version, then run `/install /passive /norestart` (or `/quiet` during silent Setup). |

`MsvcRuntime.iss` checks the x64 runtime registration in both registry views,
requires `Installed=1`, major version 14 and a version at least 14.44.35211.0,
and checks the system CRT files. Existing local runtime DLLs in an all-users
destination cause a safe failure, since they could shadow the system runtime.
The installer does not delete them or perform an implicit scope migration.

The common application payload is verified before the prerequisite runs.
Execution waits for Microsoft's installer to finish and rechecks compatibility.
Exit 0 or 1638 is accepted only with a compatible runtime afterward; Burn's
newer-version HRESULT is handled equivalently. Cancellation (1602 or its HRESULT)
and other failures stop before application installation. Results 3010/1641
require a Windows restart and a new Setup run before any JobTracker files are
changed. Retry in the same invocation preserves that restart requirement.
The `/norestart` flag prevents the prerequisite from automatically restarting.
Ready-page text, status and failure/restart messages have English/Russian forms.

An installed machine-wide prerequisite remains installed if later JobTracker
installation is cancelled or fails; JobTracker never rolls it back or uninstalls
it. Cleanup only removes the downloaded executable in Setup's private temporary
directory. Microsoft logs are created there; the JobTracker Setup log records
the prerequisite result code. Pass `/LOG` when collecting installer evidence.

`MSVC-NOTICE.txt` is installed under `licenses/` in both scopes. The unmodified
Release CRT comes from the Visual Studio `VC/Redist` directory, as covered by
Microsoft's [Visual Studio 2022 redistribution list](https://learn.microsoft.com/en-us/visualstudio/releases/2022/redistribution)
and [redistribution guidance](https://learn.microsoft.com/en-us/cpp/windows/redistributing-visual-cpp-files).
Redistribution remains subject to a valid Visual Studio license and its terms;
the notice does not grant rights or replace stage 14's distribution audit.
Windows 10/11 provides the Universal CRT. App-Local CRT servicing requires an
updated JobTracker installer; system deployment can be serviced independently.

Stage-09 evidence covers both compilations, a real non-elevated current-user
install/reinstall/uninstall, all CRT hashes, actual App-Local module loading,
96 PE files/994 CRT import entries, 30 result-policy assertions, native Microsoft
download/cancellation/hash rejection and retained fixtures. The 148-file common
archive is unchanged; no replacement GitHub asset is required for this stage.
The harness never executes the system prerequisite. Missing/old-runtime machines,
actual AllUsers/UAC, real prerequisite cancellation/reboots, GUI and clean Windows
acceptance remain unverified. See the stage-09 report and `build.md` commands.

## Upgrade, reinstall and coexistence policy (stage 10)

`InstallationPolicy.iss` is shared by Online and Local installers. Before any
download/prerequisite and again before application installation, it validates
the scoped uninstall registration, its native previous path, scope marker and
the installed executable's numeric version. `DisplayVersion` must be exactly
three decimal components in the range 0..65535, without leading zeros; its
fourth Windows version word is zero. Malformed, unreadable or contradictory
metadata stops Setup. Native registry errors are distinguished from missing
keys, and both registry views are inspected. Legacy Scope-only markers remain
supported when the executable and registration agree. New markers also record
the version. These are local consistency checks, not signatures or protection
against an account that deliberately rewrites its own files and registry.

Within one scope, a newer installer upgrades and an equal version repairs or
reinstalls; an older installer is rejected. Numerical ordering handles multi-digit
and high unsigned Windows version words. The policy wraps Inno 6.7.1's signed
packed comparison at the sign boundary. Repair replaces ancillary files when
the main executable/identity are verifiable; a missing or corrupt executable or
contradictory identity requires uninstall with data preserved, then reinstall.

Inno's native `UsePreviousAppDir=yes` still selects the registered directory and
the directory page remains editable. A different directory for an already
registered scope is rejected with an explicit uninstall/reinstall instruction.
This is the supported relocation workflow: retaining two binary directories
under one AppId would leave an old executable and uninstaller able to interfere
with the replacement. Setup does not silently relocate or remove the old copy.

New destinations must be empty. Paths must be absolute local drive paths;
ambiguous dot/space components, relative/network/device paths and inaccessible
or redirected ancestors are rejected. Handle-based resolution covers short-name
and drive aliases. Registered installations cannot use equal or nested roots;
unregistered installation markers in ancestors also block nesting. Local mode
now uses the same exact inventory and per-destination copy checks as Online mode.
No recursive cleanup of an existing destination is performed.

Both modes retain their version-independent AppIds, scope-specific registration
and distinct localized shortcuts. Before overwriting a known shortcut, Setup
checks that its target is this executable. After success it removes only owned
obsolete English/Russian names in the selected scope's shell folders; disabling
the desktop task also removes the owned desktop link. A shortcut pointing
elsewhere is preserved and blocks installation. New installers and uninstallers
share a global lifetime lock; a concurrent or inaccessible lock fails closed.
Pre-stage-10 uninstallers do not participate in that lock.

### Shared-data restriction

The same account's current-user and all-users copies still resolve to the same
`QStandardPaths::AppLocalDataLocation/Data` store. Only **exactly equal release
versions** may coexist. Current-user Setup checks machine-wide installations.
All-users Setup checks every regular profile in `ProfileList`, plus loaded user
hives, for incompatible current-user copies. An unloaded or unreadable profile
blocks all-users installation: sign in to it and keep its session open, or use
current-user mode. Setup does not load offline hives, impersonate other users,
read their application data or modify their registrations. This deliberately
restrictive policy can block deployment on multi-user machines until all profile
states are visible. Temporary, stale or inaccessible profiles need operator
resolution; do not bypass the check.

To upgrade two equal-version copies, uninstall one with personal data preserved,
upgrade the remaining copy, then reinstall the other at that same new version.
There is no supported intermediate mixed-version state. Setup never opens
SQLite, launches the application, runs migrations or deletes personal data.
Uninstall uses its own scope and file log and does not remove the machine CRT.

Equal versions do not prove that simultaneous application sessions are safe.
Old installers, manually copied executables, previously migrated data after
complete uninstall, external registry/filesystem edits and legacy uninstaller
races are outside this installer policy. Keep backups and do not use an older
binary with data opened by a newer release. No database backward compatibility
or global per-user schema history is claimed. Actual dual-scope installs,
multi-session/ACL behavior, UAC and clean Windows remain VM acceptance work.

The standalone `Test-InstallationPolicy.ps1` compiles a test-only consumer of
these production functions. It verifies version/path/registry policies and can
exercise the real non-elevated current-user installer using only a fresh owned
fixture. Historical versions are coherent synthetic PE/registry/marker fixtures,
not old JobTracker releases. No production source or build depends on the tests.
Exact commands, results and manual limits are in `build.md` and report `10.md`.

## Manual closure before changes (stage 11)

`ManualClose.iss` is shared by Online/Local Setup and their uninstallers. It
probes the selected `{app}\bin\JobTrackerApp.exe` using `CreateFileW` with
write access, all sharing modes and `OPEN_EXISTING`; it never writes or
truncates bytes. Windows refuses write access to a mapped executable image.
The gate checks this exact file rather than enumerating process names, so an
independent copy named `JobTrackerApp.exe` does not block it. File sharing also
applies across Windows sessions. A lock can belong to another program, and an
access failure does not identify a PID or user: both stop the operation safely.
The message shows the affected path and asks for manual closure, including
asking the other session's user when applicable. It does not impersonate users
or attempt to close, kill, signal or restart any process.

English/Russian Retry/Cancel messages recheck the file on Retry. Silent runs skip
that dialog and must be rerun after closing. For unattended Setup, supply both
`/VERYSILENT` and `/SUPPRESSMSGBOXES`: Inno otherwise shows its native preparation
error summary. The early silent uninstall gate needs no such summary dialog.
Setup checks before downloads/prerequisites, again after them, at `ssInstall`,
and through a `Check` callback immediately before each payload/CRT/notice copy.
The per-file check also probes the destination itself. Uninstall checks at
initialization, `usAppMutexCheck`, and immediately before deletion at `usUninstall`.
Unsafe paths still use the existing reparse-point/access validation.

`CloseApplications=no` and `RestartApplications=no` remain explicit. Setup rejects
`/CLOSEAPPLICATIONS`, `/FORCECLOSEAPPLICATIONS`, `/RESTARTAPPLICATIONS`, `/NOCANCEL`
and `/LOADINF`. Inno command-line/INF settings can override compiled close and
cancel settings; refusing `/LOADINF` avoids trusting a second read of mutable
input. `/SAVEINF` remains available, but those files cannot be replayed here.

Inno 6.7.1 swallows exceptions from `BeforeInstall` and `Check`. Therefore a
late file refusal returns False, latches further guarded copies off and requests
acknowledged native cancellation without another confirmation. The native
handler requires an enabled, visible progress form and Cancel button; the code
temporarily enables/shows them and immediately restores their state. This may
briefly show the progress window even with `/VERYSILENT`. Native cancellation
then enters rollback before shortcuts/INI/registry commit. The dedicated test
requires exit 5 and a rollback log, not just a nonzero exit. Revalidate this
source-dependent sequence when updating the compiler.

These are availability checks, not a lock spanning the whole transaction. A
process can start between the last check and a native operation; Windows image
sharing is the final protection for loaded binaries. Do not reopen the affected
copy until maintenance ends. External file renames, hard-link manipulation,
legacy uninstallers and concurrent administrative changes are not serialized by
this gate. Uninstall has no per-file script callback. A failure after some
upgrade files were copied can require rerunning the same/newer installer after
closing the app; generic rollback does not prove the old version was restored.
The manual-close gate does not open or delete personal data. Other-scope copies retain the shared-data
limitations above; no migration or concurrent database safety claim is made.

## Safe uninstall and optional personal data (stage 12)

`PersonalData.iss` keeps personal data outside Inno's uninstall file log.
The application sets only `QCoreApplication::applicationName` to `JobTracker`;
Qt 6.10.3 resolves normal Windows `AppLocalDataLocation` to the account's Local
AppData known folder plus `JobTracker`. `StoragePaths.cpp` adds `Data`, containing
`database.sqlite` and `Resumes/`. Both installation scopes share this identity.
The current source has no `QSettings` or QML `Settings` usage. Storage paths,
database schema, migrations and application source are unchanged.

Program removal preserves data by default. **Only a verified, non-elevated
CurrentUser uninstall offers deletion**, after program removal finishes. The
English/Russian prompt shows the exact Local AppData `JobTracker` folder and
defaults to No. Yes requests permanent removal of that entire application
folder, including SQLite sidecars, imported CVs and any other contents. No,
closing the prompt, cancellation before completion, and every silent uninstall
preserve data. There is no command-line deletion switch. Setup/rollback and
DeinitializeUninstall never invoke data cleanup.

AllUsers and every elevated uninstall preserve data without inspecting another
account's folders, even when elevation used the same account. The uninstaller
cannot reliably establish the original interactive account after elevation.
For CurrentUser, the initial HKCU install record must identify this exact
installation; token SID ownership is then checked for Local AppData and every
object selected for cleanup. A missing/mismatched record or owner fails closed.

Native `usDone` alone does not prove complete uninstall. Cleanup also requires
the no-restart callback, absence of the entire program directory (unknown
leftovers also block cleanup), absence of both localized shortcut names for this
scope, and no CurrentUser or AllUsers registration in either registry view.
Unreadable registration blocks cleanup. These checks run again after consent
while the existing global installer lifecycle lock remains held. Remaining
installations or uncertain removal produce a localized preservation message.
The deletion path never enumerates other profiles. Manual/unregistered copies
cannot be exhaustively discovered; the prompt tells their users to keep data.

Cleanup accepts only the exact `JobTracker` child of the resolved Local AppData
folder. It pins every ancestor top-down without delete sharing, opens children
with `OPEN_REPARSE_POINT`, checks resolved handle paths and owner SIDs, rejects
reparse points and file hardlinks, and opens all files exclusively before any
mutation. Native enumeration distinguishes completion from an error. Read-only,
inaccessible, locked, aliased or ambiguous paths preserve the entire tree during
preflight. Paths over 240 characters, depth over 32, or trees over 8192 handles
are conservatively refused. Local AppData and its other children are never
deletion targets.

Deletion uses `SetFileInformationByHandle` on the already validated handles,
bottom-up. It never performs a recursive pathname delete, follows a junction,
changes ACLs/ownership, force-closes an application, or queues deletion at reboot.
Cleanup is not transactional: a late Windows failure, crash or concurrent new
file can leave part of the explicitly confirmed data tree behind. The error
message states that completed deletions cannot be restored. The code stops at
the first failure and releases remaining handles without deleting them.

The standalone tests in `tests/deployment/Test-PersonalData.ps1` use only new
synthetic `dist/` data. Native core tests cover deletion, Unicode names, locks,
owner-SID mismatch, hardlinks, junctions and unsafe roots. Separate native test
uninstallers exercise complete/partial removal and silent preservation. Optional
real CurrentUser integration checks installation identity, shared-data refusal
and default preservation, then uninstalls only its owned fixture. No actual
personal data is read, and no JobTracker application is launched. These checks
do not verify the interactive Yes/No dialog, real cross-account elevation,
AllUsers/dual-scope coexistence, ACL-denied registry access, concurrent hostile
mutation or clean-machine acceptance. Canonical commands are in `build.md`.

Local evidence includes real mapped test images, exact-path isolation, closed
reruns, silent EN/RU cancellation, late-boundary/file cancellation and a real
non-elevated CurrentUser install/reinstall/uninstall. Synthetic process fixtures
never run the JobTracker application. Interactive Retry/dialog appearance,
native delayed-uninstall timing, another Windows session, AllUsers and clean-VM
acceptance remain unverified. Exact commands and evidence are in `build.md` and
`For-Agent/Task-Report/windows-release/11.md`.

## Integration verification (stage 13)

Use the canonical stage-13 commands in `For-Agent/Docs/build.md` and the reusable
matrix in `tests/deployment/Windows-Release-Checklist.md`. Record the exact new
archive/installer hashes and observed results in the stage report. Native
component tests, real CurrentUser installation, interactive dialogs and clean
Windows acceptance are separate evidence levels. In particular, a safely
rejected missing GitHub asset is not a successful Online installation, synthetic
version fixtures are not historical-release migration tests, and the deployment
probe is not JobTracker startup. Retain earlier sharing failures even after a
successful rerun; do not weaken the manual-close guard or claim a root-cause fix.
