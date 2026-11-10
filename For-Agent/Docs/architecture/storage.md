# Storage, Schema, And Managed Files

Read this document for SQLite schema or migrations, repository behavior,
transactions, managed CV files, path safety, hydration, or startup recovery.

## Persistence And Product Data

Jobs, CVs, and company identities are currently backed by SQLite.

`StoragePaths` defaults to the `Data` directory under Qt's `AppLocalDataLocation`.
The database (`database.sqlite`) and managed CV directory (`Resumes`) are siblings
under the selected data directory. Bootstrap passes that directory to all three
persistence workers.

`SqliteDatabase` owns the Qt SQL connection lifetime and schema migration.
Storage-specific SQL helpers execute standalone statements and translate Qt SQL
failures into exceptions that identify the failed operation. Repositories use the
same contextual error boundary for prepared queries. `SqlTransaction` starts one
transaction and requires an explicit commit. If it is still active at destruction,
it attempts rollback. A failed rollback records `DatabaseRollbackFailed` because
the database state is uncertain.

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

Every `SqliteDatabase` connection enables foreign-key enforcement.
`SchemaMigrator` reads `PRAGMA user_version` and returns immediately for version
4. It rejects newer versions, initializes version 0 directly at version 4, and
applies the remaining migrations in order for versions 1 through 3.
Initialization and upgrades run `PRAGMA foreign_key_check` before committing
their transaction. The SQL for initialization and each upgrade step lives in a
separate C++ source file.

Version 1 databases first receive the CV identity migration to version 2, then
continue through the company identity migration. The `v2 -> v3` step trims and
case-folds existing job company names, creates one company per normalized
identity, rebuilds jobs with required company foreign keys, and preserves
technology rows. Blank legacy company names abort the transaction with a clear
error. The `v3 -> v4` step adds nullable `cvs.archived_at`, so every existing CV
remains active.

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

`CvManagedFileStore` checks that the source is a readable local file with a
PDF, DOC, or DOCX filename extension. It buffers the entire file in a
`QByteArray` and derives the size and SHA-256 from those bytes. Only an identity
miss reaches `stageAndFinalize()`, which generates a managed filename and writes
a `.part` file in bounded chunks. It handles partial writes, then flushes,
closes, and renames the file.

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
before deleting its unlinked database row. On a database failure, the service
asks the store to restore the tombstone to a vacant original path before the
transaction guard attempts rollback. A failed rename leaves the tombstone for
startup recovery. After commit, the store tries to remove the tombstone. If
cleanup fails, the service reports the row as deleted with file cleanup pending.

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
