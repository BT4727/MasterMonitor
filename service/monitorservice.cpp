#include "monitorservice.h"
#include <QDir>
#include <QFile>
MonitorService::MonitorService(QObject *parent)
    : QObject{parent}
{
    // 初始化每通道线程资源，并建立录像与检测服务之间的协作关系。
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
    detectionService = new DetectionService(this);
    recordManager->setDetectionService(detectionService);
}

MonitorService::~MonitorService()
{
    // 确保对象销毁时所有采集、处理和录像线程均已停止。
    stopMonitor();
}

void MonitorService::startMonitor(const QString &videoDirectory)
{
    // 为各通道创建采集/处理线程，并连接占位视频与检测服务。
    if (running) {
        return;
    }

    QDir directory(videoDirectory);
    const QString modelPath = QStringLiteral(
        "E:/learn/ClassThree/yolov11-yolov8-dnn-Classify-Detect-Obb-Pose-Seg-main/"
        "data/model/yolo11n.onnx");
    detectionService->start(ChannelCount, modelPath);

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

        captureTasks[index]->setDetectionService(detectionService);
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
    // 连接指定通道的线程启动、帧处理和任务对象清理信号。
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
    // 按采集、录像、检测、处理的依赖顺序关闭通道资源。
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

    detectionService->stop();
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
    // 将新的录像输出目录转交给录像管理器。
    if (recordManager != nullptr) {
        recordManager->setOutputDirectory(
            recordDirectory);
    }
}

void MonitorService::startRecord(const QString &username, const QString &recordDirectory)
{
    // 用户登录后按通道启动录像，并将采集任务连接到负载均衡录像线程。
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
    // 停止接收新录像帧，排空录像任务并重置当前用户状态。
    if (!recording)
        return;
    // 禁止新帧提交，再让录像线程排空缓存并关闭当前片段。
    if (running) {
        for (int index = 0; index < ChannelCount; ++index)
            captureTasks[index]->setRecordEnabled(false);
    }
    recordManager->stopRecord();
    recording = false;
    recordingUsername.clear();
    detectionService->resetRecordingState();
}

void MonitorService::setDetectionPreview(
    bool singleChannel, int channelIndex, bool detectionEnabled)
{
    // 将当前预览模式和移动侦测开关同步到检测服务。
    detectionService->setPreview(singleChannel, channelIndex, detectionEnabled);
}


QImage MonitorService::getChannelImage(
    int channelIndex)
{
    // 返回指定通道最近处理完成的预览图像。
    if (!running
        || channelIndex < 0
        || channelIndex >= ChannelCount) {
        return QImage();
    }

    return processTasks[channelIndex]->getImage();
}
