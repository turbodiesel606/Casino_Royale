# QML And Qt Model Contracts

Read this document when a task changes QML-facing properties, commands, signals,
model roles, selection, filtering, sorting, linked views, dashboard projections,
or GUI-thread publication.

Responsibility placement and the presentation-versus-durable boundary are
defined once in [boundaries.md](boundaries.md). Application context-property
exposure and page reachability are documented in [application.md](application.md).

## Selection Contracts

`StableIdSelectionTracker` is the shared preview selection contract for the jobs, CV,
company, and contact controllers. Each controller composes one tracker with its
proxy model and ID role. The tracker owns the selected domain ID and derived
proxy row plus the visible row count, reconciles proxy insert, remove, move,
reset, layout, and data changes, and applies the common fallback policy: choose
the first visible row when the selected ID is hidden, or clear selection when no
row is visible.

Controller-owned filter and sort updates reconcile selection and visible count
as one completed model update so transient proxy batches do not publish
intermediate fallback selections or counts. Controllers translate the tracker's
generic visible-count change into their domain-specific count and summary
notifications. The CV and company controllers change their linked views' filter
only when the selected ID changes. The views also update when a source row is
added, removed, or assigned to a different CV or company.

`BulkIdSelectionTracker` is the separate checkbox-selection contract for Job
Applications and CV Library. It stores a set of domain IDs, derives all-visible
and partial-visible states from the current proxy, and selects only visible
rows on Select All. The tracker drops IDs that leave the visible set. The
controllers clear all checked IDs when a filter changes or an active filter is
cleared. Switching the CV Library between Active and Archived also clears the
checkboxes. Sorting preserves the checked IDs. Preview selection is independent.

## Controller And Proxy Contracts

Controller model-pointer properties are constant because their model objects do
not change after construction. Model content changes are published through the
models' row, reset, layout, and data signals. Controllers expose separate
notify signals for selected ID, selected proxy index, selected data, individual
filters, visible counts, and result summaries.

`RelationFilterProxyModel` is the shared linked-view contract for CV
applications, company jobs, and company contacts. Each controller configures the
proxy with the source relation role and selected domain ID. The proxy preserves
source role names and reacts directly to source insertion, removal, movement,
reset, and relation-role data changes without manual source-row caches. An empty
selected ID produces an empty linked view.

`LimitedSortedProxyModel` owns dashboard recency projections. Recent
applications expose the five newest jobs by typed creation timestamp; recent CVs
expose the five newest active CVs by typed update timestamp. Both projections use
the domain ID as a deterministic tie-break and rebuild after relevant source
structure or data changes. Recent applications expose the job model's
`dateLabel` role to dashboard delegates as `appliedDateLabel`.

The CV category summary computes its total and sorted category counts from
source documents in the current Active or Archived view. Text and language
filters do not limit those counts. The first row is always `All CVs`. An empty
category is its own bucket. Switching the category filter between empty and
`All` can change that bucket's selected flag without emitting
`categorySummaryChanged`. Source row insertions, removals, resets, and changes
to the category or archived roles tell consumers to reread the summary. For
example, inserting an active CV emits the signal even while the Archived
view shows the same counts.

## Domain Values And Public Roles

Persisted value objects keep typed dates, timestamps, URLs, and closed job
choices where the domain supports them. `JobApplication` also carries the joined
company display name and CV filename hydrated by `JobRepository`, preserving the
existing QML contract without exposing either repository. Contacts are
in-memory-only and still carry presentation-ready strings such as initials and
timestamp labels. Job, CV, and company list models derive their remaining
initials, accent colors, status/date labels, count labels, and file-size labels
while preserving the existing QML role names and displayed values. The job, CV,
and company models also publish typed `createdAt` and `updatedAt` roles for
backend sorting; jobs add typed status, work-format, and applied-date roles.

`RoleFilterProxyModel` compares `QDate` and `QDateTime` values directly and is the
sole owner of normalized search text. Controllers expose their existing search
properties and notifications while delegating normalized search storage and
matching to the proxy. Selected-item maps project explicit public-role
whitelists, so internal model roles are not exposed accidentally; QML continues
to receive its existing string roles and selected-item map fields.

## Mutation Admission And Feedback

`storageMutationGate` exposes read-only `importOrUpdateAllowed`, `removalAllowed`,
and `shuttingDown` state, plus retry-notice visibility/message and the current
blocked-dialog title/message. During normal operation, its typed
`operationBlocked` event is emitted for every rejected attempt, even when the
blocking reason is unchanged. Attempts rejected after shutdown begins are
silent. Availability checks used by the picker and deletion confirmations
reserve nothing; commands always perform final backend admission. Form
validation runs first.

The main window emits `closeWaitSelected()` when the user chooses Wait and
`interruptAndExitSelected()` after the interrupt action blocks admission and
requests cancellation. Automatic dismissal when work drains emits neither
signal. Bootstrap consumes these signals for [application logging](logging.md).

`StorageMutationFeedback` renders one modal dialog and a persistent banner. The
gate tracks which categories were attempted, announces availability after their
blocking work finishes, and keeps an announced notice current when work resumes.
Dismissing the banner clears retry interest in both operation categories.
Admission clears interest only in the admitted category. Retrying requires a
new command. The shell positions ordinary completion notifications
below the banner. Repeated attempts reopen or focus the same dialog, and an open
dialog updates when the blocker changes or finishes.

Selection eligibility keeps deletion/restore buttons enabled during conflicts.
Blocked commands preserve drafts, replacement CVs, and checkbox selections.
`addCvs()` returns admission status so QML can retain a rejected picker selection.
Updates use `applicationUpdateBlocked` for editor/navigation cleanup without a
duplicate error toast. `applicationDeletionRejected` and CV `operationFailed`
report genuine eligibility, access, or persistence errors; completion signals
belong only to admitted operations. See [threading.md](threading.md) for lifetime
and accounting and [removal.md](removal.md) for the exclusive operation category.

## Contract Rules

- Keep model role IDs and names stable while QML consumes them.
- Wrap row insertion, removal, movement, and reset operations with the matching
  Qt begin/end notifications. Emit `dataChanged` with the affected indexes and
  roles for in-place updates.
- Emit `Q_PROPERTY` notify signals whenever exposed state changes, and avoid
  emitting change notifications when the value is unchanged unless the contract
  requires a refresh.
- Identify selected items by stable domain ID. A proxy row gives the current
  position. Apply the [selection contracts](#selection-contracts) for preview
  fallback and for checkbox selection when the view changes.
- Update matching tests whenever properties, signals, model roles, transaction
  behavior, or QML-facing commands change.
