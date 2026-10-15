# Task 10 — Upgrades, reinstalls, downgrades and coexistence safety

**Execution protocol:** Run this task only when selected by `Tasks/Windows-Release/00_RUN_NEXT.md`. Read `AGENTS.md`, `00_RUN_NEXT.md`, `00_AGREED_REQUIREMENTS.md` as needed, and the existing `For-Agent/Task-Report/windows-release/STATUS.md` (or initialize it in task 01). Read only relevant previous reports, never the complete report history. Finish with `For-Agent/Task-Report/windows-release/10.md` and then update `STATUS.md`. Do not proceed to task 11.


**Rules for this task:** Read `AGENTS.md`, `For-Agent/Docs/build.md`, `For-Agent/Docs/windows-release-requirements.md` and `For-Agent/Task-Report/windows-release/STATUS.md` and the previous stage report if present. Use the repository as source of truth. Change only what this stage needs; preserve C++/QML business logic, storage paths, SQLite migrations, and existing Debug/Linux workflows. No CI/CD, Linux packaging, automatic updates, code signing or unauthorized GitHub publication. Run all feasible checks, distinguish pass/fail/not run, write this stage’s separate report, update `STATUS.md`, and STOP after this stage. Do not delete existing builds or user files without authorization.

## Goal
Make version-specific installers safe for upgrades and mixed per-user/all-users deployments.

## Work
1. Within each mode: allow newer-over-older, allow same-version repair/reinstall, **reject older-over-newer**. Compare trustworthy installed-version metadata; reuse prior installation directory unless user explicitly changes it.
2. Independently track per-user and all-users installations. Updating or uninstalling one must not change the other's executable, registration or shortcuts. Reject collisions where two modes select the same binary directory.
3. Audit cross-mode version interaction: both installations under one Windows account may use the same `QStandardPaths::AppLocalDataLocation` SQLite/CV storage. Define and implement the safest installer-level compatibility policy that does not require business-logic/SQLite-schema changes. Do not claim mixed-version database safety without proof; document residual risks or block unsafe installation combinations.
4. Installer must not run SQLite migrations; user data is always preserved during upgrades/reinstalls.
5. Test version order, both installation scopes, registry metadata and previous-path reuse; identify any scenarios requiring a Windows VM.

## Done when
Upgrade/repair/downgrade and coexistence rules are implemented, testable and documented, including shared-data risk.

## Handoff
Write `For-Agent/Task-Report/windows-release/10.md` using `Tasks/Windows-Release/00_REPORT_TEMPLATE.md`. Include changed files, actual commands, PASS/FAIL/NOT_RUN evidence, limitations and the next-stage prerequisites. After the report is saved, update `For-Agent/Task-Report/windows-release/STATUS.md`. Do not automatically start the next numbered task.
