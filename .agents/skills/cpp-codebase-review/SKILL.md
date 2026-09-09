---
name: cpp-codebase-review
description: Review the complete JobTracker C++ backend architecture for global drift, cross-module duplication, overlapping responsibilities, ownership and threading risks, storage integrity, test coverage, and CMake integration. Use only for an explicitly requested whole-backend or codebase-wide architecture review, not for a bounded diff.
---

# C++ Codebase Review

Use this deliberate broad mode only when the requested scope is the complete C++
backend or a codebase-wide architecture, responsibility, consolidation, or
duplication assessment. Use `cpp-code-review` for ordinary changed-diff review.

## Context And Artifact Selection

1. Apply `AGENTS.md` and record current worktree state.
2. Read `For-Agent/Docs/architecture/overview.md`, then load each domain
   architecture document as its module enters the inventory.
3. Read `For-Agent/Docs/coding-style.md`. Read testing, build, extraction, and
   artifact guidance only when their concerns enter the requested review.
4. Inventory prior artifact filenames and metadata before opening any artifact.
   Read only the newest broad artifact that overlaps the review and any newer
   narrow artifact that materially updates it.

Artifacts are navigation aids, not evidence. Verify all current claims against
live source, CMake registration, tests, and the current diff.

## Deliberate Codebase Inventory

- Inventory every current production C++ module, its public and QML-facing
  contracts, its registered sources, and its matching test translation units.
- Map dependency direction, authoritative state, derived state, object ownership,
  lifetime, thread affinity, signal delivery, transaction ownership, filesystem
  effects, and publication paths.
- Inspect architecture drift, cross-module duplication, responsibility overlap,
  competing state or validation authorities, error-path divergence, and broad
  consolidation opportunities.
- Use targeted searches to find candidate patterns, then inspect every reported
  instance before classifying it. Similar syntax alone is not duplication.

## Review Priorities

Lead with findings ordered by severity:

- correctness, data loss, and behavior regressions;
- QObject ownership, lifetime, thread affinity, queued delivery, and shutdown;
- transaction, SQLite-connection, managed-file, and recovery integrity;
- broken model, signal, Q_PROPERTY, QML, or public contracts;
- architecture drift and responsibility overlap;
- harmful exact, structural, or semantic duplication;
- production architecture reshaped solely for tests;
- missing tests, CMake registration, or verification.

Preserve independent scrutiny for concurrency, ownership, lifetime, storage, and
data-integrity risks even when prior research exists.

## Evidence And Output

Use source, symbol, and tight line references for every material finding,
architecture claim, ownership or concurrency conclusion, and recommendation that
requires code changes. Routine inventory narration may reuse active verified
context without extra exploration.

Return:

- reviewed scope, worktree state, and modules inventoried;
- findings ordered by severity;
- architecture and dependency-direction assessment;
- duplication and responsibility-overlap map grouped by canonical owner;
- justified similarities that should remain separate;
- consolidation recommendations with ownership, dependency, contract, test, and
  migration consequences;
- verification performed, missing tests, manual checks, residual risks, and a
  final accept, revise, or block recommendation.

When saving a durable review, follow `For-Agent/Docs/artifacts.md` and write it
under `For-Agent/Review/`.

## Boundaries

- Remain read-only unless the user separately authorizes implementation.
- Do not run builds, CTest, or the GUI unless requested or included in the review.
- Do not recommend consolidation across different domain meanings, invariants,
  ownership, lifetime, thread affinity, or reasons to change.
