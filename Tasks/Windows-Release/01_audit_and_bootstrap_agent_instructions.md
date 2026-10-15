# Task 01 — Audit and bootstrap agent instructions

**Execution protocol:** Run this task only when selected by `Tasks/Windows-Release/00_RUN_NEXT.md`. Read `AGENTS.md`, `00_RUN_NEXT.md`, `00_AGREED_REQUIREMENTS.md` as needed, and the existing `For-Agent/Task-Report/windows-release/STATUS.md` (or initialize it in task 01). Read only relevant previous reports, never the complete report history. Finish with `For-Agent/Task-Report/windows-release/01.md` and then update `STATUS.md`. Do not proceed to task 02.


**Safety and scope:** Work only on this stage; preserve application behavior and existing instructions, do not publish or delete files without permission. Windows-first Qt 6.10.3; Linux Qt 6.10.3 later. No CI/CD, signing, auto-updates, Linux packaging. Verify only actual actions.

## Goal
Make existing Codex instructions safe and accurate **before** touching the build system. This is a bootstrap, NOT the final documentation rewrite.

## Work
1. Inspect current CMake targets/presets, Qt SDK references, Debug launch method, QML/SQLite needs, installer-independent storage paths, tests, and the actual instruction hierarchy (`AGENTS.md`, `For-Agent/Docs/`, `.agents/skills/`, `.codex/`, `README.md`, `BuildCommands.txt`, `qt_bins.txt`). Check the current tree rather than trusting older reports.
2. Create `For-Agent/Docs/windows-release-requirements.md`: a compact, stable record of the agreed requirements: Windows 10/11 x64, MSVC 2022, Qt 6.10.3 **on Windows and Linux**; Inno Setup version-locked Online Installer; source private and binaries in `https://github.com/turbodiesel606/Releases`; manual publishing; per-user and all-users installations may coexist; English/Russian; chosen paths and optional desktop icon; temporary icon; MSVC hybrid runtime; upgrade/same-version allowed, downgrade forbidden; manual app closure; cautious user-data removal with preservation by default; no CI/CD, Linux packaging, auto-update or signing now.
3. Initialize `For-Agent/Task-Report/windows-release/STATUS.md` from `Tasks/Windows-Release/00_STATUS_TEMPLATE.md` (all 16 stages, initially `PENDING`) and create `For-Agent/Task-Report/windows-release/01.md` using `00_REPORT_TEMPLATE.md`. Mark 01 `DONE` only after this stage is actually complete; subsequent stages will maintain the status and their own reports.
4. Minimally amend `AGENTS.md` to route Windows Release tasks to `Tasks/Windows-Release/00_RUN_NEXT.md` and `For-Agent/Task-Report/windows-release/STATUS.md`. Minimally amend `For-Agent/Docs/build.md` to explicitly permit adding/running verified Release/install/package commands **as they are implemented**. Keep build documentation canonical; reports belong in `For-Agent/Task-Report/`, not `For-Agent/Docs/`. Preserve all existing restrictions, build commands and task routing. Do not describe nonexistent commands as operational.
5. Inspect `.gitignore`, which currently ignores the entire `Tasks/` directory. Add a narrow exception so the `Tasks/Windows-Release/` task definitions can be tracked in Git while unrelated legacy tasks remain ignored. This supports recovery on another checkout. Do not stage or commit changes unless explicitly authorized.
6. List stale documentation to fix in stage 15; do not perform broad documentation rewrites now.

## Done when
The next fresh Codex session can enter via `Tasks/Windows-Release/00_RUN_NEXT.md`, read `STATUS.md`, select stage 02, and use new commands without violating the canonical build-document rule. Deliver an audit summary and exact file changes.

## Handoff
Write `For-Agent/Task-Report/windows-release/01.md` using `Tasks/Windows-Release/00_REPORT_TEMPLATE.md`. Include changed files, actual commands, PASS/FAIL/NOT_RUN evidence, limitations and the next-stage prerequisites. After the report is saved, update `For-Agent/Task-Report/windows-release/STATUS.md`. Do not automatically start the next numbered task.
