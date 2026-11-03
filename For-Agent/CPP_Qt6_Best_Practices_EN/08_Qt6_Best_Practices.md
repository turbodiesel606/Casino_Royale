# Qt 6: Objects, Threads, SQL, and QML

### QObject Ownership Through the Parent–Child Model

A parent `QObject` destroys its children according to Qt's ownership model. Lifetime decisions account for this model rather than automatically replacing it with smart pointers; independent owners of the same object must not introduce a risk of double deletion.

**Details:** [Qt 6 — Object Trees & Ownership](https://doc.qt.io/qt-6/objecttrees.html); [Qt 6 — QObject destruction](https://doc.qt.io/qt-6/qobject.html#dtor.QObject).

### QPointer for Non-Owning Access to QObject

`QPointer<T>` tracks an object derived from `QObject` and becomes null when that object is destroyed. It is suitable for non-owning references to objects with independent lifetimes, but it neither keeps an object alive nor replaces inter-thread synchronization.

**Details:** [Qt 6 — QPointer](https://doc.qt.io/qt-6/qpointer.html#details); [Qt 6 — accessing QObject across threads](https://doc.qt.io/qt-6/threads-qobject.html#accessing-qobject-subclasses-from-other-threads).

### Thread Affinity and Cross-Thread Calls

Use of a `QObject` accounts for its thread affinity and the thread-safety guarantees of the particular API. Cross-thread communication uses signals, slots, and queued calls with an appropriate context; executing queued calls requires a running event loop in the receiving thread.

**Details:** [Qt 6 — Threads and QObjects](https://doc.qt.io/qt-6/threads-qobject.html); [Qt 6 — QObject: Thread Affinity](https://doc.qt.io/qt-6/qobject.html#thread-affinity).

### Safe Lambdas and Asynchronous Callbacks

For lambdas tied to an object's lifecycle, `QObject::connect` with a context object is preferred. The context governs the connection's lifetime but does not extend the lifetime of arbitrary captured objects; capture safety, including the safety of capturing `this`, is ensured separately.

**Details:** [Qt 6 — Automatic Connection Management](https://doc.qt.io/qt-6/signalsandslots.html#automatic-connection-management); [Qt 6 — context-bound connections and capture lifetimes](https://doc.qt.io/qt-6/qobject.html).

### SQL Connection Thread Affinity

A `QSqlDatabase` connection, including an SQLite connection, is used in its associated thread; copying the handle does not create an independent connection. Creating a connection in the required thread or performing a supported transfer follows Qt's rules; `moveToThread` is available from Qt 6.8 onward and has restrictions.

**Details:** [Qt 6 — Threads and the SQL Module](https://doc.qt.io/qt-6/threads-modules.html#threads-and-the-sql-module); [Qt 6 — QSqlDatabase](https://doc.qt.io/qt-6/qsqldatabase.html#details); [Qt 6 — QSqlDatabase::moveToThread](https://doc.qt.io/qt-6/qsqldatabase.html#moveToThread).

### Separation of C++ and QML

QML primarily describes presentation and user interaction, while C++ handles business logic and data processing. Communication uses the intended data interfaces; C++ dependencies on the internal QML object tree and direct manipulation of that tree are minimized.

**Details:** [Qt 6 — Separate UI from Business Logic](https://doc.qt.io/qt-6/qtquick-bestpractices.html#separate-ui-from-business-logic); [Qt 6 — Exposing Data from C++ to QML](https://doc.qt.io/qt-6/qtquick-bestpractices.html#exposing-data-from-c-to-qml).
