#include "snapshotmodel.h"
#include "../util/dbconn.h"

#include <QSqlError>
#include <QSqlQuery>
SnapshotModel::SnapshotModel()
{
    // 为图片模型创建独立数据库连接名。
    connectionName = QStringLiteral("snapshot_%1").arg(reinterpret_cast<quintptr>(this), 0, 16);
}

SnapshotModel::~SnapshotModel()
{
    // 关闭并移除该模型持有的数据库连接。
    if (!database.isValid())
        return;

    const QString currentConnectionName =
        database.connectionName();

    database.close();
    database = QSqlDatabase();

    QSqlDatabase::removeDatabase(currentConnectionName);
}

void SnapshotModel::setCurrentUser(const QString &username)
{
    // 设置图片关联查询所使用的用户。
    currentUsername = username.trimmed();
}

bool SnapshotModel::openDatabase(QString &errorMessage)
{
    // 创建或打开图片模块专用数据库连接。
    if (!QSqlDatabase::contains(connectionName)) {
        database = QSqlDatabase::addDatabase(
            QStringLiteral("QMYSQL"),
            connectionName);

        DBConn::configureDatabase(database);
    } else {
        database = QSqlDatabase::database(
            connectionName,
            false);
    }

    if (!database.isValid()) {
        errorMessage = QStringLiteral("图片数据库连接无效");
        return false;
    }

    if (!database.isOpen() && !database.open()) {
        errorMessage = database.lastError().text();
        return false;
    }

    return true;
}

bool SnapshotModel::insertSnapshot(
    int channelNo,
    const QString &snapshotType,
    const QDateTime &createTime,
    const QDateTime &frameTime,
    const QString &snapshotPath,
    qulonglong videoId,
    qulonglong eventId,
    qint64 offsetMs,
    qulonglong &snapshotId,
    QString &errorMessage)
{
    // 写入图片元数据以及事件、录像和时间点关联信息。
    snapshotId = 0;

    if (!openDatabase(errorMessage))
        return false;

    QSqlQuery query(database);

    query.prepare(QStringLiteral(
        "INSERT INTO snapshot "
        "(channel_no, snapshot_type, create_time, frame_time, "
        "snapshot_path, video_id, event_id, offset_ms) "
        "VALUES (:channel_no, :snapshot_type, :create_time, "
        ":frame_time, :snapshot_path, :video_id, :event_id, :offset_ms)"));

    query.bindValue(
        QStringLiteral(":channel_no"),
        channelNo);

    query.bindValue(
        QStringLiteral(":snapshot_type"),
        snapshotType);

    query.bindValue(
        QStringLiteral(":create_time"),
        createTime.toString(
            QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")));

    query.bindValue(
        QStringLiteral(":frame_time"),
        frameTime.toString(
            QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")));

    query.bindValue(
        QStringLiteral(":snapshot_path"),
        snapshotPath);

    if (videoId == 0)
        query.bindValue(QStringLiteral(":video_id"), QVariant());
    else
        query.bindValue(QStringLiteral(":video_id"), videoId);

    if (eventId == 0)
        query.bindValue(QStringLiteral(":event_id"), QVariant());
    else
        query.bindValue(QStringLiteral(":event_id"), eventId);

    if (offsetMs < 0)
        query.bindValue(QStringLiteral(":offset_ms"), QVariant());
    else
        query.bindValue(QStringLiteral(":offset_ms"), offsetMs);

    if (!query.exec()) {
        errorMessage = query.lastError().text();
        return false;
    }

    snapshotId = query.lastInsertId().toULongLong();
    return snapshotId != 0;
}

bool SnapshotModel::searchSnapshots(
    const QDateTime &startTime,
    const QDateTime &endTime,
    int channelNo,
    QVector<SnapshotInfo> &snapshots,
    QString &errorMessage)
{
    // 按时间和通道倒序查询图片，并联查关联录像路径。
    snapshots.clear();

    if (!openDatabase(errorMessage))
        return false;

    QString sql = QStringLiteral(
        "SELECT s.id, s.channel_no, s.snapshot_type, s.create_time, "
        "s.frame_time, s.snapshot_path, s.video_id, s.event_id, "
        "s.offset_ms, v.video_path AS video_path "
        "FROM snapshot AS s "
        "LEFT JOIN video_record AS v ON s.video_id = v.id "
        "WHERE s.frame_time >= :start_time "
        "AND s.frame_time < :end_time ");

    if (channelNo > 0)
        sql += QStringLiteral("AND s.channel_no = :channel_no ");

    sql += QStringLiteral(
        "ORDER BY s.frame_time DESC, s.id DESC");

    QSqlQuery query(database);

    if (!query.prepare(sql)) {
        errorMessage = query.lastError().text();
        return false;
    }

    query.bindValue(
        QStringLiteral(":start_time"),
        startTime.toString(
            QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")));

    query.bindValue(
        QStringLiteral(":end_time"),
        endTime.toString(
            QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")));

    if (channelNo > 0) {
        query.bindValue(
            QStringLiteral(":channel_no"),
            channelNo);
    }

    if (!query.exec()) {
        errorMessage = query.lastError().text();
        return false;
    }

    while (query.next()) {
        SnapshotInfo snapshot;

        snapshot.id = query.value(QStringLiteral("id")).toULongLong();
        snapshot.channelNo = query.value(QStringLiteral("channel_no")).toInt();
        snapshot.snapshotType = query.value(QStringLiteral("snapshot_type")).toString();
        snapshot.createTime = query.value(QStringLiteral("create_time")).toDateTime();
        snapshot.frameTime = query.value(QStringLiteral("frame_time")).toDateTime();
        snapshot.snapshotPath = query.value(QStringLiteral("snapshot_path")).toString();
        snapshot.videoId = query.value(QStringLiteral("video_id")).toULongLong();
        snapshot.eventId = query.value(QStringLiteral("event_id")).toULongLong();
        snapshot.offsetMs = query.value(QStringLiteral("offset_ms")).isNull()
            ? -1 : query.value(QStringLiteral("offset_ms")).toLongLong();
        snapshot.videoPath = query.value(QStringLiteral("video_path")).toString();

        snapshots.append(snapshot);
    }

    return true;
}

bool SnapshotModel::findRelatedVideo(SnapshotInfo &snapshot, QString &errorMessage)
{
    // 根据通道和图片时间查找最匹配的录像及其播放偏移。
    if (currentUsername.isEmpty())
        return true;
    if (!openDatabase(errorMessage))
        return false;

    QSqlQuery query(database);
    query.prepare(QStringLiteral(
        "SELECT id, start_time, video_path FROM video_record "
        "WHERE user_id = (SELECT id FROM admin_user WHERE username = :username) "
        "AND channel_no = :channel_no AND start_time <= :frame_time "
        "AND (end_time IS NULL OR end_time >= :frame_time) "
        "AND record_status IN (0, 1) ORDER BY start_time DESC, id DESC LIMIT 1"));
    query.bindValue(QStringLiteral(":channel_no"), snapshot.channelNo);
    query.bindValue(QStringLiteral(":username"), currentUsername);
    query.bindValue(QStringLiteral(":frame_time"),
                    snapshot.frameTime.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")));

    if (!query.exec()) {
        errorMessage = query.lastError().text();
        return false;
    }

    if (query.next()) {
        snapshot.videoId = query.value(QStringLiteral("id")).toULongLong();
        snapshot.videoPath = query.value(QStringLiteral("video_path")).toString();
        const QDateTime startTime = query.value(QStringLiteral("start_time")).toDateTime();
        snapshot.offsetMs = qMax(qint64(0), startTime.msecsTo(snapshot.frameTime));
    }
    return true;
}

bool SnapshotModel::deleteSnapshot(
    qulonglong snapshotId,
    QString &errorMessage)
{
    // 删除指定图片的数据库记录。
    if (!openDatabase(errorMessage))
        return false;

    QSqlQuery query(database);

    query.prepare(QStringLiteral(
        "DELETE FROM snapshot WHERE id = :id"));

    query.bindValue(
        QStringLiteral(":id"),
        snapshotId);

    if (!query.exec()) {
        errorMessage = query.lastError().text();
        return false;
    }

    return true;
}
