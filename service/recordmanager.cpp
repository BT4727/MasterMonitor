#include "recordmanager.h"
#include <QMetaObject>
#include <QDebug>
RecordManager::RecordManager(QObject *parent)
    : QObject(parent)
{
    running = false;
    captureThreadCount = 0;
    recordThreadCount = 0;

    for (int index = 0;
         index < MaxRecordThreadCount;
         index++) {
        recordThreads[index] = nullptr;
        recordTasks[index] = nullptr;
    }
}

RecordManager::~RecordManager()
{
    stopRecord();
}

void RecordManager::startRecord(
    int captureCount,
    const QString &outputDirectory, const QString &username)
{
    if (running
        || captureCount <= 0
        || captureCount > MaxCaptureThreadCount) {
        return;
    }

    captureThreadCount = captureCount;

    // 4路采集对应2个录制线程；
    // 50路采集对应13个录制线程。
    recordThreadCount =
        qMax((captureThreadCount + 3) / 4, 2);

    if (recordThreadCount > MaxRecordThreadCount) {
        recordThreadCount = MaxRecordThreadCount;
    }

    for (int index = 0;
         index < recordThreadCount;
         index++) {
        recordThreads[index] = new QThread(this);
        recordTasks[index] = new RecordTask(
            index,
            outputDirectory, username);

        recordThreads[index]->setObjectName(
            QStringLiteral("recordThread%1")
                .arg(index + 1));

        recordTasks[index]->moveToThread(
            recordThreads[index]);

        initConnect(index);
        recordThreads[index]->start();
    }

    running = true;
}

void RecordManager::initConnect(int threadIndex)
{
    connect(
        this,
        &RecordManager::outputDirectoryChanged,
        recordTasks[threadIndex],
        &RecordTask::setOutputDirectory,
        Qt::QueuedConnection);

    connect(
        recordThreads[threadIndex],
        &QThread::finished,
        recordTasks[threadIndex],
        &QObject::deleteLater);
}

void RecordManager::connectCaptureTask(
    int channelIndex,
    MonitorTask *captureTask)
{
    if (!running
        || captureTask == nullptr
        || channelIndex < 0
        || channelIndex >= captureThreadCount) {
        return;
    }

    const int recordThreadIndex =
        getRecordThreadIndex(channelIndex);

    connect(
        captureTask,
        &MonitorTask::sendRecordFrame,
        recordTasks[recordThreadIndex],
        &RecordTask::recordFrame,
        Qt::BlockingQueuedConnection);
}

void RecordManager::setOutputDirectory(
    const QString &directory)
{
    if (!running) {
        return;
    }

    emit outputDirectoryChanged(directory);
}

void RecordManager::stopRecord()
{
    if (!running) {
        return;
    }

    running = false;

    // 先让所有录制任务关闭文件并更新最后一条记录。
    for (int index = 0; index < recordThreadCount; index++) {
        QMetaObject::invokeMethod(
            recordTasks[index],
            "finishAllRecords",
            Qt::BlockingQueuedConnection);
    }

    for (int index = 0; index < recordThreadCount; index++) {
        recordThreads[index]->quit();
    }

    for (int index = 0; index < recordThreadCount; index++) {
        recordThreads[index]->wait();
        delete recordThreads[index];
        recordThreads[index] = nullptr;
        recordTasks[index] = nullptr;
    }

    captureThreadCount = 0;
    recordThreadCount = 0;
}

int RecordManager::getRecordThreadIndex(
    int channelIndex) const
{
    // 轮询分配，录制任务数量会随采集通道数扩展。
    return channelIndex % recordThreadCount;
}
