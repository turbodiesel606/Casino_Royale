# Windows Release — execute the next task (Codex entry point)

This document is the **single entry point** for the user command: "Execute the next pending Windows Release task from `Tasks/Windows-Release/00_RUN_NEXT.md`. Do exactly one task."

## Select exactly one task

1. Work in the **existing JobTracker repository/workspace**. Do not create a fresh project, move source files, or work from a detached ZIP copy. Changes and reports must remain in the same workspace across separate Codex chats.
2. Read the root `AGENTS.md`, this file, and `Tasks/Windows-Release/00_TASK_INDEX.md`. Apply other project instructions selectively as appropriate.
3. Open `For-Agent/Task-Report/windows-release/STATUS.md` **if it exists**. This is the persistent task-status index. If absent, select task **01** only; task 01 must initialize it.
4. Select the **lowest-numbered task whose status is not `DONE`**. If its status is `BLOCKED` or `IN_PROGRESS`, **resume that task**; do not skip it. If `STATUS.md` conflicts with an individual report or the repository, verify and reconcile the inconsistency before choosing a different task. If all 16 tasks are `DONE`, report completion and do not change the project.
5. Read **only** the selected task file and its explicitly relevant prerequisites. Read its existing task-specific report, if any, and the most recent previous-stage report if needed. Do **not** load all 16 tasks or historical reports into context.
6. Execute **that single selected task** in full. Do not start the following numbered task in this chat. Respect the user-approved scope and `AGENTS.md`: no CI/CD, Linux packaging, automatic updates, code signing or unauthorised Git/GitHub writes.
7. Before ending, write/update `For-Agent/Task-Report/windows-release/NN.md` according to `00_REPORT_TEMPLATE.md`. Include concrete changes, actual command results, tests that are `PASS` / `FAIL` / `NOT_RUN`, limitations, and next action.
8. **After writing the report**, update `For-Agent/Task-Report/windows-release/STATUS.md` with the task status (`DONE`, `BLOCKED`, or `IN_PROGRESS`), the report link, concise verification summary, active blockers, and the `Next task` field. `DONE` means this stage's implementation obligations are fulfilled; external VM/asset tests can be `NOT_RUN` only if explicitly documented. Unfinished essential implementation must remain `BLOCKED` or `IN_PROGRESS`.
9. Tell the user which task was executed, where the report is, whether it is `DONE`/`BLOCKED`, and what to do next. **Stop.**

## First session / bootstrap

Task **01** must:

- inspect actual instruction hierarchy and current source before changes;
- initialize `For-Agent/Task-Report/windows-release/STATUS.md` (16 rows, stages 02–16 `PENDING`), and its own `01.md` report;
- add a **small**, non-destructive pointer in root `AGENTS.md` pointing to this task entry point and the task-report location, while preserving existing instructions;
- update `For-Agent/Docs/build.md` only with minimal permission/description for adding **implemented, verified** release commands, not fictional future command lines;
- create a stable requirements document `For-Agent/Docs/windows-release-requirements.md`, or reconcile with an existing one;
- inspect `.gitignore`: the current project ignores `Tasks/`. Adjust that rule narrowly so `Tasks/Windows-Release/` can be version-controlled while unrelated existing `Tasks/` content remains ignored. Do not run `git add` / `git commit` without explicit authorization.

All detailed documentation synchronization is still reserved for task 15.

## Persistent locations

- Task definitions: `Tasks/Windows-Release/01_*.md` through `16_*.md` (read-only task inputs; do not edit unless the user asks).
- Status index: `For-Agent/Task-Report/windows-release/STATUS.md`.
- One report per task: `For-Agent/Task-Report/windows-release/NN.md`.
- Stable build commands: `For-Agent/Docs/build.md`.
- Stable accepted requirements: `For-Agent/Docs/windows-release-requirements.md` (created in task 01).
- Existing historical reports elsewhere in `For-Agent/Task-Report/` must be preserved.

## Reliability rules

- Prior task reports are a **handoff**, not proof that the code still matches them. Check source, diffs, and tool output when a fact is material.
- Never claim success without real verification. Distinguish `PASS`, `FAIL`, and `NOT_RUN`, especially for untested clean Windows VMs or missing GitHub assets.
- If an essential prerequisite is missing, record `BLOCKED`, leave clear resolution steps, and **do not silently skip to the next task**.
- Do not automatically commit, push, publish to GitHub, delete existing build trees, or remove user data. Existing project Git restrictions continue to apply.
- Preserve the exact task numbering and scope. No merging or omitting steps.
