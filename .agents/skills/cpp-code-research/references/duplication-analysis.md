# Duplication Analysis

Read this reference only for architecture, maintainability, duplication, or overlapping-responsibility research.

## Search By Responsibility

Inspect exact, structural, and semantic duplication across:

- classes, structs, functions, methods, and helpers;
- controllers, models, services, repositories, storage classes, and bootstrap wiring;
- validation, normalization, formatting, filtering, selection, error handling, cancellation, and transactions;
- SQL statements, schema migration operations, test fixtures, test setup, and CMake target definitions;
- model roles, signals, properties, and repeated property-update sequences.

Search by responsibility and behavior, not only identical text. Compare similarly named methods, repeated branches, constants, SQL fragments, contracts, and state transitions.

## Evidence Standard

For each possible cluster:

1. Inspect every relevant implementation and cite its file, line, and symbol.
2. Identify the responsibility, invariants, lifecycle stage, owner, thread context, domain meaning, and reasons to change.
3. Classify the relationship as one of:
   - exact duplication;
   - structural duplication;
   - semantic duplication;
   - overlapping responsibility;
   - intentional similarity.
4. Identify an existing implementation that could be the canonical owner. Do not propose a new abstraction until reuse or extension of existing code has been found insufficient.
5. Select one direction:
   - reuse the existing implementation;
   - extend the existing implementation;
   - extract a shared implementation;
   - keep the implementations separate.
6. Evaluate consolidation risks: dependency direction, coupling, ownership, lifetime, thread affinity, QML contracts, storage and schema boundaries, tests, and CMake registration.

Similar fields, syntax, or control flow are not enough. Keep implementations separate when their domain meaning, lifecycle stage, ownership, thread context, invariants, or reasons to change differ.

## Deliverable

Group findings by repeated responsibility. For every cluster include:

- Classification.
- Affected files, lines, and symbols.
- Repeated responsibility and shared invariants.
- Existing canonical-owner candidate.
- Recommended direction and why.
- Consolidation risks and required test or contract changes.

Also list intentional similarities that should remain separate and explain the boundary that justifies them. Order consolidation recommendations by maintenance risk and expected benefit.
