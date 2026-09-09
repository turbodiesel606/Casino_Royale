# Build And Verification

This project uses CMake presets and Qt 6.

Run all commands from `D:\Project_CV\Root` in PowerShell.

Run commands one at a time. Do not combine build or test commands with `;`, `&&`, pipes, or other shell separators.

Do not delete build directories, CMake caches, generated files, Qt deployment output, or other build artifacts without explicit user approval.

## Build Execution Requirements

Always run every `cmake --build ...` command with elevated access from the first attempt. Do not first attempt a build with restricted or sandboxed access.

Allow enough execution time for the build to finish and return its final exit code. If the command runner yields a still-running build, keep waiting on that same process until it completes. Do not infer completion from a period without output, do not report a final build status while it is still running, and do not launch a duplicate build process.

If an external command timeout terminates the build, confirm that no build process remains before retrying once with a longer timeout and elevated access.

Keep normal configure, build, and deployment independent of tests. When `JOBTRACKER_BUILD_TESTS=OFF`, CMake must not require Qt Test or another test-only package, create or register test targets, compile test sources, deploy test dependencies, or otherwise change the `JobTrackerApp` build. Keep test-only package discovery, targets, registration, compilation, and deployment inside the test-enabled configuration.

## Windows Build

Configure when the build tree is missing or stale, or when CMake, presets, source registration, or build options changed:

```powershell
cmake --preset windows-debug-local
```

Targeted production build for an ordinary bounded change:

```powershell
cmake --build --preset windows-debug-local --target JobTrackerApp
```

Full production preset build when the validation policy requires broader coverage:

```powershell
cmake --build --preset windows-debug-local
```

## Windows Build With Tests

Configure when the test build tree is missing or stale, or when CMake, presets, source registration, or build options changed:

```powershell
cmake --preset windows-debug-tests-local
```

Build one directly associated test target:

```powershell
cmake --build --preset windows-debug-tests-local --target <test-target>
```

Run one directly associated CTest suite:

```powershell
ctest --preset windows-debug-tests-local -R "^<test-name>$"
```

Build the full test-enabled preset when the validation policy requires broader coverage:

```powershell
cmake --build --preset windows-debug-tests-local
```

Run the full registered test set when the validation policy requires broader coverage:

```powershell
ctest --preset windows-debug-tests-local
```

## Linux Build

Not required yet, but will appear in the future.

## Validation Policy

Select verification scope from the final diff. For a bounded code, QML, resource, storage, or runtime-behavior change, use the targeted `JobTrackerApp` command above to build the smallest affected production target that provides meaningful compile and link confidence.

Expand to the full production preset build when shared infrastructure, cross-target APIs, build-system changes, several affected modules, merge/release readiness, or a diagnosed failure requires broader coverage.

When tests are required, use the targeted test commands above to build and run the directly associated test target or CTest suite by default.

Use the full test-enabled build and registered test set when schema or migrations, storage semantics, worker/runtime or concurrency behavior, shared infrastructure, several subsystems, cross-module contracts, unexpected targeted-test failures, or merge/release readiness requires broader verification.

Do not repeat an unchanged successful configure, build, or test run without a concrete reason. Reconfigure when the build tree is missing or stale, or when CMake, presets, source registration, or build options changed.

For documentation-only, workflow-only, or instruction-only changes, do not run a C++ build unless the change affects build behavior.

If a command fails, capture these details and diagnose before retrying or expanding scope:

- Exact command.
- Exit code.
- Key error excerpt.

After each build command, report the command and status: `success` or `error`.

For `ctest`, report the final result: `passed` or `failed`.
