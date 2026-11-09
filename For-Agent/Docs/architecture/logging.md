# Application logging

JobTracker records operational outcomes through a bounded queue. Producers pass
owned values to `ApplicationLog::record(LogEvent) noexcept`. A single consumer
formats and writes each entry. Business decisions, transaction handling, result
delivery and shutdown proceed independently of logger availability, admission
and write success.

## Policy for new and changed behavior

Assess each changed behavior at the boundary that can confirm its outcome. Add
any missing event, producer and verification in the same task. When existing
coverage suffices or a change has no meaningful operational consequence, explain
that briefly in the final review.

| Level | When to use it |
| --- | --- |
| ERROR | A required technical step failed. Identify the operation, stage and bounded cause. Report a failed rollback, restoration or delivery as its own consequence when it leaves storage uncertain or a committed change unpublished. |
| WARNING | A state or concurrency rule rejected work, an item was protected or missing, loading used fallback values, or a committed operation left cleanup pending. Give the observed state and the consequence. |
| INFO | Significant confirmed milestones, including admission, committed work, observed cancellation boundaries, recovery and controlled lifecycle transitions. Make the wording match the milestone. |

### Ownership and evidence

- The admission gate or accepting controller owns an explicit rejection or
  acceptance. Acceptance confirms admission or enqueueing; execution may not
  have started. The worker reports any subsequent failure to start accepted work.
- The service that owns the transaction reports persistence after commit or
  after failure handling. A repository reports a dataset failure where the
  cursor result is known. A failed read during an executing batch belongs to
  the persistence stage, even if the prefix permits the operation to continue.
- Report file cleanup and compensation from the owner that knows whether they
  succeeded. A committed row and a pending file deletion can both be true.
  Report a rollback or failed restoration separately when it adds a distinct
  consequence. Report recovery counts from work actually completed.
- The dispatching worker owns request/result posting failures. The GUI receiver
  owns correlation checks and model publication. Distinguish persisted state,
  delivered results and visible rows. Claim each only when its owner confirmed it.
- For batches, count actual item results. Distinguish committed, failed, skipped
  and unprocessed items. Group equivalent failures by operation, stage and cause
  when possible. Keep technical causes separate from missing-record warnings.
- The requester records cancellation requested and the counts of active and
  queued work. The worker records a stop at the boundary that observed it, with
  processed/unprocessed counts when known. If the pre-commit observation saw
  the request and commit succeeded, record the committed outcome with that fact.
  A later check of a token cannot establish the order of a past commit.
- Bootstrap records startup success after required initialization, dataset
  reads, recovery and root-window creation all complete. A partial cursor still
  supplies its rows to the business code. The success decision uses explicit
  completion information. Shutdown completion requires normal exit, joined
  workers and confirmed required cleanup.
- Record one event per meaningful outcome at its owner. Reuse an existing
  technical cause across layers without repeating it at each layer. Keep
  per-row reads, progress ticks, navigation, paint and polling out of the log.
  Record significant batch totals and distinct consequences.

### Payloads and catalog changes

The canonical [event catalog](../../../src/logging/LogCatalog.inc) defines
membership, severity, component and message. Add a row there for a new meaning.
The enum, descriptor table, severity lookup, maximum payload width and
structural checks all derive from those rows. Use existing components and payload forms where they fit.

A plain `{name}` consumes a fixed `LogFact`. `{name:number}` consumes an integer.
The formatter supplies the application version and platform. Producers supply
optional details and grouped counts in template order using the named members
in [LogEvent.hpp](../../../src/logging/LogEvent.hpp). Members of type `LogCount`
require explicit values. Omitted members of `LogRemovalCounts` default to zero
so the removal service can accumulate outcomes. Type, descriptor or formatting
checks must reject missing required counts, incompatible field types, extra
fields and malformed templates. Extend formatting only for a genuinely new
form. Keep each complete line within the existing format.

Permitted facts include enum-backed stages and causes, counts, flags, the
application version/platform and an existing technical operation number.
Keep raw driver errors, exception text, SQL, paths and filenames, record IDs,
names, contact details, CV contents, URLs and other user-entered data out of
events. Queue entries own bounded values and carry no QObject, pointer or
borrowed view into producer state.

### Semantic verification

For a new or changed event producer, exercise the production path at the boundary
that confirms the business outcome. In the owning subsystem's tests, compare the
emitted diagnostic with independently expected values for the operation, stage,
cause, counts, and correlation as applicable. Confirm the event reports the actual
committed, failed, partial, cancelled, or rejected result. Check durable state
and publication where relevant. A failed, partial, or cancelled operation must
never emit an unqualified success. Reuse sufficient existing coverage and focus
new checks on the affected paths.

A synthetic `LogEvent` passed directly to the formatter verifies rendering only.
Schema and type checks cannot establish the meaning or order of fields that
share a type. Choose distinguishable values wherever a swap could go unnoticed,
such as different processed and pending counts.

In the catalog suite, register exactly one primary semantic formatting example
for every event. Derive the required event IDs from the canonical catalog. Fail
the coverage check for a missing event, a duplicate primary example, or an unknown
ID in any example. Allow additional boundary or payload-variant examples for
known IDs. Do not use fixed per-severity totals or a separate membership list.
Specify the expected semantic values and rendered facts independently of the
production formatter. Verify severity, wording, partial or optional fields,
correlation, and privacy. For writer or admission changes, use the writer suite.
See the [suite reference](../testing-reference.md#test-suite-responsibilities).

The current [catalog test](../../../tests/logging/LogCatalogTest.cpp) has one
primary formatting example per catalog event. `rendersExpectedMessages()`
checks the examples it is given. `catalogDeterminesSeverity()` visits every
catalog row for a separate severity check. Because these checks run
independently, removing or duplicating a valid primary example can pass.

## Queue and lifetime

`App::start` creates the session after naming the application and before
constructing `AppBootstrap`. Each producer timestamps its entry in UTC and
briefly acquires the inbox mutex. The deque holds up to 4,096 entries. A full
or closed inbox discards incoming records at every severity. The consumer takes
one entry in enqueue order and releases the mutex before accessing a file.
Timestamps from different producers can appear out of order.

The session outlives the controllers and workers. Session teardown closes
admission, wakes the consumer, drains accepted entries and joins the consumer
after its final flush. Forced termination can lose pending or buffered records.

## Files

The writer uses `QStandardPaths::StateLocation/Logs` independently of the
business data directory. Each UTF-8 line contains the UTC timestamp with
millisecond precision, severity, component and message. Numbered segments
start at `jobtracker-000001.log`. A restart appends to the newest segment
when it has a complete tail and room for the next line. The writer rotates
before a line including its newline would exceed 5 MiB.

The retention budget is 50 MiB across recognized segments. When estimated
usage exceeds the budget, the writer flushes and closes its handle before a
fresh inventory. Deletion uses the sizes currently present under recognized
names, including when another program has replaced the former active path.
It prunes older closed segments in numeric order and skips unrelated files
and symbolic links. The next write checks the current newest segment and tail.
It flushes after errors, at idle, before rotation and on shutdown. During
sustained traffic it checks a one-second flush deadline between writes. After
a write failure the next record gets a fresh attempt. A partial trailing line
stays in its closed segment. Multiple processes sharing a log directory have
no coordination guarantee.
