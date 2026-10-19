# Windows Offline Installer

This reference describes the existing native x64 installer for Windows 10/11.
It produces one `Installer/output/<run-id>/JobTracker-Setup-<version>.exe` containing the
Release application, Qt/QML/plugins/SQLite, scope-specific MSVC inputs, and the
runtime notice. Installation requires no Qt/Visual Studio SDK, application
archive, remote manifest, or dependency download.

Use [build.md](build.md) for the application Release build and backend tests.
Use [installer-testing.md](installer-testing.md) for standalone verification and
the reusable acceptance checklist. These are procedures and implementation
contracts, not a record of results for any particular candidate.

## Implementation Files

All paths below are relative to [Installer/](../../Installer/README.md).

| File | Responsibility |
|---|---|
| `JobTracker.iss` | Inno Setup entry point: native x64 Setup, EN/RU UI, scopes, shortcuts, and lifecycle callbacks. |
| `Stage-Release.ps1` | Fresh direct CMake Release staging, shared build/payload validation, and private staging evidence. |
| `Build-Installer.ps1` | Validates direct staging and generated identity, prepares local runtime inputs, snapshots bytes, generates explicit hashed file entries, invokes ISCC, and records private build evidence. |
| `PayloadValidation.ps1`, `payload-files.txt` | Canonical exact payload inventory and validation of production identity/hash, Qt version, x64 Release PE files, safe paths, relative metadata, reparse points, and alternate streams. Shared by the builder and standalone tests. |
| `PayloadSafety.iss` | Native destination/local-file safety checks used by Setup. |
| `Prepare-MsvcRuntime.ps1`, `MsvcRuntime.lock.json` | Verify and snapshot pinned local Microsoft inputs against the selected toolset; generate runtime include/evidence. |
| `MsvcRuntime.iss`, `MSVC-NOTICE.txt` | App-Local/system runtime policy and delivered Microsoft runtime notice. |
| `InstallationPolicy.iss` | Registration/version/path validation, maintenance/coexistence rules, and shared install/uninstall lock. |
| `ManualClose.iss` | Exact installed-executable closure checks and native cancellation gates; never terminates or restarts JobTracker. |
| `PersonalData.iss` | Default-preserve uninstall and guarded optional CurrentUser data cleanup. |
| `JobTracker.rc.in`, `JobTrackerIdentity.cmake.in`, `JobTrackerIdentity.iss.in` | CMake-generated Windows resource and packaging identity templates. |
| `JobTracker.ico` | Current temporary seven-size application/Setup/shortcut icon. |
| `output/` | Ignored generated customer Setup executables; source files above remain versionable. |

## Tools And Local Inputs

- Windows x64 application toolchain from [build.md](build.md), including exactly
  Qt 6.10.3, Visual Studio 2022/MSVC v143, and CMake 3.22+.
- PowerShell 7.2+ for the installer scripts.
- Inno Setup 7+ with native `SetupArchitecture=x64` support. The wrapper checks
  the finished Setup for AMD64/PE32+. Inno 6's I386 launcher is rejected even
  with an x64 payload/install mode.
  Native handle declarations use pointer-sized types for the x64 engine.
- A current x64 production Release build with tests OFF, its generated identity,
  a fresh direct CMake stage, and authorized local MSVC inputs matching
  [MsvcRuntime.lock.json](../../Installer/MsvcRuntime.lock.json).

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

First configure/build `windows-release` with your local `QTDIR`, following
[Windows Build](build.md#windows-build). The following commands run in the same
PowerShell 7.2+ session and consume that already built production Release tree.
They do not implicitly configure/build the application.

### Direct Release Staging

~~~powershell
$installerBuild = (Resolve-Path -LiteralPath build/qt-6.10.3/windows-release).Path
~~~

~~~powershell
$stageResult = & ./Installer/Stage-Release.ps1 -BuildDirectory $installerBuild -PassThru
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

The reviewed inventory lists 148 application/Qt files. Embedded QML resources
are allowed; loose C++ sources/headers, tests, PDBs, CMake files and internal
documents must not ship. Keep probes and logs outside the payload. The common
stage alone lacks the scope-specific MSVC inputs of the finished installer.

### Compile Setup

If ISCC.exe from Inno Setup 7+ is on PATH, use:

~~~powershell
$installerResult = & ./Installer/Build-Installer.ps1 -BuildDirectory $installerBuild -StageDirectory $installerStage -PassThru
~~~

If it is not on PATH, add `-IsccPath <absolute-path-to-ISCC.exe>` to that
invocation. An explicit invalid path fails without fallback. Discovery is
local only; the existing Inno version and native x64 output checks still apply.
To select another authentic local redistributable matching the lock, also add
`-RedistributablePath <absolute-path-to-vc_redist.x64.exe>`. Otherwise the
builder uses the selected Visual Studio instance's existing default described
above. No inputs are downloaded or silently repinned.

Every invocation reserves `Installer/output/<run-id>/` and writes exactly
`JobTracker-Setup-<version>.exe` inside it. Rebuilding the same version
requires no manual moving or deletion: earlier Setup bytes and evidence stay
in their own run directories. The builder retains stage/identity/hash checks,
verified private MSVC snapshots and explicit hashed Inno file entries. It
prints absolute result, compiler-log and evidence paths before compilation.

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
pwsh -NoProfile -File Installer/Prepare-MsvcRuntime.ps1 -BuildDirectory $installerBuild -RedistributablePath <absolute-path-to-vc_redist.x64.exe> -OutputDirectory "$installerRoot/runtime-input-check"
~~~

This verifies/copies local inputs only; it never executes the prerequisite.

## Version, Generated Files, And Evidence

Root `project(JobTracker VERSION ...)` in
[CMakeLists.txt](../../CMakeLists.txt) owns the application and Setup version.
Windows numeric metadata adds `.0`; the executable remains
`JobTrackerApp.exe`, independent of the QML module's version.
Reconfigure/rebuild after version or icon changes; do not edit generated identity.

Generated RC/CMake/Inno identity lives under
`<build>/Installer/<configuration>/`. Private snapshots, generated includes,
runtime copies, logs, and `installer-build.json` live under
`build/installer/<run-id>/`. JSON records local hashes, source state, compiler,
identity, and payload inputs; it is not a remote trust anchor or customer asset.
Only the versioned Setup belongs in its `Installer/output/<run-id>/` directory.
The publisher is blank and normal builds are unsigned.

Keep each Setup with its own private evidence. Changed payload, source, runtime,
or engine inputs require a matching candidate/evidence set for verification;
dated reports cannot prove that current bytes match.

## Scope, Runtime, And Maintenance

| Scope | Default location / registration | MSVC behavior |
|---|---|---|
| CurrentUser (default) | `%LocalAppData%/Programs/JobTracker`; HKCU; `JobTracker.CurrentUser` | Ten verified App-Local x64 Release DLLs beside the EXE; no machine prerequisite or admin rights. |
| AllUsers | `%ProgramFiles%/JobTracker`; HKLM; `JobTracker.AllUsers`; UAC | Check system x64 registry/DLL identity; extract and run the embedded official redistributable only when insufficient. |

Runtime failure/cancellation stops before application copying. Restart-required
results require reboot and rerunning Setup. A later installation failure or
JobTracker uninstall does not roll back/remove the shared machine runtime.

Fresh destinations are editable. EN/RU UI includes a language-selection dialog,
system-language selection with English fallback, Start Menu shortcuts, and an
unchecked desktop option. Same-scope newer-version upgrades and same-version
repair retain the validated registered directory, stable AppId, and personal
data. Downgrade, missing/inconsistent registry/PE/marker versions, unsafe paths,
and relocation fail closed.

An opposite-scope copy blocks every version, including equal versions.
Switch scope by manual uninstall with data preserved, then reinstall; existing
dual-scope layouts must remove the other copy first. AllUsers checks available
profiles/hives and rejects unreadable state without loading offline hives.
Separate AppIds do not isolate per-account SQLite/CV data.

Close JobTracker manually and keep it closed during install/repair/upgrade/
uninstall. Gates check the exact selected executable and recheck later
boundaries; Retry rechecks, and Setup never force-closes or restarts it.
Unattended Setup requires both `/VERYSILENT` and `/SUPPRESSMSGBOXES`;
unsafe automatic-close/INF overrides are refused. These guards are not a
transaction-wide file lock or a guarantee of full upgrade rollback.

## Personal Data

The usual per-account store is `%LocalAppData%/JobTracker/Data`, including
`database.sqlite` and `Resumes/`, independent of installation directory/scope.
Setup never starts JobTracker, opens its database, or performs migrations.

Uninstall preserves personal data by default. Only verified non-elevated
CurrentUser removal can offer EN/RU default-No cleanup after complete
program/shortcut removal and the no-restart gate. Explicit Yes permanently
deletes the entire verified owned `%LocalAppData%/JobTracker` tree, including
SQLite sidecars, CVs, and other files there. There is no command-line data path
or data-delete switch.

Silent, elevated/AllUsers, remaining-registration/shared, incomplete,
inaccessible, or ambiguous cases preserve data. AllUsers cleanup never resolves
a data folder or enumerates other profiles. The cleanup core pins ancestors,
checks SID ownership, and rejects reparse points, hardlinks, locked/read-only/
inaccessible files before setting deletion disposition. Removal uses validated
handles and never schedules reboot deletion. It is irreversible and
nontransactional: a late failure may leave partial deletion. Unknown manually
copied applications cannot be discovered exhaustively.

## Changes And Common Failures

When Qt, the MSVC toolset, runtime inputs, Inno Setup, or payload dependencies
change, review the affected inventory/identity/lock and source contracts
explicitly. Do not bypass validation or silently repin inputs. Rebuild matching
evidence and select the relevant [installer suites](installer-testing.md).
Recheck native cancellation behavior after an Inno upgrade; Check/BeforeInstall
exceptions alone do not abort its file-copy loop. Review required notices,
provenance, and entitlement separately from functional tests.

| Failure | Existing handling / next diagnosis |
|---|---|
| Missing or stale generated identity | Wrapper rejects; configure/build the intended production Release tree. |
| Inventory/hash/Qt/PE mismatch, unsafe path, reparse point or stream | Payload validation rejects; identify the changed input rather than relaxing checks. |
| Missing/mismatched runtime or incompatible toolset | Preparation reports required paths/version/integrity; supply authorized matching local inputs or explicitly review the lock/toolset. Discovery does not guarantee a usable input. |
| Existing or unsafe run directory | Wrapper refuses collisions and reparse points. Each invocation selects a fresh run directory, so earlier same-version Setup files remain unchanged. |
| Inno compilation or native identity failure | Retain `compile.log` and candidate state under private work; inspect compiler diagnostics and native x64 support. |
| Opposite scope, damaged identity, relocation, unreadable profile | Policy fails closed; resolve the actual installation state without weakening guards. |
| Running application or late sharing failure | Stop safely, retain logs and owned fixture state, and close the app cooperatively. A retry is not proof of a root-cause fix or atomic rollback. |
| DXC/DXIL deployment warnings | Investigate affected graphics/runtime dependencies and clean-host behavior; a successful developer-host probe alone is insufficient. |

Earlier local documentation recorded an intermittent Windows sharing error 32
without an established cause. If it recurs, collect a narrow trace for the exact
path and failing operation, timestamp, process identity, and access/share flags.
Do not force-close processes, blindly retry, or reuse historical recovery scripts.

## Distribution Review And Unavailable History

Existing repository documentation identified unresolved questions about Lottie
licensing, complete notices/source/replacement materials, ICU/Mesa/D3D binary
provenance, Inno redistribution, and owner entitlement. The former
`LICENSE_ISSUES.md` and `REDISTRIBUTION.md` reports and their referenced
supporting evidence are absent from this checkout. Their detailed conclusions
cannot be reconstructed from missing links; this cleanup does not resolve them.
Keep these concerns for a release-specific review before publication.
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
`$JobTrackerSetup` (the finished canonical `Installer/output/<run-id>/JobTracker-Setup-<version>.exe`).
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

