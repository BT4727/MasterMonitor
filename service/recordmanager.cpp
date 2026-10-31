#include "recordmanager.h"
#include <QMetaObject>
#include <QDebug>
RecordManager::RecordManager(QObject *parent)
    : QObject(parent)
{
    // 初始化录像线程池状态和任务指针。
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
    // 管理器销毁前停止全部录像线程。
    stopRecord();
}

void RecordManager::setDetectionService(DetectionService *service)
{
    // 将检测服务传递给后续创建的录像任务。
    detectionService = service;
}

void RecordManager::startRecord(
    int captureCount,
    const QString &outputDirectory, const QString &username)
{
    // 根据采集通道数创建录像线程池并启动各录像任务。
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
        recordTasks[index]->setDetectionService(detectionService);

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
    // 连接录像目录变更和录像线程退出后的任务清理信号。
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
    // 将指定采集通道的帧分配给对应录像线程。
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
        &RecordTask::submitFrame,
        Qt::DirectConnection);
}

void RecordManager::setOutputDirectory(
    const QString &directory)
{
    // 向运行中的录像任务异步广播输出目录变更。
    if (!running) {
        return;
    }

    emit outputDirectoryChanged(directory);
}

void RecordManager::stopRecord()
{
    // 先排空并收尾录像任务，再依次停止和回收录像线程。
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
    // 以轮询方式将通道均匀分配到录像线程。
    // 轮询分配，录制任务数量会随采集通道数扩展。
    return channelIndex % recordThreadCount;
}
