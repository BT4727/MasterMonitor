#ifndef ADMINUSERMODEL_H
#define ADMINUSERMODEL_H
#include <QString>
#include <QDateTime>
struct AdminUserRecord
{
    QString username;
    QString passwordMd5;
    bool enabled = false;
    int errorTimes = 0;
    QDateTime lockedUntil;
};

class AdminUserModel
{
public:
    enum class QueryResult {
        Found,
        NotFound,
        DatabaseError
    };
    AdminUserModel();
    QueryResult findByUsername(
        const QString &username,
        AdminUserRecord &record) const;

    bool updateLoginFailure(
        const QString &username,
        int errorTimes,
        const QDateTime &lockedUntil) const;

    bool clearLoginFailures(
        const QString &username) const;
};

#endif // ADMINUSERMODEL_H
