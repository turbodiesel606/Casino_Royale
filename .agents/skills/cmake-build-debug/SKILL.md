---
name: cmake-build-debug
description: Build and diagnose the JobTracker Qt/CMake project. Use when Codex needs to configure, build, run CTest, inspect CMake presets, investigate build failures, update build instructions, or verify changes that affect CMake, Qt deployment, QML resources, tests, or runtime behavior.
---

# CMake Build Debug

Use this skill for JobTracker build, test, and CMake verification work.

## Rules

1. Work from the actual repository root.
2. Use PowerShell on Windows and a suitable native shell on Linux; PowerShell scripts require PowerShell.
3. For normal application configuration, builds, development runtime preparation, and backend tests, use `For-Agent/Docs/build.md`. Read its relevant section before execution.
4. Run build and test commands one at a time.
5. Do not combine build or test commands with shell separators, pipes, or chained commands.
6. Run every `cmake --build ...` command with elevated Codex tool access (`sandbox_permissions: require_escalated`) from the first attempt; do not first try restricted/sandboxed access. This is tool-access permission, not a request for Windows administrator elevation or Linux `sudo`.
7. Wait for the active build process to finish and return its final exit code. If the command runner yields a still-running build, keep waiting on that same process until it completes; do not infer completion from silence or start a duplicate build.
8. If an external timeout terminates a build, confirm that no build process remains before retrying once with a longer timeout under the same tool-access policy.
9. Do not delete build directories, caches, generated files, or deployment artifacts without explicit user approval.
10. If a command fails, capture the exact command, exit code, and key error excerpt, then diagnose before retrying or expanding scope. Do not continue blindly.
11. Keep non-test configure, build, and deployment independent of tests. With `JOBTRACKER_BUILD_TESTS=OFF`, do not require test-only packages, create or register test targets, compile test sources, deploy test dependencies, or change the production application artifact.

## Scope Selection

Follow the [routine workflow](../../../For-Agent/Docs/build.md#routine-codex-workflow).
For routine code tasks on Windows and Linux, use Debug on the current host and
build `JobTrackerApp_dev_runtime` to include production application/runtime
deployment. Build and run required tests in a separate Debug test preset.
Broader coverage stays in Debug. Release builds and Release tests require an
explicit user request; a Debug failure or broader verification does not
authorize them. Other configurations, staging, and packaging also require an
explicit user request.

Select scope from the final diff and affected contracts using `test-and-review`
for production-build and independent-review criteria, and
`For-Agent/Docs/testing.md` for test selection and expansion. Its subsystem map
provides actual registered suite names, including the controller suites; it is
an initial aid, not proof that one target is sufficient. Use
`For-Agent/Docs/build.md` for configuration freshness and command forms. Do not
repeat successful unchanged checks without new evidence, edits, or risk.

## Canonical Commands

For normal application configuration, builds, development runtime preparation,
and backend tests, use the canonical forms in `For-Agent/Docs/build.md`. Verified
preset names, build targets, and CTest filters may be substituted for its examples
as described there. Do not invent names or options, or maintain a second command
catalog in this skill.

## Reporting

After each build command, report the command and status: `success` or `error`.

For `ctest`, report `passed`, `failed`, `not run`, or `no matching tests` under
`For-Agent/Docs/testing.md`. State the preset/configuration/filter, how many tests
actually ran, and what remains unverified. A successful process exit with zero
matching tests is not a passing suite. When tests were expected, inspect the
preset, test registration, configuration, and filter before claiming success.
Do not describe zero matches as an executed failing test. Keep configure/build
failures and test-start errors distinct from test assertion failures.

For workflow-only or documentation-only changes, do not run a C++ build unless build behavior changed.
