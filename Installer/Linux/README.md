# Linux Release runtime and DEB

The native package target is **Ubuntu 24.04 amd64**, using exactly Qt 6.10.3
from the shared Online Installer `gcc_64` kit. Other distributions,
architectures, cross builds and multi-config packaging are unsupported.
Local-test DEBs use placeholder metadata; release DEBs require an explicitly
supplied, confirmed maintainer. An extracted-package SQL probe provides
developer-host runtime evidence only.

Use [build.md](../../For-Agent/Docs/build.md) for shared action and preset
selection, and [build-linux.md](../../For-Agent/Docs/build-linux.md) for Linux
toolchain, ordinary build/test commands, and development-runtime details. Use the
[Linux deployment checks](../../tests/deployment/linux/README.md) for artifact
integrity, disposable-target installation and package lifecycle procedures.

## Requirements

C++20, CMake/CPack **3.22+**, Ninja, Bash, the matching Qt Quick/QML/Sql SDK,
GNU core utilities, `dpkg`, `dpkg-deb`, `dpkg-shlibdeps`, `file` and `readelf`.
`desktop-file-validate` supports separate desktop validation. No Qt Test,
standalone probe, Python or image converter is required by production
configure/build/stage/package commands. Debian packaging tools are required
only for the opt-in DEB workflow, not ordinary application builds.
CMake adjusts installed runtime paths; `patchelf` is not a prerequisite.
The minimum CMake version is not itself a claim of testing on 3.22.

The one-command installer launcher requires `flock` to prevent overlapping
installer runs against the same build tree and stops if it is unavailable.
This is a launcher prerequisite; ordinary application compilation does not
require it.

Preserve LF line endings for the Bash scripts. Invoke `Stage.sh` and `Package.sh`
through `bash`, so transferring sources through ZIP does not require preserved
executable bits.

Set `QTDIR` to the installed kit containing `bin/` and `lib/cmake/`, then run
commands sequentially from the repository root as an ordinary user. Stop on
failure and retain the command, exit code and logs. Do not use sudo for
configuration, builds, staging or packaging. Local user presets are optional
and ignored; shared presets are the documented workflow.

## One-command release DEB

Use the [one-time launcher setup](../README.md#one-time-local-setup): install
the existing requirements and configure a real confirmed `Name <email>`.
Supply the Qt kit through `QTDIR`, `INSTALLER_QT_ROOT` in the ignored local
configuration, or a compatible DEB preset's `CMAKE_PREFIX_PATH`.
Copy `Create-Installer.example.conf` to the ignored
`Create-Installer.local.conf` only when local overrides are needed; fill
`INSTALLER_MAINTAINER` unless a release preset or
`JOBTRACKER_DEB_MAINTAINER` environment value already supplies it.

Leave `INSTALLER_PRESET` unset to preserve automatic preset selection. A custom
local DEB preset must exist and inherit `linux-deb-release`. The ordinary
`linux-release-local` preset is not a DEB substitute; a Qt path configured only
there is not automatically reused by this launcher.

```bash
bash ./Installer/Linux/Create-Installer.sh
```

The launcher also works by absolute path from another working directory. It
configures `linux-deb-release` (or a compatible local descendant), builds
`JobTrackerApp` with tests OFF and `JOBTRACKER_DEB_LOCAL_TEST=OFF`, then calls
`Package.sh`. No ordinary Release build, standalone stage, tests, installation,
or application launch is added. Missing real metadata fails before configuring;
there is no local-test fallback. `--help` lists `--preset`, `--qt-root`,
`--maintainer`, `--parallel` and `--config` overrides. Command line overrides
the local configuration, which overrides existing preset/environment defaults.
The [shared launcher reference](../README.md#overrides-and-preset-selection)
documents precedence, cache preservation and exclusive build-tree locking.

The completed DEB goes directly into `Installer/Linux/output/`, for example
`jobtracker_0.1.0-1_amd64_20261021T120000Z_0123456789ab.deb`. Date/time is UTC;
the random suffix prevents same-second collisions. Filename suffixes do not
change the package Version/revision. Root `CMakeLists.txt` owns the version.
Logs/result live under `build/installer-create/linux-<UTC-time>.<id>/`;
the printed `build/linux-deb/create-<run-id>/package/` contains CPack/dependency
evidence, `package.sha256`, and `result.txt` identifying the exact final path,
version, size and hash. Use that result, never the newest file in output.
Failure stops immediately, returns nonzero, identifies its step/log and retains
private evidence. No old outputs or caches are deleted.

The manual commands below and manual installation procedures remain available.

## Build and stage

For a standalone runtime stage, build the ordinary production Release
configuration (tests OFF, DEB OFF). To build a DEB, go directly to
[Build a DEB](#build-a-deb); neither this ordinary build nor `Stage.sh` is a
prerequisite for packaging.

```bash
cmake --preset linux-release
cmake --build --preset linux-release --target JobTrackerApp --parallel 1
```

After a successful build, use automatic fresh staging:

```bash
bash Installer/Linux/Stage.sh build/qt-6.10.3/linux-release
```

The wrapper atomically allocates `build/linux-deb/stage-<run-id>/` and stages
into its `stage/` child. `install.log` and `paths.txt` stay beside that child.
The original explicit form remains supported (choose a new path each time):

```bash
bash Installer/Linux/Stage.sh build/qt-6.10.3/linux-release 'build/linux-deb/my fresh stage'
```

An explicit stage is reserved at that exact location. Its log/evidence uses a
separate fresh run directory under `build/linux-deb/`. Both forms print exact
absolute paths. The build-directory argument is always required; the wrapper
never selects an arbitrary newest build. Staging does not rebuild.

Validation requires the matching source root, Ninja, native Ubuntu 24.04
amd64, Release, tests OFF, the Qt 6.10.3 kit, and the installed layout. Existing,
empty, outside-root and symlink output paths are rejected. `Common.sh` shares
these checks and directory/log handling between the two Linux wrappers.
Parents may be created, but the selected output itself must not exist. Spaces
are supported. Failed runs and previous artifacts are never cleaned up.

CMake installation uses `--config Release`, `--prefix /opt/jobtracker` and a
separate absolute `DESTDIR`; never configure the stage path as the runtime
prefix. The stage contains:

- `opt/jobtracker/bin/JobTrackerApp`, relative `bin/qt.conf`, private `lib/`,
  `plugins/`, `qml/`, `translations/` and existing notices.
- `usr/share/applications/jobtracker.desktop` and the seven maintained icons
  under `usr/share/icons/hicolor/<size>/apps/jobtracker.png`.

Full Linux installation without an absolute non-root `DESTDIR` is rejected.
For everyday Debug work, use the canonical
[routine workflow](../../For-Agent/Docs/build.md#routine-codex-workflow),
[Linux development runtime](../../For-Agent/Docs/build-linux.md#linux-development-runtime-and-startup),
and optional [fresh runtime](../../For-Agent/Docs/build.md#fresh-development-runtime).

## Build a DEB

Both package presets use Release, tests OFF and packaging ON, with independent
build directories. `Package.sh` invokes the same installation/runtime/desktop
rules through CPack, so no separate ordinary Release build or staging run is
needed. Shared presets use `QTDIR` as described above.

### Build a local-test DEB

Use the dedicated configure/build pair. It inherits production Release
settings but has its own cache at `build/qt-6.10.3/linux-deb-local-test`:

```bash
cmake --preset linux-deb-local-test
cmake --build --preset linux-deb-local-test --target JobTrackerApp --parallel 1
bash Installer/Linux/Package.sh build/qt-6.10.3/linux-deb-local-test
```

This selects Release, `JOBTRACKER_BUILD_TESTS=OFF`,
`JOBTRACKER_ENABLE_DEB_PACKAGING=ON` and `JOBTRACKER_DEB_LOCAL_TEST=ON`.
Ordinary Linux production and test presets explicitly inherit packaging OFF.
Do not turn the everyday `linux-release` cache into the package build.

### Build a release DEB

The `linux-deb-release` configure/build pair uses
`build/qt-6.10.3/linux-deb-release` and explicitly sets
`JOBTRACKER_DEB_LOCAL_TEST=OFF`. Supply the confirmed `Name <email>` through
`JOBTRACKER_DEB_MAINTAINER`; no release identity is provided by the repository:

```bash
read -r -p 'Confirmed DEB maintainer (Name <email>): ' deb_maintainer
cmake --preset linux-deb-release "-DJOBTRACKER_DEB_MAINTAINER=$deb_maintainer"
cmake --build --preset linux-deb-release --target JobTrackerApp --parallel 1
bash Installer/Linux/Package.sh build/qt-6.10.3/linux-deb-release
```

Alternatively, set `JOBTRACKER_DEB_MAINTAINER` in `cacheVariables` of a local
configure preset inheriting `linux-deb-release`, with a matching build preset.
Missing or malformed maintainer data causes a configuration error; the release
preset never switches to local-test mode. Confirm publisher/licensing information
and review existing notices separately before distribution; supplying a maintainer
does not establish release acceptance.

### Package outputs

The launcher supplies the backward-compatible `--timestamped-output` switch
to `Package.sh`. This mode accepts the build and optional fresh work argument,
with no output-directory argument, and publishes directly into the shared
`Installer/Linux/output/`. The positional manual forms below retain their
per-run output directories.

With work/output arguments omitted, `Package.sh` reserves unique directories
under `build/linux-deb/package-<run-id>/` and `Installer/Linux/output/package-<run-id>/`.
The work and output identifiers may differ; use the printed absolute paths.
The original explicit form also remains supported:

```bash
bash Installer/Linux/Package.sh build/qt-6.10.3/linux-deb-local-test 'build/linux-deb/my package work' 'Installer/Linux/output/my package result'
```

You may supply only the work argument and let the final output be automatic.
Each explicit destination must be fresh and strictly below its allowed root:
`build/linux-deb/` for work and `Installer/Linux/output/` for the final package.
Repeated same-version packaging automatically preserves all earlier packages.
The wrapper checks native command exits and final bytes, refuses existing
outputs and symlinks, and leaves failed work/logs available. It never rebuilds,
downloads, installs or launches the application. Reconfigure/build first after
changing build inputs; an existing executable alone does not prove freshness.
Timestamped mode publishes the completed, CPack-validated DEB with an exclusive
hard link. In that mode work/final output must be on the same filesystem; a
cross-device link fails without a partial final file. Legacy mode retains its
exclusive copy and supports output on a different filesystem. Neither mode
overwrites a collision.

CPack uses the same install/runtime/desktop rules with `CPACK_SET_DESTDIR=ON`
and `/opt/jobtracker`; absolute `/usr/share` destinations remain alongside
`/opt` in its stage. The live-cache guard rejects Debug/test-enabled inputs and
ordinary caches. `cpack -C Release` cannot convert a Debug build. Use the
wrapper instead of the generated `package` target, so work stays in a fresh
reserved directory.

Results and evidence:

- Final DEB: `Installer/Linux/output/<run-id>/jobtracker_<version>-<revision>_amd64.deb`;
  local-test packages append `~localtest` to the revision.
- Work: `cpack.log`, `paths.txt`, `package.sha256`, CPack's package/stage copy and
  `dependency-check/` under the printed `build/linux-deb/` run.
- `result.txt`: UTF-8 key=value fields `version`, `packageVersion`, `installer`,
  `size`, `sha256`, `evidence`, `log`. Both output modes record the actual final
  filename and bytes; the launcher copies this record to its log directory.
- `dependency-check/stage-root.txt` identifies the exact CPack stage;
  `dpkg-shlibdeps.log`, `elf-files.txt` and `system-depends.txt` retain the
  strict dependency evidence. Package stage and final paths are printed.

Root `project(VERSION)` owns the application version. The separate
`JOBTRACKER_DEB_REVISION` defaults to `1`. Local-test mode retains the existing
placeholder `Unspecified local-test maintainer <local-test@invalid>` and an
explicit local-engineering description; this is not a public release identity.
Release mode uses the supplied maintainer and omits the `~localtest` suffix and
local-engineering description. Both modes retain the same validation and
packaging safeguards.

## Runtime and dependency policy

The official Qt QML deployment API supplies the imported QML modules, their
plugins and linked Qt libraries. Installation verifies required Qt 6.10.3,
QML, SQLite, XCB/Wayland, translations and existing SDK notice files. A second
scan checks every shipped ELF for missing/conflicting dependencies and rejects
Qt/ICU resolution outside the private payload. The executable uses
`$ORIGIN/../lib`; plugin loader paths are relative. `qt.conf` uses `Prefix = ..`.
No SDK path or system Qt package is needed by the delivered private runtime.

Qt and ICU libraries are private. The ELF loader, glibc, C++ runtime, graphics
drivers and desktop/system libraries remain system dependencies. The strict
`dpkg-shlibdeps` pass maps only bundled Qt/ICU SONAMEs to this package, removes
that self-dependency and requires system-library metadata. The final Depends
must exactly match the strict all-ELF scan plus `libssl3t64 (>= 3.0.0)` for the
dynamically loaded OpenSSL backend. Apt may need network access for missing
system dependencies; the DEB is not an OS-independent offline distribution.

SQLite is the only SQL driver. Existing plugin selection retains Compose/IBus,
GTK/desktop-portal themes, XCB/Wayland and GIF/ICO/JPEG/SVG/WebP. PNG support is
built into Qt Gui. TIFF, Lottie and VirtualKeyboard remain excluded. The
Qt 6.10.3 Linux deployment workaround excludes whole categories and includes
selected plugins explicitly; Qt's system-directory exclusion policy remains.

Existing Qt license text and six SDK module SBOMs remain in the private
payload. They describe upstream modules, not a new release/legal audit.
No SDK, source tree, tests, probe or personal database/CV belongs in the DEB.
Installed files remain separate from the unchanged per-user
`QStandardPaths::AppLocalDataLocation/Data` storage location.

## Inspection and target checks

Use the **exact** path printed by the packaging invocation:

```bash
production_deb=/absolute/path/printed/by/Package.sh/jobtracker_VERSION_amd64.deb
sha256sum -- "$production_deb"
dpkg-deb --info "$production_deb"
dpkg-deb --contents "$production_deb"
dpkg-deb --field "$production_deb"
desktop-file-validate Installer/Linux/jobtracker.desktop
```

Inspect root:root archive ownership, safe modes/symlinks, relative loader paths,
private Qt/QML/plugins, system dependencies, desktop entry/icons and absence of
source/SDK/tests/personal data. The independent
[QSQLITE probe](../../tests/deployment/linux/README.md#standalone-qsqlite-probe)
may run beside the unchanged `qt.conf` in a fresh extraction of that exact DEB.
It must load private Qt/ICU/SQLite and pass its in-memory query. It is never
linked into production or shipped. This does not prove clean-machine operation.

The desktop entry uses `Icon=jobtracker` and direct
`/opt/jobtracker/bin/JobTrackerApp` Exec/TryExec paths. The 16/24/32/48/64/128/256
PNG icons retain the corresponding embedded bytes of `../Windows/JobTracker.ico`;
normal builds need no image conversion. Standard Ubuntu desktop/icon triggers
handle integration; JobTracker adds no custom maintainer scripts or cache files.

Do not install, reinstall, upgrade, remove or purge JobTracker on the developer
laptop. Follow the [separate disposable-target guide](../../tests/deployment/linux/README.md)
when authorized: clean dependency resolution, installed QSQLITE, main QML and
private loaded paths, real native Wayland, XCB through XWayland, actual menu
launch, same-version reinstall, ordered upgrade, remove/purge and synthetic
data preservation. Keep functional/UI regression separate from deployment
checks. Never report build/static/extracted-runtime evidence as those native
desktop or lifecycle checks.
