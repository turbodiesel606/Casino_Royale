# Build And Verification

This is the sole owner of the detailed shared everyday workflow and action
selection. Read it when build verification or build behavior is relevant, then
use only the platform reference needed for the task:
[Windows](build-windows.md) or [Linux](build-linux.md). Those files own toolchain,
preset, command, runtime-layout, and platform diagnostic details.
Use [testing.md](testing.md) to select, write, and assess tests.

## Routine Codex Workflow

Code, QML, CMake, resource, storage, or runtime-behavior changes require the
smallest sufficient production build unless verification is impossible; apply
[test-and-review](../../.agents/skills/test-and-review/SKILL.md) for sufficient
verification and report any limitation. Documentation/instruction-only changes
without build impact and read-only code investigations require no compilation,
tests, deployment, or application launch. An explicitly requested build/test
investigation follows its requested scope.

Use **Debug on the current host OS**. Prefer a matching local preset as
described below; configure when the tree or changed inputs require it.
Keep the production tree at `JOBTRACKER_BUILD_TESTS=OFF`.

Select the branch using [testing.md](testing.md#test-scope) and applicable
project verification rules. The shorter branch is allowed only when those
rules justify not running intermediate tests; test-selection criteria remain
unchanged.

- **No intermediate tests required:** after the final changes and any necessary
  configure, build `JobTrackerApp_dev_runtime` directly in the production tree.
  Do not run a separate preliminary `JobTrackerApp` build. On Windows and Linux,
  the runtime target depends on `JobTrackerApp`, so the same invocation
  compiles/links the application as needed before preparing its development runtime.
- **Tests required:** preserve this order:

  1. Build `JobTrackerApp` in the production tree.
  2. Configure as needed and build selected test targets in a **separate Debug
     tests-ON tree**.
  3. Run the selected tests through **CTest separately from building**.
  4. After the final changes and successful required checks, build the ordinary
     `JobTrackerApp_dev_runtime` in the production tree.

Runtime preparation builds the application incrementally if necessary; no
forced full rebuild is required.

Intermediate cycles may build `JobTrackerApp` without repeated deployment.
Prepare runtime earlier when startup or new dependencies need verification;
final preparation must reflect the latest application state. Reuse successful
checks that remain current. Launch the GUI only when the task requires it and
the runtime build has completed successfully for the current changes.

Broader verification expands production targets or test coverage within Debug;
it does not automatically add Release or another OS. Release builds, Release
tests, other configurations, staging, and packaging require an explicit user
request, including after a Debug failure.

## Action And Target Selection

Run commands from the repository root, one configure/build/test command at a
time, and check each final exit code. On failure, stop and diagnose before
continuing; do not launch an existing executable after a compile, link, or
deployment failure. Apply
[cmake-build-debug](../../.agents/skills/cmake-build-debug/SKILL.md) for process
completion, failure handling, and diagnosis.

Configure when the build tree is absent or stale, or CMake, presets, options,
source registration, SDK, or relevant cache inputs changed; inspect the resulting
cache. Otherwise reuse the configured tree. Select only the needed actions from
the [Windows command catalog](build-windows.md#build-and-test-commands) or
[Linux command catalog](build-linux.md#build-and-test-commands), substituting
verified preset names, targets, and CTest filters.

### Target Selection

| Target or action | Purpose |
| --- | --- |
| `JobTrackerApp` | Application compile/link only. |
| `JobTrackerApp_dev_runtime` | Incremental application build and ordinary local runtime preparation. |
| `JobTrackerApp_dev_runtime_fresh` | Optional application build and runtime preparation in a new directory. |
| Build without `--target` | Full configured `ALL` build; does not invoke opt-in runtime preparation, stage, package, or clean output. |
| `JobTrackerTests` | Tests-ON only: builds all test executables and their prerequisites without building the application; does not run tests. |
| Named test target | Builds only that test and its prerequisites; names come from the [test map](testing.md#subsystem-to-test-map). |
| CTest | Runs selected built tests, or the full relevant suite when scope requires it. |

## Shared Presets And Local Qt Selection

Prefer a matching `*-local` preset when it exists and points to the installed
kit. On Windows, use `windows-debug-local` for routine production configure/build
and `windows-debug-tests-local` for required tests when those local presets are
available. On Linux, use `linux-debug-local` for production and
`linux-debug-tests-local` for required tests when those local presets are
available. Select their names explicitly; CMake does not automatically replace
a shared preset with its local variant.

`CMakeUserPresets.json` is optional, local, and ignored. Preserve local
preferences on both platforms. User presets implicitly include the shared file
and may override `CMAKE_PREFIX_PATH`; an explicit kit path there removes the
need for `QTDIR`. Check that matching local configure/build/test presets exist
with `cmake --list-presets=all`.

`CMakePresets.json` is maintained and portable. If no suitable local preset is
available, use the shared preset after setting `QTDIR` in the current shell to
the required Qt 6.10.3 kit. `windows-debug` requires `QTDIR` to point to the
installed `msvc2022_64` kit; `linux-debug` requires the installed `gcc_64` kit.
Shared presets do not use the Qt path from their corresponding local variants.
They read `QTDIR` through `CMAKE_PREFIX_PATH`: select the kit containing `bin/`
and `lib/cmake/`, not its parent version
directory. An unset or outdated `QTDIR` can cause Qt discovery to fail. The
variable remains available to child processes and need not be reset before
every build in that session. Platform references provide the exact assignments
and preset maps.

An inherited local preset normally shares its parent's output directory.
Do not switch SDKs in a populated cache. Inspect `CMAKE_PREFIX_PATH`, `Qt6_DIR`,
and resolved Qt component paths after configuration. Preserve an incompatible
cache and configure the selected preset with an explicit fresh `-B build/<new-name>`;
then build that exact directory rather than the preset's original directory.

## Development Runtime Selection

The ordinary target preserves its established launch path and existing files;
a populated old directory alone does not prove current deployment. It does not
prune stale files. Both development targets preserve previous output and neither
creates an installer/package or forces a full rebuild.

Test dependency preparation does not deploy the application, and application
runtime targets do not build or run tests. To launch from a test-enabled tree,
explicitly prepare that tree's application runtime and use its launch path.
Routine production verification remains in its separate tests-OFF tree.

Launching uses the account's persistent JobTracker data, including startup CV
recovery. Use a disposable account when personal data must remain untouched.
Build/runtime preparation and application startup are separate evidence.

### Fresh Development Runtime

`JobTrackerApp_dev_runtime_fresh` is explicit and optional, with no dependency
from `ALL`, ordinary runtime preparation, tests, or packaging. Choose it when
verification needs an empty destination; ordinary preparation remains the final
routine action. Each invocation deploys the current application into a new
directory and prints its absolute path and launch command. See the
[Windows](build-windows.md#fresh-development-runtime) or
[Linux](build-linux.md#fresh-development-runtime) details.

## Optional Tests

Apply [production/test isolation](testing.md#production-and-test-isolation).
Select targets and filters from the [test map](testing.md#subsystem-to-test-map),
expanding only under [test scope](testing.md#test-scope). Platform catalogs give
selected/full build and CTest command forms and test-runtime details.

Shared and local test presets preserve `outputOnFailure` and schema 3 /
CMake 3.22 compatibility. Follow [test outcomes](testing.md#test-outcomes)
for result interpretation, reporting, and empty-selection handling.
Standalone deployment consumers remain separate CMake projects under
`tests/deployment/`.

## Explicit Release, Staging, And Packaging

When asked to create or update an installer, prefer the native unattended
`Installer/Windows/Create-Installer.ps1` or `Installer/Linux/Create-Installer.sh`
entry point documented in [Installer/README.md](../../Installer/README.md).
They configure/build Release and reuse existing staging/package checks; they
never run acceptance tests or install/launch the result. Ordinary development
verification does not invoke these launchers.

Development runtime preparation is unnecessary for packaging. For an explicit
request, use the [Windows installer build sequence](installer.md#build-sequence)
or the Linux [standalone staging](../../Installer/Linux/README.md#build-and-stage)
and [DEB commands](../../Installer/Linux/README.md#build-a-deb).
Ordinary builds, standalone stages, placeholder local-test DEBs, and release
DEBs with confirmed metadata are distinct workflows. Native DEB support remains
limited to Ubuntu 24.04 amd64; a DEB task needs no separate ordinary Release
build or staging run. See [installer navigation](../../Installer/README.md)
for the relevant deployment checks.

## Preset Inspection And Validation

```bash
cmake --list-presets
cmake --list-presets=all
ctest --list-presets
```

Host conditions filter configure presets. A build/test name listed on another
OS does not prove it can execute there. Preserve caches and failed output;
rerun affected checks after changed inputs or diagnosed failures. Compile/link,
package inspection, extracted runtime, native GUI, and clean-target installation
are distinct results.
