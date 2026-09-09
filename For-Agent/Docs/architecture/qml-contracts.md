# QML And Qt Model Contracts

Read this document when a task changes QML-facing properties, commands, signals,
model roles, selection, filtering, sorting, linked views, dashboard projections,
or GUI-thread publication.

Responsibility placement and the presentation-versus-durable boundary are
defined once in [boundaries.md](boundaries.md). Application context-property
exposure and page reachability are documented in [application.md](application.md).

## Selection Contracts

`StableIdSelectionTracker` is the shared selection contract for the jobs, CV,
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
notifications. Linked selection-dependent models refresh only when the effective
selected ID changes.

`BulkIdSelectionTracker` is the separate checkbox-selection contract for Job
Applications and CV Library. It stores a set of domain IDs, derives all-visible
and partial-visible states from the current proxy, and reconciles source removal
and proxy layout changes without coupling checkbox selection to preview state.

## Controller And Proxy Contracts

Controller model-pointer properties are constant because their model objects do
not change after construction. Model content changes are published through the
models' row, reset, layout, and data signals. Scalar controller properties use
semantic notify signals: selected ID, selected proxy index, selected data,
individual filters, visible counts, result summaries, and the CV category summary
notify only for their own contract changes.

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
structure or data changes. The dashboard's existing QML role names remain
available, including the `appliedDateLabel` alias. CV category summaries notify
when rows are inserted, removed, or reset and when category-role data changes.

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

## Contract Rules

- Keep model role IDs and names stable while QML consumes them.
- Wrap row insertion, removal, movement, and reset operations with the matching
  Qt begin/end notifications. Emit `dataChanged` with the affected indexes and
  roles for in-place updates.
- Emit `Q_PROPERTY` notify signals whenever exposed state changes, and avoid
  emitting change notifications when the value is unchanged unless the contract
  requires a refresh.
- Preserve selection by stable domain ID across filtering and sorting instead of
  relying on proxy row numbers.
- Update matching tests whenever properties, signals, model roles, transaction
  behavior, or QML-facing commands change.
