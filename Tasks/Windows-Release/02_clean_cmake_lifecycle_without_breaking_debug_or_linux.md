# Task 02 — Clean CMake lifecycle without breaking Debug or Linux

**Execution protocol:** Run this task only when selected by `Tasks/Windows-Release/00_RUN_NEXT.md`. Read `AGENTS.md`, `00_RUN_NEXT.md`, `00_AGREED_REQUIREMENTS.md` as needed, and the existing `For-Agent/Task-Report/windows-release/STATUS.md` (or initialize it in task 01). Read only relevant previous reports, never the complete report history. Finish with `For-Agent/Task-Report/windows-release/02.md` and then update `STATUS.md`. Do not proceed to task 03.


**Rules for this task:** Read `AGENTS.md`, `For-Agent/Docs/build.md`, `For-Agent/Docs/windows-release-requirements.md` and `For-Agent/Task-Report/windows-release/STATUS.md` and the previous stage report if present. Use the repository as source of truth. Change only what this stage needs; preserve C++/QML business logic, storage paths, SQLite migrations, and existing Debug/Linux workflows. No CI/CD, Linux packaging, automatic updates, code signing or unauthorized GitHub publication. Run all feasible checks, distinguish pass/fail/not run, write this stage’s separate report, update `STATUS.md`, and STOP after this stage. Do not delete existing builds or user files without authorization.

## Goal
Separate configure/build/test/install/package in CMake and remove deployment side effects, keeping existing developer workflows usable.

## Work
1. Inspect existing `POST_BUILD` `windeployqt`, Linux automatic `deploy/` deletion/install, repeated `install(TARGETS JobTrackerApp ...)`, and QML target setup.
2. Consolidate installation rules and remove conflicting/duplicated automatic deployment. `cmake --build` should not automatically package or erase a deployment tree.
3. Keep normal Windows Debug startup convenient; if removing `POST_BUILD` makes the executable fail outside the Qt-equipped developer environment, implement and document a deliberate development launch method that does not accidentally create a commercial package.
4. Keep Linux Debug build/startup functional (Linux packaging is out of scope); use appropriate platform guards. Preserve Qt QML resource bundling and SQLite storage logic.
5. Require Qt **at least 6.10** in CMake where needed by the fixed-string adapter. The approved production SDK remains 6.10.3 for both OSes; exact release pinning will be handled in stage 03.
6. Append verified commands to `For-Agent/Docs/build.md` as soon as each exists.

## Done when
Windows Debug config/build/tests remain usable; Linux configure/build is tested if available or identified as unverified; no duplicate install rules or destructive POST_BUILD deploy. Report exact command results.

## Handoff
Write `For-Agent/Task-Report/windows-release/02.md` using `Tasks/Windows-Release/00_REPORT_TEMPLATE.md`. Include changed files, actual commands, PASS/FAIL/NOT_RUN evidence, limitations and the next-stage prerequisites. After the report is saved, update `For-Agent/Task-Report/windows-release/STATUS.md`. Do not automatically start the next numbered task.
