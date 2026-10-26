#ifndef OPERATIONLOGMODEL_H
#define OPERATIONLOGMODEL_H

#include <QDateTime>
#include <QSqlDatabase>
#include <QString>
#include <QVector>

struct OperationLogInfo
{
    qulonglong id = 0;
    QString username;
    int channelNo = 0;
    QDateTime createTime;
    QString operationType;
    QString targetType;
    qulonglong targetId = 0;
    bool success = false;
    QString description;
};

class OperationLogModel
{
public:
    OperationLogModel();
    ~OperationLogModel();

    bool insertLog(const QString &username, int channelNo, const QString &operationType,
                   const QString &targetType, qulonglong targetId, bool success,
                   const QString &description, QString &errorMessage);
    bool searchPage(const QDateTime &startTime, const QDateTime &endTime, int channelNo,
                    int page, int pageSize, QVector<OperationLogInfo> &logs,
                    int &totalCount, QString &errorMessage);
    bool deleteByIds(const QVector<qulonglong> &logIds, QString &errorMessage);
    bool deleteBefore(const QDateTime &time, QString &errorMessage);
    bool deleteRange(const QDateTime &startTime, const QDateTime &endTime,
                     int channelNo, QString &errorMessage);

private:
    bool openDatabase(QString &errorMessage);
    bool ensureChannelColumn(QString &errorMessage);
    QVariant findUserId(const QString &username, QString &errorMessage);

    QString connectionName;
    QSqlDatabase database;
};

#endif // OPERATIONLOGMODEL_H
