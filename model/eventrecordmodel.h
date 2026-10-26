#ifndef EVENTRECORDMODEL_H
#define EVENTRECORDMODEL_H
#include <QDateTime>
#include <QSqlDatabase>
#include <QString>
#include <QVariantMap>
#include <QVector>

struct EventLogInfo
{
    qulonglong id = 0;
    QDateTime createTime;
    int channelNo = 0;
    QString eventType;
    int imageCount = 0;
    qulonglong videoId = 0;
    QString videoPath;
};

class EventRecordModel
{
public:
    EventRecordModel();
    ~EventRecordModel();
    bool insertDiskLowEvent(int remainingPercent,const QString &storagePath,QString &errorMessage);
    bool searchPage(const QDateTime &startTime, const QDateTime &endTime, int channelNo,
                    int page, int pageSize, QVector<EventLogInfo> &events,
                    int &totalCount, QString &errorMessage);
    bool deleteByIds(const QVector<qulonglong> &eventIds, QString &errorMessage);
    bool deleteBefore(const QDateTime &time, QString &errorMessage);
    bool deleteRange(const QDateTime &startTime, const QDateTime &endTime,
                     int channelNo, QString &errorMessage);
private:
    bool openDatabase(QString &errorMessage);
    bool deleteMatchingEvents(const QString &condition, const QVariantMap &bindings,
                              QString &errorMessage);

    QString connectionName;
    QSqlDatabase database;
};

#endif // EVENTRECORDMODEL_H
