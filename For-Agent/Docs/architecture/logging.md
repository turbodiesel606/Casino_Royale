# Application logging

JobTracker records operational outcomes asynchronously. The event catalog in
[`LogCatalog.inc`](../../../src/logging/LogCatalog.inc) defines the severity,
component and English message template for each of the 62 event types. Startup
uses the CMake project version and a fixed platform name.

## Producer contract

`ApplicationLog::record(LogEvent) noexcept` accepts bounded, owned values.
Pass the event's facts in template order. Counts, fixed phrases and existing
operation numbers are suitable for the log. Paths, SQL text, exception
messages, CV contents, user-entered values and QObject pointers cannot be
supplied as payloads. Formatting takes place on the consumer thread.

The transaction owner records the result after commit or failure handling.
Worker results are recorded before GUI delivery. Delivery or model publication
can fail independently of persistence. A rollback or file restoration that
fails has its own consequence event. A commit qualifies as following a
cancellation request only when cancellation was observed before commit.

## Queue and lifetime

`App::start` creates the session after naming the application and before
constructing `AppBootstrap`. Each producer timestamps its entry in UTC and
briefly acquires the inbox mutex. The deque holds up to 4,096 entries. A full
or closed inbox discards incoming records at every severity. The consumer takes
one entry in enqueue order and releases the mutex before accessing a file.
Timestamps from different producers can appear out of order.

The session outlives the controllers and workers. On normal exit bootstrap
reports completion after joining all three workers and verifying cleanup.
Session teardown closes admission, wakes the consumer, drains accepted entries
and joins the consumer after its final flush. Forced termination can lose
pending or buffered records. Storage operations and UI outcomes never depend
on admission or successful delivery to disk.

## Files

The writer uses `QStandardPaths::StateLocation/Logs` independently of the
business data directory. Each UTF-8 line contains the UTC timestamp with
millisecond precision, severity, component and message. Numbered segments
start at `jobtracker-000001.log`. A restart appends to the newest segment
when it has a complete tail and room for the next line. The writer rotates
before a line including its newline would exceed 5 MiB.

The retention budget is 50 MiB across recognized segments. The writer prunes
old closed segments in numeric order and skips unrelated files and symbolic
links. It flushes after errors, at idle, before rotation and on shutdown.
During sustained traffic it checks a one-second flush deadline between writes.
After a write failure the next record gets a fresh attempt. A partial trailing
line stays in its closed segment. Multiple processes sharing a log directory
have no coordination guarantee.
