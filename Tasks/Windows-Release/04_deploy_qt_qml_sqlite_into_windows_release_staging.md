# Task 04 — Deploy Qt/QML/SQLite into Windows Release staging

**Execution protocol:** Run this task only when selected by `Tasks/Windows-Release/00_RUN_NEXT.md`. Read `AGENTS.md`, `00_RUN_NEXT.md`, `00_AGREED_REQUIREMENTS.md` as needed, and the existing `For-Agent/Task-Report/windows-release/STATUS.md` (or initialize it in task 01). Read only relevant previous reports, never the complete report history. Finish with `For-Agent/Task-Report/windows-release/04.md` and then update `STATUS.md`. Do not proceed to task 05.


**Rules for this task:** Read `AGENTS.md`, `For-Agent/Docs/build.md`, `For-Agent/Docs/windows-release-requirements.md` and `For-Agent/Task-Report/windows-release/STATUS.md` and the previous stage report if present. Use the repository as source of truth. Change only what this stage needs; preserve C++/QML business logic, storage paths, SQLite migrations, and existing Debug/Linux workflows. No CI/CD, Linux packaging, automatic updates, code signing or unauthorized GitHub publication. Run all feasible checks, distinguish pass/fail/not run, write this stage’s separate report, update `STATUS.md`, and STOP after this stage. Do not delete existing builds or user files without authorization.

## Goal
Create a deterministic and correctly installed **Windows Release** staging tree, not yet the installer.

## Work
1. Use Qt 6.10.3 CMake deployment APIs for the current Qt Quick/QML executable target (`JobTrackerApp`), preferably `qt_generate_deploy_qml_app_script` and `install(SCRIPT ...)`. Avoid running both duplicate deployment paths.
2. Use CMake install to populate a clean staging directory outside source; install the executable, Qt 6.10.3 libraries, QML imports/plugins, image/platform plugins, SQL `QSQLITE` plugin and required non-Qt runtime dependencies.
3. Coordinate with the future hybrid MSVC runtime (stage 09). By default Qt's Windows deployment API may copy compiler runtime DLLs; design a **common Qt payload without unintended MSVC-runtime duplication**, e.g. review `NO_COMPILER_RUNTIME`, and explicitly defer completing the per-mode MSVC runtime policy until stage 09.
4. Do not include Qt SDK headers, debug DLLs, import libraries or toolchain development files in release staging.
5. Validate layout, Qt platform plugin, QML loading, SQLite driver availability and clean SDK-independent process environment where feasible. Lack of required MSVC Runtime at this stage is a known pending dependency, not proof of a final clean-machine pass.

## Done when
`cmake --install` (or documented equivalent) creates a Release staging tree with correct Qt/QML/SQLite dependencies, without post-build copying or developer PATH dependency. Report what still needs MSVC stage 09.

## Handoff
Write `For-Agent/Task-Report/windows-release/04.md` using `Tasks/Windows-Release/00_REPORT_TEMPLATE.md`. Include changed files, actual commands, PASS/FAIL/NOT_RUN evidence, limitations and the next-stage prerequisites. After the report is saved, update `For-Agent/Task-Report/windows-release/STATUS.md`. Do not automatically start the next numbered task.
