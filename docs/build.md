# Build And Verification

This project uses CMake presets and Qt 6.

Run all commands from `D:\Project_CV\Root` in PowerShell.

Run commands one at a time. Do not combine build or test commands with `;`, `&&`, pipes, or other shell separators.

Do not delete build directories, CMake caches, generated files, Qt deployment output, or other build artifacts without explicit user approval.

## Windows Build

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

## Linux Build

Not required yet, but will appear in the future.

## Validation Policy

After code, QML, CMake, resource, storage, or runtime-behavior changes, run the normal Windows build unless impossible.

Run the test preset and `ctest` when the change affects business logic, storage, parsing, algorithms, or high-risk behavior.

For documentation-only, workflow-only, or instruction-only changes, do not run a C++ build unless the change affects build behavior.

If a command fails, stop and report:

- Exact command.
- Exit code.
- Key error excerpt.

After each build command, report the command and status: `success` or `error`.

For `ctest`, report the final result: `passed` or `failed`.
