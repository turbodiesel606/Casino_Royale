#ifndef JOBTRACKER_SRC_CVS_CVREPOSITORY_HPP
#define JOBTRACKER_SRC_CVS_CVREPOSITORY_HPP

#include "CvDocument.hpp"

#include <optional>

class QSqlDatabase;

class CvRepository final
{
public:
	explicit CvRepository(QSqlDatabase& database);

	QVector<CvDocument> findAll() const;
	std::optional<CvDocument>
		findByIdentity(
			const QString& sha256,
			const QString& originalFileName) const;
	void insert(const CvDocument& document) const;
	bool updateFavorite(const QString& cvId, bool isFavorite) const;

private:
	QSqlDatabase& database_;
};

#endif // JOBTRACKER_SRC_CVS_CVREPOSITORY_HPP
