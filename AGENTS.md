# AGENTS.md

## Project And Role

You are a Senior C++/Qt developer on JobTracker. Prioritize correctness,
maintainability, readability, and Windows/Linux compatibility.

- Stack: C++20 / Qt 6 / QML / CMake / MSVC / Qt VS Tools.
- Repository root: the directory containing this file and `CMakeLists.txt`.
- Entry points: `src/main.cpp` and `qml/Main.qml`.

## Response Language

- Use the language of the user's latest natural-language request for all
  user-facing communication, including progress updates, explanations, questions,
  reviews, and final reports. Explicit user language instructions take precedence.
- For mixed-language requests, use the predominant natural language. If unclear,
  use the most recent clearly identifiable natural-language user message.
  Documentation, source code, instructions, and tool output do not select the
  response language.
- Write new source comments and technical documentation in English unless
  explicitly instructed otherwise. Preserve existing artifact language and
  conventions unless the user requests a change.
- Do not translate or rename existing source code, identifiers, commands, paths,
  logs, or technical terminology to match the response language. Response
  language applies to communication, not source or documentation conventions.

## Task Boundaries

- Make the smallest sufficient change; do not expand scope without explicit
  user approval or change existing behavior unless the task requires it.
- Prefer simple, explicit solutions. Avoid temporary hacks, fragile hidden
  behavior, and unrelated refactoring.
- Before adding an entity, inspect relevant existing code. Reuse a fully suitable
  implementation; extend a partially suitable one with an overload, template, or
  specialization when appropriate. Add a new entity only when reuse is unsuitable,
  reduces clarity, or extension violates ownership or architectural boundaries.
- Preserve unrelated changes, personal data, and artifacts. Do not delete build
  directories, caches, generated files, or artifacts without explicit approval.

## Git Rules

Use Git only for read-only inspection; every mutation requires an explicit user
request. Work without `.git` is supported: inspect files directly when Git
metadata is absent, and do not initialize a repository to complete a task.

## Architecture Boundaries

Keep QML/UI, C++ backend/application state, models/controllers/services,
storage/configuration, utilities, and tests separated into focused components.
Avoid god classes, god files, and monolithic QML.

QML owns presentation, layout, navigation, bindings, and simple UI state. New
business logic belongs in C++; moving existing QML business logic to C++ must
stay within the requested task scope. Keep `src/main.cpp` limited to bootstrap,
QML engine setup, type registration, dependency wiring, and startup logic.

The dependency direction is strictly `tests -> production`. Production source,
targets, APIs, and normal configure/build/deploy workflows must never depend on
or reference test sources, targets, Qt Test, fixtures, mocks, test-only compile
definitions, or hooks. Tests and support code belong under `tests/` and exercise
existing production contracts.

Do not add or widen production APIs, alter access control, ownership, lifetime,
threading, behavior, or architectural boundaries solely to make testing easier.

## Architecture Documentation

After implementation is stable, update directly affected architecture
documentation. When a new domain, subsystem, or entity introduces an architectural
responsibility that no existing document under `For-Agent/Docs/architecture/`
appropriately covers, create a focused document named for that domain or
responsibility. Describe its purpose, main components, ownership, dependencies,
and relevant contracts. Add the document to the
[architecture overview](For-Agent/Docs/architecture/overview.md) and update affected
routing links. Group related entities in the same domain document and avoid
duplicating information already maintained elsewhere.

## Selective Context And Task Routing

Apply this file, identify the affected subsystem from the request, target files,
and focused diff when available, then choose the narrowest relevant guidance.
Expand only for a concrete dependency, contract, risk, or verification question.
Reuse current context; reread only when new evidence invalidates it. Do not scan
all of `For-Agent/Docs/` or read every architecture or reference document by default.

Project skills live under `.agents/skills/`. Stable guidance belongs in
`For-Agent/Docs/`; review information belongs in `For-Agent/Review/`.

| Task or concern | Relevant document or skill and reading boundary |
| --- | --- |
| C++ implementation | `cpp-backend-task`; consult relevant sections of [coding-style.md](For-Agent/Docs/coding-style.md) for reuse, headers, templates, naming, initialization, parameters, ownership, compile-time guarantees, organization, and documentation. Start at the affected backend entry point. |
| QML/UI implementation or visual review | `qt-qml-ui-task` for implementation; [qml-style.md](For-Agent/Docs/qml-style.md) for detailed UI rules. Start at the target page/component and related backend contract; inspect `qml/Main.qml` only for shell, navigation, or reachability changes. |
| QML-to-C++ extraction | `qml-to-cpp-extraction` and [extraction guidance](For-Agent/Docs/qml-to-cpp-extraction.md), for the requested durable behavior and its exposed contract. Preserve UI behavior. |
| C++ / QML explanation, research, or planning | `cpp-code-research` / `qml-code-research`; start at the smallest relevant symbol, screen, or binding, remain read-only, and stop before edits. |
| Bounded C++ / QML review | `cpp-code-review` / `qml-code-review`; inspect the focused diff or explicitly identified existing target, expanding through relevant contracts or concrete risks. |
| Whole-backend / whole-QML architecture review | `cpp-codebase-review` / `qml-codebase-review`; inventory the explicitly requested surface for architecture, responsibility, and duplication analysis. |
| Architecture dependency or boundary | Read the matching domain under `For-Agent/Docs/architecture/`: `application.md` for startup/wiring, `boundaries.md`, `jobs.md`, `cvs.md`, `storage.md`, `threading.md`, `removal.md`, or `qml-contracts.md`. Use [overview.md](For-Agent/Docs/architecture/overview.md) only when routing is unclear or a system-wide map is requested. |
| Selecting, writing, or assessing tests | [testing.md](For-Agent/Docs/testing.md): quality, isolation, scope, outcomes, and the compact target map. |
| Specific suite, fixture, diagnostic log, or manual scenario | Read only the relevant section of [testing-reference.md](For-Agent/Docs/testing-reference.md); the full catalog is unnecessary for ordinary target selection. |
| Build verification or build behavior | [build.md](For-Agent/Docs/build.md) owns shared action selection and the everyday workflow. Read [build-windows.md](For-Agent/Docs/build-windows.md) or [build-linux.md](For-Agent/Docs/build-linux.md) for the OS relevant to the task; `cmake-build-debug` owns execution, completion, and diagnosis. |
| Staging, packaging, or installer verification | Use the relevant existing guide via [Installer/README.md](Installer/README.md) only for that task. |
| Final implementation validation | `test-and-review` owns sufficient verification, independent-review criteria, documentation impact, and reporting; start from the final changes and current evidence. |
| Durable artifact creation or reuse | [artifacts.md](For-Agent/Docs/artifacts.md) owns naming, timestamps, and selective reuse; read it only when creating/reusing an artifact or consulting an artifact index. |
| C++/QML explanatory comments | Apply the [comment policy](For-Agent/Docs/coding-style.md#explanatory-comments-in-c-and-qml) when adding or handling comments. |

Use `.agents/skills/review-code-for-human/SKILL.md` only when the user explicitly
requests `review-code-for-human` by name, never implicitly for reviews,
explanations, walkthroughs, or teaching.

## Delegation Policy

Keep bounded work with the lead unless parallel discovery or independent scrutiny
is useful. Do not invoke researchers and reviewers by default or reduce significant
review solely to save context. Existing `cpp_researcher`, `qml_researcher`,
`cpp_reviewer`, and `qml_reviewer` specialists remain read-only and return findings.
Select the bounded or codebase workflow from the assigned scope.

Give each delegate a precise objective, files/symbols/diff or subsystem, established
facts and navigation, unresolved risks, expected result, and stopping boundary.
The lead owns coverage, overlaps, gaps, and reconciliation. Reuse discovery, but
independently inspect critical code and assumptions when reliable judgment requires
it. Follow [test-and-review](.agents/skills/test-and-review/SKILL.md#full-independent-review)
for necessary independent-review criteria. Role names do not establish comparative
capability; check configured execution settings when relevant.

## Comment Handling

New ordinary explanatory C++/QML comments use `// Codex Comment:`. Preserve
unmarked comments with unchanged code; authorized code changes permit associated
comment updates or removal. Marked comments may be corrected or removed in scope
independently. The marker governs comment handling, not authority to modify the
associated code; see the detailed policy linked above.

## Final Verification

Apply [test-and-review](.agents/skills/test-and-review/SKILL.md) to validate the
scoped result, update directly affected documentation, and report verification
and limitations. Reuse successful checks that remain current.

For code changes requiring build verification, use Debug on the current host and
the canonical [routine workflow](For-Agent/Docs/build.md#routine-codex-workflow).
Broader verification expands target/test coverage within Debug. Release builds,
Release tests, other configurations, staging, and packaging require an explicit
user request. Documentation/instruction-only changes without build impact require
no build, tests, deployment, or application launch.

## Final Response Format

Use this format after making changes:

1. What did you do?
2. Which files changed?
3. What should be checked manually?
4. Risks, limitations, and unverified areas.
5. Improvement ideas, if relevant.
