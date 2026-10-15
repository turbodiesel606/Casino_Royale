# Task 08 — GitHub version-locked Online Installer and secure downloads

**Execution protocol:** Run this task only when selected by `Tasks/Windows-Release/00_RUN_NEXT.md`. Read `AGENTS.md`, `00_RUN_NEXT.md`, `00_AGREED_REQUIREMENTS.md` as needed, and the existing `For-Agent/Task-Report/windows-release/STATUS.md` (or initialize it in task 01). Read only relevant previous reports, never the complete report history. Finish with `For-Agent/Task-Report/windows-release/08.md` and then update `STATUS.md`. Do not proceed to task 09.


**Rules for this task:** Read `AGENTS.md`, `For-Agent/Docs/build.md`, `For-Agent/Docs/windows-release-requirements.md` and `For-Agent/Task-Report/windows-release/STATUS.md` and the previous stage report if present. Use the repository as source of truth. Change only what this stage needs; preserve C++/QML business logic, storage paths, SQLite migrations, and existing Debug/Linux workflows. No CI/CD, Linux packaging, automatic updates, code signing or unauthorized GitHub publication. Run all feasible checks, distinguish pass/fail/not run, write this stage’s separate report, update `STATUS.md`, and STOP after this stage. Do not delete existing builds or user files without authorization.

## Goal
Upgrade stage 07 into a version-specific Online Installer.

## Work
1. Public binary repository: `https://github.com/turbodiesel606/Releases`; source repository remains private. No GitHub publishing or CI.
2. Generate a Setup EXE tied to a **specific** CMake version/tag (`v<version>`), asset name and HTTPS GitHub Releases download URL. Never use `latest`. New release => new installer.
3. At packaging time embed expected SHA-256 for the exact stage-06 archive; fail closed if it is missing or does not match. Show progress and meaningful EN/RU download errors. Handle cancellation, network loss, HTTP failures and missing assets; clean temporary files.
4. Verify checksum **before** extraction. Validate archive paths and prevent directory traversal, arbitrary-file overwrite and extraction outside the intended destination. Prefer documented Inno Setup native download/extraction APIs; avoid external download executables unless unavoidable. Do not expose private credentials.
5. Provide a local test mechanism (fixture or local HTTPS/mock endpoint) independent of publication, without allowing a public installer to contain a fake URL/checksum. If the GitHub asset is not yet published, compile and test every feasible part and mark public end-to-end download unverified.

## Done when
A compiled, version-specific Online Installer enforces integrity and fails safely on download errors. It is **not yet public-ready** until stages 09–16 are completed.

## Handoff
Write `For-Agent/Task-Report/windows-release/08.md` using `Tasks/Windows-Release/00_REPORT_TEMPLATE.md`. Include changed files, actual commands, PASS/FAIL/NOT_RUN evidence, limitations and the next-stage prerequisites. After the report is saved, update `For-Agent/Task-Report/windows-release/STATUS.md`. Do not automatically start the next numbered task.
