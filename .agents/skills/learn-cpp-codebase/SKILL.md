---
name: learn-cpp-codebase
description: Teach the JobTracker C++ codebase progressively from scratch and save a human-facing result after every phase. Use when Codex needs to onboard a learner, explain the complete backend architecture, provide a guided codebase tour or study plan, trace end-to-end workflows, explain Qt and QML-facing contracts, or connect C++ modules with storage and tests.
---

# Learn the JobTracker C++ Codebase

Teach the codebase as a connected system. Build the learner's mental model progressively instead of producing an alphabetical file summary.

## Required Context

Read these files before teaching:

1. `AGENTS.md`
2. `For-Agent/Docs/architecture.md`
3. `For-Agent/Docs/artifacts.md`
4. `For-Agent/Docs/coding-style.md`
5. `For-Agent/Docs/testing.md`

## Teaching Rules

- Assume no prior knowledge of JobTracker. Ask about the learner's C++ and Qt familiarity only when it would materially change the explanation; otherwise begin with plain language.
- Teach responsibilities, ownership, dependencies, and data flow before low-level implementation details.
- Explain C++ or Qt syntax only when the learner requests it or when it is necessary to understand behavior.
- Introduce project terms in a short glossary before relying on them.
- Cite the relevant file and symbol for every important architectural or behavioral claim.
- Distinguish verified behavior, architectural guidance, inference, and known gaps.
- Explain one coherent subsystem or execution path at a time. Do not dump the entire source inventory without interpretation.
- Preserve the phase order below for a complete from-scratch tour. For a narrower request, use only the relevant phases while retaining their internal order.
- End each phase with a concise recap, recommended files to read, and optional comprehension questions.
- Remain read-only with respect to production code. Create the required learning artifacts, but do not implement, refactor, or fix code unless the user explicitly changes the task scope.

## Phase 1: Project Inventory

Establish the physical shape of the project before explaining behavior.

1. Inspect `CMakeLists.txt`, `src/main.cpp`, C++ files under `src/`, and tests under `tests/`.
2. Identify executable and test targets, source directories, QML resources, and generated resources.
3. Group files by architectural layer and product domain rather than listing them alphabetically.
4. Create a short glossary covering the project-specific meaning of domain object, model, proxy model, controller, service, repository, storage, bootstrap, QML context property, and managed file.

Deliver a directory-and-target map that explains why each area exists.

## Phase 2: Application Startup

Trace the startup chain from the process entry point into the live application graph.

1. Start at `src/main.cpp`.
2. Follow startup into `App` and `AppBootstrap`.
3. Explain application creation, exception boundaries, dependency construction order, QObject ownership, signal connections, QML context-property exposure, QML engine loading, and shutdown-relevant lifetime ordering.
4. Connect every constructed backend object to its consumers.

Deliver a startup sequence and dependency-wiring explanation.

## Phase 3: Architecture Map

Identify the major backend roles and their dependency direction.

For each important type, record:

- Domain and layer.
- Primary responsibility.
- Owned and non-owned dependencies.
- Public C++ contract.
- QML-facing contract, when present.
- Storage interaction, when present.
- Related tests.

Map domain entities, models, proxy models, controllers, services, repositories, storage, bootstrap classes, and shared utilities. Confirm that dependencies follow the documented direction and explicitly note exceptions or uncertainty.

Deliver a compact ownership-and-dependency map.

## Phase 4: Domain-by-Domain Study

Study domains in this order:

1. Jobs: `src/jobs/`.
2. CVs: `src/cvs/`.
3. Companies and contacts: `src/directory/`.
4. Dashboard: `src/dashboard/`.
5. Shared infrastructure: `src/common/`, `src/storage/`, and `src/app/`.

For each domain:

1. Begin at its user-facing controller or read model.
2. Follow its models, services, repositories, domain values, and shared helpers.
3. Explain who owns the authoritative state and who exposes derived state.
4. Identify signals, model notifications, cross-domain coordination, and persistence boundaries.
5. Match the domain to its tests and known gaps.

Deliver one self-contained domain lesson at a time.

## Phase 5: Vertical Workflow Traces

Trace complete user operations instead of stopping at class boundaries.

For each representative workflow, follow this shape when applicable:

`QML action -> controller -> service -> repository -> SQLite or managed file -> model refresh -> QML update`

At every step, identify:

- Caller and receiving symbol.
- Input and output data shapes.
- Validation and transformation.
- Thread and ownership boundary.
- Persistence or filesystem effect.
- Model, property, or signal notification.
- User-visible result and failure path.

Include the most important current workflows, such as application creation, initial hydration, stable-ID selection through filtering or sorting, CV import or opening, company linkage, dashboard projection, and schema startup or migration.

Deliver numbered end-to-end execution traces with exact source anchors.

## Phase 6: Qt-Specific Contracts

Explain Qt mechanisms in the context where JobTracker uses them:

- `QObject` ownership and parent-child lifetime.
- Signals, slots, direct delivery, and queued delivery.
- `Q_PROPERTY`, notify signals, and constant model properties.
- `Q_INVOKABLE` or other QML-callable commands.
- `QAbstractItemModel` roles and change notifications.
- Proxy filtering, sorting, linked views, and stable-ID selection.
- QML-facing proxy indexes versus canonical domain IDs.
- GUI-thread model mutation and Qt SQL thread affinity.

Connect each concept to at least one concrete project example and test when available.

## Phase 7: Storage and Schema

Explain durable data from database startup through domain hydration.

1. Trace storage-path resolution and SQLite connection ownership.
2. Explain schema initialization, version checks, forward migrations, transactions, rollback, and foreign-key verification.
3. Map tables and relationships to domain values and repositories.
4. Explain prepared queries, row-to-domain hydration, serialization, and contextual error propagation.
5. Explain managed CV file staging, finalization, cleanup, reconciliation, and crash-recovery limits.
6. Identify which product data remains non-persisted.

Treat the live migrator, repository queries, and storage tests as the source of truth.

## Phase 8: Tests and Final Synthesis

Use tests as executable documentation.

1. Match each important subsystem and workflow with its test target and representative test cases.
2. Explain what each test proves rather than merely naming the test file.
3. Identify meaningful missing coverage and behavior that requires manual verification.
4. Produce a final compact architecture map.
5. List the most important startup, persistence, selection, and cross-screen execution paths.
6. Provide a recommended source-reading order for independent study.
7. Summarize current architectural risks, limitations, and unverified areas.
8. Offer a short teach-back checklist so the learner can test their mental model.

## Phase Artifact Rules

Treat the saved artifact as part of each phase's definition of done, not as an optional final export.

1. Save every phase result directly under `For-Human/Architecture/`.
2. Use one Markdown file per phase with these names:
   - `cpp-Architecture-YYYY-MM-DD-HHMM-phase-01-project-inventory.md`
   - `cpp-Architecture-YYYY-MM-DD-HHMM-phase-02-application-startup.md`
   - `cpp-Architecture-YYYY-MM-DD-HHMM-phase-03-architecture-map.md`
   - `cpp-Architecture-YYYY-MM-DD-HHMM-phase-04-domain-study.md`
   - `cpp-Architecture-YYYY-MM-DD-HHMM-phase-05-workflow-traces.md`
   - `cpp-Architecture-YYYY-MM-DD-HHMM-phase-06-qt-contracts.md`
   - `cpp-Architecture-YYYY-MM-DD-HHMM-phase-07-storage-schema.md`
   - `cpp-Architecture-YYYY-MM-DD-HHMM-phase-08-tests-synthesis.md`
3. Use the local Asia/Baku completion time for `YYYY-MM-DD-HHMM`. Put `Created: YYYY-MM-DD HH:MM local time (Asia/Baku)` immediately after the title.
4. Include the phase status, inspected scope, learner-oriented explanation, architecture or data-flow relationships, exact source anchors, recap, recommended reading, comprehension questions, and unverified areas.
5. Save the artifact before reporting the phase as complete. Link the saved file in the phase response.
6. During Phase 4, create the phase artifact after the first completed domain lesson, update the same file after every remaining domain lesson, and mark it complete only after all listed domains are covered.
7. When resuming an unfinished phase, update its existing phase artifact instead of creating duplicates. Never overwrite an artifact from a separate completed learning run.
8. Use `For-Human/Architecture/` for this skill's learning outputs. Do not duplicate them under `For-Agent/Research/` unless the user explicitly requests both locations.

## Delivery Modes

- For interactive learning, teach one phase or domain per response and let the learner choose when to continue or go deeper.
- For a single complete report, cover every phase in order but keep file inventories compact and emphasize relationships and workflows.
- In every delivery mode, apply the phase artifact rules and save each phase independently.
- Do not run builds or tests for explanation-only work. If the user requests current build or test verification, follow `cmake-build-debug` and the documented build commands.

## Completion Standard

Finish only when the learner has received:

- A project inventory and glossary.
- A startup and dependency-construction explanation.
- An ownership and architecture map.
- Lessons for every C++ domain.
- Representative vertical workflow traces.
- Explanations of the Qt contracts used by the project.
- A storage and schema walkthrough.
- A test-to-subsystem map and final reading path.
- Eight saved phase artifacts under `For-Human/Architecture/`, with Phase 4 updated across all domain lessons.

State clearly which facts were not verified or which areas were intentionally deferred.
