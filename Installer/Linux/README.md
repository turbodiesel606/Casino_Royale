# Linux Release runtime and DEB

The native package target is **Ubuntu 24.04 amd64**, using exactly Qt 6.10.3
from the shared Online Installer `gcc_64` kit. Other distributions,
architectures, cross builds and multi-config packaging are unsupported.
The local-test DEB uses placeholder release metadata. An extracted-package SQL
probe provides developer-host runtime evidence only.

Use the [build reference](../../For-Agent/Docs/build.md) for common presets,
optional tests and Qt selection. Use the
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
Preserve LF line endings for the Bash scripts. Invoke `Stage.sh` and `Package.sh`
through `bash`, so transferring sources through ZIP does not require preserved
executable bits.

Set `QTDIR` to the installed kit containing `bin/` and `lib/cmake/`, then run
commands sequentially from the repository root as an ordinary user. Stop on
failure and retain the command, exit code and logs. Do not use sudo for
configuration, builds, staging or packaging. Local user presets are optional
and ignored; shared presets are the documented workflow.

## Build and stage

Build the ordinary production Release configuration (tests OFF, DEB OFF):

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
Building `JobTrackerApp` alone only compiles/links. For routine development,
build the tests-OFF Debug application, run required tests in a separate Debug
test tree, then use `JobTrackerApp_dev_runtime` after final changes and successful
checks. It prepares a local application/runtime copy under the build tree's
`dev-runtime/<configuration>/` directory, as
documented in the [build reference](../../For-Agent/Docs/build.md#linux-development-runtime-and-startup).
That separate install component supports Debug and test-enabled trees and is
excluded from full installation/CPack. Its distinct deployment script includes
`qmltooling` for debugging/profiling; production keeps the tooling exclusions.
`QT_QML_DEBUG` remains Debug-only.

For an optional fresh local copy, use
`cmake --build --preset linux-debug --target JobTrackerApp_dev_runtime_fresh --parallel 1`.
It uses the same development component in a new
`dev-runtime-fresh/Debug/<run-id>/` directory and prints its exact path and launch
command. It copies no old runtime and is never invoked by ordinary builds or
packaging. Both development targets preserve relative RPATH and `qt.conf`, clear
inherited `DESTDIR` only for the child installation, and preserve existing outputs.
They do not force a full rebuild or create a Release distribution. The Release
staging/packaging guards and requirements in this guide remain unchanged.

## Build a local-test DEB

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

With work/output arguments omitted, `Package.sh` reserves unique directories
under `build/linux-deb/package-<run-id>/` and `dist/linux-deb/package-<run-id>/`.
The work and output identifiers may differ; use the printed absolute paths.
The original explicit form also remains supported:

```bash
bash Installer/Linux/Package.sh build/qt-6.10.3/linux-deb-local-test 'build/linux-deb/my package work' 'dist/linux-deb/my package result'
```

You may supply only the work argument and let the final output be automatic.
Each explicit destination must be fresh and strictly below its allowed root.
Repeated same-version packaging automatically preserves all earlier packages.
The wrapper checks native command exits and final copy bytes, refuses existing
outputs and symlinks, and leaves failed work/logs available. It never rebuilds,
downloads, installs or launches the application. Reconfigure/build first after
changing build inputs; an existing executable alone does not prove freshness.

CPack uses the same install/runtime/desktop rules with `CPACK_SET_DESTDIR=ON`
and `/opt/jobtracker`; absolute `/usr/share` destinations remain alongside
`/opt` in its stage. The live-cache guard rejects Debug/test-enabled inputs and
ordinary caches. `cpack -C Release` cannot convert a Debug build. Use the
wrapper instead of the generated `package` target, so work stays in a fresh
reserved directory.

Results and evidence:

- Final DEB: `dist/linux-deb/<run-id>/jobtracker_<version>-<revision>~localtest_amd64.deb`.
- Work: `cpack.log`, `paths.txt`, `package.sha256`, CPack's package/stage copy and
  `dependency-check/` under the printed `build/linux-deb/` run.
- `dependency-check/stage-root.txt` identifies the exact CPack stage;
  `dpkg-shlibdeps.log`, `elf-files.txt` and `system-depends.txt` retain the
  strict dependency evidence. Package stage and final paths are printed.

Root `project(VERSION)` owns the application version. The separate
`JOBTRACKER_DEB_REVISION` defaults to `1`. Local-test mode retains the existing
placeholder `Unspecified local-test maintainer <local-test@invalid>` and an
explicit local-engineering description; this is not a public release identity.
A future release must explicitly set `JOBTRACKER_DEB_LOCAL_TEST=OFF` and supply
confirmed metadata. No release preset or new legal metadata is provided here.

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
PNG icons retain the corresponding embedded bytes of `../JobTracker.ico`;
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
