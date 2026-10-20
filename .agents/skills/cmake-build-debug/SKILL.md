---
name: cmake-build-debug
description: Build and diagnose the JobTracker Qt/CMake project. Use when Codex needs to configure, build, run CTest, inspect CMake presets, investigate build failures, update build instructions, or verify changes that affect CMake, Qt deployment, QML resources, tests, or runtime behavior.
---

# CMake Build Debug

Use this skill for JobTracker build, test, and CMake verification work.

## Rules

1. Work from the actual repository root.
2. Use PowerShell on Windows and a suitable native shell on Linux; PowerShell scripts require PowerShell.
3. Select documentation and command forms under [Canonical Commands](#canonical-commands).
4. Run build and test commands one at a time.
5. Do not combine build or test commands with shell separators, pipes, or chained commands.
6. Wait for the active build process to finish and return its final exit code. If the command runner yields a still-running build, keep waiting on that same process until it completes; do not infer completion from silence or start a duplicate build.
7. If an external timeout terminates a build, confirm that no build process remains before retrying once with a longer timeout.
8. Do not delete build directories, caches, generated files, or deployment artifacts without explicit user approval.
9. If a command fails, capture the exact command, exit code, and key error excerpt, then diagnose before retrying or expanding scope. Do not continue blindly.
10. Keep non-test configure, build, and deployment independent of tests. With `JOBTRACKER_BUILD_TESTS=OFF`, do not require test-only packages, create or register test targets, compile test sources, deploy test dependencies, or change the production application artifact.

## Scope Selection

Follow the [routine workflow](../../../For-Agent/Docs/build.md#routine-codex-workflow)
for current-host Debug verification, production/test separation, final runtime
preparation, and explicitly requested configurations or packaging.

Select scope from the final changes and affected contracts using `test-and-review`
for sufficient production-build and independent-review criteria, and
[testing.md](../../../For-Agent/Docs/testing.md) for test selection and expansion.
Its subsystem map is an initial aid, not proof that one target is sufficient.
Read only the relevant section of [testing-reference.md](../../../For-Agent/Docs/testing-reference.md)
when suite, fixture, diagnostic-log, or manual-scenario details are needed.
Do not repeat successful unchanged checks without new evidence, edits, or risk.

## Canonical Commands

When build verification or build behavior is relevant, use
[build.md](../../../For-Agent/Docs/build.md) for shared action selection,
configuration freshness, and target/runtime selection. Then read only the
[Windows](../../../For-Agent/Docs/build-windows.md#build-and-test-commands) or
[Linux](../../../For-Agent/Docs/build-linux.md#build-and-test-commands) reference
for the OS relevant to the task. For staging or packaging, use the existing
installer guides linked from `build.md`.
Substitute verified preset names, build targets, and CTest filters as described
there; do not invent names or options or maintain a second command catalog here.

## Reporting

After each build command, report the command and status: `success` or `error`.

For CTest, follow [test outcomes](../../../For-Agent/Docs/testing.md#test-outcomes)
for classification, reporting, and empty-selection diagnosis. State any remaining
verification gaps.
