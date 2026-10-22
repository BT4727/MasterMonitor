#include "eventrecordmodel.h"
#include "../util/dbconn.h"

#include <QSqlError>
#include <QSqlQuery>
EventRecordModel::EventRecordModel()
{
    connectionName = QStringLiteral("eventRecord_%1").arg(reinterpret_cast<quintptr>(this), 0, 16);
}

EventRecordModel::~EventRecordModel()
{
    if (!database.isValid())
        return;

    const QString currentConnectionName = database.connectionName();

    database.close();
    database = QSqlDatabase();

    QSqlDatabase::removeDatabase(currentConnectionName);
}

bool EventRecordModel::openDatabase(QString &errorMessage)
{
    if (!QSqlDatabase::contains(connectionName)) {
        database = QSqlDatabase::addDatabase(
            QStringLiteral("QMYSQL"),
            connectionName);

        DBConn::configureDatabase(database);
    } else {
        database = QSqlDatabase::database(connectionName, false);
    }

    if (!database.isValid()) {
        errorMessage = QStringLiteral("创建磁盘告警数据库连接失败");
        return false;
    }

    if (!database.isOpen() && !database.open()) {
        errorMessage = database.lastError().text();
        return false;
    }

    return true;
}

bool EventRecordModel::insertDiskLowEvent(
    int remainingPercent,
    const QString &storagePath,
    QString &errorMessage)
{
    if (!openDatabase(errorMessage))
        return false;

    QSqlQuery query(database);

    query.prepare(QStringLiteral(
        "INSERT INTO event_record "
        "(channel_no, event_type, description) "
        "VALUES (NULL, :event_type, :description)"));

    query.bindValue(QStringLiteral(":event_type"), QStringLiteral("disk_low"));
    query.bindValue(
        QStringLiteral(":description"),
        QStringLiteral("录像存储空间剩余 %1%，路径：%2")
            .arg(remainingPercent)
            .arg(storagePath));

    if (!query.exec()) {
        errorMessage = query.lastError().text();
        return false;
    }

    return true;
}