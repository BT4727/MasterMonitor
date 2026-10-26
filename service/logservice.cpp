#include "logservice.h"

#include <QDebug>

bool LogService::recordOperation(const QString &username, int channelNo,
                                 const QString &operationType, const QString &targetType,
                                 qulonglong targetId, bool success, const QString &description)
{
    QString errorMessage;
    const bool recorded = operationLogModel.insertLog(
        username, channelNo, operationType, targetType,
        targetId, success, description, errorMessage);

    if (!recorded)
        qWarning() << "写入操作日志失败：" << errorMessage;

    return recorded;
}

bool LogService::searchOperationLogs(const QDateTime &startTime, const QDateTime &endTime,
                                     int channelNo, int page, int pageSize,
                                     QVector<OperationLogInfo> &logs, int &totalCount,
                                     QString &errorMessage)
{
    return operationLogModel.searchPage(
        startTime, endTime, channelNo, page, pageSize, logs, totalCount, errorMessage);
}

bool LogService::searchEventLogs(const QDateTime &startTime, const QDateTime &endTime,
                                 int channelNo, int page, int pageSize,
                                 QVector<EventLogInfo> &events, int &totalCount,
                                 QString &errorMessage)
{
    return eventRecordModel.searchPage(
        startTime, endTime, channelNo, page, pageSize, events, totalCount, errorMessage);
}

bool LogService::deleteOperationLogs(const QVector<qulonglong> &ids, QString &errorMessage)
{
    return operationLogModel.deleteByIds(ids, errorMessage);
}

bool LogService::deleteEventLogs(const QVector<qulonglong> &ids, QString &errorMessage)
{
    return eventRecordModel.deleteByIds(ids, errorMessage);
}

bool LogService::deleteOperationLogsBefore(const QDateTime &time, QString &errorMessage)
{
    return operationLogModel.deleteBefore(time, errorMessage);
}

bool LogService::deleteEventLogsBefore(const QDateTime &time, QString &errorMessage)
{
    return eventRecordModel.deleteBefore(time, errorMessage);
}

bool LogService::deleteOperationLogsRange(const QDateTime &startTime,
                                          const QDateTime &endTime, int channelNo,
                                          QString &errorMessage)
{
    return operationLogModel.deleteRange(startTime, endTime, channelNo, errorMessage);
}

bool LogService::deleteEventLogsRange(const QDateTime &startTime,
                                      const QDateTime &endTime, int channelNo,
                                      QString &errorMessage)
{
    return eventRecordModel.deleteRange(startTime, endTime, channelNo, errorMessage);
}
