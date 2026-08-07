---
name: review-code-for-human
description: Analyze and explain a requested JobTracker code fragment, function, method, class, or module for a human who already understands C++, Qt, QML, and standard-library syntax. Use for source-backed execution-flow walkthroughs, recursive nested-call tracing, architectural responsibility, design rationale, ownership/lifetime/thread/error analysis, and evidence-based improvement review; remain read-only unless implementation is explicitly requested.
---

# Review Code for Human

Produce a source-backed explanation of how requested code works, why its verified design choices matter, and where its real risks lie. Optimize for human understanding of behavior and interactions, not language instruction.

## Workflow

1. Identify the exact target and scenario. If the target is ambiguous, locate likely definitions and ask only when choosing incorrectly would materially change the analysis.
2. Read `AGENTS.md` and only the directly relevant `For-Agent/Docs/` guidance. Check recent research/review/task artifacts only when they overlap the requested scope; treat them as context, then verify current facts in live source.
3. Inspect the requested implementation, its actual callers, dependencies, connected QML or C++ contracts, and relevant tests. Use targeted symbol searches rather than a broad project survey.
4. Trace every project-defined call recursively. For each call, locate its real definition and follow the chain until reaching a simple infrastructure operation, an obvious Qt or standard-library operation, a trivial accessor, or code unrelated to the scenario.
5. Record evidence with file names, class and function names, and exact line numbers or tight line ranges whenever possible.
6. Separate confirmed facts from inference. Never infer a caller, thread, ownership rule, error path, or authorial intent without evidence.
7. Return the analysis using the required structure below.

## Analysis Boundaries

- Remain read-only. Do not modify code, create a patch, or run builds/tests unless the user explicitly requests it.
- Explain only architecture directly connected to the requested code.
- Assume the reader understands C++, Qt, QML, and standard-library syntax.
- Do not explain ordinary syntax, declarations, `if`, `return`, or routine calls. Explain syntax only when it materially affects ownership, lifetime, overload resolution, template deduction, threading, or behavior.
- Group code by purpose; do not narrate it line by line.
- Base claims on current project files. Treat comments, tests, and docs as supporting evidence, not substitutes for implementation.
- When a reason cannot be proven, write: “The exact reason is not visible from the code. The most likely explanation is ...”

## Output Contract

Use the following headings and order in the response.

## Main Goal

State concisely:

- the problem the code solves;
- its start-to-finish execution flow;
- the important internal calls;
- the evidence-backed design rationale;
- the main confirmed problems and improvement opportunities.

## 1. Context and Architecture

Explain the relevant subsystem, the target's responsibility, collaborating entities, owned responsibility and boundaries, and its place in the scenario. Avoid a full-project architecture tour.

## 2. Overall Execution Flow

Describe the actual call origin, inputs, main stages, components invoked, and returned result or changed state. Include one verified chain such as:

`call source → target function → nested function → service → repository → result`

Replace the example nodes with actual symbols. Do not include assumed stages.

## 3. Deep Function Analysis

Divide the target into logical blocks. For each block explain:

- purpose and necessity;
- data read and modified;
- side effects;
- maintained conditions or invariants;
- errors and early returns;
- state already changed before failure, when applicable.

## 4. Analysis of All Nested Calls

For every relevant project-defined call, recursively provide:

- actual definition location and purpose;
- inputs and return value;
- side effects, errors, and edge cases;
- how the caller consumes the result.

Stop only at the recursion boundaries defined in the workflow. Summarize trivial boundary calls without teaching syntax.

## 5. Why It Is Implemented This Way

For each important verified decision, explain the problem solved, layer placement, operation order, parameter-passing choice, copy/move behavior, synchronous or asynchronous execution, thread choice, signal/callback/queue/service/repository use, realistic alternatives, and trade-offs. Clearly label inferred rationale with the required uncertainty phrase.

## 6. Ownership, Lifetime, and Threads

When applicable, map:

- object ownership and lifetime;
- captures, copies, and moves;
- nullable or invalidatable pointers;
- thread execution and return to the GUI thread;
- shared cross-thread data and synchronization;
- race, deadlock, use-after-free, and thread-affinity risks;
- cancellation and post-worker completion behavior.

Write “Not applicable” for this scenario rather than inventing concurrency or ownership concerns.

## 7. Error and State Handling

Explain state transitions, failure creation and handling, UI notification, prior side effects, transaction or partial-save behavior, the supported exception-safety level (`basic`, `strong`, `no-throw`, none, or not established), and the handling of repeated or conflicting calls.

## 8. Possible Improvements

Separate recommendations into:

### Confirmed Problems

Include only code-supported defects or risks such as lifetime, responsibility, duplication, error handling, threading, atomicity, testability, dependency, or invariant problems.

### Possible Improvements

Include non-mandatory readability, maintainability, testability, extensibility, safety, or performance improvements.

### Changes That Are Not Recommended

Identify tempting but unnecessary, excessive, or harmful changes. Do not introduce patterns for their own sake.

For every item specify:

- current problem or perceived problem;
- concrete change;
- expected benefit;
- possible disadvantage;
- severity: `critical`, `recommended`, or `optional`.

Do not present speculation as a confirmed problem.

## 9. Final Execution Model

End with a compact summary:

- **Input:** what enters the target.
- **Execution:** the main processing stages.
- **Output:** what is returned, saved, published, emitted, or forwarded.
- **Responsibility:** what the target owns and explicitly does not own.
- **Main Risks:** the three to five most important verified risks or limitations.

## Evidence Quality

- Prefer direct links or `path:line` references to current project files.
- State missing evidence and unverified behavior explicitly.
- Distinguish static source conclusions from build, test, or runtime verification.
- If no problem is supported, say so; do not manufacture findings to fill a section.
