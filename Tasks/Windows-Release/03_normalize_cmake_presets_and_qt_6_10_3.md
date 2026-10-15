# Task 03 — Normalize CMake presets and Qt 6.10.3

**Execution protocol:** Run this task only when selected by `Tasks/Windows-Release/00_RUN_NEXT.md`. Read `AGENTS.md`, `00_RUN_NEXT.md`, `00_AGREED_REQUIREMENTS.md` as needed, and the existing `For-Agent/Task-Report/windows-release/STATUS.md` (or initialize it in task 01). Read only relevant previous reports, never the complete report history. Finish with `For-Agent/Task-Report/windows-release/03.md` and then update `STATUS.md`. Do not proceed to task 04.


**Rules for this task:** Read `AGENTS.md`, `For-Agent/Docs/build.md`, `For-Agent/Docs/windows-release-requirements.md` and `For-Agent/Task-Report/windows-release/STATUS.md` and the previous stage report if present. Use the repository as source of truth. Change only what this stage needs; preserve C++/QML business logic, storage paths, SQLite migrations, and existing Debug/Linux workflows. No CI/CD, Linux packaging, automatic updates, code signing or unauthorized GitHub publication. Run all feasible checks, distinguish pass/fail/not run, write this stage’s separate report, update `STATUS.md`, and STOP after this stage. Do not delete existing builds or user files without authorization.

## Goal
Make shared CMake configuration portable while establishing reproducible Windows Release settings.

## Work
1. Inspect `CMakePresets.json` and `CMakeUserPresets.json`; remove machine-only `D:/...` paths and inconsistent Qt 6.9.x paths from **shared** configuration. Do not overwrite the user's local SDK settings without a reason.
2. Preserve working `windows-debug-local`, Windows tests, and existing Linux Debug presets; add coherent `windows-release`/Release-test configure, build, and test presets suited to MSVC 2022 x64. Ensure Debug-only `QT_QML_DEBUG` and `/JMC` do not leak into Release.
3. Make the build require the approved Qt **6.10.3** for production configurations and record that Linux must also use 6.10.3. Use a clear SDK discovery mechanism (local user preset or environment), not a developer-specific shared path.
4. Ignore `CMakeUserPresets.json` and build outputs in `.gitignore`. If already tracked, report the safe untracking operation; do not run forbidden Git mutations.
5. Check configure/build/test presets, validate JSON, and document only commands that were actually confirmed.

## Done when
Clean Windows Debug and Release configure/build work with an explicit local Qt SDK path; Linux shared presets are not broken; no stale shared Qt 6.9.x requirement remains.

## Handoff
Write `For-Agent/Task-Report/windows-release/03.md` using `Tasks/Windows-Release/00_REPORT_TEMPLATE.md`. Include changed files, actual commands, PASS/FAIL/NOT_RUN evidence, limitations and the next-stage prerequisites. After the report is saved, update `For-Agent/Task-Report/windows-release/STATUS.md`. Do not automatically start the next numbered task.
