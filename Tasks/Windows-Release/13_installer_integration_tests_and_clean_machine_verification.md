# Task 13 — Installer integration tests and clean-machine verification

**Execution protocol:** Run this task only when selected by `Tasks/Windows-Release/00_RUN_NEXT.md`. Read `AGENTS.md`, `00_RUN_NEXT.md`, `00_AGREED_REQUIREMENTS.md` as needed, and the existing `For-Agent/Task-Report/windows-release/STATUS.md` (or initialize it in task 01). Read only relevant previous reports, never the complete report history. Finish with `For-Agent/Task-Report/windows-release/13.md` and then update `STATUS.md`. Do not proceed to task 14.


**Rules for this task:** Read `AGENTS.md`, `For-Agent/Docs/build.md`, `For-Agent/Docs/windows-release-requirements.md` and `For-Agent/Task-Report/windows-release/STATUS.md` and the previous stage report if present. Use the repository as source of truth. Change only what this stage needs; preserve C++/QML business logic, storage paths, SQLite migrations, and existing Debug/Linux workflows. No CI/CD, Linux packaging, automatic updates, code signing or unauthorized GitHub publication. Run all feasible checks, distinguish pass/fail/not run, write this stage’s separate report, update `STATUS.md`, and STOP after this stage. Do not delete existing builds or user files without authorization.

## Goal
Perform a release-readiness verification pass on the **actual generated artifacts**.

## Work
1. Configure/build Windows Release, run available C++/Qt tests with the appropriate test-enabled configuration, stage, produce archive/checksum and compile Setup. Re-check QML imports, Qt plugins, SQLite QSQLITE, application start and absence of dependency on Qt SDK/Visual Studio paths.
2. Build an explicit matrix: fresh per-user, fresh all-users, both coexist, upgrade each independently, same-version repair, downgrade rejection, custom install path, EN/RU UI, no desktop shortcut by default, uninstall scopes and data preservation.
3. Negative tests: offline/network failure, missing GitHub asset, SHA mismatch, canceled download, malformed archive/path traversal, MSVC runtime missing/present/failing, JobTracker running during update/uninstall, safe cancellation and retained files.
4. Evaluate mixed-version SQLite safety, clean Windows 10/11 x64 (VMs where available) and actual GitHub public download only if matching release asset exists. Never upload without authorization.
5. Add reproducible smoke-test/checklist tools where helpful, but do not add CI/CD or production-target test dependencies. Record PASS/FAIL/NOT RUN with evidence and blockers; do not fabricate clean-machine results.

## Done when
There is a concrete test matrix with evidence and unresolved problems; fix genuine failures in scope before closing this stage.

## Handoff
Write `For-Agent/Task-Report/windows-release/13.md` using `Tasks/Windows-Release/00_REPORT_TEMPLATE.md`. Include changed files, actual commands, PASS/FAIL/NOT_RUN evidence, limitations and the next-stage prerequisites. After the report is saved, update `For-Agent/Task-Report/windows-release/STATUS.md`. Do not automatically start the next numbered task.
