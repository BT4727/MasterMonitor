#ifndef RECORDMANAGER_H
#define RECORDMANAGER_H

#include <QObject>
#include <QThread>
#include <QString>

#include "recordtask.h"
#include "monitortask.h"

class RecordManager : public QObject
{
    Q_OBJECT
public:
    explicit RecordManager(QObject *parent = nullptr);
    ~RecordManager() override;

    void startRecord(
        int captureThreadCount,
        const QString &outputDirectory, const QString &username);

    void connectCaptureTask(
        int channelIndex,
        MonitorTask *captureTask);

    void setOutputDirectory(
        const QString &outputDirectory);

    void stopRecord();
    void setDetectionService(DetectionService *service);

signals:
    void outputDirectoryChanged(const QString &directory);
private:
    void initConnect(int threadIndex);
    int getRecordThreadIndex(int channelIndex) const;

    static constexpr int MaxCaptureThreadCount = 50;
    static constexpr int MaxRecordThreadCount = 13;

    bool running;
    DetectionService *detectionService = nullptr;
    int captureThreadCount;
    int recordThreadCount;

    QThread *recordThreads[MaxRecordThreadCount];
    RecordTask *recordTasks[MaxRecordThreadCount];

};

#endif // RECORDMANAGER_H
