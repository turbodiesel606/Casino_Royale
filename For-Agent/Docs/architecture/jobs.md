# Job Creation And Update

Read this document for Add Job, job edit/save/discard, job validation, the shared
job-save FIFO, job persistence orchestration, or job model publication.

Read [threading.md](threading.md) when changing shared worker/runtime mechanics,
[cvs.md](cvs.md) when changing CV identity/import behavior, and
[storage.md](storage.md) when changing repositories, schema, managed-file
primitives, or transaction infrastructure.

## Add Job Flow

Add Job is implemented as a QML-to-C++ workflow.

`JobFormPage.qml` gathers form fields and calls
`jobApplicationsController.createApplication(formValues, selectedCvUrl)`.

`JobApplicationsController::createApplication()` maps the raw `QVariantMap` to
`JobApplicationDraft` and calls `JobApplicationValidator::preflight()`
synchronously. The canonical preflight normalizes and validates the draft
without reading the CV, touching the filesystem, opening SQLite, or waiting for
the worker. Invalid input emits `saveFailed(fieldErrors, message)` and returns
before operation-ID allocation, queue mutation, pending-state publication, or
worker startup.

Validation takes precedence over an active removal conflict. Valid create and
update requests use the shared import/update admission counter. During normal
operation, a conflict opens the gate's global blocked dialog before operation-ID
allocation and preserves the current form. Updates additionally emit
`applicationUpdateBlocked` to cancel a pending protected action and retain the
editing session. A missing application, an already-pending update, or invalid
fields trigger `applicationUpdateRejected` before enqueueing.

For valid input, the controller reserves job-save admission and enqueues the
normalized create or update payload in its composed
`SerialOperationQueue<QueuedJobSave>`. The value-level queue assigns a
monotonically increasing operation ID and owns normalized waiters, one active
entry, its cancellation identity, completion suppression, correlation checks,
and drained transitions. The controller retains domain admission, worker
submission, signals, mutation-gate coordination, and completion side effects.
It publishes pending state, emits `applicationQueued(operationId)` for creates,
and schedules the front request. `pendingSaveCount` is the active-plus-waiting
total, and `saving` is true exactly while that total is non-zero.

When a request becomes active, the serial queue creates its shared
`CancellationState`, and the controller submits the typed request to
`JobSaveWorker`. Its runtime starts the worker thread lazily and reuses it until
shutdown. After the executor's initial cancellation check, it lazily constructs
its persistence context and opens the same database file under an independent
unique Qt connection name before service preparation. The existing GUI
connection remains responsible for startup hydration and synchronous favorite
updates.

The worker executes the complete durable pipeline in order:

1. Check the cancellation state and cooperative cancellation.
2. Lazily initialize the worker-owned persistence context.
3. Call `AddJobService::prepareValidated()` for the controller-preflighted draft
   to validate/read the selected CV into a memory buffer. The checked `prepare()`
   entry point still defensively validates drafts; `create()` rejects failed
   canonical preflight before buffering.
4. Block on the application-owned `CvLockWrapper` mutex, then recheck
   cancellation after acquiring it.
5. Begin the transaction and resolve/import the CV, then resolve the company.
6. Insert the job and commit through `AddJobService::complete()` on the worker
   thread. On failure, the transaction's rollback attempt precedes cleanup of
   any newly finalized CV under the same lock. Failed cleanup adds a quarantine
   warning to the outcome.
7. Queue one value-only `AddJobSaveOutcome` to the GUI facade.

Failures during worker initialization, CV processing, or SQLite operations
produce value outcomes, as does observed cancellation. If Qt cannot post a
request or its result, the facade logs `BackgroundWorkDeliveryFailed` and the
controller can remain waiting for that save. The controller accepts a delivered
outcome only when its operation ID and cancellation token match the active
request. Updates must also match the pending application ID. The controller
emits `applicationSaveCompleted` for creates or `applicationUpdateCompleted`
for updates when completion is enabled, then releases the active slot and
advances the FIFO.

The gate reservation outlasts the FIFO slot until completion signals, related
model publication, update-state changes, and pending/drained notifications have
returned. Accepted waiters retain their own reservations across advancement.

`JobApplicationsController` publishes a successful create on the GUI thread. It
appends the job, emits `companyResolved`, reapplies the current sort, and then
emits `cvUsed` followed by `applicationCreated`. The worker transports no
model, repository, SQL handle/query, or staged-file object across threads.

`CancellationState` protects only its boolean with a mutex. Cancellation checks
never hold that mutex during file I/O, SQL, signal emission, or queued delivery.
`cancelCreateApplication()` cancels only an active create request and then
continues the shared FIFO. `cancelAllJobSaves()` drops waiting creates and
updates before they execute. It requests cancellation of the active save and
suppresses its completion signal. The controller emits `saveQueueDrained`
when the pending count reaches zero.

Cancellation remains effective during source buffering and at the explicit
checks before persistence. Waiting for `CvLockWrapper` itself is a blocking
mutex acquisition and cannot be interrupted, but create and replacement-CV
paths recheck cancellation after the lock is acquired and before opening their
transaction. `CvImportService::importPreparedDocument()` and
`CvManagedFileStore::stageAndFinalize()` currently do not consult cancellation;
once persistence begins, the operation runs through commit or rollback and file
compensation.

`JobApplicationFactory` owns draft trimming, status/date defaults,
case-insensitive technology deduplication, typed conversion, and final job
domain-object construction. `JobApplicationValidator::preflight()` uses the
factory to normalize create and update drafts, then checks their fields. The
validator checks stored `JobApplication` values directly using the same field
rules. It returns field-addressable errors for required values,
optional HTTP/HTTPS URLs with a required host, ISO dates, and allowed
status/work-format choices.

The worker-owned `AddJobService` owns durable Add Job orchestration:

- delegation to the factory and validator;
- worker-safe managed-file preparation without database access;
- durable company resolution by normalized name;
- database-thread duplicate resolution by `(sha256, original_file_name)`;
- staged-file finalization and CV insertion;
- job insertion;
- shared-mutex serialization and SQLite transaction handling;
- completed-file cleanup on ordinary failures.

The shared `CvImportService` identity, restore, staging, and cleanup contract is
defined in [cvs.md](cvs.md#shared-cv-import-service).

`AppBootstrap` connects `companyResolved` to
`CompanyDirectoryController::publishCompany()`. It connects `cvUsed`,
`cvReplaced`, and `applicationsDeleted` to `CvLibraryController` to keep each
CV's job links current. On restart, CV and company links are reconstructed
from persisted jobs.

`JobFormPage.qml` keeps unsaved form values and the selected CV while the user
navigates between pages. Synchronous `saveFailed` preserves the entered values
and selected CV while applying inline errors and the global validation message.
`applicationQueued` resets the form immediately after the normalized values have
been captured; the form stays enabled and ready for another submission. Final
completion outcomes belong to previously queued requests and never change the
current form. Discard resets current fields and errors, clears the CV selection,
restores the `Applied` status, and neither cancels queued work nor deletes a
selected source file.

`Main.qml` owns presentation-only completion notifications, a single protected
action guard for Job Description edits, and close confirmation shared by Add
Job, Add CV, and removal work. Notifications are non-modal, display one at a time
for 15 seconds, and queue later outcomes. Closing first checks the job editor.
An update in progress defers the close request. Success resumes it, and failure
abandons it. A dirty edit prompts for Save, Don't Save, or Cancel. If close
proceeds and work is still pending, the shell offers Wait or Interrupt and Exit.
Wait resumes hidden notifications while work continues. Interrupt and Exit
discards notifications, cancels both add queues, requests removal cancellation,
and closes only after all pending counts reach zero. An in-progress removal item
reaches commit or failure handling before the batch stops. If work drains while
the confirmation is open, the dialog closes and the app stays open.

The shell hosts `StorageMutationFeedback` for the gate's repeated-attempt dialog
and persistent availability banner. That banner bypasses the ordinary outcome
queue, remains current if work starts again, and never retries an action.
Interrupt and Exit closes admission and feedback before requesting cancellation.
Opening Add Job and editing an unsaved draft remain available during removal;
the validated Save command performs admission. The editor enters its saving
state only on `applicationUpdateQueued`.

## Update Job Flow

`JobDescriptionPane.qml` has explicit read-only, editing, and saving states.
It loads the selected application's saved values on component completion and
when the selection data changes outside edit mode. Edit reloads the values and
captures the baseline for comparing the fields and optional replacement-CV URL.
Save Changes and Discard appear only during the edit session. Read-only tech
values are chips, and editing uses a comma-separated field.

Explicit Save Changes opens the apply confirmation in the pane. Submission calls
`JobApplicationsController::updateApplication(applicationId, formValues,
replacementCvUrl)`. The controller runs
`JobApplicationValidator::preflight()` before operation-ID allocation, storage
reservation, queue mutation, or worker work. It rejects invalid fields
synchronously and admits a valid normalized update to the same GUI-owned
`SerialOperationQueue` used by creates. `updatingApplication` remains true from
admission through the final update outcome.

When the update becomes active, `JobSaveWorker` calls
`UpdateJobService::prepareValidated()`, which reloads the target application
through `JobRepository::findById()`, trusts the controller-preflighted normalized
draft, and optionally buffers a PDF, DOC, or DOCX replacement. The separate
checked `prepare()` entry point retains service-level validation for callers that
do not already hold a preflight result. Both entry points use the private
`prepareImpl` template in `UpdateJobService.hpp` to look up the job. The
checked path validates the draft after that lookup, so a missing job takes
precedence over field errors there. Every update acquires the shared
`CvLockWrapper` mutex before starting its transaction, including
metadata-only updates. Replacement updates recheck cancellation after
acquisition, resolve/import the CV, then resolve the company and update the job.
Metadata-only updates do not perform CV identity or file work, but they still
hold the shared mutex for company resolution, job update, and commit. The
existing case-sensitive `(sha256, original_file_name)` identity is preserved.

The service updates the job row, replaces ordered technology rows, and commits
one SQLite transaction. The job ID and `created_at` remain stable; `updated_at`
is refreshed. Archived exact CV duplicates restore, and the previous CV row and
managed file remain in the Library. A successful rollback preserves the
original job/CV relationship. If the replacement file was finalized before
failure, the service attempts cleanup under the shared lock. See
[managed-file recovery](storage.md#permanent-deletion-and-startup-recovery).

After commit, the GUI controller looks up the job in `JobApplicationListModel`.
If the row is missing, it records `CommittedChangePublicationFailed` and skips
company and CV publication. When completion is enabled, the editor receives a
failure explaining that the update was saved but the view could not refresh.
If the row is present, the model updates it in place and emits only the changed
roles. Company-role changes refresh directory counts, status changes refresh
dashboard metrics, and CV changes move the application link from the previous
CV to the replacement.
`CvLibraryController` updates restored archive roles before moving links. It
also publishes a replacement whose CV ID equals the previous ID, so an archived
CV can be restored in place. Validation and persistence failures return field
errors or a message to the editor.

`Main.qml::requestProtectedAction()` guards the Job Applications tab, sidebar
navigation, Add Job, the global Add CV picker, extensible app-owned actions, and
application close. A dirty editor shows one modal Unsaved changes dialog with
Save, Don't Save, and Cancel. Save submits directly and executes the stored
action only after update success; Don't Save reloads the latest saved model state
before executing it; Cancel preserves the draft and cancels the action.
Navigation attempted during an update is deferred until success and abandoned on
failure. A clean edit session exits to read-only without prompting. The editor's
own CV picker and confirmation are deliberately outside this guard. After an
update-driven close continues, the existing pending-work close guard still
handles unrelated create, CV import, and removal work.
