#ifndef SNAPSHOTMODEL_H
#define SNAPSHOTMODEL_H

#include <QDateTime>
#include <QSqlDatabase>
#include <QString>
#include <QVector>

struct SnapshotInfo
{
    qulonglong id = 0;
    int channelNo = 0;
    QString snapshotType;
    QDateTime createTime;
    QDateTime frameTime;
    QString snapshotPath;
    qulonglong videoId = 0;
    qulonglong eventId = 0;
    qint64 offsetMs = -1;
    QString videoPath;
};

class SnapshotModel
{
public:
    SnapshotModel();
    ~SnapshotModel();
    void setCurrentUser(const QString &username);

    bool insertSnapshot(
        int channelNo,
        const QString &snapshotType,
        const QDateTime &createTime,
        const QDateTime &frameTime,
        const QString &snapshotPath,
        qulonglong videoId,
        qulonglong eventId,
        qint64 offsetMs,
        qulonglong &snapshotId,
        QString &errorMessage);

    bool searchSnapshots(
        const QDateTime &startTime,
        const QDateTime &endTime,
        int channelNo,
        QVector<SnapshotInfo> &snapshots,
        QString &errorMessage);

    bool deleteSnapshot(
        qulonglong snapshotId,
        QString &errorMessage);

    bool findRelatedVideo(SnapshotInfo &snapshot, QString &errorMessage);

private:
    bool openDatabase(QString &errorMessage);

    QString connectionName;
    QString currentUsername;
    QSqlDatabase database;
};

#endif
