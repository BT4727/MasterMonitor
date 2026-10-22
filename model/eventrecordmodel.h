#ifndef EVENTRECORDMODEL_H
#define EVENTRECORDMODEL_H
#include <QSqlDatabase>
#include <QString>

class EventRecordModel
{
public:
    EventRecordModel();
    ~EventRecordModel();
    bool insertDiskLowEvent(int remainingPercent,const QString &storagePath,QString &errorMessage);
private:
    bool openDatabase(QString &errorMessage);

    QString connectionName;
    QSqlDatabase database;
};

#endif // EVENTRECORDMODEL_H
