#include "operationlogmodel.h"
#include "../util/dbconn.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QVariant>

OperationLogModel::OperationLogModel()
{
    connectionName = QStringLiteral("operationLog_%1")
        .arg(reinterpret_cast<quintptr>(this), 0, 16);
}

OperationLogModel::~OperationLogModel()
{
    if (!database.isValid())
        return;

    const QString currentConnectionName = database.connectionName();
    database.close();
    database = QSqlDatabase();
    QSqlDatabase::removeDatabase(currentConnectionName);
}

bool OperationLogModel::openDatabase(QString &errorMessage)
{
    if (!QSqlDatabase::contains(connectionName)) {
        database = QSqlDatabase::addDatabase(QStringLiteral("QMYSQL"), connectionName);
        DBConn::configureDatabase(database);
    } else {
        database = QSqlDatabase::database(connectionName, false);
    }

    if (!database.isValid()) {
        errorMessage = QStringLiteral("创建操作日志数据库连接失败");
        return false;
    }

    if (!database.isOpen() && !database.open()) {
        errorMessage = database.lastError().text();
        return false;
    }

    return ensureChannelColumn(errorMessage);
}

bool OperationLogModel::ensureChannelColumn(QString &errorMessage)
{
    QSqlQuery checkQuery(database);
    if (!checkQuery.exec(QStringLiteral(
            "SHOW COLUMNS FROM operation_log LIKE 'channel_no'"))) {
        errorMessage = checkQuery.lastError().text();
        return false;
    }

    if (checkQuery.next())
        return true;

    QSqlQuery alterQuery(database);
    const QString sql = QStringLiteral(
        "ALTER TABLE operation_log "
        "ADD COLUMN channel_no TINYINT UNSIGNED NULL "
        "COMMENT '关联通道1-4；无通道操作为空' AFTER user_id, "
        "ADD KEY idx_operation_channel_time (channel_no, create_time), "
        "ADD CONSTRAINT chk_operation_channel "
        "CHECK (channel_no IS NULL OR channel_no BETWEEN 1 AND 4)");

    if (!alterQuery.exec(sql)) {
        errorMessage = alterQuery.lastError().text();
        return false;
    }

    return true;
}

QVariant OperationLogModel::findUserId(const QString &username, QString &errorMessage)
{
    if (username.trimmed().isEmpty())
        return QVariant();

    QSqlQuery query(database);
    query.prepare(QStringLiteral(
        "SELECT id FROM admin_user WHERE username = :username LIMIT 1"));
    query.bindValue(QStringLiteral(":username"), username.trimmed());

    if (!query.exec()) {
        errorMessage = query.lastError().text();
        return QVariant();
    }

    if (!query.next())
        return QVariant();

    return query.value(QStringLiteral("id"));
}

bool OperationLogModel::insertLog(const QString &username, int channelNo,
                                  const QString &operationType, const QString &targetType,
                                  qulonglong targetId, bool success,
                                  const QString &description, QString &errorMessage)
{
    errorMessage.clear();

    if (!openDatabase(errorMessage))
        return false;

    const QVariant userId = findUserId(username, errorMessage);
    if (!errorMessage.isEmpty())
        return false;

    QSqlQuery query(database);
    query.prepare(QStringLiteral(
        "INSERT INTO operation_log "
        "(user_id, channel_no, create_time, operation_type, target_type, "
        "target_id, operation_result, operation_description) "
        "VALUES (:user_id, :channel_no, CURRENT_TIMESTAMP, :operation_type, "
        ":target_type, :target_id, :operation_result, :description)"));

    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":channel_no"),
                    channelNo >= 1 && channelNo <= 4 ? QVariant(channelNo) : QVariant());
    query.bindValue(QStringLiteral(":operation_type"), operationType.left(20));
    query.bindValue(QStringLiteral(":target_type"),
                    targetType.isEmpty() ? QVariant() : QVariant(targetType.left(20)));
    query.bindValue(QStringLiteral(":target_id"),
                    targetId == 0 ? QVariant() : QVariant::fromValue(targetId));
    query.bindValue(QStringLiteral(":operation_result"), success ? 1 : 0);
    query.bindValue(QStringLiteral(":description"), description.left(500));

    if (!query.exec()) {
        errorMessage = query.lastError().text();
        return false;
    }

    return true;
}

bool OperationLogModel::searchPage(const QDateTime &startTime, const QDateTime &endTime,
                                   int channelNo, int page, int pageSize,
                                   QVector<OperationLogInfo> &logs, int &totalCount,
                                   QString &errorMessage)
{
    logs.clear();
    totalCount = 0;

    if (!openDatabase(errorMessage))
        return false;

    QString condition = QStringLiteral(
        " FROM operation_log AS operation_log "
        "LEFT JOIN admin_user AS admin_user ON operation_log.user_id = admin_user.id "
        "WHERE operation_log.create_time >= :start_time "
        "AND operation_log.create_time < :end_time ");

    if (channelNo >= 1 && channelNo <= 4)
        condition += QStringLiteral("AND operation_log.channel_no = :channel_no ");

    QSqlQuery countQuery(database);
    countQuery.prepare(QStringLiteral("SELECT COUNT(*)") + condition);
    countQuery.bindValue(QStringLiteral(":start_time"), startTime);
    countQuery.bindValue(QStringLiteral(":end_time"), endTime);
    if (channelNo >= 1 && channelNo <= 4)
        countQuery.bindValue(QStringLiteral(":channel_no"), channelNo);

    if (!countQuery.exec() || !countQuery.next()) {
        errorMessage = countQuery.lastError().text();
        return false;
    }

    totalCount = countQuery.value(0).toInt();

    QSqlQuery query(database);
    query.prepare(
        QStringLiteral(
            "SELECT operation_log.id, COALESCE(admin_user.username, '未识别用户') AS username, "
            "operation_log.channel_no, operation_log.create_time, "
            "operation_log.operation_type, operation_log.target_type, "
            "operation_log.target_id, operation_log.operation_result, "
            "operation_log.operation_description ")
        + condition
        + QStringLiteral(
            "ORDER BY operation_log.create_time DESC, operation_log.id DESC "
            "LIMIT :page_size OFFSET :page_offset"));

    query.bindValue(QStringLiteral(":start_time"), startTime);
    query.bindValue(QStringLiteral(":end_time"), endTime);
    if (channelNo >= 1 && channelNo <= 4)
        query.bindValue(QStringLiteral(":channel_no"), channelNo);
    query.bindValue(QStringLiteral(":page_size"), pageSize);
    query.bindValue(QStringLiteral(":page_offset"), (page - 1) * pageSize);

    if (!query.exec()) {
        errorMessage = query.lastError().text();
        return false;
    }

    while (query.next()) {
        OperationLogInfo log;
        log.id = query.value(QStringLiteral("id")).toULongLong();
        log.username = query.value(QStringLiteral("username")).toString();
        log.channelNo = query.value(QStringLiteral("channel_no")).toInt();
        log.createTime = query.value(QStringLiteral("create_time")).toDateTime();
        log.operationType = query.value(QStringLiteral("operation_type")).toString();
        log.targetType = query.value(QStringLiteral("target_type")).toString();
        log.targetId = query.value(QStringLiteral("target_id")).toULongLong();
        log.success = query.value(QStringLiteral("operation_result")).toInt() == 1;
        log.description = query.value(QStringLiteral("operation_description")).toString();
        logs.append(log);
    }

    return true;
}

bool OperationLogModel::deleteByIds(const QVector<qulonglong> &logIds, QString &errorMessage)
{
    if (logIds.isEmpty())
        return true;

    if (!openDatabase(errorMessage))
        return false;

    QStringList placeholders;
    for (int index = 0; index < logIds.size(); ++index)
        placeholders.append(QStringLiteral(":id%1").arg(index));

    QSqlQuery query(database);
    query.prepare(QStringLiteral("DELETE FROM operation_log WHERE id IN (%1)")
                      .arg(placeholders.join(QLatin1Char(','))));

    for (int index = 0; index < logIds.size(); ++index)
        query.bindValue(placeholders.at(index), logIds.at(index));

    if (!query.exec()) {
        errorMessage = query.lastError().text();
        return false;
    }

    return true;
}

bool OperationLogModel::deleteBefore(const QDateTime &time, QString &errorMessage)
{
    if (!openDatabase(errorMessage))
        return false;

    QSqlQuery query(database);
    query.prepare(QStringLiteral(
        "DELETE FROM operation_log WHERE create_time < :delete_time"));
    query.bindValue(QStringLiteral(":delete_time"), time);

    if (!query.exec()) {
        errorMessage = query.lastError().text();
        return false;
    }

    return true;
}

bool OperationLogModel::deleteRange(const QDateTime &startTime,
                                    const QDateTime &endTime, int channelNo,
                                    QString &errorMessage)
{
    if (!openDatabase(errorMessage))
        return false;

    QString sql = QStringLiteral(
        "DELETE FROM operation_log "
        "WHERE create_time >= :start_time AND create_time < :end_time ");
    if (channelNo >= 1 && channelNo <= 4)
        sql += QStringLiteral("AND channel_no = :channel_no");

    QSqlQuery query(database);
    query.prepare(sql);
    query.bindValue(QStringLiteral(":start_time"), startTime);
    query.bindValue(QStringLiteral(":end_time"), endTime);
    if (channelNo >= 1 && channelNo <= 4)
        query.bindValue(QStringLiteral(":channel_no"), channelNo);

    if (!query.exec()) {
        errorMessage = query.lastError().text();
        return false;
    }

    return true;
}
