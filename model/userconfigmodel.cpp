#include "userconfigmodel.h"
#include "../util/dbconn.h"

#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
UserConfigModel::UserConfigModel() {}
UserConfigModel::Result UserConfigModel::findConfigPath(
    const QString &username,
    QString &configPath) const
{
    configPath.clear();

    QSqlDatabase database =
        DBConn::getInstance()->getDb();

    if (!database.isOpen() && !database.open())
        return Result::DatabaseError;

    QSqlQuery query(database);

    query.prepare(QStringLiteral(
        "SELECT config.config_path "
        "FROM user_config_file AS config "
        "INNER JOIN admin_user AS user "
        "ON user.id = config.user_id "
        "WHERE user.username = :username "
        "LIMIT 1"));

    query.bindValue(
        QStringLiteral(":username"), username);

    if (!query.exec()) {
        qWarning() << "查询用户配置路径失败："
                   << query.lastError().text();
        return Result::DatabaseError;
    }

    if (!query.next())
        return Result::NotFound;

    configPath = query.value(
                          QStringLiteral("config_path")).toString();

    return Result::Success;
}

UserConfigModel::Result UserConfigModel::saveConfigPath(
    const QString &username,
    const QString &configPath) const
{
    QSqlDatabase database =
        DBConn::getInstance()->getDb();

    if (!database.isOpen() && !database.open())
        return Result::DatabaseError;

    QSqlQuery userQuery(database);

    userQuery.prepare(QStringLiteral(
        "SELECT id FROM admin_user "
        "WHERE username = :username LIMIT 1"));

    userQuery.bindValue(
        QStringLiteral(":username"), username);

    if (!userQuery.exec())
        return Result::DatabaseError;

    if (!userQuery.next())
        return Result::NotFound;

    const qulonglong userId =
        userQuery.value(QStringLiteral("id")).toULongLong();

    QSqlQuery saveQuery(database);

    saveQuery.prepare(QStringLiteral(
        "INSERT INTO user_config_file "
        "(user_id, config_path) "
        "VALUES (:user_id, :config_path) "
        "ON DUPLICATE KEY UPDATE "
        "config_path = VALUES(config_path)"));

    saveQuery.bindValue(
        QStringLiteral(":user_id"), userId);
    saveQuery.bindValue(
        QStringLiteral(":config_path"), configPath);

    if (!saveQuery.exec()) {
        qWarning() << "保存用户配置路径失败："
                   << saveQuery.lastError().text();
        return Result::DatabaseError;
    }

    return Result::Success;
}
