---
name: test-and-review
description: Review and validate JobTracker changes before final response. Use when Codex needs an implementation review, risk check, build/test decision, final verification pass, scope audit, or concise summary of changed files, manual checks, limitations, and follow-up ideas.
---

# Test And Review

Use this skill as the integrated incremental final review and validation pass.

## Incremental Final Verification

Start from the final diff, changed files, and verification evidence. Reuse the instructions, architecture context, source relationships, and contracts already established during implementation. Do not restart repository research, reread unchanged documentation, or retrace understood dependencies merely because implementation ended.

Expand only when the final diff introduces a new dependency, an implementation assumption changed, an unexpected build or test result occurs, a new risk becomes visible, or a concrete verification question requires broader evidence.

## Review Checklist

1. Inspect the final diff and changed files. Confirm the change stays within the user's scope and that no unrelated files, comments, or behavior changed.
2. Check whether `AGENTS.md`, docs, skills, or subagent instructions changed the expected workflow only when those files are part of the final diff or the implementation exposed a workflow conflict.
3. Check `For-Agent/Docs/artifacts.md` only when review artifacts were created or reused.
4. Verify the final diff against the applicable architecture documents already established for the task. Open another document, or reread one, only when the incremental expansion rules require it; use the overview only when routing is unclear or the review is explicitly system-wide.
5. Verify the test dependency remains strictly `tests -> production`: no production source, target, API, normal build path, or deployment step may depend on test code, Qt Test, fixtures, mocks, test-only settings, or test targets, and production must not be reshaped solely for tests.
6. Verify affected QML-facing properties, commands, signals, and model roles remain compatible with their consumers.
7. Confirm added, removed, or renamed C++, QML, and test files are registered correctly in `CMakeLists.txt`.
8. When testing is relevant, use `For-Agent/Docs/testing.md` to choose the directly associated test target or suite. Broaden to the full relevant test preset only under the risk gates in `AGENTS.md` or when targeted results expose broader impact.
9. After the implementation is stable, perform one documentation-impact check from the final changed behavior and contracts. Inspect only directly affected guidance, update it or record that no update is required, and do not repeat the check unless a later edit changes the impact.
10. Read `For-Agent/Docs/build.md` only when build/test commands are needed or build behavior changed. Use `cmake-build-debug` for the smallest meaningful affected production target and applicable tests; do not repeat an unchanged successful run without a concrete reason.
11. Report verification results without claiming unverified behavior works.
12. List modified files, required manual checks, residual risks, limitations, and unverified areas.

## Windows Release Verification

When the changed surface is release documentation, packaging or installer policy,
use `For-Agent/Docs/windows-release.md` for behavior, `build.md` for commands,
`testing.md` for test selection and `tests/deployment/Windows-Release-Checklist.md`
for acceptance cases. Reuse established context instead of reopening every stage.

Check version/asset/URL/hash identity against the actual manifest and generated
inputs. Keep production, test probes and installer harnesses separate. Treat
native harness tests, real scoped installation, GUI/clean-machine checks and
redistribution clearance as distinct evidence. A synthetic prior-version fixture
does not verify historical application migrations; a 404 verifies safe rejection
only; the staging probe does not launch JobTracker.

Carry unresolved results and the `packaging/windows/REDISTRIBUTION.md` publication
hold into the handoff. A later passing run does not erase earlier sharing failures
or establish their cause. For documentation-only changes, validate commands,
links and input preservation without repeating builds or real installations.
For numbered tasks, save only the selected report, update STATUS afterward and
stop; do not turn final validation into the next release stage.

## Full Independent Review

Completion alone is not a reason to invoke a full independent reviewer. Apply the canonical independent-review risk gates in `AGENTS.md` and use a reviewer only when one of those gates or an explicit user request applies.

## Review Stance

For code review requests, lead with concrete findings ordered by severity.

For implementation tasks, keep the final response concise and focused on what changed, why, and what was verified.

For documentation-only, workflow-only, or instruction-only changes, do not run a C++ build unless build behavior changed.

Do not claim code works unless it was verified.
