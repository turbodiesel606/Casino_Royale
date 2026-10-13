# Job And CV Removal

Read this document for job deletion, CV archive/restore/permanent deletion,
bulk selection removal, cancellation between removal items, or linked-CV safety.
Read [storage.md](storage.md) when changing repository guards, transactions,
managed paths, tombstones, or startup recovery.

Jobs and CVs share one canonical database and one managed `Resumes` store; the
Library and Job Applications never create separate CV copies. Both controllers
use `BulkIdSelectionTracker` for checkbox selection by stable domain ID. Select
all affects only visible proxy rows, filters and CV Active/Archived view changes
clear checkbox selection, sorting preserves it, and preview selection remains a
separate `StableIdSelectionTracker` concern.

Each removal batch is best-effort and processes selected IDs independently with
one transaction per item. Deleting a job removes only that job and its cascaded
technology rows. Removing an active linked CV archives it without touching the
file; removing an active unlinked CV permanently deletes its row and managed
file. Archived CVs can be restored. Permanent deletion of an archived mixed
selection deletes unlinked items and reports linked items as skipped. Successful
IDs leave checkbox selection; failed and skipped IDs remain selected.

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
uses one templated receiver for its three kinds. Controllers retain runtime
shutdown, active-token, operation-ID, and token-identity checks because earlier
requests of the same kind can still have late outcomes. No runtime active-kind
field is needed. Item statuses and linked/archive eligibility remain runtime
data, and QML-facing commands and completion summaries remain unchanged.

Archived CVs stay in the source model so jobs, linked views, and startup recovery
keep their canonical references. They are excluded from the normal CV Library
view and Dashboard recent-CV projection. Deleting the last job linked to an
archived CV updates its link count but does not restore or delete it. Controller
batch summaries report deleted, archived, restored, skipped, failed, and canceled
outcomes, including item-specific failure details.

Job deletion, active-CV removal, archived-CV deletion, and manual restore share
one exclusive admission category. Accepted queued imports/updates block this
category; an active removal batch blocks every new persistence command and
another removal batch. Eligibility errors are separate from admission conflicts.
Rejected batches emit blocked feedback rather than completion outcomes and keep
their selections. They never enter a queue or interrupt current work.

Eligible buttons remain clickable during conflicting work. Deletion confirmation
checks availability without reserving it, and its confirmed command checks again.
Ownership lasts through result/model publication and completion/pending-state
signals. After a previous conflict, the global banner reports when the requested
category is available; retry always requires a new user action and admission.

`DataRemovalWorker` and `StorageMutationGate` ownership, admission, cancellation,
and thread-affinity rules are defined in [threading.md](threading.md).
