#ifndef PLAYBACKSERVICE_H
#define PLAYBACKSERVICE_H
#include "../model/videorecordmodel.h"

#include <QDate>
#include <QTime>
#include <QVector>

class PlaybackService
{
public:
    PlaybackService();
    void setCurrentUser(const QString &username);
    bool findRecordById(qulonglong recordId, VideoRecordInfo &record, QString &errorMessage);
    bool searchPage(const QDateTime &startTime, const QDateTime &endTime, int channelNo,
                    int page, int pageSize, QVector<VideoRecordInfo> &records,
                    int &totalCount, QString &errorMessage,
                    const QString &recordType = QStringLiteral("normal"));
    bool searchRecords(
        const QDate &date,
        const QTime &startTime,
        const QTime &endTime,
        int channelNo,
        const QString &recordType,
        QVector<VideoRecordInfo> &records,
        QString &errorMessage);
private:
    VideoRecordModel videoRecordModel;
};

#endif // PLAYBACKSERVICE_H
