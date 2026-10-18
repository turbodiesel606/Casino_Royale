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

Select build and test scope from the final diff and the affected targets.

- For a bounded production change, build the smallest affected production target that gives meaningful compile and link confidence. The current application target is `JobTrackerApp`.
- Build the complete relevant preset when shared infrastructure, cross-target APIs, build-system changes, several affected modules, merge/release readiness, or a diagnosed failure requires broader coverage.
- Build and run the directly associated test target or CTest suite by default. Use the full relevant test preset for schema or migrations, storage semantics, worker/runtime or concurrency behavior, shared infrastructure, several subsystems, cross-module contracts, unexpected targeted-test failures, or merge/release readiness.
- Do not repeat an unchanged successful configure, build, or test run without a concrete reason. A source or configuration edit after the run, a changed verification scope, or diagnosis of a failure is a concrete reason.

## Canonical Commands

For normal application configuration, builds, development runtime preparation,
and backend tests, use only the commands in `For-Agent/Docs/build.md`. It owns
preset names, exact command forms, and targeted versus broader verification;
do not maintain a second command catalog in this skill.

## Reporting

After each build command, report the command and status: `success` or `error`.

For `ctest`, report the final result: `passed` or `failed`.

For workflow-only or documentation-only changes, do not run a C++ build unless build behavior changed.
