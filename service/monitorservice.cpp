#include "monitorservice.h"
#include <QDir>
#include <QFile>
MonitorService::MonitorService(QObject *parent)
    : QObject{parent}
{
    running = false;

    qRegisterMetaType<cv::Mat>("cv::Mat");

    for (int index = 0; index < ChannelCount; index++) {
        captureThreads[index] = nullptr;
        processThreads[index] = nullptr;

        captureTasks[index] = nullptr;
        processTasks[index] = nullptr;

        frameSemaphores[index] = nullptr;
    }

    recordManager = new RecordManager(this);
}

MonitorService::~MonitorService()
{
    stopMonitor();
}

void MonitorService::startMonitor(const QString &videoDirectory)
{
    if (running) {
        return;
    }

    QDir directory(videoDirectory);

    QStringList videoFiles =
        directory.entryList(
            QStringList{
                "*.mp4",
                "*.avi",
                "*.mkv",
                "*.mov"
            },
            QDir::Files,
            QDir::Name);

    // -1表示只使用占位视频，不尝试打开摄像头。
    int cameraIndexes[ChannelCount] = {0, -1, -1, -1};
    for (int index = 0; index < ChannelCount; index++) {
        QString videoPath = directory.filePath(
            QString("channel%1.mp4").arg(index + 1));

        // 缺少同编号文件时，从现有视频中选择。
        if (!QFile::exists(videoPath)) {
            if (videoFiles.isEmpty()) {
                videoPath.clear();
            } else {
                int fileIndex =
                    index % static_cast<int>(videoFiles.size());

                videoPath = directory.filePath(
                    videoFiles.at(fileIndex));
            }
        }

        frameSemaphores[index] = new QSemaphore(1);

        captureThreads[index] = new QThread(this);
        processThreads[index] = new QThread(this);

        captureThreads[index]->setObjectName(
            QString("captureThread%1").arg(index + 1));

        processThreads[index]->setObjectName(
            QString("processThread%1").arg(index + 1));

        // 移入线程的任务不能设置GUI线程中的parent。
        captureTasks[index] = new MonitorTask(
            index,
            cameraIndexes[index],
            videoPath,
            frameSemaphores[index]);

        processTasks[index] = new ProcessTask(
            frameSemaphores[index]);

        captureTasks[index]->moveToThread(
            captureThreads[index]);

        processTasks[index]->moveToThread(
            processThreads[index]);

        initConnect(index);

    }

    running = true;

    for (int index = 0; index < ChannelCount; index++) {
        processThreads[index]->start();
        captureThreads[index]->start();
    }
}

void MonitorService::initConnect(int channelIndex)
{
    connect(
        captureThreads[channelIndex],
        &QThread::started,
        captureTasks[channelIndex],
        &MonitorTask::startMonitor);

    connect(
        captureTasks[channelIndex],
        &MonitorTask::sendFrame,
        processTasks[channelIndex],
        &ProcessTask::processFrame,
        Qt::QueuedConnection);

    connect(
        captureThreads[channelIndex],
        &QThread::finished,
        captureTasks[channelIndex],
        &QObject::deleteLater);

    connect(
        processThreads[channelIndex],
        &QThread::finished,
        processTasks[channelIndex],
        &QObject::deleteLater);
}

void MonitorService::stopMonitor()
{
    if (!running) {
        return;
    }

    running = false;

    // 先通知全部采集线程停止。
    for (int index = 0; index < ChannelCount; index++) {
        captureThreads[index]->requestInterruption();
        captureThreads[index]->quit();
    }

    // 等采集线程释放设备，确保不会继续发送帧。
    for (int index = 0; index < ChannelCount; index++) {
        captureThreads[index]->wait();
    }

    // 采集线程已不再发送帧，可以关闭录像并更新记录
    stopRecord();

    // 然后停止处理线程。
    for (int index = 0; index < ChannelCount; index++) {
        processThreads[index]->quit();
    }

    for (int index = 0; index < ChannelCount; index++) {
        processThreads[index]->wait();
    }

    // 任务对象已由finished -> deleteLater清理。
    // 信号量必须等两个线程都结束后再删除。
    for (int index = 0; index < ChannelCount; index++) {
        delete captureThreads[index];
        delete processThreads[index];
        delete frameSemaphores[index];

        captureThreads[index] = nullptr;
        processThreads[index] = nullptr;

        captureTasks[index] = nullptr;
        processTasks[index] = nullptr;

        frameSemaphores[index] = nullptr;
    }
}

void MonitorService::setRecordDirectory(const QString &recordDirectory)
{
    if (recordManager != nullptr) {
        recordManager->setOutputDirectory(
            recordDirectory);
    }
}

void MonitorService::startRecord(const QString &username, const QString &recordDirectory)
{
    if (!running || username.trimmed().isEmpty())
        return;
    if (recording && recordingUsername == username) {
        setRecordDirectory(recordDirectory);
        return;
    }
    stopRecord();
    recordManager->startRecord(ChannelCount, recordDirectory, username);
    for (int index = 0; index < ChannelCount; ++index)
        recordManager->connectCaptureTask(index, captureTasks[index]);
    recordingUsername = username;
    recording = true;
    for (int index = 0; index < ChannelCount; ++index)
        captureTasks[index]->setRecordEnabled(true);
}

void MonitorService::stopRecord()
{
    if (!recording)
        return;
    // 禁止新帧并等待阻塞发送完成，之后才能销毁录制线程。
    if (running) {
        for (int index = 0; index < ChannelCount; ++index)
            captureTasks[index]->setRecordEnabled(false);
    }
    recordManager->stopRecord();
    recording = false;
    recordingUsername.clear();
}


QImage MonitorService::getChannelImage(
    int channelIndex)
{
    if (!running
        || channelIndex < 0
        || channelIndex >= ChannelCount) {
        return QImage();
    }

    return processTasks[channelIndex]->getImage();
}
