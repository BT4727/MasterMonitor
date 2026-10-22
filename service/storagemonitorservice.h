#ifndef STORAGEMONITORSERVICE_H
#define STORAGEMONITORSERVICE_H
#include "../model/eventrecordmodel.h"

#include <QObject>
#include <QTimer>
class StorageMonitorService : public QObject
{
    Q_OBJECT
public:
    explicit StorageMonitorService(QObject *parent = nullptr);

    void start(const QString &storagePath);
    void setStoragePath(const QString &storagePath);
    void stop();

signals:
    void storageSpaceWarning(const QString &message);

private slots:
    void checkStorageSpace();

private:
    static constexpr int CheckIntervalMs = 60000;
    static constexpr double WarningRatio = 0.10;

    QTimer checkTimer;
    QString storagePath;
    bool warningActive = false;
    EventRecordModel eventRecordModel;
};

#endif // STORAGEMONITORSERVICE_H
