# Review Artifacts

Use durable review artifacts to preserve review context and avoid repeating the same review work.

## Artifact Location

- `For-Agent/Review/`: code review files and review findings.

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

1. Check `For-Agent/Review/` before repeating a broad review.
2. Use artifacts as context only. Verify current facts against the actual source, instructions, and diff before making claims or findings.
3. Prefer the most recent relevant artifact by timestamp.
4. Determine recency from the filename timestamp first.
5. If a legacy artifact has only `YYYY-MM-DD` in the filename, inspect its beginning for a `Created:` timestamp.
6. If no time is available, treat the date-only artifact as the earliest artifact for that date.
7. If multiple artifacts are relevant, read the newest broad artifact first, then any newer narrow artifact for the current scope.

Do not treat an old artifact as current proof. Artifacts prevent redundant discovery; they do not replace verification.
