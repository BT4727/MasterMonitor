#include "logservice.h"

#include <QDebug>

bool LogService::recordOperation(const QString &username, int channelNo,
                                 const QString &operationType, const QString &targetType,
                                 qulonglong targetId, bool success, const QString &description)
{
    // 写入一次操作结果，并在数据库失败时输出诊断信息。
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
    // 分页查询操作日志。
    return operationLogModel.searchPage(
        startTime, endTime, channelNo, page, pageSize, logs, totalCount, errorMessage);
}

bool LogService::searchEventLogs(const QDateTime &startTime, const QDateTime &endTime,
                                 int channelNo, int page, int pageSize,
                                 QVector<EventLogInfo> &events, int &totalCount,
                                 QString &errorMessage)
{
    // 分页查询异常事件记录。
    return eventRecordModel.searchPage(
        startTime, endTime, channelNo, page, pageSize, events, totalCount, errorMessage);
}

bool LogService::deleteOperationLogs(const QVector<qulonglong> &ids, QString &errorMessage)
{
    // 按编号删除指定操作日志。
    return operationLogModel.deleteByIds(ids, errorMessage);
}

bool LogService::deleteEventLogs(const QVector<qulonglong> &ids, QString &errorMessage)
{
    // 按编号删除指定异常事件。
    return eventRecordModel.deleteByIds(ids, errorMessage);
}

bool LogService::deleteOperationLogsBefore(const QDateTime &time, QString &errorMessage)
{
    // 删除指定时间之前的操作日志。
    return operationLogModel.deleteBefore(time, errorMessage);
}

bool LogService::deleteEventLogsBefore(const QDateTime &time, QString &errorMessage)
{
    // 删除指定时间之前的异常事件。
    return eventRecordModel.deleteBefore(time, errorMessage);
}

bool LogService::deleteOperationLogsRange(const QDateTime &startTime,
                                          const QDateTime &endTime, int channelNo,
                                          QString &errorMessage)
{
    // 按时间范围和可选通道删除操作日志。
    return operationLogModel.deleteRange(startTime, endTime, channelNo, errorMessage);
}

bool LogService::deleteEventLogsRange(const QDateTime &startTime,
                                      const QDateTime &endTime, int channelNo,
                                      QString &errorMessage)
{
    // 按时间范围和可选通道删除异常事件。
    return eventRecordModel.deleteRange(startTime, endTime, channelNo, errorMessage);
}
