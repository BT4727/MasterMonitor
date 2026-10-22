#ifndef VIDEORECORDMODEL_H
#define VIDEORECORDMODEL_H

#include <QDateTime>
#include <QSqlDatabase>
#include <QString>
#include <QVector>

struct VideoRecordInfo
{
    qulonglong id = 0;
    int channelNo = 0;
    QString recordType;
    QDateTime startTime;
    QDateTime endTime;
    QString videoPath;
    qulonglong videoSize = 0;
    int recordStatus = 0;
};

class VideoRecordModel
{
public:
    VideoRecordModel();
    ~VideoRecordModel();
    void setCurrentUser(const QString &username);
    bool findRecordById(qulonglong recordId, VideoRecordInfo &record, QString &errorMessage);
    bool searchPage(const QDateTime &startTime, const QDateTime &endTime, int channelNo,
                    int page, int pageSize, QVector<VideoRecordInfo> &records,
                    int &totalCount, QString &errorMessage);

    bool insertRecord(
        int channelNo,
        const QString &recordType,
        const QDateTime &startTime,
        const QString &videoPath,
        qulonglong &recordId,
        QString &errorMessage);

    bool finishRecord(
        qulonglong recordId,
        const QDateTime &endTime,
        qulonglong videoSize,
        int recordStatus,
        QString &errorMessage);

    bool searchRecords(
        const QDateTime &startTime,
        const QDateTime &endTime,
        int channelNo,
        const QString &recordType,
        QVector<VideoRecordInfo> &records,
        QString &errorMessage);

private:
    bool openThreadDatabase(QString &errorMessage);

    QString connectionName;
    QString currentUsername;
    QSqlDatabase threadDatabase;
};

#endif // VIDEORECORDMODEL_H
