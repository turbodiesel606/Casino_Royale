# Task 07 — Inno Setup foundation: two installation modes and UI

**Execution protocol:** Run this task only when selected by `Tasks/Windows-Release/00_RUN_NEXT.md`. Read `AGENTS.md`, `00_RUN_NEXT.md`, `00_AGREED_REQUIREMENTS.md` as needed, and the existing `For-Agent/Task-Report/windows-release/STATUS.md` (or initialize it in task 01). Read only relevant previous reports, never the complete report history. Finish with `For-Agent/Task-Report/windows-release/07.md` and then update `STATUS.md`. Do not proceed to task 08.


**Rules for this task:** Read `AGENTS.md`, `For-Agent/Docs/build.md`, `For-Agent/Docs/windows-release-requirements.md` and `For-Agent/Task-Report/windows-release/STATUS.md` and the previous stage report if present. Use the repository as source of truth. Change only what this stage needs; preserve C++/QML business logic, storage paths, SQLite migrations, and existing Debug/Linux workflows. No CI/CD, Linux packaging, automatic updates, code signing or unauthorized GitHub publication. Run all feasible checks, distinguish pass/fail/not run, write this stage’s separate report, update `STATUS.md`, and STOP after this stage. Do not delete existing builds or user files without authorization.

## Goal
Create a compilable, locally testable **installer skeleton**, before adding Internet download.

## Work
1. Add `packaging/windows/JobTracker.iss` and minimal packaging inputs/scripts. Use the verified payload from stage 06 as a **local installation test fixture**, not a claimed final Online Installer.
2. Support explicit current-user (no UAC; default `%LocalAppData%\Programs\JobTracker`) and all-users (admin/UAC; `%ProgramFiles%\JobTracker`) installation. Allow a custom valid folder; preserve the chosen path for upgrades.
3. Allow independent, simultaneous installations in both modes: appropriate registry/uninstall scope, shortcuts, install directories and future update behavior. Inspect documented Inno Setup privilege and AppId semantics; do not assume a shared AppId alone guarantees independence.
4. English default for unsupported locales, Russian translation, automatic supported system-language selection with manual user choice, and localized custom messages. Always Start Menu shortcut, desktop shortcut optional and unchecked by default, correct per-mode scope and uninstall entry. Use stage 05 temporary icon and executable metadata.
5. Compile script and test mode selection/install/uninstall registration where possible. Do not implement data deletion yet (stage 12); never delete user data by default.

## Done when
A local-payload installer can be compiled and both scopes are independently represented; list deferred requirements explicitly.

## Handoff
Write `For-Agent/Task-Report/windows-release/07.md` using `Tasks/Windows-Release/00_REPORT_TEMPLATE.md`. Include changed files, actual commands, PASS/FAIL/NOT_RUN evidence, limitations and the next-stage prerequisites. After the report is saved, update `For-Agent/Task-Report/windows-release/STATUS.md`. Do not automatically start the next numbered task.
