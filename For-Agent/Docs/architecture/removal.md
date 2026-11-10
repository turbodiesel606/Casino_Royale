# Job And CV Removal

Read this document for job deletion, CV archive/restore/permanent deletion,
bulk selection removal, cancellation between removal items, or linked-CV safety.
Read [storage.md](storage.md) when changing repository guards, transactions,
managed paths, tombstones, or startup recovery.

Jobs and CVs share one canonical database and one managed `Resumes` store; the
Library and Job Applications never create separate CV copies. Both controllers
use the checked domain IDs for bulk removal. The
[selection contracts](qml-contracts.md#selection-contracts) describe which rows
remain checked after filter, view, and sort changes and how preview selection works.

Each removal batch is best-effort and processes selected IDs independently with
one transaction per item. Deleting a job removes only that job and its cascaded
technology rows. Removing an active linked CV archives it and preserves its
file. Removing an active unlinked CV deletes its database row. Archived CVs can
be restored. If every checked archived CV is linked, the UI disables permanent
deletion and the controller rejects a direct call before worker admission. For
a mixed selection, the worker rechecks each CV, deletes eligible unlinked items,
and reports linked items as skipped. For either kind of permanent
deletion, `Deleted` confirms the row deletion committed. The item message also
reports an already-missing file or tombstone cleanup pending for startup.
Successful IDs leave checkbox selection. Failed and skipped IDs stay selected.

`DataRemovalTypes.hpp` defines the shared removal protocol. The four supported
`DataRemovalKind` values are template arguments of `DataRemovalRequest<Kind>`
and `DataRemovalBatchOutcome<Kind>`; their `kind_` members are static constants,
not per-message mutable fields. Public constraints reject unsupported values.
Controller commands submit a concrete request type, and `DataRemovalService`
selects its existing per-item handler with `if constexpr` inside one shared
batch implementation. Service and executor templates are defined in their
headers. `DataRemovalWorker` inherits typed submission and queued result delivery
from `SingleActiveWorkerFacade<DataRemovalExecutor>`; constrained request traits
adapt all four kinds to that shared contract.

The worker returns `DeleteJobsOutcome`, `RemoveActiveCvsOutcome`,
`RestoreArchivedCvsOutcome`, and `DeleteArchivedCvsOutcome` through
`jobsDeletionCompleted`, `activeCvsRemovalCompleted`,
`archivedCvsRestorationCompleted`, and `archivedCvsDeletionCompleted`,
respectively. The jobs controller consumes only job outcomes; the CV controller
instantiates the same private submission and completion templates for its three
CV removal kinds, with their definitions in `CvLibraryController.hpp`. The receiver publishes model and
checkbox changes before `cvMutationCompleted`, then releases removal admission
after pending-state notifications. Controllers retain runtime shutdown, active-token, operation-ID,
and token-identity checks because earlier requests of the same kind can still
have late outcomes. No runtime active-kind field is needed. Item statuses and
linked/archive eligibility remain runtime
data, and QML-facing commands and completion summaries remain unchanged.

Archived CVs stay in the source model so jobs, linked views, and startup recovery
keep their canonical references. They are excluded from the normal CV Library
view and Dashboard recent-CV projection. Deleting the last job linked to an
archived CV updates its link count but does not restore or delete it. Controller
batch summaries report deleted, archived, restored, skipped, failed, and canceled
outcomes, including item-specific failure details.

The service reports job batch totals from processed outcomes and retains the
first technical stage/cause separately from missing records. Active-CV removal
groups equivalent transaction-start or lookup errors before the archive/delete
choice. An incomplete link cursor can return a prefix and the item can still
commit. Its lookup diagnostic and its actual committed outcome are separate.
See the [event ownership policy](logging.md#ownership-and-evidence).

Job deletion, active-CV removal, archived-CV deletion, and manual restore share
one exclusive admission category. Accepted queued imports/updates block this
category. An active removal batch blocks new persistence commands, including
another removal batch. Eligibility errors are separate from admission conflicts.
During normal operation the gate reports conflicting requests through blocked
feedback. Once shutdown begins it rejects new requests silently. Both paths
preserve the selection. Only admitted batches reach the worker and can emit a
completion outcome.

Eligible buttons remain clickable during conflicting work. Deletion confirmation
checks availability without reserving it, and its confirmed command checks again.
Ownership lasts through result/model publication and completion/pending-state
signals. After a previous conflict, the global banner reports when the requested
category is available; retry always requires a new user action and admission.

`DataRemovalWorker` and `StorageMutationGate` ownership, admission, cancellation,
and thread-affinity rules are defined in [threading.md](threading.md).
