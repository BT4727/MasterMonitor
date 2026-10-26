#ifndef MONITORSERVICE_H
#define MONITORSERVICE_H

#include <QObject>
#include <QThread>
#include <QImage>
#include <QSemaphore>
#include <QString>

#include "monitortask.h"
#include "processtask.h"
#include "recordmanager.h"
class MonitorService : public QObject
{
    Q_OBJECT
public:
    explicit MonitorService(QObject *parent = nullptr);
    ~MonitorService() override;

    void startMonitor(const QString &videoDirectory);
    void startRecord(const QString &username, const QString &recordDirectory);
    void stopRecord();
    void setRecordDirectory(const QString &recordDirectory);
    QImage getChannelImage(int channelIndex);
    void setDetectionPreview(bool singleChannel, int channelIndex,
                             bool detectionEnabled);

public slots:
    void stopMonitor();
private:
    void initConnect(int channelIndex);

    static const int ChannelCount = 4;

    bool running;
    bool recording = false;
    QString recordingUsername;

    QThread *captureThreads[ChannelCount];
    QThread *processThreads[ChannelCount];

    MonitorTask *captureTasks[ChannelCount];
    ProcessTask *processTasks[ChannelCount];

    QSemaphore *frameSemaphores[ChannelCount];
    RecordManager *recordManager;
    DetectionService *detectionService;
};

#endif // MONITORSERVICE_H
