---
name: review-code-for-human
description: Produce an exhaustive, source-backed human review of a JobTracker code fragment, function, class, module, or end-to-end workflow. Use when Codex must begin with a verified logic flow, explain every relevant entity in context, trace the complete caller-to-dependency path, show every participating project-defined function body completely in consecutive snippets without reproducing header declarations, explain each snippet in detail, infer rationale with clear uncertainty, and assess architectural improvements. Keep the review read-only unless the user explicitly requests changes.
---

# Review Code for Human

Explain the complete relevant execution model for a reader who already understands C++, Qt, QML, and standard-library syntax. Optimize for understanding entities, responsibilities, interactions, state changes, ownership, threads, errors, and design trade-offs.

Treat "complete relevant code" as:

- the full body of every project-defined function, method, QML handler, or JavaScript function that participates in the traced scenario;
- consecutive implementation snippets that preserve every source line and its original order when a function is too long for one readable block;
- all meaningful success, failure, cancellation, retry, and publication branches in those bodies;
- prose summaries with clickable links for relevant header contracts, types, signals, properties, and member state instead of reproduced declaration code.

Do not dump unrelated files or unrelated methods merely to increase coverage. Completeness applies to the requested scenario and its real call chain, not the entire repository.

## Review Workflow

1. Identify the exact target and scenario. Resolve ambiguity from the repository when possible; ask only when different interpretations would produce materially different reviews.
2. Read `AGENTS.md` and the directly relevant `For-Agent/Docs/` guidance. Check recent research, review, or task artifacts only when they overlap the scope, and verify every current fact against live source.
3. Inspect the target, its real callers, every relevant project-defined nested call, connected QML or C++ contracts, state publication, and focused tests.
4. Build an internal entity inventory before writing. Include every QML component or handler, C++ class, struct, enum, request/result type, controller, worker, service, repository, storage helper, model, and signal that materially participates in the scenario.
5. For every inventoried entity, record its definition location, responsibility, owned state, owner and lifetime, thread affinity, inputs, outputs, callers, callees, and observable side effects. Mark genuinely inapplicable fields rather than silently skipping them.
6. Trace the scenario from its true entry point to its observable outcome. Follow every relevant project-defined call until reaching Qt, the standard library, SQLite or operating-system primitives, a trivial accessor, or logic unrelated to the scenario.
7. Collect implementation bodies in execution order. Do not reproduce header or standalone declaration code. Show each complete function body as one snippet or as consecutive labeled parts without omitting or reordering lines, and explain every snippet in detail immediately after it before continuing.
8. Build the diagram from verified symbols and transitions.
9. Audit coverage before answering: every diagram entity must have a prose definition and explanation, while every non-trivial project-defined call must have its complete implementation shown through source-faithful snippets and detailed explanations. Resolve missing coverage before producing the response.
10. Explain the stages in diagram order, then give evidence-aware rationale and architectural improvement notes.

## Completeness Rules

- Make completeness more important than brevity for this skill.
- Never replace relevant source lines with `...`, `// omitted`, summaries, pseudocode, or prose.
- Never show only the happy-path lines from a relevant function. Include its material guards, early returns, error branches, cancellation checks, cleanup, retries, and result publication.
- Never mention a meaningful project-defined nested call only by name. Show its complete called body and explain how its result returns to the caller.
- Do not reproduce header code or standalone declarations. Summarize the central and collaborating entities' contracts, ownership, member state, signals, properties, and relevant types in prose with clickable header links.
- If a participating function is defined inline in a header, show its complete function body because it is implementation; omit the surrounding declaration-only code.
- For a collaborating entity outside the central target, describe the contract and state needed for the scenario in prose and show the full body of every called function.
- After every implementation snippet, explain that snippet in detail before showing the next snippet.
- When omitting unrelated code from a file or class, state exactly what category was omitted and why. Do not hide omissions inside code blocks.
- Explain every entity that appears in the diagram or code. Do not collapse several non-trivial entities into a vague label such as "backend," "database layer," or "helper."
- Explain Qt, standard-library, SQLite, and operating-system calls at their behavioral boundary; do not attempt to reproduce external library implementations.
- Group only genuinely trivial accessors or value conversions, while still naming their locations and roles.
- For broad workflows, allow a long answer. If a hard output limit prevents completion, label the response as partial, list every uncovered entity or function, and never claim that the review is complete.

A review is incomplete if any of these are true:

- a diagram entity is not defined and explained;
- a relevant project-defined call has no source body shown;
- a relevant function body contains hidden gaps or ellipses;
- a request, result, state object, model, signal, repository, or service is used without explaining what it represents;
- ownership, lifetime, or thread affinity is material but omitted;
- an important failure or downstream publication path is described without its code;
- a standalone header or declaration block is reproduced without containing a participating inline function implementation;
- an implementation snippet is not followed by its detailed explanation;
- the final coverage audit contains an unexplained omission.

## Required Output Order

### 1. Logic Flow

Make this the first substantive section of the response. Do not place a summary, conclusion, or architectural preamble before it.

- Use a Mermaid `sequenceDiagram` for time-ordered collaboration, queued work, signals, callbacks, or thread transitions.
- Use a Mermaid `flowchart` for branching, validation, state transitions, or error paths.
- Use both when the scenario has significant collaboration and branching.
- Use exact class, function, signal, model, service, repository, and storage names from the source.
- Show the real caller, target, all important nested calls, relevant branches, and final state publication.
- Mark GUI-thread, worker-thread, queued, transactional, asynchronous, and ownership boundaries when they affect behavior.
- Do not add an unverified node merely to make the diagram look complete.

### 2. Complete Related Code, Step by Step

Walk through the diagram from entry to outcome. Number the stages and keep each stage tied to a diagram node or transition.

For every stage:

1. Give clickable declaration and definition locations, but do not reproduce declaration code.
2. Before first use, summarize the entity's contract, ownership, member state, signals, properties, and relevant types in prose.
3. Show the complete original body of every project-defined function, method, QML handler, or JavaScript function used by the stage as a source-faithful snippet.
4. If a body is long, split it into consecutive labeled parts such as `Part 1`, `Part 2`, and so on, preserving every source line and its original order.
5. Immediately after each snippet, identify every entity, type, parameter, local, member, dependency, and result introduced there and explain its role.
6. Explain that snippet in detailed execution order, including inputs, preconditions, invariants, branches, state reads and writes, copies and moves, side effects, errors, cleanup, outputs, and the next recipient of each result.
7. Only after explaining the current snippet may you show the next part or continue to another function. Follow every relevant nested project-defined call with its complete body through the same snippet-and-explanation pattern before returning to the caller.
8. Explain how the next stage consumes the result.

Use code blocks only for implementation bodies. Do not show `.hpp` include guards, includes, class or struct declarations, enum declarations, function prototypes, signals, properties, or member lists. Summarize those elements in prose with clickable source links. A participating function implemented inline in a header is the only exception: show its complete body while omitting unrelated declaration-only context.

Preserve the source exactly inside implementation snippets. Do not add explanatory comments that are absent from the source. Put a detailed explanation immediately below each block.

Include upstream invocation and downstream observable effects. For an end-to-end workflow, this normally includes the QML caller, C++ boundary, request and result types, controller state, worker dispatch, service orchestration, repositories or storage, model mutation, connected signals, and final UI handling when each is present in live source.

### 3. Why This Implementation Works This Way

Keep this section evidence-aware, but do not omit a major design choice already demonstrated by the code.

- State documented or directly provable rationale as fact.
- When rationale is not documented, write: `The exact rationale is not documented. The most likely reason is ...`
- Compare only realistic alternatives that illuminate the current choice.
- Explain the main trade-off involving responsibility placement, ordering, copying or moving, synchronous versus asynchronous work, transaction boundaries, signals, queues, repositories, or model publication when relevant.
- Do not present inferred authorial intent as confirmed history.

### 4. Architectural Improvements

Include this section only to the extent that the source supports useful advice.

For each relevant item, state:

- the current issue or limitation and its source evidence;
- the concrete architectural change;
- the expected benefit;
- the main cost or disadvantage;
- priority: `critical`, `recommended`, or `optional`.

Separate confirmed defects from optional design improvements. Consider responsibility boundaries, coupling, cohesion, ownership, threading, state and error handling, atomicity, testability, and duplication. Do not recommend a new abstraction or design pattern merely because it is possible. If no material architectural improvement is justified, say so directly.

### 5. Coverage and Verification

End with a coverage audit rather than an unsupported claim of completeness.

- List every material entity and project-defined function covered by the review.
- List any intentionally omitted file sections, methods, or entities and give the exact reason each is unrelated or a permitted boundary.
- State whether the analysis is static source review, build-verified, test-verified, or manually runtime-verified.
- State all missing evidence and unverified behavior explicitly.

## Evidence and Safety Rules

- Base the review on current source. Treat documentation, comments, artifacts, and tests as supporting evidence rather than replacements for implementation.
- Distinguish confirmed behavior from assumptions and inference.
- Cite exact symbols and tight line references wherever possible.
- Distinguish static source conclusions from build, test, and runtime verification.
- State missing evidence or unverified behavior explicitly.
- Remain read-only: do not edit code, create patches, build, or run tests unless the user explicitly requests those actions.
- Do not manufacture defects to fill the improvement section.
