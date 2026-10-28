# JobTracker Installers

## Windows installation model

The single offline Windows Setup supports independent copies in separate
user-selected folders on any suitable local drive. Installation scope chooses
default paths and shortcut scope; another copy or scope does not block the
selected destination. Each copy has its own runtime, local ownership metadata,
shortcuts, and `unins000.exe`. No JobTracker uninstall registration is created,
so remove a copy with its local uninstaller rather than Windows Installed apps.

Same-folder reinstall repairs or updates the selected copy. A reliably known
newer version rejects a same-folder downgrade; it does not prevent installing
an older version elsewhere. Reinstall after manual deletion or repair a copied,
moved, or partially deleted folder to establish correct local bookkeeping.
Unknown files and other copies are preserved. Folders must not contain one
another or overlap the shared personal-data directory.

All copies run by one Windows account share
`%LOCALAPPDATA%\JobTracker\Data`. Setup never changes this data. Uninstall keeps
it by default and always keeps it silently. An interactive, explicit default-No
confirmation can delete exactly the displayed user's `Data` folder, including
when elevated, while copies run, or when program removal was incomplete.
Cleanup starts after confirmation and reports actual Windows failures; it does
not delete the parent `JobTracker` folder or originals stored outside `Data`.
Program removal follows immediately. If owned-file removal is incomplete, it
reports failure and retains the local uninstaller/bookkeeping for retry or
repair; this does not change the separately reported data-cleanup result.
See the [Windows lifecycle and data guide](../For-Agent/Docs/installer.md#scope-runtime-and-maintenance)
for account identity, moved folders, shortcuts, and partial-removal details.

## One-command Release installers

The native launchers configure, build `JobTrackerApp` with tests OFF, and call
the existing staging/packaging implementation sequentially. They never install,
launch, download dependencies, run tests, or prepare a development runtime.
They locate this checkout from their own location, so an absolute script path
also works from another working directory. Run Linux as an ordinary user.

After the local setup below, run from the repository root:

```powershell
pwsh -NoProfile -File ./Installer/Windows/Create-Installer.ps1
```

For trusted, reviewed scripts transferred through an archive and blocked by
PowerShell execution policy (for example, `RemoteSigned`), use this alternative:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File ./Installer/Windows/Create-Installer.ps1
```

Run the appropriate Windows command, not both, from the repository root.
`Bypass` applies only to the newly started PowerShell process and its children;
it does not change persistent execution-policy settings or remove file zone
metadata. It is unnecessary when execution policy already permits the scripts.

```bash
bash ./Installer/Linux/Create-Installer.sh
```

### One-time local setup

Install the existing platform prerequisites documented below. The launchers
reuse `CMakeUserPresets.json`, `QTDIR`, Inno's existing PATH discovery and the
selected Visual Studio instance's local redistributable discovery. Existing
user presets are never rewritten. When those defaults suffice, no extra
Windows configuration is required. For manual commands using trusted transferred
scripts blocked by `RemoteSigned`, follow the dedicated-session guidance in the
[Windows build sequence](../For-Agent/Docs/installer.md#build-sequence).

Optional machine overrides live outside `build/`, in ignored local files:

```powershell
Copy-Item ./Installer/Windows/Create-Installer.example.psd1 ./Installer/Windows/Create-Installer.local.psd1
```

```bash
cp ./Installer/Linux/Create-Installer.example.conf ./Installer/Linux/Create-Installer.local.conf
```

Edit only needed settings, replacing the clearly marked placeholders. Windows
uses a PowerShell data file; Linux sources a trusted local Bash file.

| Windows data key | Linux variable | Purpose |
| --- | --- | --- |
| `ConfigurePreset` | `INSTALLER_PRESET` | Existing production configure preset. Default: installed matching `windows-release-local` / `linux-deb-release-local`, otherwise shared `windows-release` / `linux-deb-release`. |
| `QtRoot` | `INSTALLER_QT_ROOT` | Qt 6.10.3 kit containing `bin/` and `lib/cmake/Qt6/`; optional when the preset or `QTDIR` supplies it. |
| `IsccPath` | — | Existing local Inno 7+ `ISCC.exe` when absent from PATH. |
| `RedistributablePath` | — | Existing local Microsoft x64 redistributable matching the unchanged runtime lock when VS discovery is insufficient. |
| — | `INSTALLER_MAINTAINER` | **Real confirmed** `Name <email>` for a release DEB; required unless supplied by the preset or `JOBTRACKER_DEB_MAINTAINER` environment variable. No placeholder or local-test fallback. |
| `Parallel` | `INSTALLER_PARALLEL` | Optional build job count (1–1024); otherwise retain CMake/environment defaults such as `CMAKE_BUILD_PARALLEL_LEVEL`. |

Native Linux support remains Ubuntu 24.04 amd64. Supply the Qt kit through
`QTDIR`, `INSTALLER_QT_ROOT` in the ignored local configuration, or a compatible
DEB preset's `CMAKE_PREFIX_PATH`. `INSTALLER_PRESET` is optional; leave it unset
to preserve automatic selection. A custom local DEB preset must exist and inherit
`linux-deb-release`. The ordinary `linux-release-local` preset is not a DEB
substitute, and a Qt path configured only there is not automatically reused.
No ordinary `linux-release` build or separate `Stage.sh` invocation is required.
Maintainer information is local release metadata; the repository supplies no
real identity.

### Overrides and preset selection

Use `-Help` / `--help` for concise options. Windows accepts `-ConfigurePreset`,
`-QtRoot`, `-IsccPath`, `-RedistributablePath`, `-Parallel`, `-ConfigFile`, and
`-PassThru`. Linux accepts `--preset`, `--qt-root`, `--maintainer`, `--parallel`,
and `--config`. An explicitly selected missing/invalid preset or SDK fails;
it never silently falls back. Tool/SDK paths in settings are absolute or
relative to the repository; an explicit configuration-file path is relative to
the caller. Paths may contain spaces and non-ASCII characters.

Precedence is command line, then local file, then existing preset/environment
defaults. For Qt those defaults are the inherited preset `CMAKE_PREFIX_PATH`
(including its environment expansion), then its usual `QTDIR` input. For the
maintainer they are preset `JOBTRACKER_DEB_MAINTAINER`, then the same-named
environment variable. Tool discovery remains as described above. Explicit Qt
overrides replace the preset prefix for this configure only; conflicting
explicit `Qt6_DIR` or cached Qt component paths are rejected.

Launchers inspect self-contained shared/user presets following the repository's
schema-3 convention, including inheritance and common environment/source macros.
Preset includes/unsupported macros fail clearly; use a local preset in
`CMakeUserPresets.json` inheriting the dedicated shared Release preset.
The resolved `binaryDir` must be inside this checkout's `build/`. A build-tree
exclusive lock prevents overlapping launchers from configuring/building it.
Avoid running manual builds against that tree concurrently. Incompatible caches
are preserved: select a local preset with a fresh `binaryDir` or restore matching
settings instead of deleting or silently switching the cache.
When a same-named build preset exists, it must select that configure preset
(and Release on Windows); its environment/native settings are retained. Without
a pair the launcher builds the resolved directory with the configure preset's
environment. It never builds a different preset's tree.

On Linux, literal `null` environment entries are removed from configure/build
child processes, including paired builds. Build overrides and
`inheritConfigureEnvironment` determine the effective build entries; an empty
string remains defined. A selected preset or its ancestors must not reference a
removed variable through `$env{...}` or `$penv{...}` macros. The launcher rejects
that combination before configuration, because clearing the parent environment
could otherwise change CMake's macro expansion or select another build directory.
This conservative check scans the selected ancestry as written, including
overridden fields and metadata. Use independent preset values for those
paths/settings and avoid references to removed variables in that ancestry.

### Files, results and failures

Root `project(JobTracker VERSION ...)` in `CMakeLists.txt` remains the sole
application version source. Each invocation reconfigures and builds current
Release sources incrementally, then uses fresh private staging/work. UTC date
and time (`yyyyMMddTHHmmssZ`) plus a random suffix distinguish repeated builds:

```text
Installer/Windows/output/JobTracker-Setup-0.1.0-20261021T120000Z-0123456789ab.exe
Installer/Linux/output/jobtracker_0.1.0-1_amd64_20261021T120000Z_0123456789ab.deb
```

Finished files are directly in those output directories. Atomic, non-overwriting
publication exposes only validated bytes; previous files/directories remain.
The distribution suffix does not change application/installer identity, DEB
Version/revision, or upgrade behavior.

Launcher logs and result records are under
`build/installer-create/windows-<UTC-time>-<id>/` or `linux-<UTC-time>.<id>/`.
Windows prints staging logs under `build/installer-stage-<id>/` and builder
`compile.log` / `installer-build.json` under `build/installer/<id>/`.
Its launcher `result.json` and optional `-PassThru` object identify the exact
installer, build, stage, compiler and evidence. Linux prints its private
`build/linux-deb/create-<run-id>/package/` directory with `cpack.log`,
`dependency-check/`, `package.sha256`, and a `result.txt` key=value record
(version, packageVersion, installer, size, sha256, evidence, log, provenance,
provenanceSha256). The linked [Linux input receipt](Linux/README.md#package-input-provenance)
identifies the source/build inputs without requiring Git. That record is
also copied to the launcher log directory. Checksums name the actual final DEB.
Neither workflow selects a newest file from shared output.

Missing tools/configuration fail before expensive work when possible. A failed
required process stops the sequence with nonzero exit status, the failed step
and log path; private failed evidence remains. No success result is returned.
Linux final publication requires work/output on the same filesystem (an atomic
hard link); a separate mounted output filesystem fails without copying partial
bytes. These safeguards do not replace the separate acceptance procedures.

## Existing manual workflows

- **Windows 10/11 x64:** [Windows offline installer guide](../For-Agent/Docs/installer.md).
  [Windows/Stage-Release.ps1](Windows/Stage-Release.ps1) stages the production
  Release build; [Windows/Build-Installer.ps1](Windows/Build-Installer.ps1)
  verifies and embeds Qt/QML/SQLite and pinned local MSVC inputs in one offline
  Setup. Each invocation writes
  `Installer/Windows/output/<run-id>/JobTracker-Setup-<version>.exe`; private evidence
  stays under `build/`. See [Windows installer checks](../For-Agent/Docs/installer-testing.md).
- **Ubuntu 24.04 amd64:** [Linux runtime and DEB guide](Linux/README.md).
  Choose `linux-deb-local-test` for placeholder engineering metadata or
  `linux-deb-release` for a confirmed release maintainer, then invoke
  `bash Installer/Linux/Package.sh ...` with automatic fresh directories.
  Separate runtime staging uses `linux-release` and
  `bash Installer/Linux/Stage.sh ...`; it is not a prerequisite for packaging.
  Run from the repository root. Work/staging stays under
  `build/linux-deb/`; final DEBs go under `Installer/Linux/output/<run-id>/`.
  See [Linux deployment checks](../tests/deployment/linux/README.md).

These manual packaging workflows consume an already built Release application and preserve old
outputs. Qt is private in both payloads. The Ubuntu DEB may still require
network access for missing declared system dependencies. Local-test packages use
placeholder metadata; release packages require explicitly supplied metadata.

[Shared build guidance](../For-Agent/Docs/build.md) owns action and local/shared
preset selection. Use the [Windows](../For-Agent/Docs/build-windows.md) or
[Linux](../For-Agent/Docs/build-linux.md) reference for the relevant OS's SDK,
command, and development-runtime details.
