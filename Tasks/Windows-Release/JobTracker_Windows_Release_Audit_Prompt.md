# JobTracker — Audit of the 16 Windows Release Tasks

## Objective

Independently audit the current project against
`JobTracker_Windows_Release_16_Tasks_Acceptance.md`.

Determine which expected results of the 16 tasks have actually been delivered, which are partially delivered, which are missing, and which remain unverified. Then produce a concrete list of remaining work in the correct order.

**This is an audit, not an implementation task. Do not automatically fix anything. Modifying the `src/` directory is prohibited under all circumstances.**

Work with the project's current working tree, including existing uncommitted changes. Even if you performed the earlier tasks in this same chat, do not treat your own previous responses or `DONE` labels as evidence.

## 1. Sources and Audit Scope

Find the primary checklist among the attachments in the current chat or at:
`Tasks/Windows-Release/JobTracker_Windows_Release_16_Tasks_Acceptance.md`.

Read it in full: all items 01–16, acceptance criteria, general reporting requirements, artifact summary, final matrix, and exclusions from scope.

For clarification, use the existing:
- `Tasks/Windows-Release/00_AGREED_REQUIREMENTS.md`, `00_COVERAGE_MATRIX.md`, `01_*.md` … `16_*.md`;
- `For-Agent/Docs/windows-release-requirements.md`;
- `For-Agent/Task-Report/windows-release/STATUS.md` and `01.md` … `16.md`;
- `AGENTS.md`, applicable nested instructions, `For-Agent/Docs/`, `.agents/skills/`, and related project documents.

Read targeted sections of additional documents as needed. Do not reread the entire chat history instead of analyzing the implementation. If the primary checklist is missing, report that fact; do not present a reconstruction from memory as a complete audit against it.

This is a separate audit of **all 16 stages**, not execution of the next task through `00_RUN_NEXT.md`. Earlier instructions to “fix,” “perform the next stage,” or “update STATUS.md” are not assignments for this session. Other applicable project rules remain in effect.

If documents contradict each other, quote both formulations and record a `REQUIREMENT_CONFLICT`; do not silently choose the more convenient interpretation. Do not add new mandatory requirements. Linux packaging, CI/CD, in-application automatic updates, and actual code signing are deferred and are not unfinished work within these 16 tasks. Preserving Linux Debug and checking the Qt 6.10.3 requirements remain within the audit scope.

## 2. Strict No-Implementation-Changes Mode

**`src/` may only be read.** Do not create, modify, format, delete, move, or rename files or directories within it, change their attributes/permissions, or allow scripts or generators to write there. This prohibition also applies to a “small fix just to get the tests running.”

Do not modify any other existing project files either: CMake, presets, packaging, tests, QML, instructions, original tasks, the checklist, previous reports, or `STATUS.md`. Do not commit, switch branches, run reset/restore/clean/stash, or publish anything to GitHub. Do not install tools or system components without separate authorization.

Only the following are allowed:
1. A new, separate audit report directory, described below.
2. New isolated build/staging/test directories outside the source tree that do not overwrite previous results. They may contain logs, temporary test data, and auxiliary verification scripts, but must not substitute for the implementation being audited.

Before running checks, record HEAD, the working-tree state, and the list of files already modified. Save a snapshot of paths and SHA-256 content hashes for files in `src/`, including untracked files, plus information about links without traversing external targets. After the checks, compare against the initial snapshot and inspect working-tree changes. Do not attribute pre-existing changes to yourself.

If an unexpected write to `src/` or any other existing file is detected, stop potentially modifying commands and describe the affected paths. Do not automatically roll back changes, as this could destroy the owner's work.

If a proposed defect fix requires modifying `src/`, record it among the limitations and future work. **Do not make the change, and do not bypass the prohibition by moving business logic into another directory.**

## 3. Check Every Requirement

Build a coverage matrix with a separate row for every checklist item, not merely 16 summary rows. Assign stable identifiers such as `T01-01`, `T01-02`, and `T12-05`; use `GLOBAL-xx` for general criteria. Preserve the original order and meaning of the requirements.

For each row, provide:
- The requirement and a reference to its source item.
- The actual implementation and evidence.
- Implementation status and, separately, verification status.
- The specific gap, risk, or limitation.
- A reference to the remaining work, if any is required.

**Implementation statuses:**
- `IMPLEMENTED` — the requirement is fulfilled and supported by the implementation.
- `PARTIAL` — only part of it is fulfilled.
- `NOT_IMPLEMENTED` — the requirement is not implemented; state where you looked for evidence.
- `UNDETERMINED` — the available materials are insufficient to reach a conclusion.
- `N/A` — only when non-applicability is justified and permitted by the task.

**Verification statuses:** `PASS`, `FAIL`, `NOT_RUN`. For `PASS`, specify the method: configuration/code inspection, automated test, or actual execution. A documentation requirement can be verified by reading; whether the installer works on clean Windows cannot be verified merely by reading an `.iss` file.

The absence of a VM, SDK, or published GitHub asset means the corresponding test cannot be performed, not necessarily that the implementation is missing. Conversely, the existence of a file or a `DONE` label does not prove that a requirement is fulfilled.

Account for the progression between stages: the intermediate local installer from task 07 may have been replaced by the Online Installer in task 08, and the initial `PENDING` statuses from task 01 should have changed. Do not require intermediate states to be restored. Assess historical completion using the available materials; do not replace missing evidence with certainty.

Assess equivalent paths, script names, and archive formats allowed by the checklist based on actual behavior and documentation. Do not turn an example filename into a mandatory requirement.

## 4. Evidence and Safe Verification

For implementation findings, provide exact `path:lines`, target/preset names, parameters, and an explanation of how they produce the required behavior. For verification, provide the actual command, working directory, tool versions, exit code, and log path. Do not expose secrets or the contents of personal CVs.

Distinguish `CURRENT_AUDIT` — performed now — from `PREVIOUS_EVIDENCE` — supported by earlier materials. For earlier logs, verify whether they apply to the current code and artifacts. An outdated log is not evidence of current success.

Inspect artifacts on disk rather than relying solely on mentions in reports: EXE, staging, archive, checksum/manifest, Setup, and notices. Trace the relationship between the CMake version, EXE, payload, embedded SHA-256, and release URL. Recalculate checksums where the files are available. A missing old artifact and a missing mechanism for generating it are different findings. A file timestamp alone does not prove which sources were used to build it.

Perform the available safe configure/build/tests/deploy/package checks of the existing implementation. Inspect scripts for side effects before executing them. Use separate output directories and process-local environment settings only; do not modify presets or sources for the audit. Document deviations from the standard commands: successfully checking an adapted command does not, by itself, verify the documented command.

Do not allow the application or tests to run against the owner's real SQLite database, CVs, or settings. Data-path isolation must be confirmed from the current code: changing the working directory alone is not sufficient. Potentially unsafe installation, uninstallation, UAC, runtime prerequisite, and data-cleanup tests may only run in an available disposable VM/Sandbox/test environment with dummy data, not on the owner's primary system.

Do not terminate the user's processes, disable system protection, or automatically elevate privileges. If no safe environment is available, record `NOT_RUN`, the reason, and the exact scenario for later verification. Do not fix a failing test during this audit.

One executed test may support multiple requirements. Do not run a full rebuild for every row. A single environment blocker must not prevent independent static inspection of the remaining stages.

## 5. Pay Particular Attention to Critical Dependencies

Check the following in addition to, not instead of, the full checklist:
- Preservation of Debug/Linux, exactly Qt 6.10.3 for the release, and separation of build/deploy/package.
- Actual Qt/QML/QSQLITE contents and independence from the developer's SDK/PATH.
- Consistency between the archive, version, GitHub asset URL, and verified SHA-256; network failures and safe extraction.
- Both MSVC Runtime branches, with no accidental app-local runtime in the branch documented as using the system runtime.
- Independence of per-user/all-users installations, rejection of a shared physical binary directory, upgrade/reinstall/downgrade behavior, and the risk of different versions sharing SQLite data.
- Manual application closure, safe cancellation, data preservation by default, UAC/profile ownership, the second installation, and unsafe data-deletion paths.
- Consistency between instructions and actual commands, and between previous reports and the current implementation.
- Licenses, provenance, and notices for the files **actually distributed**. Where external information is needed to reach a conclusion, use official sources and record the URL and verification date. If verification is not possible, explicitly state the limitation; do not present a technical audit as a legal guarantee.

## 6. Remaining Work

Define concrete, small work items ordered by dependencies and risk. Do not execute them now.

For each work item, provide:
`ID → priority → checklist items → finding → required action → expected files → acceptance check → dependencies → responsible party`.

Priorities: `P0` — data-loss risk/critical security; `P1` — mandatory functionality or release blocker; `P2` — other mandatory gaps. List optional improvements separately and do not count them as violations of the assignment.

Also specify `src/ required: NO / YES / UNKNOWN`. For `YES` or `UNKNOWN`, justify the conclusion, identify the expected area, and state that a separate decision from the owner is required. Do not propose silently lifting the prohibition.

Separate the results into:
1. Missing or incorrect implementation that needs to be fixed.
2. Implemented functionality whose verification has not been performed.
3. External actions required from the owner: VM, tools, asset publication, or licensing decisions.
4. Optional improvements outside the original assignment.

Do not write “finish the installer” without specifying the behavior, reason, and acceptance criterion. If all mandatory functionality is implemented but `NOT_RUN` checks remain, say exactly that.

## 7. Output Files

Create a new directory that did not previously exist:
`For-Agent/Task-Report/windows-release/acceptance-audit-<YYYYMMDD-HHMMSS>/`.

Save the following there:
- `AUDIT.md` — a summary of all 16 tasks, significant findings, discrepancies with previous reports, verification scope, and overall conclusion.
- `COVERAGE.md` — the complete item-by-item requirements matrix with evidence and results.
- `REMAINING_WORK.md` — the prioritized list of remaining actions from section 6.
- `logs/` and supporting snapshots — only the evidence necessary for the audit actually performed, without secrets or personal data.

Do not update `STATUS.md`, `01.md` … `16.md`, or the checklist itself. Record discrepancies with them in the new report. If saving the report at this location is impossible, provide it in your response and clearly state that the files were not created.

`AUDIT.md` must include the date, HEAD, initial working-tree state, checklist version/hash, tools, audited artifacts, and the result of the `src/` integrity comparison.

## 8. Final Chat Response

Write your response and reports in English. In the chat, provide:
1. A direct answer: whether all expected results have been delivered, and what cannot be confirmed.
2. A 16-row table: task number, implementation result, what was verified, and what remains.
3. The most important gaps and the order of the next actions.
4. Separate conclusions for **implementation completeness** and **readiness for publication** (`GO` / `NO-GO` with reasons). Mandatory unperformed checks prevent an unconditional `GO`.
5. Exact paths to the created reports and confirmation of whether `src/` changed.

Do not claim “everything is ready” merely because the status says `16/16 DONE`, and do not promise that there are no errors whatsoever. State the limits of the evidence.

**Perform the audit and prepare the remaining-work list. Stop after reporting. Do not fix any of the findings, especially anything in `src/`.**
