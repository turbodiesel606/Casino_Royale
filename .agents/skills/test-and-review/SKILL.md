---
name: test-and-review
description: Review and validate JobTracker changes before final response. Use when Codex needs an implementation review, risk check, build/test decision, final verification pass, scope audit, or concise summary of changed files, manual checks, limitations, and follow-up ideas.
---

# Test And Review

Use this skill for the final review and validation pass.

## Review Checklist

1. Confirm the change stays within the user's scope.
2. Check that no unrelated files or behavior were changed.
3. Check whether `AGENTS.md`, docs, skills, or subagent instructions changed the expected workflow.
4. Check whether build/test verification is required by `For-Agent/Docs/build.md` and `For-Agent/Docs/testing.md`.
5. Run applicable verification, or explain why it is not applicable or impossible.
6. List modified files.
7. Identify manual checks, risks, limitations, and unverified areas.

## Review Stance

For code review requests, lead with concrete findings ordered by severity.

For implementation tasks, keep the final response concise and focused on what changed, why, and what was verified.

Do not claim code works unless it was verified.
