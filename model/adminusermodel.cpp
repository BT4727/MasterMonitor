#include "adminusermodel.h"

#include "adminusermodel.h"
#include "../util/dbconn.h"

#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

AdminUserModel::QueryResult AdminUserModel::findByUsername(const QString &username, AdminUserRecord &record) const
{
    // 查询账户凭据、启用状态和登录失败锁定信息。
    record = AdminUserRecord();

    DBConn *connection = DBConn::getInstance();
    QSqlDatabase database = connection->getDb();

    if (!database.isOpen() && !database.open()) {
        qWarning() << "查询用户信息时数据库连接失败："
                   << database.lastError().text();
        return QueryResult::DatabaseError;
    }

    QSqlQuery query(database);

    const QString sql = QStringLiteral(
        "SELECT username, password, status, "
        "error_times, locked_until "
        "FROM admin_user "
        "WHERE username = :username "
        "LIMIT 1");

    if (!query.prepare(sql)) {
        qWarning() << "用户查询语句准备失败："
                   << query.lastError().text();
        return QueryResult::DatabaseError;
    }

    query.bindValue(QStringLiteral(":username"), username);

    if (!query.exec()) {
        qWarning() << "用户查询失败："
                   << query.lastError().text();
        return QueryResult::DatabaseError;
    }

    if (!query.next())
        return QueryResult::NotFound;

    record.username = query.value(
                               QStringLiteral("username")).toString();

    record.passwordMd5 = query.value(
                                  QStringLiteral("password")).toString();

    record.enabled = query.value(
                              QStringLiteral("status")).toInt() == 1;

    record.errorTimes = query.value(
                                 QStringLiteral("error_times")).toInt();

    if (query.value(QStringLiteral("locked_until")).isNull()) {
        record.lockedUntil = QDateTime();
    } else {
        record.lockedUntil = query.value(
                                      QStringLiteral("locked_until")).toDateTime();
    }

    return QueryResult::Found;
}

bool AdminUserModel::updateLoginFailure(
    const QString &username,
    int errorTimes,
    const QDateTime &lockedUntil) const
{
    // 更新账户连续登录失败次数及可选锁定截止时间。
    DBConn *connection = DBConn::getInstance();
    QSqlDatabase database = connection->getDb();

    if (!database.isOpen() && !database.open()) {
        qWarning() << "更新登录失败次数时数据库连接失败："
                   << database.lastError().text();
        return false;
    }

    QSqlQuery query(database);

    const QString sql = QStringLiteral(
        "UPDATE admin_user "
        "SET error_times = :error_times, "
        "locked_until = :locked_until "
        "WHERE username = :username");

    if (!query.prepare(sql)) {
        qWarning() << "更新登录失败次数语句准备失败："
                   << query.lastError().text();
        return false;
    }

    query.bindValue(
        QStringLiteral(":error_times"),
        errorTimes);

    if (lockedUntil.isValid()) {
        query.bindValue(
            QStringLiteral(":locked_until"),
            lockedUntil);
    } else {
        query.bindValue(
            QStringLiteral(":locked_until"),
            QVariant());
    }

    query.bindValue(
        QStringLiteral(":username"),
        username);

    if (!query.exec()) {
        qWarning() << "更新登录失败次数失败："
                   << query.lastError().text();
        return false;
    }

    return true;
}

bool AdminUserModel::clearLoginFailures(
    const QString &username) const
{
    // 登录成功或锁定到期后清除账户失败次数和锁定时间。
    DBConn *connection = DBConn::getInstance();
    QSqlDatabase database = connection->getDb();

    if (!database.isOpen() && !database.open()) {
        qWarning() << "清除登录失败次数时数据库连接失败："
                   << database.lastError().text();
        return false;
    }

    QSqlQuery query(database);

    const QString sql = QStringLiteral(
        "UPDATE admin_user "
        "SET error_times = 0, "
        "locked_until = NULL "
        "WHERE username = :username");

    if (!query.prepare(sql)) {
        qWarning() << "清除登录失败次数语句准备失败："
                   << query.lastError().text();
        return false;
    }

    query.bindValue(
        QStringLiteral(":username"),
        username);

    if (!query.exec()) {
        qWarning() << "清除登录失败次数失败："
                   << query.lastError().text();
        return false;
    }

    return true;
}
