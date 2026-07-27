#include "CompanyRepository.hpp"

#include "utils/Utils.hpp"

#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QUuid>

#include <stdexcept>

namespace {

QString normalizedCompanyName(const QString& name)
{
    return name.trimmed().toCaseFolded();
}

Company companyFromQuery(const QSqlQuery& query)
{
    Company company;
    company.id_ = query.value(QStringLiteral("id")).toString();
    company.name_ = query.value(QStringLiteral("display_name")).toString();
    company.createdAt_ = QDateTime::fromString(
        query.value(QStringLiteral("created_at")).toString(),
        Qt::ISODate);
    company.updatedAt_ = QDateTime::fromString(
        query.value(QStringLiteral("updated_at")).toString(),
        Qt::ISODate);
    return company;
}

}

CompanyRepository::CompanyRepository(QSqlDatabase& database)
    : database_(database)
{
}

QVector<Company> CompanyRepository::findAll() const
{
    QSqlQuery query{database_};
    if (!query.exec(QStringLiteral(
            "SELECT id, display_name, created_at, updated_at "
            "FROM companies ORDER BY normalized_name"))) {
        utils::throwQueryError(query);
    }

    QVector<Company> companies;
    while (query.next()) {
        companies.append(companyFromQuery(query));
    }
    return companies;
}

Company CompanyRepository::findOrCreateByName(const QString& name) const
{
    const auto displayName = name.trimmed();
    const auto normalizedName = normalizedCompanyName(displayName);
    if (normalizedName.isEmpty()) {
        throw std::invalid_argument("Company name must not be blank.");
    }

    QSqlQuery findQuery{database_};
    findQuery.prepare(QStringLiteral(
        "SELECT id, display_name, created_at, updated_at "
        "FROM companies WHERE normalized_name = ?"));
    findQuery.addBindValue(normalizedName);
    if (!findQuery.exec()) {
        utils::throwQueryError(findQuery);
    }
    if (findQuery.next()) {
        return companyFromQuery(findQuery);
    }

    const auto now = QDateTime::currentDateTimeUtc();
    Company company;
    company.id_ = QUuid::createUuid().toString(QUuid::WithoutBraces);
    company.name_ = displayName;
    company.createdAt_ = now;
    company.updatedAt_ = now;

    QSqlQuery insertQuery{database_};
    insertQuery.prepare(QStringLiteral(
        "INSERT INTO companies (id, display_name, normalized_name, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?)"));
    insertQuery.addBindValue(company.id_);
    insertQuery.addBindValue(company.name_);
    insertQuery.addBindValue(normalizedName);
    insertQuery.addBindValue(now.toString(Qt::ISODateWithMs));
    insertQuery.addBindValue(now.toString(Qt::ISODateWithMs));
    if (!insertQuery.exec()) {
        utils::throwQueryError(insertQuery);
    }

    return company;
}
