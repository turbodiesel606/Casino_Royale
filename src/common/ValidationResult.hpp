#ifndef JOBTRACKER_SRC_COMMON_VALIDATIONRESULT_HPP
#define JOBTRACKER_SRC_COMMON_VALIDATIONRESULT_HPP

#include <QStringList>

struct ValidationResult
{
    bool isValid_ = true;
    QStringList messages_;
};

#endif // JOBTRACKER_SRC_COMMON_VALIDATIONRESULT_HPP
