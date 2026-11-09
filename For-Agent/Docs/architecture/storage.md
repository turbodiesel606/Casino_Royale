# Storage, Schema, And Managed Files

Read this document for SQLite schema or migrations, repository behavior,
transactions, managed CV files, path safety, hydration, or startup recovery.

## Persistence And Product Data

Jobs, CVs, and company identities are currently backed by SQLite.

`StoragePaths` resolves the application data location, creates the managed resume
folder, and provides the database path.

`SqliteDatabase` owns the Qt SQL connection lifetime and schema migration.
Storage-specific SQL helpers execute standalone statements and translate Qt SQL
failures into exceptions that identify the failed operation. Repositories use the
same contextual error boundary for prepared queries. `SqlTransaction` starts one
transaction, requires an explicit successful commit, and automatically rolls
back while still active.

The current SQLite schema is version 4 and contains:

- `cvs`, which stores managed CV file metadata, uses
  `(sha256, original_file_name)` as a unique identity pair, and represents
  library archival with nullable `archived_at`.
- `companies`, which stores a display name and a unique trimmed, case-folded
  normalized name for each durable company identity.
- `jobs`, where every row references one persisted company and one persisted CV
  through the non-null `jobs.company_id` and `jobs.cv_id` foreign keys.
- `job_technologies`, which stores a job's ordered technology rows. Its
  `(job_id, position)` primary key preserves ordering, and its `job_id` foreign
  key references `jobs.id` with `ON DELETE CASCADE`.

`SchemaMigrator` reads `PRAGMA user_version`, rejects unsupported newer
databases, selects the direct version-4 initialization or applies each forward
migration sequentially, verifies foreign keys, and commits through
`SqlTransaction`. The explicit initialization, `v1 -> v2`, `v2 -> v3`, and
`v3 -> v4` SQL live in separate version-specific implementation units.

Version 1 databases first receive the CV identity migration to version 2, then
continue through the company identity migration. The `v2 -> v3` step trims and
case-folds existing job company names, creates one company per normalized
identity, rebuilds jobs with required company foreign keys, and preserves
technology rows. Blank legacy company names abort the transaction with a clear
error. The `v3 -> v4` step adds nullable `cvs.archived_at`, so every existing CV
remains active. Initialization and all upgrades run transactionally and verify
foreign keys before commit.

## Repositories And Typed Values

`CvRepository` loads and inserts CV metadata, persists favorite and archive
changes with an updated timestamp, reconstructs each CV's linked application IDs
with a left join from `cvs.id` to `jobs.cv_id`, and deletes a CV only through a
guarded `NOT EXISTS` query when no job references it.

`CompanyRepository` loads durable companies and resolves Add Job company names
through the same trimmed, case-folded identity rule.

`JobRepository` loads, finds, inserts, updates, and deletes jobs by durable
company and CV IDs. An update replaces the job metadata and ordered technology
rows inside its caller-owned transaction while preserving the application ID and
creation timestamp. Job deletion relies on the existing `job_technologies`
cascade and never removes the referenced company or CV. Its read query joins
`companies.display_name` and `cvs.original_file_name` so the existing QML-facing
company and CV display roles remain unchanged. It then loads each job's
technologies from `job_technologies` in `position` order.

`CvRepository` includes `fixed_str/FixedStringQt.hpp` for its shared joined
`SELECT` and CV `INSERT` statements. `FIXED_QSTRING` exposes their compile-time
SQL text as a `QString` backed by static UTF-16 storage. The repository binds
runtime values, such as the CV ID, as query parameters.

Repositories map job applied dates to `QDate` and job, CV, and company
timestamps to `QDateTime`. Both use ISO text in SQLite, including the CV archive
timestamp. Job URLs map to `QUrl`, and job status and work format map to closed
enums. Writes serialize these values back to text. `Company::website_` is an
in-memory `QUrl`. The `companies` table stores identity, names, and timestamps.

`CvDocument::isArchived_` records archive membership independently of timestamp
parsing. Repository reads derive it from whether `archived_at` is SQL `NULL`:
non-null values, including malformed or empty text, remain archived even when
`archivedAt_` is invalid. Archive/restore operations and model publication pass
the explicit state through `applyArchiveState()`; filters and eligibility checks
use the flag. New inserts require the flag to agree with timestamp validity and
reject inconsistent input instead of silently losing an archive marker. No
separate Boolean database column is stored.

Treat `SchemaMigrator`, the repository queries, and storage tests as the source
of truth for the current persisted schema and relationships.

The production bootstrap hydrates the company directory from
`CompanyRepository` and publishes companies resolved by job creates or updates
immediately. Company-linked job rows use the same durable IDs after restart.
Contacts remain non-persisted, and there is no company/contact edit or delete
workflow.

## Managed CV Path And File Boundary

File open and permanent deletion share `CvManagedPathResolver`. It requires
the stored name to match a top-level path under `Resumes` and rejects absolute,
traversal, nested, mismatched, and symbolic-link paths. An existing target must
be a regular file. A valid reference can also point to a missing file.
`CvFileAccessService` requires a readable file before calling the platform
desktop opener. Permanent deletion can still remove an unlinked row whose file
is missing.

`CvManagedFileStore` owns the managed CV filesystem boundary. It validates local
PDF, DOC, and DOCX inputs and reads the complete source into the preparation's
`QByteArray`. Size and SHA-256 describe those exact bytes. Buffering creates no
managed filename, `.part`, or final file. Only an identity miss calls
`stageAndFinalize()`, which generates a unique name, writes bounded chunks with
partial-write handling, flushes/closes the `.part`, and renames it.

Source preparation observes cancellation before inspection, between one-megabyte
read chunks, and after the final read. After the caller's pre-persistence check,
`stageAndFinalize()` currently does not consult the cancellation token. Write or
finalization failures attempt `.part` removal through the local scope guard;
preparation RAII and startup recovery remain fallbacks. `finalFilePath_` is set
only after successful rename so callers can compensate even if CV insertion
throws. No product size limit or streaming fallback is imposed: peak memory
includes the complete selected CV.

### Permanent Deletion And Startup Recovery

Permanent deletion renames an existing managed file to a `.delete` tombstone
before deleting its unlinked database row. If the database operation fails and
the original path is vacant, the store attempts to rename the tombstone back.
A failed rename leaves it for startup recovery. After commit, the store tries
to remove the tombstone. If cleanup fails, the service reports the row as
deleted with file cleanup pending.

Bootstrap calls `CvRepository::findAll()` once and passes the same CV snapshot
to `CvManagedFileStore::reconcile()` and then to `CvLibraryController`. Recovery
restores a tombstone when the snapshot contains its original filename and the
original path is absent. Otherwise it removes the tombstone. It removes stale
`.part` files and moves completed top-level files whose names are absent from
the snapshot to `Resumes/Quarantine`. A failed rename or removal aborts recovery.

Bootstrap tracks whether it finished reading jobs, technologies, CVs with their
job links, and companies. A terminal cursor error preserves the rows already
read and suppresses the `ApplicationStarted` event. Recovery still runs on a
partial CV snapshot. If the cursor omitted a CV row, recovery can quarantine
its file or remove its tombstone while that row still exists. See
[logging.md](logging.md).

## Storage And Transaction Rules

- Let services define business-operation transaction boundaries and coordinate
  repositories. Keep repositories focused on persistence operations and mapping.
- Make schema upgrades forward-only, versioned, transactional, and safe for both
  new databases and every supported prior version.
- Verify foreign-key state and required invariants before committing schema
  migrations.
- For permanent CV deletion, rename an existing file to a tombstone before the
  database delete. Attempt restoration on transaction failure and account for
  pending file cleanup after commit. Reconcile remaining tombstones at startup.

Read [removal.md](removal.md) for the product behavior built on the guarded
repository and tombstone operations. Read [threading.md](threading.md) when a
storage operation moves across a worker boundary or uses a thread-owned Qt SQL
connection.
