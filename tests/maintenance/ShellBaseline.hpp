#ifndef JOBTRACKER_TESTS_MAINTENANCE_SHELLBASELINE_HPP
#define JOBTRACKER_TESTS_MAINTENANCE_SHELLBASELINE_HPP

#include <QQuickItem>

namespace {

template<typename Predicate>
QQuickItem* visibleItem(QQuickItem* item, const Predicate& match)
{
    if (!item || !item->isVisible())
        return nullptr;
    if (match(item))
        return item;
    for (auto* child : item->childItems()) {
        if (auto* found = visibleItem(child, match))
            return found;
    }
    return nullptr;
}

}

#endif // JOBTRACKER_TESTS_MAINTENANCE_SHELLBASELINE_HPP
