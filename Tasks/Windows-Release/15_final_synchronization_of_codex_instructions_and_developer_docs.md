# Task 15 — Final synchronization of Codex instructions and developer docs

**Execution protocol:** Run this task only when selected by `Tasks/Windows-Release/00_RUN_NEXT.md`. Read `AGENTS.md`, `00_RUN_NEXT.md`, `00_AGREED_REQUIREMENTS.md` as needed, and the existing `For-Agent/Task-Report/windows-release/STATUS.md` (or initialize it in task 01). Read only relevant previous reports, never the complete report history. Finish with `For-Agent/Task-Report/windows-release/15.md` and then update `STATUS.md`. Do not proceed to task 16.


**Rules for this task:** Read `AGENTS.md`, `For-Agent/Docs/build.md`, `For-Agent/Docs/windows-release-requirements.md` and `For-Agent/Task-Report/windows-release/STATUS.md` and the previous stage report if present. Use the repository as source of truth. Change only what this stage needs; preserve C++/QML business logic, storage paths, SQLite migrations, and existing Debug/Linux workflows. No CI/CD, Linux packaging, automatic updates, code signing or unauthorized GitHub publication. Run all feasible checks, distinguish pass/fail/not run, write this stage’s separate report, update `STATUS.md`, and STOP after this stage. Do not delete existing builds or user files without authorization.

## Goal
**Only now**, after implementation/testing, comprehensively synchronize documentation and agent skills with real commands, files and capabilities.

## Work
1. Treat `For-Agent/Docs/build.md` as the canonical verified command catalog: Windows Debug/test/Release, stage/install, archive, Setup generation, local smoke tests, and existing Linux Debug; Qt 6.10.3 requirement for both platforms. Include only commands that exist and state which were actually run.
2. Create/finalize `For-Agent/Docs/windows-release.md` (toolchain, Qt/QML/SQLite deployment, archive/checksum, GitHub release URL, Inno Setup, MSVC hybrid, two modes, versioning, running-process policy, safe uninstall, EN/RU, icon, smoke tests, known blockers, future signing). Keep command duplication low.
3. Update root `AGENTS.md` task routing and `.agents/skills/cmake-build-debug/SKILL.md`; create `.agents/skills/windows-release-packaging/SKILL.md` using existing skill frontmatter. Both skills must defer actual commands to canonical docs. Preserve existing security, Git and test policies.
4. Update `For-Agent/Docs/testing.md`, `.agents/skills/test-and-review/SKILL.md`, `README.md`, `.gitignore`; modernize or replace `BuildCommands.txt`/`qt_bins.txt`. Create/update the originally requested developer docs `docs/BUILDING.md`, `docs/WINDOWS-RELEASE.md`, and `docs/WINDOWS-INSTALLER.md`, or maintain equivalent existing files with an explicit rationale. Cross-link them with `For-Agent/Docs/` rather than duplicating commands. Explain Qt 6.10.3 on Linux without implementing Linux packaging. Check `For-Agent/Docs/artifacts.md` and architecture docs only if actual contracts changed; otherwise leave them untouched.
5. Preserve historical reports in `For-Agent/Task-Report/` and `For-Agent/Review/`; the only task-report writes allowed are under `For-Agent/Task-Report/windows-release/` for this workflow. Do not alter `.codex/config.toml` or C++/QML subagents absent a specific incompatibility. Ensure copyright/notices reference actual payload.
6. Compare all documented commands, targets, paths, versions, asset names, SHA flow, stage reports, `STATUS.md`, and test statuses with the implementation. Avoid unnecessary rebuilds just for prose changes.

## Done when
A new Codex session can execute the Windows Release workflow by following AGENTS + canonical docs, with no stale Qt 6.9 claims or contradictory build instructions. Report modified and intentionally unchanged docs.

## Handoff
Write `For-Agent/Task-Report/windows-release/15.md` using `Tasks/Windows-Release/00_REPORT_TEMPLATE.md`. Include changed files, actual commands, PASS/FAIL/NOT_RUN evidence, limitations and the next-stage prerequisites. After the report is saved, update `For-Agent/Task-Report/windows-release/STATUS.md`. Do not automatically start the next numbered task.
