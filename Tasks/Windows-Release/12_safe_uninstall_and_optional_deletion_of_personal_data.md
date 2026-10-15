# Task 12 — Safe uninstall and optional deletion of personal data

**Execution protocol:** Run this task only when selected by `Tasks/Windows-Release/00_RUN_NEXT.md`. Read `AGENTS.md`, `00_RUN_NEXT.md`, `00_AGREED_REQUIREMENTS.md` as needed, and the existing `For-Agent/Task-Report/windows-release/STATUS.md` (or initialize it in task 01). Read only relevant previous reports, never the complete report history. Finish with `For-Agent/Task-Report/windows-release/12.md` and then update `STATUS.md`. Do not proceed to task 13.


**Rules for this task:** Read `AGENTS.md`, `For-Agent/Docs/build.md`, `For-Agent/Docs/windows-release-requirements.md` and `For-Agent/Task-Report/windows-release/STATUS.md` and the previous stage report if present. Use the repository as source of truth. Change only what this stage needs; preserve C++/QML business logic, storage paths, SQLite migrations, and existing Debug/Linux workflows. No CI/CD, Linux packaging, automatic updates, code signing or unauthorized GitHub publication. Run all feasible checks, distinguish pass/fail/not run, write this stage’s separate report, update `STATUS.md`, and STOP after this stage. Do not delete existing builds or user files without authorization.

## Goal
Protect SQLite, imported CVs, settings and all other user-generated data.

## Work
1. Inspect actual Qt `QStandardPaths::AppLocalDataLocation` app identity and `StoragePaths.cpp` to determine the exact data location, without guessing or changing storage behavior.
2. Uninstall removes only the selected mode's program files, registration and installer-created shortcuts. Data **must remain by default**.
3. Provide an optional, explicit, bilingual EN/RU confirmation to delete the uninstalling user's personal data. Never delete data due to cancellation, installer errors or a partial uninstall.
4. Per-user and all-users copies may share the same data. If another relevant installation remains, **preserve shared data regardless of deletion preference** and explain why. Never enumerate/delete other Windows accounts' data. If running elevated under a different account, or ownership cannot be verified, preserve data.
5. Validate target paths before deletion, avoid symlink/reparse-point traversal and never recursively delete broad parents such as `%LocalAppData%`. Favor a conservative no-delete outcome over uncertain cleanup.
6. Test normal uninstall, explicit deletion, other-mode coexistence, UAC account mismatch, cancellation, locked files, unsafe paths where feasible.

## Done when
Uninstall and optional deletion obey the default-safe policy with no unrelated user data exposed to deletion.

## Handoff
Write `For-Agent/Task-Report/windows-release/12.md` using `Tasks/Windows-Release/00_REPORT_TEMPLATE.md`. Include changed files, actual commands, PASS/FAIL/NOT_RUN evidence, limitations and the next-stage prerequisites. After the report is saved, update `For-Agent/Task-Report/windows-release/STATUS.md`. Do not automatically start the next numbered task.
