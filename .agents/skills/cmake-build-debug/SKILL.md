---
name: cmake-build-debug
description: Build and diagnose the JobTracker Qt/CMake project. Use when Codex needs to configure, build, run CTest, inspect CMake presets, investigate build failures, update build instructions, or verify changes that affect CMake, Qt deployment, QML resources, tests, or runtime behavior.
---

# CMake Build Debug

Use this skill for JobTracker build, test, and CMake verification work.

## Rules

1. Work from `D:\Project_CV\Root`.
2. Use PowerShell.
3. Read `docs/build.md` before running build or test commands.
4. Run build and test commands one at a time.
5. Do not combine build commands with shell separators, pipes, or chained commands.
6. Do not delete build directories, caches, generated files, or deployment artifacts without explicit user approval.
7. If a command fails, stop and report the exact command, exit code, and key error excerpt.

## Normal Windows Build

Configure:

```powershell
cmake --preset windows-debug-local
```

Build:

```powershell
cmake --build --preset windows-debug-local
```

## Windows Build With Tests

Configure:

```powershell
cmake --preset windows-debug-tests-local
```

Build:

```powershell
cmake --build --preset windows-debug-tests-local
```

Run tests:

```powershell
ctest --preset windows-debug-tests-local
```

## Reporting

After each build command, report the command and status: `success` or `error`.

For `ctest`, report the final result: `passed` or `failed`.

For workflow-only or documentation-only changes, do not run a C++ build unless build behavior changed.
