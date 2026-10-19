# Build And Verification

This is the authoritative reference for application configuration, presets,
targets and development runtime preparation. Use the [testing policy](testing.md)
for test selection and the [installer navigation](../../Installer/README.md) for
packaging and platform deployment checks.

## Toolchain And Execution

Require C++20, CMake **3.22+**, and exactly **Qt 6.10.3**. Production requires
Quick/QML and Sql; optional test builds also require Qt Test. Windows 10/11 x64
uses Visual Studio 2022 / MSVC v143 with Qt's `msvc2022_64` kit. Linux uses Ninja
and the `gcc_64` kit. Native DEB support is limited to Ubuntu 24.04 amd64.

Run from the repository root, one configure/build/test command at a time,
checking the final exit code before continuing. Use PowerShell on Windows and
a native shell such as Bash on Linux. Apply the tool-access, process-wait and
failure-handling rules in
[cmake-build-debug](../../.agents/skills/cmake-build-debug/SKILL.md).

CMake 3.22 is the intended minimum, not a claim of execution on that version.
Preset schema 3 is compatible with that minimum.

## Shared Presets And Local Qt Selection

`CMakePresets.json` is maintained and portable. Its `CMAKE_PREFIX_PATH` reads
`QTDIR`, which must name the kit containing `bin/` and `lib/cmake/`, not the
parent Qt version directory. Illustrative selections:

```powershell
$env:QTDIR = 'C:/Qt/6.10.3/msvc2022_64'
```

```bash
export QTDIR=/path/to/Qt/6.10.3/gcc_64
```

The real `CMakeUserPresets.json` is optional, local and ignored. Preserve local
preferences on both platforms; common commands do not depend on them. Local
presets may inherit a shared preset and override `CMAKE_PREFIX_PATH`. User
presets implicitly include the shared file. An inherited local preset normally
shares its parent's build directory: do not switch SDKs in a populated cache.
Inspect `CMAKE_PREFIX_PATH`, `Qt6_DIR` and the resolved Qt component paths after
configuration. If an incompatible cache cannot safely be reused, preserve it
and configure the shared preset with an explicit fresh `-B build/<new-name>`;
then build that exact directory rather than the preset's original directory.

| Configure/build preset | Directory below `build/qt-6.10.3/` | Configuration | Tests | DEB |
| --- | --- | --- | --- | --- |
| `windows-debug` | `windows-debug` | Debug (multi-config) | OFF | N/A |
| `windows-release` | `windows-release` | Release (multi-config) | OFF | N/A |
| `linux-debug` | `linux-debug` | Debug | OFF | OFF |
| `linux-release` | `linux-release` | Release | OFF | OFF |
| `linux-deb-local-test` | `linux-deb-local-test` | Release | OFF | ON, local-test ON |

Optional `windows-debug-tests`, `windows-release-tests`, `linux-debug-tests`
and `linux-release-tests` have matching configure/build/test presets and
separate directories of the same names. Only these set tests ON. Ordinary
Linux test presets inherit explicit DEB OFF from `linux-base`; packaging has
its own production cache. Reconfiguring `linux-release` explicitly restores
DEB OFF even if an earlier invocation enabled it. Preserved stale CPack files
are not authority to package an ordinary cache; the wrapper checks the live cache.

## Windows Build

Configure after CMake/preset/options/source-registration changes or when the
build tree is absent or stale:

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug --target JobTrackerApp
cmake --preset windows-release
cmake --build --preset windows-release --target JobTrackerApp
```

Omit `--target JobTrackerApp` for a full production preset build when the
validation scope requires it. Visual Studio presets select x64; the build
preset selects Debug or Release. `QT_QML_DEBUG` and MSVC Just My Code stay
Debug-only. Normal builds do not stage, package, clean or invoke `windeployqt`.

### Local Development Runtime And Startup

After the first build or a Qt/QML dependency change, explicitly prepare the
Windows development runtime beside the Debug executable:

```powershell
cmake --build --preset windows-debug --target JobTrackerApp_dev_runtime
```

This opt-in target builds the application if needed and uses the matching
SDK's `windeployqt` for Qt/QML/platform/SQLite dependencies, without compiler
runtime deployment. It preserves existing files. Debug output is not a
redistributable Release package; a populated old directory alone is not proof
of current deployment.

```powershell
& .\build\qt-6.10.3\windows-debug\Debug\JobTrackerApp.exe
```

Launching uses the account's persistent JobTracker data, including startup CV
recovery. Use a disposable account when personal data must remain untouched.
Build/runtime preparation and application startup are separate evidence.

## Linux Build

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug --target JobTrackerApp --parallel 1
cmake --preset linux-release
cmake --build --preset linux-release --target JobTrackerApp --parallel 1
```

Each single-config Ninja tree selects its matching build type. One build job is
suitable for a memory-constrained host; it does not change preset settings.
Normal builds use build-tree Qt lookup/embedded QML and do not install or clean
output. The [Linux guide](../../Installer/Linux/README.md) owns explicit fresh
staging and the dedicated `linux-deb-local-test` package workflow. Installed
paths remain `/opt/jobtracker` and `/usr/share` desktop resources. Staging uses
an absolute separate `DESTDIR`, never a changed runtime prefix.

## Optional Tests

Production must never depend on test code, targets, Qt Test, fixtures, probes,
or test-only hooks. With tests OFF, no test-only package discovery, compilation,
registration or deployment is allowed. Standalone deployment consumers remain
separate CMake projects under `tests/deployment/`.

Select the relevant registered target/suite from [testing.md](testing.md).
Examples (not a requirement to run these for every change):

```powershell
cmake --preset windows-debug-tests
cmake --build --preset windows-debug-tests --target JobTrackerCommonTests
ctest --preset windows-debug-tests -R "^JobTrackerCommonTests$"
```

```bash
cmake --preset linux-debug-tests
cmake --build --preset linux-debug-tests --target JobTrackerCommonTests --parallel 1
ctest --preset linux-debug-tests -R '^JobTrackerCommonTests$'
```

Substitute the matching Release test presets for Release coverage. Omit the
build target and CTest filter only when the validation policy requires the
full test-enabled build/suite. A zero-test selection is `no matching tests`,
not PASS. Build/distribution verification does not imply business/UI testing.

## Preset Inspection And Validation

```bash
cmake --list-presets
cmake --list-presets=all
ctest --list-presets
```

Host conditions filter configure presets. A build/test name listed on another
OS does not prove it can execute there. Use
[test-and-review](../../.agents/skills/test-and-review/SKILL.md) for sufficient
verification and documentation impact. Reuse successful unchanged evidence;
rerun affected checks after changed inputs or diagnosed failures. Preserve
caches and failed output. Compile/link, package inspection, extracted runtime,
native GUI and clean-target installation are distinct results.
