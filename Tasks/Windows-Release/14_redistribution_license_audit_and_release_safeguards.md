# Task 14 — Redistribution/license audit and release safeguards

**Execution protocol:** Run this task only when selected by `Tasks/Windows-Release/00_RUN_NEXT.md`. Read `AGENTS.md`, `00_RUN_NEXT.md`, `00_AGREED_REQUIREMENTS.md` as needed, and the existing `For-Agent/Task-Report/windows-release/STATUS.md` (or initialize it in task 01). Read only relevant previous reports, never the complete report history. Finish with `For-Agent/Task-Report/windows-release/14.md` and then update `STATUS.md`. Do not proceed to task 15.


**Rules for this task:** Read `AGENTS.md`, `For-Agent/Docs/build.md`, `For-Agent/Docs/windows-release-requirements.md` and `For-Agent/Task-Report/windows-release/STATUS.md` and the previous stage report if present. Use the repository as source of truth. Change only what this stage needs; preserve C++/QML business logic, storage paths, SQLite migrations, and existing Debug/Linux workflows. No CI/CD, Linux packaging, automatic updates, code signing or unauthorized GitHub publication. Run all feasible checks, distinguish pass/fail/not run, write this stage’s separate report, update `STATUS.md`, and STOP after this stage. Do not delete existing builds or user files without authorization.

## Goal
Perform compliance and final release-security checks against the **actual** stage-13 Windows payload.

## Work
1. Inventory every Qt DLL, Qt plugin/QML module, SQLite-related component, MSVC runtime and third-party binary actually distributed.
2. Inspect the existing `For-Agent/Task-Report/JobTracker_Commercialization_Research.md` as background, and check licenses/provenance against the deployed versions. Record required third-party notices, relevant LGPL/GPL obligations, access to corresponding Qt sources/relinkability where applicable, and Microsoft redistribution conditions. Do not imply successful deployment alone establishes legal compliance.
3. Check that package contains no secrets/private sources/developer-only files; URL is pinned to release version; checksum is embedded and validated; trust/signing is explicitly deferred, not faked.
4. Add the applicable notices and legal files or clear release blockers. Leave user-approved publisher identity and future SignTool integration easy to configure; do not create certificates/keys or claim SmartScreen trust.
5. Distinguish legal questions requiring owner/legal review from matters verified directly; no silent GPL-only module inclusion in a proprietary distribution.

## Done when
A real payload inventory, redistribution checklist and identified release blockers exist, without automatically publishing a release.

## Handoff
Write `For-Agent/Task-Report/windows-release/14.md` using `Tasks/Windows-Release/00_REPORT_TEMPLATE.md`. Include changed files, actual commands, PASS/FAIL/NOT_RUN evidence, limitations and the next-stage prerequisites. After the report is saved, update `For-Agent/Task-Report/windows-release/STATUS.md`. Do not automatically start the next numbered task.
