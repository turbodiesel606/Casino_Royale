# Task 09 — Implement hybrid MSVC Runtime deployment

**Execution protocol:** Run this task only when selected by `Tasks/Windows-Release/00_RUN_NEXT.md`. Read `AGENTS.md`, `00_RUN_NEXT.md`, `00_AGREED_REQUIREMENTS.md` as needed, and the existing `For-Agent/Task-Report/windows-release/STATUS.md` (or initialize it in task 01). Read only relevant previous reports, never the complete report history. Finish with `For-Agent/Task-Report/windows-release/09.md` and then update `STATUS.md`. Do not proceed to task 10.


**Rules for this task:** Read `AGENTS.md`, `For-Agent/Docs/build.md`, `For-Agent/Docs/windows-release-requirements.md` and `For-Agent/Task-Report/windows-release/STATUS.md` and the previous stage report if present. Use the repository as source of truth. Change only what this stage needs; preserve C++/QML business logic, storage paths, SQLite migrations, and existing Debug/Linux workflows. No CI/CD, Linux packaging, automatic updates, code signing or unauthorized GitHub publication. Run all feasible checks, distinguish pass/fail/not run, write this stage’s separate report, update `STATUS.md`, and STOP after this stage. Do not delete existing builds or user files without authorization.

## Goal
Make JobTracker run on clean Windows without Visual Studio under **both** install modes.

## Work
1. Current-user installs: bundle licensed App-Local MSVC Runtime DLLs alongside `JobTracker.exe`, requiring no administrator privileges.
2. All-users installs: detect compatible machine-wide Visual C++ Redistributable x64; if absent, download and install the official Microsoft package using documented secure means and licensing. Check installer result codes, cancellation/reboots and failures. Do not use unofficial mirrors.
3. Reconcile stage-04 Qt deployment defaults, the stage-06 archive layout, and the stage-08 installer so the common payload does not accidentally force App-Local runtime for all-users. A distinct runtime sidecar/archive or mode-specific install step is acceptable.
4. Verify x64 architecture, runtime version, redistribution terms and integrity of downloaded prerequisites; fail safely when checks cannot be completed. Do not assume internet access to Microsoft is equivalent to successful installation.
5. Test with and without preinstalled MSVC runtime on actual/isolated Windows systems when available; do not claim untested outcomes.

## Done when
Both installation paths use the intended runtime strategy, payload and setup scripts are consistent, and verification limitations are recorded.

## Handoff
Write `For-Agent/Task-Report/windows-release/09.md` using `Tasks/Windows-Release/00_REPORT_TEMPLATE.md`. Include changed files, actual commands, PASS/FAIL/NOT_RUN evidence, limitations and the next-stage prerequisites. After the report is saved, update `For-Agent/Task-Report/windows-release/STATUS.md`. Do not automatically start the next numbered task.
