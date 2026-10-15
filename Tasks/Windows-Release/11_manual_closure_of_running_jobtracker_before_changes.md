# Task 11 — Manual closure of running JobTracker before changes

**Execution protocol:** Run this task only when selected by `Tasks/Windows-Release/00_RUN_NEXT.md`. Read `AGENTS.md`, `00_RUN_NEXT.md`, `00_AGREED_REQUIREMENTS.md` as needed, and the existing `For-Agent/Task-Report/windows-release/STATUS.md` (or initialize it in task 01). Read only relevant previous reports, never the complete report history. Finish with `For-Agent/Task-Report/windows-release/11.md` and then update `STATUS.md`. Do not proceed to task 12.


**Rules for this task:** Read `AGENTS.md`, `For-Agent/Docs/build.md`, `For-Agent/Docs/windows-release-requirements.md` and `For-Agent/Task-Report/windows-release/STATUS.md` and the previous stage report if present. Use the repository as source of truth. Change only what this stage needs; preserve C++/QML business logic, storage paths, SQLite migrations, and existing Debug/Linux workflows. No CI/CD, Linux packaging, automatic updates, code signing or unauthorized GitHub publication. Run all feasible checks, distinguish pass/fail/not run, write this stage’s separate report, update `STATUS.md`, and STOP after this stage. Do not delete existing builds or user files without authorization.

## Goal
Never force-close JobTracker during install, upgrade, reinstall or uninstall.

## Work
1. Configure Inno Setup to disable automatic Restart Manager application closing/restart. Do not use `taskkill /F` or equivalent.
2. Detect a currently running affected JobTracker instance before replacing/removing its binaries; show localized EN/RU message asking the user to close it manually; support retry and cancel.
3. Since both modes can coexist with the same EXE filename, distinguish affected executable path when feasible; avoid blocking/closing unrelated copies. Do not mistake a different user session/process for the target without a safe documented policy.
4. Recheck state before destructive file operations, and preserve user data on abort/failure.
5. Test available running/closed/cancel scenarios, and document process detection limitations.

## Done when
The installer/uninstaller never forcibly terminates the application and does not replace in-use binaries, with verified cancellation behavior.

## Handoff
Write `For-Agent/Task-Report/windows-release/11.md` using `Tasks/Windows-Release/00_REPORT_TEMPLATE.md`. Include changed files, actual commands, PASS/FAIL/NOT_RUN evidence, limitations and the next-stage prerequisites. After the report is saved, update `For-Agent/Task-Report/windows-release/STATUS.md`. Do not automatically start the next numbered task.
