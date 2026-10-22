#ifndef SNAPSHOTSERVICE_H
#define SNAPSHOTSERVICE_H

#include "../model/snapshotmodel.h"

#include <QImage>
#include <QString>
#include <QVector>

class SnapshotService
{
public:
    void setCurrentUser(const QString &username);
    bool saveManualSnapshot(
        const QImage &image,
        int channelNo,
        const QString &filePath,
        const QDateTime &frameTime,
        qulonglong videoId,
        qint64 offsetMs,
        SnapshotInfo &snapshot,
        QString &errorMessage);

    // 移动侦测接入后传入事件对应的三帧；此处不负责检测。
    bool saveEventSnapshots(
        const QVector<QImage> &images,
        const QVector<QDateTime> &frameTimes,
        int channelNo,
        qulonglong eventId,
        const QString &directory,
        QVector<SnapshotInfo> &snapshots,
        QString &errorMessage);

    bool searchSnapshots(
        const QDateTime &startTime,
        const QDateTime &endTime,
        int channelNo,
        QVector<SnapshotInfo> &snapshots,
        QString &errorMessage);

    bool deleteSnapshot(
        const SnapshotInfo &snapshot,
        QString &errorMessage);

    bool exportSnapshots(
        const QVector<SnapshotInfo> &snapshots,
        const QString &targetDirectory,
        QString &errorMessage);

private:
    SnapshotModel snapshotModel;
};

#endif
