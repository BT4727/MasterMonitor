#include "playbackservice.h"

void PlaybackService::setCurrentUser(const QString &username)
{
    // 设置回放查询所使用的当前用户。
    videoRecordModel.setCurrentUser(username);
}

bool PlaybackService::findRecordById(
    qulonglong recordId, VideoRecordInfo &record, QString &errorMessage)
{
    // 按录像编号读取单条录像元数据。
    return videoRecordModel.findRecordById(recordId, record, errorMessage);
}

bool PlaybackService::searchPage(const QDateTime &startTime, const QDateTime &endTime, int channelNo,
                               int page, int pageSize, QVector<VideoRecordInfo> &records,
                               int &totalCount, QString &errorMessage, const QString &recordType)
{
    // 转交分页查询条件并返回总记录数与当前页录像。
    return videoRecordModel.searchPage(
        startTime, endTime, channelNo, page, pageSize, records, totalCount, errorMessage, recordType);
}

bool PlaybackService::searchRecords(const QDate &date, const QTime &startTime, const QTime &endTime, int channelNo, const QString &recordType, QVector<VideoRecordInfo> &records, QString &errorMessage)
{
    // 将日期和时间组合为查询区间，并处理跨午夜时间范围。
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
