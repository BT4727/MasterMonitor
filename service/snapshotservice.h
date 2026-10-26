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

    bool saveEventSnapshot(const QImage &image, int channelNo, qulonglong eventId,
                           qulonglong videoId, const QDateTime &eventTime,
                           const QDateTime &frameTime, qint64 offsetMs, int sequence,
                           const QString &directory, QString &errorMessage);

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
