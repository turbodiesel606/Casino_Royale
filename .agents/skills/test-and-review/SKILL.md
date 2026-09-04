---
name: test-and-review
description: Review and validate JobTracker changes before final response. Use when Codex needs an implementation review, risk check, build/test decision, final verification pass, scope audit, or concise summary of changed files, manual checks, limitations, and follow-up ideas.
---

# Test And Review

Use this skill for the final review and validation pass.

## Review Checklist

1. Confirm the change stays within the user's scope.
2. Inspect the final diff and confirm that no unrelated files, comments, or behavior changed.
3. Check whether `AGENTS.md`, docs, skills, or subagent instructions changed the expected workflow.
4. Check `For-Agent/Docs/artifacts.md` when review artifacts were created or reused.
5. Verify affected C++ changes follow the dependency, ownership, error, threading, storage, and Qt model rules in `For-Agent/Docs/architecture.md`.
6. Verify the test dependency remains strictly `tests -> production`: no production source, target, API, normal build path, or deployment step may depend on test code, Qt Test, fixtures, mocks, test-only settings, or test targets, and production must not be reshaped solely for tests.
7. Verify affected QML-facing properties, commands, signals, and model roles remain compatible with their consumers.
8. Confirm added, removed, or renamed C++, QML, and test files are registered correctly in `CMakeLists.txt`.
9. Check that changed logic and high-risk contracts have suitable tests under `For-Agent/Docs/testing.md`.
10. Confirm affected `For-Agent/Docs/` guidance was updated, or record that documentation review found no required update.
11. Determine applicable build and test verification from `For-Agent/Docs/build.md`; use `cmake-build-debug` for the documented commands.
12. Report verification results without claiming unverified behavior works.
13. List modified files, required manual checks, residual risks, limitations, and unverified areas.

## Review Stance

For code review requests, lead with concrete findings ordered by severity.

For implementation tasks, keep the final response concise and focused on what changed, why, and what was verified.

For documentation-only, workflow-only, or instruction-only changes, do not run a C++ build unless build behavior changed.

Do not claim code works unless it was verified.
