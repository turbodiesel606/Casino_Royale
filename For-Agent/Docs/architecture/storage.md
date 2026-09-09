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

Repositories translate the schema's existing ISO text representation into typed
domain values. Job applied dates use `QDate`; job, CV, and company
creation/update timestamps use `QDateTime`; job and company URLs use `QUrl`; and
job status/work format use closed enum values. Writes serialize those values back
to schema text columns. CV archival uses the same ISO timestamp representation in
schema version 4.

Treat `SchemaMigrator`, the repository queries, and storage tests as the source
of truth for the current persisted schema and relationships.

The production bootstrap hydrates the company directory from
`CompanyRepository` and publishes companies resolved by job creates or updates
immediately. Company-linked job rows use the same durable IDs after restart.
Contacts remain non-persisted, and there is no company/contact edit or delete
workflow.

## Managed CV Path And File Boundary

`CvManagedPathResolver` is the shared canonical path boundary used by file open
and permanent deletion. It accepts only a top-level regular file beneath the
managed `Resumes` directory and rejects absolute, traversal, nested, mismatched,
and symbolic-link paths. `CvFileAccessService` delegates valid local-file URLs to
the platform desktop opener.

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

Permanent deletion first renames an existing managed file to a `.delete`
tombstone, deletes the guarded database row, then removes the tombstone. A failed
database operation restores the original filename. Startup reconciliation
restores tombstones still referenced by `cvs`, removes tombstones whose row was
committed as deleted, removes stale `.part` files, and moves any completed
top-level managed file without a matching `cvs.stored_file_name` row into
`Resumes/Quarantine`; completed orphaned user files are never silently deleted.

Bootstrap loads CV documents once, reconciles the managed store against that
snapshot, and passes the same document values to `CvLibraryController`.

## Storage And Transaction Rules

- Let services define business-operation transaction boundaries and coordinate
  repositories. Keep repositories focused on persistence operations and mapping.
- Make schema upgrades forward-only, versioned, transactional, and safe for both
  new databases and every supported prior version.
- Verify foreign-key state and required invariants before committing schema
  migrations.
- When one operation changes both SQLite and managed files, rename the managed
  file to a recoverable tombstone before the database delete, compensate on
  transaction failure, and reconcile committed or referenced tombstones at
  startup.

Read [removal.md](removal.md) for the product behavior built on the guarded
repository and tombstone operations. Read [threading.md](threading.md) when a
storage operation moves across a worker boundary or uses a thread-owned Qt SQL
connection.
