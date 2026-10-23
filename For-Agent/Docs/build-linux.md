# Linux Build Reference

Read this file for Linux toolchain, preset, command, and runtime details.
Use [build.md](build.md#routine-codex-workflow) to select actions and their order;
the command catalog below is not a required sequence.

## Toolchain And SDK

Require a C++20 compiler, CMake **3.22+**, Ninja, and exactly **Qt 6.10.3**
with the `gcc_64` kit. Production requires Quick/QML and Sql; optional test
builds also require Qt Test. Use Bash or a suitable native Linux shell from
the repository root. CMake 3.22 is the intended minimum, not a claim of execution
on that version; preset schema 3 is compatible with it.

For shared presets, an illustrative kit selection is:

```bash
export QTDIR=/path/to/Qt/6.10.3/gcc_64
```

Apply the shared [local/SDK selection rules](build.md#shared-presets-and-local-qt-selection).
The examples use shared presets; substitute corresponding `*-local` names
throughout when those presets exist and match the installed kit.
Local and shared variants use the same output directories.

## Presets And Configurations

Each Ninja tree selects its matching build type.

| Configure/build preset | Directory below `build/qt-6.10.3/` | Configuration | Tests | DEB |
| --- | --- | --- | --- | --- |
| `linux-debug` | `linux-debug` | Debug | OFF | OFF |
| `linux-debug-tests` | `linux-debug-tests` | Debug | ON | OFF |
| `linux-release` | `linux-release` | Release | OFF | OFF |
| `linux-release-tests` | `linux-release-tests` | Release | ON | OFF |
| `linux-deb-local-test` | `linux-deb-local-test` | Release | OFF | ON, local-test ON |
| `linux-deb-release` | `linux-deb-release` | Release | OFF | ON, local-test OFF |

The two test presets also have matching CTest presets and separate build
directories. The routine local names are `linux-debug-local` and, when needed,
`linux-debug-tests-local`. Release and packaging retain the
[explicit-request condition](build.md#routine-codex-workflow).

Ordinary production/test presets inherit explicit DEB OFF from `linux-base`;
packaging has its own production cache. Reconfiguring `linux-release` restores
DEB OFF even if an earlier invocation enabled it. Preserved stale CPack files
are not authority to package an ordinary cache; the wrapper checks the live cache.

## Build And Test Commands

Application configure, when required:

```bash
cmake --preset linux-debug
```

Compile/link before required intermediate tests; skip this separate build in
the no-test branch of the [routine workflow](build.md#routine-codex-workflow):

```bash
cmake --build --preset linux-debug --target JobTrackerApp --parallel 1
```

Selected tests, when required by the [test map and scope](testing.md#subsystem-to-test-map):

```bash
cmake --preset linux-debug-tests
cmake --build --preset linux-debug-tests --target JobTrackerCommonTests --parallel 1
ctest --preset linux-debug-tests -R '^JobTrackerCommonTests$'
```

Ordinary runtime preparation (the only production build in the no-test branch):

```bash
cmake --build --preset linux-debug --target JobTrackerApp_dev_runtime --parallel 1
```

Only after that command succeeds, launch when required by the task:

```bash
./build/qt-6.10.3/linux-debug/dev-runtime/Debug/bin/JobTrackerApp
```

Replace `<preset>` with a compatible preset and `<test-target>` with an actual
target from the test map. Configure with `cmake --preset <preset>` under the
shared [configuration-freshness rules](build.md#action-and-target-selection).
`--parallel 1` is suitable for a memory-constrained host and does not change
preset settings.

| Action | Command |
| --- | --- |
| Application compile/link only | `cmake --build --preset <preset> --target JobTrackerApp --parallel 1` |
| Ordinary local runtime | `cmake --build --preset <preset> --target JobTrackerApp_dev_runtime --parallel 1` |
| Fresh local runtime | `cmake --build --preset <preset> --target JobTrackerApp_dev_runtime_fresh --parallel 1` |
| Full configured build | `cmake --build --preset <preset> --parallel 1` |
| All tests in a test-enabled preset | `cmake --build --preset <preset> --target JobTrackerTests --parallel 1` |
| Selected tests in a test-enabled preset | `cmake --build --preset <preset> --target <test-target> --parallel 1` |
| Selected CTest run | `ctest --preset <preset> -R "<filter>"` |
| Full relevant CTest suite | `ctest --preset <preset>` |

The full configured build is not the command for building just the full test
suite. Building test targets does not run CTest. Linux tests use their configured
SDK directly; test preparation does not deploy the application.

## Linux Development Runtime And Startup

The original build-tree executable can use its configured SDK directly.
`JobTrackerApp_dev_runtime` builds the application if needed and installs the
opt-in `JobTrackerDevelopmentRuntime` component into
`<build-directory>/dev-runtime/<configuration>/`. Use the deployed copy shown
above for routine application/runtime verification under the shared workflow.

The component uses its own Qt QML/runtime deployment script to prepare
`bin/JobTrackerApp`, private libraries under `lib/`, plugins, QML imports,
translations, and `bin/qt.conf`. CMake installs the executable with
`$ORIGIN/../lib` as its runtime library path. System libraries remain host
dependencies. The target clears an inherited `DESTDIR` for its child install
command and preserves existing output files.

Development deployment explicitly includes `qmltooling` for QML
debugging/profiling. `QT_QML_DEBUG` remains enabled only in Debug; the production
script retains its developer-tooling exclusions. The content-based
script-generation constraint is documented beside the calls in
`Installer/Linux/Runtime.cmake`.

The development component supports Debug and Release, including test-enabled
trees, and is excluded from full installation/CPack. It does not install desktop
integration, package notices, or write to `/opt/jobtracker`. Release packaging
keeps its production-build, prefix, and `DESTDIR` guards.
For an explicitly prepared application runtime in a test-enabled tree,
substitute that tree's directory in the ordinary launch path. See
[runtime selection](build.md#development-runtime-selection) for freshness,
preservation, and persistent-data considerations.

### Fresh Development Runtime

When selected under the shared [fresh-runtime conditions](build.md#fresh-development-runtime),
`JobTrackerApp_dev_runtime_fresh` builds the application if needed and creates
`<build-directory>/dev-runtime-fresh/<configuration>/<UTC-time>-<random-id>/`.
The executable is under `bin/` in that directory. It invokes the same
`JobTrackerDevelopmentRuntime` component as ordinary preparation, including
debugging/profiling plugins, relative RPATH/`qt.conf`, and child-only `DESTDIR`
clearing. It never copies an old prepared runtime and prints the exact absolute
output path and launch command.

## Staging And DEB Navigation

Ordinary builds keep packaging OFF. For an explicit request, use the existing
[standalone Release staging](../../Installer/Linux/README.md#build-and-stage)
or [DEB guide](../../Installer/Linux/README.md#build-a-deb), which owns detailed
commands, packaging tool requirements, and installation-path safeguards.
Native DEB support is limited to Ubuntu 24.04 amd64.

`linux-deb-local-test` uses placeholder engineering metadata;
`linux-deb-release` explicitly sets `JOBTRACKER_DEB_LOCAL_TEST=OFF` and requires
confirmed `JOBTRACKER_DEB_MAINTAINER` data, with no automatic local-test fallback.
`Package.sh` uses existing installation rules through CPack, so a DEB task
requires no separate ordinary `linux-release` build or `Stage.sh` run.
