#ifndef LOGSERVICE_H
#define LOGSERVICE_H

#include "../model/eventrecordmodel.h"
#include "../model/operationlogmodel.h"

class LogService
{
public:
    bool recordOperation(const QString &username, int channelNo,
                         const QString &operationType, const QString &targetType,
                         qulonglong targetId, bool success, const QString &description);

    bool searchOperationLogs(const QDateTime &startTime, const QDateTime &endTime,
                             int channelNo, int page, int pageSize,
                             QVector<OperationLogInfo> &logs, int &totalCount,
                             QString &errorMessage);
    bool searchEventLogs(const QDateTime &startTime, const QDateTime &endTime,
                         int channelNo, int page, int pageSize,
                         QVector<EventLogInfo> &events, int &totalCount,
                         QString &errorMessage);

    bool deleteOperationLogs(const QVector<qulonglong> &ids, QString &errorMessage);
    bool deleteEventLogs(const QVector<qulonglong> &ids, QString &errorMessage);
    bool deleteOperationLogsBefore(const QDateTime &time, QString &errorMessage);
    bool deleteEventLogsBefore(const QDateTime &time, QString &errorMessage);
    bool deleteOperationLogsRange(const QDateTime &startTime, const QDateTime &endTime,
                                  int channelNo, QString &errorMessage);
    bool deleteEventLogsRange(const QDateTime &startTime, const QDateTime &endTime,
                              int channelNo, QString &errorMessage);

private:
    OperationLogModel operationLogModel;
    EventRecordModel eventRecordModel;
};

#endif // LOGSERVICE_H
