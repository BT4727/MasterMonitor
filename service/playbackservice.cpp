#include "playbackservice.h"

PlaybackService::PlaybackService() {}

void PlaybackService::setCurrentUser(const QString &username)
{
    videoRecordModel.setCurrentUser(username);
}

bool PlaybackService::findRecordById(
    qulonglong recordId, VideoRecordInfo &record, QString &errorMessage)
{
    return videoRecordModel.findRecordById(recordId, record, errorMessage);
}

bool PlaybackService::searchPage(const QDateTime &startTime, const QDateTime &endTime, int channelNo,
                               int page, int pageSize, QVector<VideoRecordInfo> &records,
                               int &totalCount, QString &errorMessage, const QString &recordType)
{
    return videoRecordModel.searchPage(
        startTime, endTime, channelNo, page, pageSize, records, totalCount, errorMessage, recordType);
}

bool PlaybackService::searchRecords(const QDate &date, const QTime &startTime, const QTime &endTime, int channelNo, const QString &recordType, QVector<VideoRecordInfo> &records, QString &errorMessage)
{
    if (!date.isValid() || !startTime.isValid() || !endTime.isValid()) {
        errorMessage = QStringLiteral("查询日期或时间无效");
        return false;
    }

    const QDateTime rangeStart(date, startTime);
    QDateTime rangeEnd(date, endTime);

    if (endTime < startTime)
        rangeEnd = rangeEnd.addDays(1);
    rangeEnd = rangeEnd.addSecs(1);

    return videoRecordModel.searchRecords(
        rangeStart,
        rangeEnd,
        channelNo,
        recordType,
        records,
        errorMessage);
}
