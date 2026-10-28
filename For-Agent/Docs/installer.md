# Windows Offline Installer

This reference describes the existing native x64 installer for Windows 10/11.
It produces one offline Setup containing the
Release application, Qt/QML/plugins/SQLite, per-copy MSVC inputs, and the
runtime notice. Installation requires no Qt/Visual Studio SDK, application
archive, remote manifest, or dependency download.

The [one-command launcher](../../Installer/README.md#one-command-release-installers)
writes `Installer/Windows/output/JobTracker-Setup-<version>-<UTC-time>-<id>.exe`
directly. The manual builder retains its
`Installer/Windows/output/<run-id>/JobTracker-Setup-<version>.exe` default.

Use [build.md](build.md) for shared build action selection and
[build-windows.md](build-windows.md) for the Windows toolchain and backend-test
commands. The Release build and installer sequence is below.
Use [installer-testing.md](installer-testing.md) for standalone verification and
the reusable acceptance checklist. These are procedures and implementation
contracts, not a record of results for any particular candidate.
The [installer architecture](architecture/installer.md) describes ownership and
the boundary with unchanged application behavior.

## Implementation Files

All paths below are relative to [Installer/Windows/](../../Installer/Windows/).
Use [Installer/README.md](../../Installer/README.md) for shared platform navigation.

| File | Responsibility |
|---|---|
| `Create-Installer.ps1`, `Create-Installer.example.psd1` | Unattended configure/build/stage/package orchestration and optional ignored local configuration. |
| `JobTracker.iss` | Inno Setup entry point: native x64 Setup, EN/RU UI, scopes, shortcuts, and lifecycle callbacks. |
| `Stage-Release.ps1` | Fresh direct CMake Release staging, shared build/payload validation, and private staging evidence. |
| `Build-Installer.ps1` | Validates direct staging and generated identity, prepares local runtime inputs, snapshots bytes, generates explicit hashed file entries, invokes ISCC, and records private build evidence. |
| `PayloadValidation.ps1`, `payload-files.txt` | Canonical exact payload inventory and validation of production identity/hash, Qt version, x64 Release PE files, safe paths, relative metadata, reparse points, and alternate streams. Shared by the builder and standalone tests. |
| `PayloadSafety.iss` | Native destination/local-file safety checks used by Setup. |
| `Prepare-MsvcRuntime.ps1`, `MsvcRuntime.lock.json` | Verify and snapshot pinned local Microsoft inputs against the selected toolset; generate runtime include/evidence. |
| `MsvcRuntime.iss`, `MSVC-NOTICE.txt` | App-Local/system runtime policy and delivered Microsoft runtime notice. |
| `InstallationPolicy.iss` | Local installation identity, version/path/ownership validation, repair/coexistence rules, and installer/uninstaller mutual exclusion. |
| `InstallOwnership.iss` | Relative owned-file ledger, local uninstall-history validation/reconstruction, preserved conflicting files, and per-copy shortcut ownership. |
| `PersonalData.iss` | Default-preserve uninstall, explicit target-account confirmation, and optional cleanup of that user's shared `Data` directory. |
| `JobTracker.rc.in`, `JobTrackerIdentity.cmake.in`, `JobTrackerIdentity.iss.in` | CMake-generated Windows resource and packaging identity templates. |
| `JobTracker.ico` | Current temporary seven-size application/Setup/shortcut icon. |
| `output/` | Ignored generated customer Setup executables; source files above remain versionable. |

## Tools And Local Inputs

- Windows x64 application toolchain from [build-windows.md](build-windows.md#toolchain-and-sdk), including exactly
  Qt 6.10.3, Visual Studio 2022/MSVC v143, and CMake 3.22+.
- PowerShell 7.2+ for the installer scripts.
- Inno Setup 7+ with native `SetupArchitecture=x64` support. The wrapper checks
  the finished Setup for AMD64/PE32+. Inno 6's I386 launcher is rejected even
  with an x64 payload/install mode.
  Native handle declarations use pointer-sized types for the x64 engine.
- A current x64 production Release build with tests OFF, its generated identity,
  a fresh direct CMake stage, and authorized local MSVC inputs matching
  [MsvcRuntime.lock.json](../../Installer/Windows/MsvcRuntime.lock.json).

The lock currently pins runtime version `14.44.35211.0`, Visual Studio redist
directory `14.44.35112`, and ten App-Local DLLs under
`VC/Redist/MSVC/14.44.35112/x64/Microsoft.VC143.CRT` in the build's selected VS
instance. Preparation checks the recorded compiler/toolset against the pinned
runtime, exact file sizes/SHA-256, Release versions, valid Microsoft signatures,
and x64 CRT PE identity. The official x64 redistributable has a signed x86
bootstrapper; its pinned bytes and signed x64 product identity identify the
bundle. It is distinct from the native x64 JobTracker Setup.

`Build-Installer.ps1 -RedistributablePath` is optional. If omitted, the wrapper
reads `CMAKE_GENERATOR_INSTANCE:INTERNAL` from the selected build's
`CMakeCache.txt` and tries
`<VS instance>/VC/Redist/MSVC/v143/vc_redist.x64.exe`. This is local path
discovery, not proof that the file exists, matches the lock, or that a suitable
system runtime is installed. An explicit path selects another existing local
package; all integrity/version/signature checks still apply. Neither the wrapper
nor runtime preparation downloads inputs or silently refreshes the lock.
`Prepare-MsvcRuntime.ps1` itself requires an explicit redistributable path.

## Build Sequence

For a request to create or update an installer, prefer the unattended launcher:

~~~powershell
pwsh -NoProfile -File ./Installer/Windows/Create-Installer.ps1
~~~

For trusted, reviewed scripts transferred through an archive and blocked by
PowerShell execution policy (for example, `RemoteSigned`), use this alternative:

~~~powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File ./Installer/Windows/Create-Installer.ps1
~~~

Run the appropriate command, not both, from the repository root. `Bypass` applies
only to the newly started PowerShell process and its children; it does not change
persistent execution-policy settings or remove file zone metadata. It is
unnecessary when execution policy already permits the scripts. The manual
sequence below retains the corresponding dedicated PowerShell session.

Use [Installer/README.md](../../Installer/README.md#one-time-local-setup) for
one-time configuration, precedence, options, cache/locking rules, UTC filenames
and logs. Existing matching local presets need no duplicate SDK configuration.
No tests, installation, app launch or development-runtime target is added.
Within a trusted PowerShell session, structured results are also available:

~~~powershell
$created = & ./Installer/Windows/Create-Installer.ps1 -PassThru
$installerBuild = $created.BuildDirectory
$installerStage = $created.StageDirectory
$installerSetup = $created.InstallerPath
$installerEvidence = $created.EvidenceDirectory
$installerMetadata = $created.MetadataPath
$installerIscc = $created.CompilerPath
~~~

The step-by-step manual sequence below remains available unchanged. The
launcher's `result.json` records the same exact invocation paths for later tests.

Run each command separately from the repository root in the same PowerShell
session, checking the exit code before continuing. CMake execution follows
[cmake-build-debug](../../.agents/skills/cmake-build-debug/SKILL.md). Preserve
existing outputs and failed evidence.

An archive transfer can leave `Zone.Identifier` metadata on unsigned PowerShell
sources, causing `RemoteSigned` to reject them before execution. For transferred
scripts you have reviewed and chosen to trust, start a dedicated session if needed:

~~~powershell
pwsh -NoProfile -ExecutionPolicy Bypass
~~~

Run the commands below and the deployment checks inside that session. The setting
applies only to that process and its children; it does not change persistent
execution policy or remove source zone metadata. Close the session afterward.
Payload and Microsoft runtime hash/signature validation remain enforced.

First configure/build the tests-OFF `windows-release` with your local `QTDIR`,
using the [Windows SDK selection](build-windows.md#toolchain-and-sdk).
Substitute matching `*-local` presets when they supply the installed kit:

~~~powershell
cmake --preset windows-release
cmake --build --preset windows-release --target JobTrackerApp
~~~

The staging and installer commands below run in the same PowerShell 7.2+ session
and consume that already built production Release tree. They do not implicitly
configure/build the application. Development runtime preparation is unnecessary
for packaging.

### Direct Release Staging

~~~powershell
$installerBuild = (Resolve-Path -LiteralPath build/qt-6.10.3/windows-release).Path
~~~

~~~powershell
$stageResult = & ./Installer/Windows/Stage-Release.ps1 -BuildDirectory $installerBuild -PassThru
~~~

~~~powershell
$installerStage = $stageResult.StageDirectory
$installerRoot = $stageResult.EvidenceDirectory
~~~

The wrapper reserves a fresh `build/installer-stage-<run-id>/stage`, runs the
existing `cmake --install ... --config Release --prefix ...` procedure, checks
the native command exit and validates the exact payload. `install.log` and
`staging.json` stay beside the stage in the private run directory. To choose
an explicit fresh stage instead, add `-StageDirectory <path-under-build>`.
Existing paths and reparse points are rejected. Failed output is retained.
The wrapper prints absolute stage/evidence/log paths; `-PassThru` returns them
as an object. It does not prepare or snapshot MSVC runtime inputs.

The stage contains `bin/`, `plugins/`, `qml/`, `translations/`, and relative
`bin/qt.conf`, including `qwindows.dll` and `qsqlite.dll`. CMake owns the sole
payload algorithm and excludes development/tooling/input plugins and
non-SQLite SQL drivers. `NO_COMPILER_RUNTIME` leaves CRT delivery to the
installer. Debug installs and the development-runtime target are not Release
stages. Linux staging uses its separate Linux wrapper.

The reviewed inventory lists 150 application/Qt files, including
`d3dcompiler_47.dll`, `dxcompiler.dll`, and `dxil.dll` deployed by `windeployqt`.
These graphics dependencies are separate from the MSVC CRT excluded by
`NO_COMPILER_RUNTIME`. Embedded QML resources are allowed; loose C++
sources/headers, tests, PDBs, CMake files and internal
documents must not ship. Keep probes and logs outside the payload. The common
stage alone lacks the per-copy MSVC inputs of the finished installer.

### Compile Setup

If ISCC.exe from Inno Setup 7+ is on PATH, use:

~~~powershell
$installerResult = & ./Installer/Windows/Build-Installer.ps1 -BuildDirectory $installerBuild -StageDirectory $installerStage -PassThru
~~~

If it is not on PATH, add `-IsccPath <absolute-path-to-ISCC.exe>` to that
invocation. An explicit invalid path fails without fallback. Discovery is
local only; the existing Inno version and native x64 output checks still apply.
To select another authentic local redistributable matching the lock, also add
`-RedistributablePath <absolute-path-to-vc_redist.x64.exe>`. Otherwise the
builder uses the selected Visual Studio instance's existing default described
above. No inputs are downloaded or silently repinned.

Every invocation reserves `Installer/Windows/output/<run-id>/` and writes exactly
`JobTracker-Setup-<version>.exe` inside it. Rebuilding the same version
requires no manual moving or deletion: earlier Setup bytes and evidence stay
in their own run directories. The builder retains stage/identity/hash checks,
verified private MSVC snapshots and explicit hashed Inno file entries. It
prints absolute result, compiler-log and evidence paths before compilation.

The backward-compatible `-TimestampedOutput` builder switch instead publishes
directly into `Installer/Windows/output/` using the launcher's UTC/random suffix.
Both modes compile privately, validate native x64/product/version, write the
approved exact `installerFileName` and bytes into metadata, then atomically move
without overwrite. Existing `-PassThru` fields are preserved in both modes.

Use that exact invocation's structured result, never an arbitrary newest file:

~~~powershell
$installerSetup = $installerResult.InstallerPath
$installerEvidence = $installerResult.EvidenceDirectory
$installerMetadata = $installerResult.MetadataPath
$installerIscc = $installerResult.CompilerPath
~~~

Without `-PassThru`, use the exact paths printed by the wrapper. Private
snapshots, generated includes, `compile.log` and `installer-build.json` live
under `build/installer/<run-id>/`, separate from customer output. Failed runs
remain available for diagnosis.

Optional standalone runtime input diagnosis (the builder already performs
this; it is not an additional normal step):

~~~powershell
pwsh -NoProfile -File Installer/Windows/Prepare-MsvcRuntime.ps1 -BuildDirectory $installerBuild -RedistributablePath <absolute-path-to-vc_redist.x64.exe> -OutputDirectory "$installerRoot/runtime-input-check"
~~~

This verifies/copies local inputs only; it never executes the prerequisite.

## Version, Generated Files, And Evidence

Root `project(JobTracker VERSION ...)` in
[CMakeLists.txt](../../CMakeLists.txt) owns the application and Setup version.
Windows numeric metadata adds `.0`; the executable remains
`JobTrackerApp.exe`, independent of the QML module's version.
Reconfigure/rebuild after version or icon changes; do not edit generated identity.

The source templates live under `Installer/Windows/`.
Generated RC/CMake/Inno identity lives under
`<build>/Installer/<configuration>/`. Private snapshots, generated includes,
runtime copies, logs, and `installer-build.json` live under
`build/installer/<run-id>/`. JSON records local hashes, source state, compiler,
identity, and payload inputs; it is not a remote trust anchor or customer asset.
Only finished Setups belong under `Installer/Windows/output/`: directly for the
launcher/timestamped mode, in a run directory for the default manual builder.
The publisher is blank and normal builds are unsigned.

Keep each Setup with its own private evidence. Changed payload, source, runtime,
or engine inputs require a matching candidate/evidence set for verification;
dated reports cannot prove that current bytes match.

## Scope, Runtime, And Maintenance

| Scope | Default location / shortcuts | MSVC behavior |
|---|---|---|
| CurrentUser (default) | `%LocalAppData%/Programs/JobTracker`; user shortcuts | Ten verified App-Local x64 Release DLLs beside the EXE; no machine prerequisite or admin rights. |
| AllUsers | `%ProgramFiles%/JobTracker`; common shortcuts; UAC | The same per-copy App-Local DLLs; check the system x64 runtime and install the embedded official redistributable when insufficient. |

Runtime failure/cancellation stops before application copying. Restart-required
results require reboot and rerunning Setup. A later installation failure or
JobTracker uninstall does not roll back/remove the shared machine runtime.
Removing one program folder does not remove another copy's Qt or App-Local CRT.

Destinations are editable in both scopes. Choose any suitable regular local
folder on an available local drive, subject to actual permissions and the path
guards. Example drive letters are not a supported-drive list. EN/RU UI includes
a language-selection dialog, system-language selection with English fallback,
Start Menu shortcuts, and an unchecked desktop option.

The Browse dialog's New Folder action selects a proposed path; it does not
create the folder on disk before installation. Typed and dialog-selected new
folders, including direct children of a drive root, use the same validation.
Canonicalization preserves one separator when appending missing components and
remains stable when repeated. Drive roots can be resolved for path checks but
cannot be selected as the installation folder.

No JobTracker uninstall entry is created in the registry:
`Uninstallable=yes` retains a local uninstaller and `CreateUninstallRegKey=no`
keeps JobTracker out of Windows Installed apps. Run `unins000.exe` inside the
selected copy's folder. Local installation metadata and file ownership govern
maintenance; installation does not enumerate all Windows profiles, discover
copies on disconnected volumes, or recover a manually removed old registration.
Registry use for Windows identity or Microsoft prerequisites is separate.

| Selected destination | Maintenance behavior |
|---|---|
| Empty or new folder | Install a new independent copy. |
| Separate folder, including another scope or available volume | Install without changing other copies. An inaccessible/disconnected other copy is not a prerequisite. |
| Identified same-version copy | Reinstall/repair its owned files and local uninstaller. |
| Identified older copy | Update only the selected copy. |
| Reliably identified newer copy | Reject overwriting it with an older version. An older version remains installable in a separate folder. |
| Whole program folder manually deleted | Install again normally; no previous EXE, INI, log, or registry entry is required. |
| Partially deleted copy or missing uninstaller | Use remaining local evidence to repair the copy. Missing files alone do not impose a permanent block. |
| Missing/damaged uninstall bookkeeping | Reconstruct records for identifiable owned files; unavailable historical ownership is not invented. |
| Unrelated nonempty folder | Refuse silent adoption; choose an empty folder or restore enough genuine local evidence to identify a repair. |

Normal same-directory maintenance retains valid uninstall history so that
previous-version owned files remain accounted for. Unrelated files are not
claimed or deleted. Installation folders must not contain one another and must
not overlap the personal-data directory. Existing local path, alias, and
reparse-point safety restrictions remain; network paths and junction/symlink
installations are not supported by this change.

`JobTracker.install.ini` records the local installation and
`JobTracker.ownership.ini` records owned relative file paths. The ownership
ledger uses `[Identity]` (`Product=JobTracker-2`, `Path`, `Count`) and numeric
entries under `[Files]`. A hash-verified same-location Inno `.dat` history is
appended during normal maintenance. Unverified, moved, or scope-changed history
is renamed with a `.preserved-n` suffix and bookkeeping is reconstructed from
the current payload and available relative ownership ledger. Unowned files
colliding with payload paths are also preserved under such suffixes. These
preserved files are not a claim that unavailable historical uninstall records
have been recovered.

If the relative ownership ledger is missing during uninstall, reconstruction
recognizes only files whose hashes match the current payload. Changed files
whose ownership cannot be proved are preserved and reported as incomplete
removal. Reinstall/repair can establish current bookkeeping without claiming
unavailable historical ownership.

No application-closure prerequisite is imposed. Installation and uninstallation
attempt the requested operations while JobTracker or another copy runs. They
do not poll, wait for, terminate, restart, or coordinate with application
processes. Installer/uninstaller mutual exclusion remains. Windows may refuse
an actual replacement/removal because a file is locked, access is denied, or
another filesystem condition applies. Retain the reported path/operation and
logs; cancellation or interruption does not guarantee atomic rollback, but
remaining identifiable local files can be repaired. Setup never resets,
restores, deletes, or migrates personal data on any success or failure path.
Program-root and ancestor directory handles pin paths against redirection while
operations run; they are filesystem safety guards, not application/profile idle
locks or a prerequisite that applications exit.

### Manual Deletion, Copying, Moving, And Shortcuts

Deleting a program folder in Explorer leaves personal data untouched and shows
no data-choice dialog. Reinstalling restores a local uninstaller even if every
old uninstaller was deleted. Copying binaries to another machine does not copy
the original machine's personal data; another Windows account continues to use
its own application-resolved per-user data location.

A copied, moved, or renamed folder must be repaired with Setup at its new
location before its uninstaller is used. Path-mismatched local metadata or
uninstall records are not executed against their source location. Repair
establishes local ownership and new bookkeeping; it cannot recover unavailable
historical records. The source installation and its shortcuts remain separate.

Shortcut identities distinguish installations. Installing another copy does not
replace another copy's shortcut, and uninstall removes only shortcuts still
owned by the selected copy. A stale shortcut does not block a new installation;
a user-retargeted shortcut is preserved. These rules apply to user/common Start
Menu links and the optional desktop links.

A successful scope change in the same directory places the active Start Menu
shortcut and any selected desktop shortcut in the newly selected user or common
shell folder.
A recorded link is reused only when its copy identity, executable target, and
shell-folder location are appropriate for that scope. Old verified owned links
are removed explicitly. A verified retargeted link is preserved and released
from this copy's records; an unreadable/unverifiable link is preserved with its
record and reported as incomplete. Pending old locations remain recorded until
handled, so a Windows removal/creation
failure is reported as incomplete and can be retried or repaired without
orphaning known shortcuts.

The desktop task also applies on reinstall. Checked to unchecked removes the
previously owned desktop link and creates no replacement; unchecked to checked
creates or safely reuses the appropriate link. A failed deletion of a verified
owned link keeps its recorded location for retry. Retargeting a link does not
authorize Setup to delete it.
Each new shortcut's intended path is recorded before creation, and Start Menu
and desktop results are retained independently if a later step fails. Repair
can therefore retry a missing or partial shortcut using the recorded location.

## Personal Data

The existing per-account store is `%LOCALAPPDATA%\JobTracker\Data`, including
`database.sqlite` and `Resumes`, independent of installation directory/scope.
Windows resolves the account's actual profile and Local AppData path; no drive,
volume label, or account name is hard-coded. Setup never starts JobTracker,
opens its database, performs migrations, or restores missing/damaged user data.
Application startup, schema validation, CV handling, concurrency, and recovery
remain unchanged. Different versions can share this data; the installer adds no
database compatibility or concurrent-access guarantee.

Program removal and personal-data cleanup have separate results. Program
removal targets the selected copy's owned files, metadata, and still-owned
shortcuts; unrelated leftovers do not have to disappear before the data choice.
Other copies and their shortcuts remain untouched.

After the data choice and any confirmed cleanup, program removal is attempted
immediately. If an owned file or shortcut cannot be removed, detailed failures
are logged and native uninstall ends with failure before destroying its local
uninstaller/log. The remaining local metadata and uninstaller support retry or
repair. Partial program removal is not reported as complete, and successful
data cleanup is not rolled back because later program removal failed.

Keep personal data is the default. Silent uninstall preserves it. At the start
of interactive program removal, uninstall offers EN/RU default-No confirmation
naming the exact target account, SID, profile, and `Data` path and explaining
that every copy used by that user shares it. The executing account is also
shown. No, dismissal, or lack of affirmative consent preserves data.

Elevation alone does not disable the choice. An elevated uninstaller resolves
the desktop shell's Windows token and known folders so another account's UAC
credentials do not silently substitute the administrator's Local AppData. If
that desktop identity cannot be resolved, a separate default-No account-choice
confirmation explicitly selects the displayed execution account before the
data-deletion confirmation. Declining preserves data. An unverifiable target is
reported without guessing or enumerating all profiles.

After affirmative confirmation, cleanup starts immediately, before program-file
removal, even if the selected copy or another copy runs, other copies remain installed or are
disconnected, or some program files could not be removed. There is no last-copy
auto-deletion, profile-idle gate, process check, or wait. The application may
recreate files while running; the installer adds no coordination or recovery.

Only the explicitly confirmed user's `Data` directory is targeted. The parent
`%LOCALAPPDATA%\JobTracker`, originals outside `Data`, unrelated outside files,
and other users' data are preserved. Missing `Data` is already complete.
The Windows account/token and known-folder path identify the target; object
ownership is not a substitute for that identity or for filesystem permissions.
A correctly targeted Data root or child owned by `BUILTIN\Administrators` is
eligible for cleanup when Windows allows deletion. This does not authorize
cleanup of other Administrators-owned paths. Cleanup never changes owners or
ACLs, enables extra privileges, or takes ownership to overcome denial.

Ordinary path safety and actual Windows errors still apply. Reparse/hardlink,
access, read-only, or sharing failures are reported with remaining paths and
Windows error codes. Cleanup continues with other safe entries after
an individual failure; incomplete cleanup is not reported as complete. Deletion is
irreversible and nontransactional: files already deleted are not restored and
remaining files are not scheduled for reboot deletion.

## Changes And Common Failures

When Qt, the MSVC toolset, runtime inputs, Inno Setup, or payload dependencies
change, review the affected inventory/identity/lock and source contracts
explicitly. Do not bypass validation or silently repin inputs. Rebuild matching
evidence and select the relevant [installer suites](installer-testing.md).
Recheck native error/cancellation behavior after an Inno upgrade. Review required
notices, provenance, and entitlement separately from functional tests.

| Failure | Existing handling / next diagnosis |
|---|---|
| Missing or stale generated identity | Wrapper rejects; configure/build the intended production Release tree. |
| Inventory/hash/Qt/PE mismatch, unsafe path, reparse point or stream | Payload validation rejects; identify the changed input rather than relaxing checks. |
| Missing/mismatched runtime or incompatible toolset | Preparation reports required paths/version/integrity; supply authorized matching local inputs or explicitly review the lock/toolset. Discovery does not guarantee a usable input. |
| Existing or unsafe run directory | Wrapper refuses collisions and reparse points. Each invocation selects a fresh run directory, so earlier same-version Setup files remain unchanged. |
| Inno compilation or native identity failure | Retain `compile.log` and candidate state under private work; inspect compiler diagnostics and native x64 support. |
| Unrelated nonempty folder, nested copies, data overlap, or unsafe path | Choose a suitable destination or repair the identifiable selected copy; unrelated files are not adopted. |
| Missing local files or damaged bookkeeping | Reinstall into the selected copy; only identifiable ownership can be reconstructed. Valid same-location history is retained. |
| Copied/moved uninstaller | Repair at the new location before removal; never run source-location records. |
| Actual sharing/access/space failure, cancellation, or interruption | Retain logs and local evidence. Incomplete owned-program removal fails and keeps local uninstall bookkeeping for retry/repair. Correct the reported condition; there is no application-close gate or guaranteed full rollback. |
| Confirmed data cleanup is incomplete | Report remaining failures independently of program removal. Already deleted files are not rolled back; other copies may recreate data. |
| DXC/DXIL deployment warnings | Investigate affected graphics/runtime dependencies and clean-host behavior; a successful developer-host probe alone is insufficient. |

Earlier local documentation recorded an intermittent Windows sharing error 32
without an established cause. If it recurs, collect a narrow trace for the exact
path and failing operation, timestamp, process identity, and access/share flags.
Do not force-close processes, blindly retry, or reuse historical recovery scripts.

## Distribution Review And Unavailable History

Release review must resolve Lottie licensing, complete notices/source/replacement
materials, ICU/Mesa/D3D binary provenance, Inno redistribution, and owner
entitlement against the actual release inputs. Historical supporting evidence
is unavailable in this checkout, so detailed prior conclusions cannot be
reconstructed. Keep these concerns for a release-specific review before publication.
The included MSVC notice is not a complete Qt/third-party notice bundle.

Technical checks, owner-tested installer operation, and signatures are distinct
from redistribution clearance. No publication, upload, signing, updater, or
CI/CD operation is performed by the normal build.

## Optional Installer Signing

Unsigned local builds require no signing tool or certificate. For a later
owner-authorized signature, use the installed Windows SDK SignTool with an
existing code-signing certificate in the current user's certificate store.
Set these session variables to the intended values: `$JobTrackerSignTool`
(full tool path), `$JobTrackerCertificateThumbprint` (certificate thumbprint),
`$JobTrackerTimestampUrl` (the certificate provider's RFC 3161 endpoint), and
`$JobTrackerSetup` (the exact finished Setup path from the launcher or manual builder).
No PFX, password, private key or secret belongs in the repository or commands.
Preserve the unsigned EXE and its `installer-build.json` in private evidence
first. Require exit code 0 after each command before proceeding:

```powershell
& $JobTrackerSignTool sign /sha1 $JobTrackerCertificateThumbprint /fd SHA256 /tr $JobTrackerTimestampUrl /td SHA256 $JobTrackerSetup
```

```powershell
& $JobTrackerSignTool verify /pa /all /tw /v $JobTrackerSetup
```

Only after signing, timestamping and verification succeed, calculate the final
digest and record it, final size, signer and timestamp in a separate private
signed-artifact receipt. The original build receipt describes the unsigned
bytes and must not be presented as the signed artifact's hash:

```powershell
Get-FileHash -LiteralPath $JobTrackerSetup -Algorithm SHA256
```

This signs only the finished Setup. Signing the application would have to occur
before staging and input hashing; signing the outer Setup does not sign its
embedded application or uninstaller. Timestamping contacts the selected service
on the signing machine; it adds no installation-time network dependency.
No signing or publication is performed by the normal build. Use an
owner-authorized certificate for actual signing.
The [Microsoft SignTool reference](https://learn.microsoft.com/en-us/windows/win32/seccrypto/signtool)
defines these options. Signing does not establish redistribution rights.

