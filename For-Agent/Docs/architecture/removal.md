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

Archived CVs stay in the source model so jobs, linked views, and startup recovery
keep their canonical references. They are excluded from the normal CV Library
view and Dashboard recent-CV projection. Deleting the last job linked to an
archived CV updates its link count but does not restore or delete it. Controller
batch summaries report deleted, archived, restored, skipped, failed, and canceled
outcomes, including item-specific failure details.

`DataRemovalWorker` and `StorageMutationGate` ownership, admission, cancellation,
and thread-affinity rules are defined in [threading.md](threading.md).
