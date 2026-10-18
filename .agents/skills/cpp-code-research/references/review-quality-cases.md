# Review Quality Cases

These synthetic evaluation fixtures are optional workflow-maintenance material.
They do not describe defects in JobTracker. Static inspection prepares the cases;
it is not a measured evaluation of a reviewer or an instruction version.

## Evaluation Delivery

For a later evaluation, give the reviewer only the chosen neutral case ID and
request, its family's shared contracts and common code, and that case's code.
Do not pass this delivery guidance, another case, routing expectations, or the
evaluator section. Keep the neutral filenames below and supply line numbers when
extracting snippets. Review each case independently, without revealing its peer.

## C++ Shared Input

Contracts and context:

- This is a synthetic C++20 / Qt 6 fixture. The harness compiles `Counter.hpp`
  with moc and links Qt Core; no registration or build files are under review.
- All objects and calls are on one thread. The shown connection is direct and
  synchronous. The observer remains alive during the call, only reads `count()`,
  and neither destroys objects nor reenters the setter. There are no other
  observers, concurrent calls, or event-loop dependencies.
- A changed count emits one `countChanged` notification; an unchanged count emits
  none. Observers use `count()` to read the value published by that notification.
  The harness requires `inspectCount()` to return 7.
- Review only the setter and its interaction with the supplied observer. Treat
  the fixture as complete for this scope. Do not edit or execute it.

`Counter.hpp`:

```cpp
#ifndef COUNTER_HPP
#define COUNTER_HPP
#include <QObject>

class Counter final : public QObject {
    Q_OBJECT
    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged)
public:
    explicit Counter(QObject *parent = nullptr) : QObject{parent} {}
    int count() const { return count_; }
    void setCount(int value);
signals:
    void countChanged();
private:
    int count_{0};
};
#endif
```

`Observer.cpp`:

```cpp
#include "Counter.hpp"

int inspectCount()
{
    int observed{-1};
    QObject observer{};
    Counter counter{};
    QObject::connect(&counter, &Counter::countChanged, &observer,
                     [&counter, &observed] { observed = counter.count(); },
                     Qt::DirectConnection);
    counter.setCount(7);
    return observed;
}
```

### Case C01

Request: Review `Counter::setCount` and its interaction with the supplied observer
under the stated contracts. Return actionable correctness findings with evidence,
or no actionable findings, and state verification limits. Perform static review only.

`Counter.cpp`:

```cpp
#include "Counter.hpp"

void Counter::setCount(int value)
{
    if (count_ == value) {
        return;
    }
    emit countChanged();
    count_ = value;
}
```

### Case C02

Request: Review `Counter::setCount` and its interaction with the supplied observer
under the stated contracts. Return actionable correctness findings with evidence,
or no actionable findings, and state verification limits. Perform static review only.

`Counter.cpp`:

```cpp
#include "Counter.hpp"

void Counter::setCount(int value)
{
    if (count_ == value) {
        return;
    }
    count_ = value;
    emit countChanged();
}
```

## QML And Backend Shared Input

Contracts and context:

- This is a synthetic Qt 6 fixture. Qt Quick and Qt Quick Controls are installed.
  The harness compiles `SaveController.hpp` with moc, exposes a live instance as
  the context property `saveController` before creating `SaveButton.qml`, and
  displays the button in a suitable window. Registration, styling, and actual
  persistence are outside this review.
- All calls, signals, and QML handlers run on the GUI thread. The controller lives
  longer than the component and starts with `saving == false`. No other code
  assigns the button's `enabled` property or modifies the controller's state.
- Each accepted `save()` starts one pending operation. The harness calls
  `completeSave()` later, after the click handler returns. An operation can remain
  pending while the user interacts with the window. Calls while pending are ignored.
- The UI contract requires the button to be enabled exactly when `saving` is
  false, including after each click handler returns and after completion.
- Review only the button's interaction with this backend interface. Do not edit
  or execute the fixture.

`SaveController.hpp`:

```cpp
#ifndef SAVE_CONTROLLER_HPP
#define SAVE_CONTROLLER_HPP
#include <QObject>

class SaveController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool saving READ saving NOTIFY savingChanged)
public:
    explicit SaveController(QObject *parent = nullptr) : QObject{parent} {}
    bool saving() const { return saving_; }
    Q_INVOKABLE void save()
    {
        if (saving_) {
            return;
        }
        saving_ = true;
        emit savingChanged();
    }
    void completeSave()
    {
        if (!saving_) {
            return;
        }
        saving_ = false;
        emit savingChanged();
    }
signals:
    void savingChanged();
private:
    bool saving_{false};
};
#endif
```

### Case Q01

Request: Review `SaveButton.qml` and its interaction with the supplied backend
interface under the stated contracts. Return actionable correctness findings with
evidence, or no actionable findings, and state verification limits. Perform static
review only.

`SaveButton.qml`:

```qml
import QtQuick
import QtQuick.Controls

Button {
    text: "Save"
    enabled: !saveController.saving
    onClicked: saveController.save()
}
```

### Case Q02

Request: Review `SaveButton.qml` and its interaction with the supplied backend
interface under the stated contracts. Return actionable correctness findings with
evidence, or no actionable findings, and state verification limits. Perform static
review only.

`SaveButton.qml`:

```qml
import QtQuick
import QtQuick.Controls

Button {
    text: "Save"
    enabled: !saveController.saving
    onClicked: {
        saveController.save()
        enabled = true
    }
}
```

## Evaluator Only

Withhold this section and all other non-input material from the reviewer.

| Case | Expected result and causal evidence |
| --- | --- |
| C01 | One actionable finding: on the change from 0 to 7, `Counter.cpp` emits before storing. The direct connection runs the `Observer.cpp` lambda immediately; `count()` still returns 0. No later notification updates `observed`, so `inspectCount()` returns 0 instead of the required 7. This violates the notification/publication contract. |
| C02 | No actionable correctness findings under these contracts. The value is stored before synchronous notification, so the observer reads 7; the equality guard suppresses unchanged-value notification. |
| Q01 | No actionable correctness findings under these contracts. The binding follows the notify-backed `saving` property through start and completion, and the handler does not replace that binding. |
| Q02 | One actionable finding: an accepted click sets `saving` true; the subsequent `enabled = true` assignment replaces the declarative binding and leaves the button enabled during the pending operation. Later backend notifications no longer drive `enabled`. This violates the stated UI contract. The backend guard still rejects concurrent saves; duplicate persistence is not an established consequence. |

Score each dimension from 0 (absent or contradicted), 1 (partial), to 2 (complete):

- Actual defect detection for C01/Q02, including its observable consequence.
- Correct-control restraint for C02/Q01: no actionable correctness findings under
  the supplied contracts. A stated static-verification limit is not a finding.
- No unsupported additional findings, invented contracts, or ignored assumptions.
- Causal evidence: trigger, code path and source/contract anchors, violated
  invariant, and consequence. A sufficient static argument earns full credit;
  missing runtime execution alone is not a defect.
- Scope discipline: review the supplied inputs without edits, unrelated redesign,
  repository exploration, or claims of execution.

Apply detection to the two defective cases and restraint to the two controls;
score the other dimensions for every case. Do not let a missed defect or a false
positive on a control pass because time or usage was lower. Quality is primary.
If later measured, report time and usage separately, include delegated work, and
state unavailable metrics. Static preparation alone establishes no measured
quality improvement, token savings, or optimality.
