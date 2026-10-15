# Task 06 — Versioned application archive and SHA-256

**Execution protocol:** Run this task only when selected by `Tasks/Windows-Release/00_RUN_NEXT.md`. Read `AGENTS.md`, `00_RUN_NEXT.md`, `00_AGREED_REQUIREMENTS.md` as needed, and the existing `For-Agent/Task-Report/windows-release/STATUS.md` (or initialize it in task 01). Read only relevant previous reports, never the complete report history. Finish with `For-Agent/Task-Report/windows-release/06.md` and then update `STATUS.md`. Do not proceed to task 07.


**Rules for this task:** Read `AGENTS.md`, `For-Agent/Docs/build.md`, `For-Agent/Docs/windows-release-requirements.md` and `For-Agent/Task-Report/windows-release/STATUS.md` and the previous stage report if present. Use the repository as source of truth. Change only what this stage needs; preserve C++/QML business logic, storage paths, SQLite migrations, and existing Debug/Linux workflows. No CI/CD, Linux packaging, automatic updates, code signing or unauthorized GitHub publication. Run all feasible checks, distinguish pass/fail/not run, write this stage’s separate report, update `STATUS.md`, and STOP after this stage. Do not delete existing builds or user files without authorization.

## Goal
Turn the working Windows staging tree into a distributable, checked, versioned payload.

## Work
1. Add a deterministic packaging script or CMake target that creates a Release-only archive of staged files. Preferred name if supported by the chosen installer extraction path: `JobTracker-<version>-windows-x64.7z`. If documented native Inno Setup extraction favors another format, choose it explicitly and explain why.
2. Keep a predictable archive root/layout and prevent accidental embedding of absolute machine paths, local secrets, development artifacts, or Debug DLLs.
3. Compute SHA-256 for the exact final archive. Provide a machine-readable checksum/manifest output consumed later by the installer build; fail on missing or mismatched version/assets.
4. Archive output belongs under ignored `dist/` (or a deliberate generated location), outside source and outside Qt SDK.
5. Verify extraction/layout and checksums; do not upload anything to GitHub.

## Done when
You can reproduce a versioned archive and matching SHA-256 from Windows Release staging; report exact commands and output paths.

## Handoff
Write `For-Agent/Task-Report/windows-release/06.md` using `Tasks/Windows-Release/00_REPORT_TEMPLATE.md`. Include changed files, actual commands, PASS/FAIL/NOT_RUN evidence, limitations and the next-stage prerequisites. After the report is saved, update `For-Agent/Task-Report/windows-release/STATUS.md`. Do not automatically start the next numbered task.
