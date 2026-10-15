# Task 05 — Temporary icon, Windows metadata and unified version

**Execution protocol:** Run this task only when selected by `Tasks/Windows-Release/00_RUN_NEXT.md`. Read `AGENTS.md`, `00_RUN_NEXT.md`, `00_AGREED_REQUIREMENTS.md` as needed, and the existing `For-Agent/Task-Report/windows-release/STATUS.md` (or initialize it in task 01). Read only relevant previous reports, never the complete report history. Finish with `For-Agent/Task-Report/windows-release/05.md` and then update `STATUS.md`. Do not proceed to task 06.


**Rules for this task:** Read `AGENTS.md`, `For-Agent/Docs/build.md`, `For-Agent/Docs/windows-release-requirements.md` and `For-Agent/Task-Report/windows-release/STATUS.md` and the previous stage report if present. Use the repository as source of truth. Change only what this stage needs; preserve C++/QML business logic, storage paths, SQLite migrations, and existing Debug/Linux workflows. No CI/CD, Linux packaging, automatic updates, code signing or unauthorized GitHub publication. Run all feasible checks, distinguish pass/fail/not run, write this stage’s separate report, update `STATUS.md`, and STOP after this stage. Do not delete existing builds or user files without authorization.

## Goal
Add Windows application identity assets without changing the internal CMake executable target.

## Work
1. Preserve `JobTrackerApp` as the internal target; if desirable configure output executable as `JobTracker.exe` using target properties, updating only affected deployment/install references.
2. Introduce a replaceable temporary `.ico` and wire it into executable resources, installer icon inputs, shortcuts, and uninstall entry configuration hooks.
3. Add appropriate Windows VERSIONINFO metadata (product/file version, neutral owner/publisher fields, description). Do not invent legal publisher identity or claim verified signing.
4. Use `project(... VERSION 0.1.0)` as the single authoritative release version, providing generated metadata/configuration for packaging without divergent hand-maintained version constants.
5. Verify that build, staging, Debug and tests still work.

## Done when
The staged EXE has a temporary icon and consistent verifiable version metadata; the asset can be swapped later with minimal effort.

## Handoff
Write `For-Agent/Task-Report/windows-release/05.md` using `Tasks/Windows-Release/00_REPORT_TEMPLATE.md`. Include changed files, actual commands, PASS/FAIL/NOT_RUN evidence, limitations and the next-stage prerequisites. After the report is saved, update `For-Agent/Task-Report/windows-release/STATUS.md`. Do not automatically start the next numbered task.
