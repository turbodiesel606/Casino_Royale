# Task 16 — Independent final review and manual GitHub release handoff

**Execution protocol:** Run only when selected by `Tasks/Windows-Release/00_RUN_NEXT.md`. Record the final independent review under `For-Agent/Task-Report/windows-release/16.md` and update `STATUS.md` after the report. Do not launch Linux packaging or CI/CD.


**Rules for this task:** Read `AGENTS.md`, `For-Agent/Docs/build.md`, `For-Agent/Docs/windows-release-requirements.md` and `For-Agent/Task-Report/windows-release/STATUS.md` and the previous stage report if present. Use the repository as source of truth. Change only what this stage needs; preserve C++/QML business logic, storage paths, SQLite migrations, and existing Debug/Linux workflows. No CI/CD, Linux packaging, automatic updates, code signing or unauthorized GitHub publication. Run all feasible checks, distinguish pass/fail/not run, write this stage’s separate report, update `STATUS.md`, and STOP after this stage. Do not delete existing builds or user files without authorization.

## Goal
Perform a final consistency check of the entire staged implementation and deliver a concise, evidence-based handoff. Do not start Linux packaging or CI/CD.

## Work
1. Reinspect produced CMake/Qt deployment, installer, runtime branches, version checks, user-data protection and agent instructions for gaps. Verify that every original requirement section 1–22 (including 22.1–22.13) is implemented, deferred explicitly, or listed as a release blocker.
2. Rerun targeted checks if late changes warrant it; do not rerun complete builds just because documentation was edited.
3. Confirm exact commands/artifacts and explain the manual sequence: build → test → stage → archive + SHA → publish **only with owner approval** to `https://github.com/turbodiesel606/Releases` tag `v<version>` → build matching version-specific Online Setup with embedded expected SHA → manually publish installer after validation.
4. Document that if the versioned asset is absent, the real GitHub download E2E test is NOT RUN, not PASS. Never fabricate tests on a clean Windows VM.
5. Final report: implemented; files changed; exact commands; artifacts that exist; pass/fail/not run evidence; public-release blockers; open risks (Qt licensing, signing, mixed-version shared DB); agent documentation synchronization; next manual steps. Include what was intentionally not implemented (CI/CD, Linux packaging, auto-update, signing).

## Done when
One coherent release readiness report and up-to-date progress file are available. Stop; do not automatically publish, refactor unrelated code, or start the next phase.

## Handoff
Write `For-Agent/Task-Report/windows-release/16.md` using `Tasks/Windows-Release/00_REPORT_TEMPLATE.md`. Include changed files, actual commands, PASS/FAIL/NOT_RUN evidence, limitations and the next-stage prerequisites. After the report is saved, update `For-Agent/Task-Report/windows-release/STATUS.md`. Do not automatically start the next numbered task.
