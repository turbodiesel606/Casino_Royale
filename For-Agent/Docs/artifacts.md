# Workflow Artifacts

Use durable artifacts to preserve useful context without rereading every prior
output or treating old conclusions as current evidence.

## Artifact Location

- `For-Agent/Review/`: code review files and review findings.
- `For-Human/Architecture/`: detailed `learn-cpp-codebase` phase artifacts and
  each learning run's compact cumulative index.

Research findings are normally returned to the active task. If the user requests
a durable research artifact or provides one from another location, apply the
same selective-loading and live-source rules below. Do not recreate the retired
`For-Agent/Research/` route.

## Discovering And Loading Artifacts

Before opening artifact bodies:

1. Inventory filenames, timestamps, titles, scope, and status metadata.
2. Identify which artifacts overlap the current symbol, subsystem, diff,
   architecture question, or learning phase.
3. Prefer a compact index or summary. Load a detailed artifact only when it
   contains context needed for the current question.
4. When several review artifacts overlap, read the newest relevant broad
   artifact first, then only newer narrow artifacts that materially update it.
5. Do not reread the same artifact in one task unless a changed assumption,
   conflicting evidence, or concrete finding requires it.

Never open every artifact in a directory by default. Live source, current
instructions, the focused diff, CMake registration, and current test evidence are
authoritative. Artifacts are context and navigation aids, not proof.

## Creating Review Artifacts

When creating a durable review artifact:

1. Include the artifact creation date and time in the filename.
2. Use a sortable 24-hour local timestamp in the filename: `YYYY-MM-DD-HHMM`.
3. Use names like `cpp-review-YYYY-MM-DD-HHMM-topic.md` or `qml-review-YYYY-MM-DD-HHMM-topic.md`.
4. Put the creation timestamp at the beginning of the file, immediately after the title.
5. Use this metadata line format: `Created: YYYY-MM-DD HH:MM local time`.

If the timezone is relevant or known, include it in the metadata line, for example: `Created: 2026-07-07 14:35 local time (Asia/Baku)`.

## Reusing Existing Review Artifacts

Before starting a review that overlaps prior review work:

1. Apply the discovery and loading rules above before repeating a broad review.
2. Verify current facts against the actual source, instructions, and diff before
   making claims or findings.
3. Determine recency from the filename timestamp first.
4. If a legacy artifact has only `YYYY-MM-DD` in the filename, inspect its beginning for a `Created:` timestamp.
5. If no time is available, treat the date-only artifact as the earliest artifact for that date.

Do not treat an old artifact as current proof. Artifacts prevent redundant discovery; they do not replace verification.

## Learning Indexes

Each `learn-cpp-codebase` run keeps its detailed phase artifacts and one compact
same-run cumulative index. The index records phase status, architecture and
contract summaries, subsystem and file pointers, ownership and threading rules,
unresolved questions, stale assumptions, and links to the detailed artifacts.

For a new or resumed phase, read the current phase artifact when present, then
the cumulative index, then only the detailed prior phase sections relevant to the
lesson or newly discovered source evidence. Update the index after every phase or
Phase 4 domain lesson. Do not delete or collapse the detailed phase artifacts.
