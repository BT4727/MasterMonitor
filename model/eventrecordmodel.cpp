#include "eventrecordmodel.h"
#include "../util/dbconn.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QVariantMap>
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

bool EventRecordModel::searchPage(const QDateTime &startTime, const QDateTime &endTime,
                                  int channelNo, int page, int pageSize,
                                  QVector<EventLogInfo> &events, int &totalCount,
                                  QString &errorMessage)
{
    events.clear();
    totalCount = 0;

    if (!openDatabase(errorMessage))
        return false;

    QString condition = QStringLiteral(
        " FROM event_record AS event_record "
        "WHERE event_record.create_time >= :start_time "
        "AND event_record.create_time < :end_time ");
    if (channelNo >= 1 && channelNo <= 4)
        condition += QStringLiteral("AND event_record.channel_no = :channel_no ");

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
            "SELECT event_record.id, event_record.create_time, "
            "event_record.channel_no, event_record.event_type, "
            "(SELECT COUNT(*) FROM snapshot "
            " WHERE snapshot.event_id = event_record.id) AS image_count, "
            "COALESCE((SELECT video_record.id FROM video_record "
            " WHERE video_record.event_id = event_record.id "
            " ORDER BY video_record.start_time DESC LIMIT 1), 0) AS video_id, "
            "COALESCE((SELECT video_record.video_path FROM video_record "
            " WHERE video_record.event_id = event_record.id "
            " ORDER BY video_record.start_time DESC LIMIT 1), '') AS video_path ")
        + condition
        + QStringLiteral(
            "ORDER BY event_record.create_time DESC, event_record.id DESC "
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
        EventLogInfo event;
        event.id = query.value(QStringLiteral("id")).toULongLong();
        event.createTime = query.value(QStringLiteral("create_time")).toDateTime();
        event.channelNo = query.value(QStringLiteral("channel_no")).toInt();
        event.eventType = query.value(QStringLiteral("event_type")).toString();
        event.imageCount = query.value(QStringLiteral("image_count")).toInt();
        event.videoId = query.value(QStringLiteral("video_id")).toULongLong();
        event.videoPath = query.value(QStringLiteral("video_path")).toString();
        events.append(event);
    }

    return true;
}

bool EventRecordModel::deleteByIds(const QVector<qulonglong> &eventIds,
                                   QString &errorMessage)
{
    if (eventIds.isEmpty())
        return true;

    QStringList placeholders;
    QVariantMap bindings;
    for (int index = 0; index < eventIds.size(); ++index) {
        const QString placeholder = QStringLiteral(":id%1").arg(index);
        placeholders.append(placeholder);
        bindings.insert(placeholder, QVariant::fromValue(eventIds.at(index)));
    }

    return deleteMatchingEvents(
        QStringLiteral("id IN (%1)").arg(placeholders.join(QLatin1Char(','))),
        bindings, errorMessage);
}

bool EventRecordModel::deleteBefore(const QDateTime &time, QString &errorMessage)
{
    QVariantMap bindings;
    bindings.insert(QStringLiteral(":delete_time"), time);
    return deleteMatchingEvents(
        QStringLiteral("create_time < :delete_time"), bindings, errorMessage);
}

bool EventRecordModel::deleteRange(const QDateTime &startTime,
                                   const QDateTime &endTime, int channelNo,
                                   QString &errorMessage)
{
    QString condition = QStringLiteral(
        "create_time >= :start_time AND create_time < :end_time");
    QVariantMap bindings;
    bindings.insert(QStringLiteral(":start_time"), startTime);
    bindings.insert(QStringLiteral(":end_time"), endTime);

    if (channelNo >= 1 && channelNo <= 4) {
        condition += QStringLiteral(" AND channel_no = :channel_no");
        bindings.insert(QStringLiteral(":channel_no"), channelNo);
    }

    return deleteMatchingEvents(condition, bindings, errorMessage);
}

bool EventRecordModel::deleteMatchingEvents(const QString &condition,
                                            const QVariantMap &bindings,
                                            QString &errorMessage)
{
    if (!openDatabase(errorMessage))
        return false;

    QSqlQuery idQuery(database);
    idQuery.prepare(QStringLiteral("SELECT id FROM event_record WHERE ") + condition);
    for (QVariantMap::const_iterator iterator = bindings.constBegin();
         iterator != bindings.constEnd(); ++iterator) {
        idQuery.bindValue(iterator.key(), iterator.value());
    }

    if (!idQuery.exec()) {
        errorMessage = idQuery.lastError().text();
        return false;
    }

    QVector<qulonglong> eventIds;
    while (idQuery.next())
        eventIds.append(idQuery.value(0).toULongLong());

    if (eventIds.isEmpty())
        return true;

    QStringList placeholders;
    for (int index = 0; index < eventIds.size(); ++index)
        placeholders.append(QStringLiteral(":event%1").arg(index));
    const QString idList = placeholders.join(QLatin1Char(','));

    if (!database.transaction()) {
        errorMessage = database.lastError().text();
        return false;
    }

    const QStringList sqlStatements = {
        QStringLiteral("UPDATE snapshot SET event_id = NULL WHERE event_id IN (%1)").arg(idList),
        QStringLiteral("UPDATE video_record SET event_id = NULL WHERE event_id IN (%1)").arg(idList),
        QStringLiteral("DELETE FROM event_record WHERE id IN (%1)").arg(idList)
    };

    for (const QString &sql : sqlStatements) {
        QSqlQuery query(database);
        query.prepare(sql);
        for (int index = 0; index < eventIds.size(); ++index)
            query.bindValue(placeholders.at(index), eventIds.at(index));

        if (!query.exec()) {
            errorMessage = query.lastError().text();
            database.rollback();
            return false;
        }
    }

    if (!database.commit()) {
        errorMessage = database.lastError().text();
        database.rollback();
        return false;
    }

    return true;
}
