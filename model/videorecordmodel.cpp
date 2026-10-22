#include "videorecordmodel.h"
#include "../util/dbconn.h"
#include <QSqlError>
#include <QSqlQuery>
#include <QThread>
#include <QVariant>

#include <QtSql/QSqlDatabase>
VideoRecordModel::VideoRecordModel()
{
    connectionName = QStringLiteral("videoRecord_%1")
    .arg(reinterpret_cast<quintptr>(this), 0, 16);
}

VideoRecordModel::~VideoRecordModel()
{
    if (threadDatabase.isValid()) {
        QString currentConnectionName =
            threadDatabase.connectionName();

        threadDatabase.close();
        threadDatabase = QSqlDatabase();

        QSqlDatabase::removeDatabase(
            currentConnectionName);
    }
}

void VideoRecordModel::setCurrentUser(const QString &username)
{
    currentUsername = username.trimmed();
}

bool VideoRecordModel::openThreadDatabase(
    QString &errorMessage)
{
    if (threadDatabase.isValid()
        && threadDatabase.isOpen()) {
        return true;
    }

    if (!QSqlDatabase::contains(connectionName)) {
        QSqlDatabase baseDatabase =
            QSqlDatabase::addDatabase(
                QStringLiteral("QMYSQL"),
                connectionName);
        DBConn::configureDatabase(baseDatabase);

        if (!baseDatabase.isValid()) {
            errorMessage =
                QStringLiteral("默认数据库连接不存在");
            return false;
        }

        threadDatabase = baseDatabase;
    } else {
        threadDatabase =
            QSqlDatabase::database(
                connectionName,
                false);
    }

    if (!threadDatabase.isOpen()
        && !threadDatabase.open()) {
        errorMessage =
            threadDatabase.lastError().text();
        return false;
    }

    return true;
}

bool VideoRecordModel::insertRecord(
    int channelNo,
    const QString &recordType,
    const QDateTime &startTime,
    const QString &videoPath,
    qulonglong &recordId,
    QString &errorMessage)
{
    recordId = 0;

    if (currentUsername.isEmpty()) {
        errorMessage = QStringLiteral("未登录，不能录制录像");
        return false;
    }

    if (!openThreadDatabase(errorMessage)) {
        return false;
    }

    QSqlQuery query(threadDatabase);

    query.prepare(QStringLiteral(
        "INSERT INTO video_record "
        "(user_id, channel_no, record_type, event_id, start_time, "
        "end_time, video_path, video_size, record_status) "
        "SELECT id, :channel_no, :record_type, NULL, "
        ":start_time, NULL, :video_path, 0, 0 FROM admin_user "
        "WHERE username = :username AND status = 1"));
    query.bindValue(QStringLiteral(":username"), currentUsername);

    query.bindValue(
        QStringLiteral(":channel_no"), channelNo);
    query.bindValue(
        QStringLiteral(":record_type"), recordType);
    query.bindValue(
        QStringLiteral(":start_time"),
        startTime.toString(
            QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")));
    query.bindValue(
        QStringLiteral(":video_path"), videoPath);

    if (!query.exec()) {
        errorMessage = query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() != 1) {
        errorMessage = QStringLiteral("录制用户不存在或已禁用");
        return false;
    }
    recordId = query.lastInsertId().toULongLong();
    return recordId != 0;
}

bool VideoRecordModel::finishRecord(
    qulonglong recordId,
    const QDateTime &endTime,
    qulonglong videoSize,
    int recordStatus,
    QString &errorMessage)
{
    if (!openThreadDatabase(errorMessage)) {
        return false;
    }

    QSqlQuery query(threadDatabase);

    query.prepare(QStringLiteral(
        "UPDATE video_record "
        "SET end_time = :end_time, "
        "video_size = :video_size, "
        "record_status = :record_status "
        "WHERE id = :id AND user_id = "
        "(SELECT id FROM admin_user WHERE username = :username)"));
    query.bindValue(QStringLiteral(":username"), currentUsername);

    query.bindValue(
        QStringLiteral(":end_time"),
        endTime.toString(
            QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")));
    query.bindValue(
        QStringLiteral(":video_size"), videoSize);
    query.bindValue(
        QStringLiteral(":record_status"), recordStatus);
    query.bindValue(
        QStringLiteral(":id"), recordId);

    if (!query.exec()) {
        errorMessage = query.lastError().text();
        return false;
    }

    return true;
}

bool VideoRecordModel::searchRecords(const QDateTime &startTime, const QDateTime &endTime, int channelNo, const QString &recordType, QVector<VideoRecordInfo> &records, QString &errorMessage)
{
    records.clear();

    if (currentUsername.isEmpty()) {
        errorMessage = QStringLiteral("请先登录");
        return false;
    }

    if (!openThreadDatabase(errorMessage))
        return false;

    QString sql = QStringLiteral(
        "SELECT id, channel_no, record_type, start_time, "
        "end_time, video_path, video_size, record_status "
        "FROM video_record "
        "WHERE user_id = (SELECT id FROM admin_user WHERE username = :username) "
        "AND start_time >= :start_time AND start_time < :end_time ");

    if (channelNo > 0)
        sql += QStringLiteral("AND channel_no = :channel_no ");

    if (!recordType.isEmpty())
        sql += QStringLiteral("AND record_type = :record_type ");

    sql += QStringLiteral("ORDER BY start_time DESC, id DESC");

    QSqlQuery query(threadDatabase);

    if (!query.prepare(sql)) {
        errorMessage = query.lastError().text();
        return false;
    }
    query.bindValue(QStringLiteral(":username"), currentUsername);

    query.bindValue(
        QStringLiteral(":start_time"),
        startTime.toString(
            QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")));

    query.bindValue(
        QStringLiteral(":end_time"),
        endTime.toString(
            QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")));

    if (channelNo > 0)
        query.bindValue(QStringLiteral(":channel_no"), channelNo);

    if (!recordType.isEmpty())
        query.bindValue(QStringLiteral(":record_type"), recordType);

    if (!query.exec()) {
        errorMessage = query.lastError().text();
        return false;
    }

    while (query.next()) {
        VideoRecordInfo record;

        record.id = query.value(QStringLiteral("id")).toULongLong();
        record.channelNo = query.value(QStringLiteral("channel_no")).toInt();
        record.recordType = query.value(QStringLiteral("record_type")).toString();
        record.startTime = query.value(QStringLiteral("start_time")).toDateTime();

        if (!query.value(QStringLiteral("end_time")).isNull())
            record.endTime = query.value(QStringLiteral("end_time")).toDateTime();

        record.videoPath = query.value(QStringLiteral("video_path")).toString();
        record.videoSize = query.value(QStringLiteral("video_size")).toULongLong();
        record.recordStatus = query.value(QStringLiteral("record_status")).toInt();

        records.append(record);
    }

    return true;
}

bool VideoRecordModel::findRecordById(
    qulonglong recordId, VideoRecordInfo &record, QString &errorMessage)
{
    record = VideoRecordInfo();
    if (currentUsername.isEmpty() || !openThreadDatabase(errorMessage)) {
        if (errorMessage.isEmpty())
            errorMessage = QStringLiteral("请先登录");
        return false;
    }
    QSqlQuery query(threadDatabase);
    query.prepare(QStringLiteral(
        "SELECT * FROM video_record WHERE id = :id "
        "AND user_id = (SELECT id FROM admin_user WHERE username = :username)"));
    query.bindValue(QStringLiteral(":id"), recordId);
    query.bindValue(QStringLiteral(":username"), currentUsername);
    if (!query.exec()) {
        errorMessage = query.lastError().text();
        return false;
    }
    if (!query.next()) {
        errorMessage = QStringLiteral("录像不存在或不属于当前用户");
        return false;
    }
    record.id = query.value(QStringLiteral("id")).toULongLong();
    record.channelNo = query.value(QStringLiteral("channel_no")).toInt();
    record.recordType = query.value(QStringLiteral("record_type")).toString();
    record.startTime = query.value(QStringLiteral("start_time")).toDateTime();
    record.endTime = query.value(QStringLiteral("end_time")).toDateTime();
    record.videoPath = query.value(QStringLiteral("video_path")).toString();
    record.videoSize = query.value(QStringLiteral("video_size")).toULongLong();
    record.recordStatus = query.value(QStringLiteral("record_status")).toInt();
    return true;
}

bool VideoRecordModel::searchPage(const QDateTime &startTime, const QDateTime &endTime, int channelNo,
                                int page, int pageSize, QVector<VideoRecordInfo> &records,
                                int &totalCount, QString &errorMessage)
{
    records.clear();
    totalCount = 0;
    if (currentUsername.isEmpty() || !openThreadDatabase(errorMessage)) {
        if (errorMessage.isEmpty())
            errorMessage = QStringLiteral("请先登录");
        return false;
    }
    QString condition = QStringLiteral(
        " FROM video_record WHERE record_type = 'normal' AND record_status = 1 "
        "AND user_id = (SELECT id FROM admin_user WHERE username = :username) ");
    if (startTime.isValid())
        condition += QStringLiteral("AND start_time >= :start_time ");
    if (endTime.isValid())
        condition += QStringLiteral("AND start_time < :end_time ");
    if (channelNo > 0)
        condition += QStringLiteral("AND channel_no = :channel_no ");
    QSqlQuery countQuery(threadDatabase);
    QSqlQuery query(threadDatabase);
    countQuery.prepare(QStringLiteral("SELECT COUNT(*)") + condition);
    query.prepare(QStringLiteral("SELECT *") + condition
        + QStringLiteral("ORDER BY start_time DESC, id DESC LIMIT :limit OFFSET :offset"));
    QSqlQuery *queries[] = {&countQuery, &query};
    for (QSqlQuery *currentQuery : queries) {
        currentQuery->bindValue(QStringLiteral(":username"), currentUsername);
        if (startTime.isValid())
            currentQuery->bindValue(QStringLiteral(":start_time"), startTime.toString("yyyy-MM-dd HH:mm:ss.zzz"));
        if (endTime.isValid())
            currentQuery->bindValue(QStringLiteral(":end_time"), endTime.toString("yyyy-MM-dd HH:mm:ss.zzz"));
        if (channelNo > 0)
            currentQuery->bindValue(QStringLiteral(":channel_no"), channelNo);
    }
    if (!countQuery.exec() || !countQuery.next()) {
        errorMessage = countQuery.lastError().text();
        return false;
    }
    totalCount = countQuery.value(0).toInt();
    const int capacity = qMax(1, pageSize);
    const int totalPages = qMax(1, (totalCount + capacity - 1) / capacity);
    const int currentPage = qBound(1, page, totalPages);
    query.bindValue(QStringLiteral(":limit"), capacity);
    query.bindValue(QStringLiteral(":offset"), (currentPage - 1) * capacity);
    if (!query.exec()) {
        errorMessage = query.lastError().text();
        return false;
    }
    while (query.next()) {
        VideoRecordInfo record;
        record.id = query.value(QStringLiteral("id")).toULongLong();
        record.channelNo = query.value(QStringLiteral("channel_no")).toInt();
        record.recordType = query.value(QStringLiteral("record_type")).toString();
        record.startTime = query.value(QStringLiteral("start_time")).toDateTime();
        record.endTime = query.value(QStringLiteral("end_time")).toDateTime();
        record.videoPath = query.value(QStringLiteral("video_path")).toString();
        record.videoSize = query.value(QStringLiteral("video_size")).toULongLong();
        record.recordStatus = query.value(QStringLiteral("record_status")).toInt();
        records.append(record);
    }
    return true;
}
