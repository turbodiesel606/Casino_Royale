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
    company.logoText_ = company.name_.left(2).toUpper();
    company.logoAccent_ = QStringLiteral("#146ce0");
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
            "SELECT id, display_name FROM companies ORDER BY normalized_name"))) {
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
        "SELECT id, display_name FROM companies WHERE normalized_name = ?"));
    findQuery.addBindValue(normalizedName);
    if (!findQuery.exec()) {
        utils::throwQueryError(findQuery);
    }
    if (findQuery.next()) {
        return companyFromQuery(findQuery);
    }

    const auto now = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    Company company;
    company.id_ = QUuid::createUuid().toString(QUuid::WithoutBraces);
    company.name_ = displayName;
    company.logoText_ = company.name_.left(2).toUpper();
    company.logoAccent_ = QStringLiteral("#146ce0");

    QSqlQuery insertQuery{database_};
    insertQuery.prepare(QStringLiteral(
        "INSERT INTO companies (id, display_name, normalized_name, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?)"));
    insertQuery.addBindValue(company.id_);
    insertQuery.addBindValue(company.name_);
    insertQuery.addBindValue(normalizedName);
    insertQuery.addBindValue(now);
    insertQuery.addBindValue(now);
    if (!insertQuery.exec()) {
        utils::throwQueryError(insertQuery);
    }

    return company;
}
