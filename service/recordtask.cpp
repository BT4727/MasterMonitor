#include "recordtask.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QMetaObject>
#include <QMutexLocker>

RecordTask::RecordTask(int recordThreadIndex, const QString &outputDirectory,
                       const QString &username, QObject *parent) : QObject(parent)
{
    // 初始化录像线程归属、用户数据模型和逐通道触发状态。
    this->recordThreadIndex = recordThreadIndex;
    this->outputDirectory = outputDirectory;
    videoRecordModel.setCurrentUser(username);
    snapshotService.setCurrentUser(username);
    for (int index = 0; index < MaxCaptureThreadCount; ++index)
        triggerArmed[index] = true;
}

void RecordTask::setDetectionService(DetectionService *service)
{
    // 关联检测服务，以便录像状态与移动侦测状态协同。
    detectionService = service;
}

void RecordTask::submitFrame(int channelIndex, cv::Mat rawFrame, cv::Mat markedFrame,
                             int personState, bool allowTrigger)
{
    // 缓存最新通道帧，并确保同一录像线程只排入一个处理任务。
    if (channelIndex < 0 || channelIndex >= MaxCaptureThreadCount || rawFrame.empty())
        return;

    bool needSchedule = false;
    {
        QMutexLocker<QMutex> locker(&pendingFrameMutex);
        pendingRawFrames[channelIndex] = rawFrame;
        pendingMarkedFrames[channelIndex] = markedFrame;
        pendingPersonStates[channelIndex] = personState;
        pendingAllowTriggers[channelIndex] = allowTrigger;
        hasPendingFrames[channelIndex] = true;

        if (!processScheduled) {
            processScheduled = true;
            needSchedule = true;
        }
    }

    if (needSchedule) {
        QMetaObject::invokeMethod(
            this,
            "processPendingFrames",
            Qt::QueuedConnection);
    }
}

void RecordTask::processPendingFrames()
{
    // 批量取出各通道最新帧，避免处理积压帧造成录制延迟。
    for (int channelIndex = 0; channelIndex < MaxCaptureThreadCount; ++channelIndex) {
        cv::Mat rawFrame;
        cv::Mat markedFrame;
        int personState = -1;
        bool allowTrigger = false;
        bool hasFrame = false;

        {
            QMutexLocker<QMutex> locker(&pendingFrameMutex);
            if (hasPendingFrames[channelIndex]) {
                rawFrame = pendingRawFrames[channelIndex];
                markedFrame = pendingMarkedFrames[channelIndex];
                personState = pendingPersonStates[channelIndex];
                allowTrigger = pendingAllowTriggers[channelIndex];
                pendingRawFrames[channelIndex].release();
                pendingMarkedFrames[channelIndex].release();
                hasPendingFrames[channelIndex] = false;
                hasFrame = true;
            }
        }

        if (hasFrame)
            recordFrame(channelIndex, rawFrame, markedFrame, personState, allowTrigger);
    }

    bool hasMoreFrames = false;
    {
        QMutexLocker<QMutex> locker(&pendingFrameMutex);
        for (int channelIndex = 0; channelIndex < MaxCaptureThreadCount; ++channelIndex) {
            if (hasPendingFrames[channelIndex]) {
                hasMoreFrames = true;
                break;
            }
        }

        if (!hasMoreFrames)
            processScheduled = false;
    }

    if (hasMoreFrames) {
        QMetaObject::invokeMethod(
            this,
            "processPendingFrames",
            Qt::QueuedConnection);
    }
}

void RecordTask::recordFrame(int channelIndex, const cv::Mat &rawFrame,
                             const cv::Mat &markedFrame, int personState, bool allowTrigger)
{
    // 根据目标状态切换普通/移动录像，并写入当前帧和事件截图。
    if (channelIndex < 0 || channelIndex >= MaxCaptureThreadCount || rawFrame.empty())
        return;
    try {
        bool motionJustFinished = false;
        if (videoWriters[channelIndex].isOpened()
            && segmentTimers[channelIndex].elapsed() >= SegmentLengthMs) {
            motionJustFinished = motionModes[channelIndex];
            finishChannelRecord(channelIndex, 1);
        }
        if (motionModes[channelIndex]
            && detectionService != nullptr
            && !detectionService->isMotionDetectionEnabled()) {
            finishChannelRecord(channelIndex, 1);
            motionJustFinished = true;
        }
        if (!motionModes[channelIndex] && personState == 0)
            triggerArmed[channelIndex] = true;

        const bool retryAllowed = !nextRetryTimes[channelIndex].isValid()
            || QDateTime::currentDateTime() >= nextRetryTimes[channelIndex];
        if (!motionJustFinished && !motionModes[channelIndex] && retryAllowed
            && allowTrigger && personState == 1 && triggerArmed[channelIndex]
            && detectionService != nullptr && detectionService->canTrigger(channelIndex)) {
            triggerArmed[channelIndex] = false;
            finishChannelRecord(channelIndex, 1);
            if (!startChannelRecord(channelIndex, rawFrame, true)) {
                nextRetryTimes[channelIndex] = QDateTime::currentDateTime().addSecs(3);
                return;
            }
        }
        if (!videoWriters[channelIndex].isOpened()) {
            if (!retryAllowed)
                return;
            if (!startChannelRecord(channelIndex, rawFrame, false)) {
                nextRetryTimes[channelIndex] = QDateTime::currentDateTime().addSecs(3);
                return;
            }
        }
        const cv::Mat &sourceFrame =
            motionModes[channelIndex] && !markedFrame.empty() ? markedFrame : rawFrame;
        cv::Mat frame = resizeRecordFrame(sourceFrame);
        if (frame.empty())
            return;
        writeFrame(channelIndex, frame);
        if (motionModes[channelIndex])
            saveEventImage(channelIndex, frame);
    } catch (const cv::Exception &exception) {
        qWarning() << "写入录像失败：" << exception.what();
        finishChannelRecord(channelIndex, 2);
        nextRetryTimes[channelIndex] = QDateTime::currentDateTime().addSecs(3);
    }
}

bool RecordTask::startChannelRecord(int channelIndex, const cv::Mat &frame, bool motion)
{
    // 创建录像文件并先登记数据库记录，失败时清理未能使用的文件。
    if (!QDir().mkpath(outputDirectory))
        return false;
    const QDateTime startTime = QDateTime::currentDateTime();
    const QString type = motion ? QStringLiteral("motion") : QStringLiteral("normal");
    const QString name = QStringLiteral("channel%1_%2_%3.mp4").arg(channelIndex + 1)
        .arg(type).arg(startTime.toString(QStringLiteral("yyyyMMdd_HHmmss_zzz")));
    const QString path = QDir(outputDirectory).filePath(name);
    const QByteArray encodedPath = QFile::encodeName(path);
    const int fourcc = cv::VideoWriter::fourcc('a', 'v', 'c', '1');
    if (!videoWriters[channelIndex].open(encodedPath.constData(), cv::CAP_FFMPEG,
                                         fourcc, RecordFps,
                                         cv::Size(RecordWidth, RecordHeight), true)) {
        qWarning() << "H.264录像文件创建失败：" << path;
        return false;
    }

    QString errorMessage;
    qulonglong recordId = 0;
    qulonglong eventId = 0;
    const bool saved = motion
        ? videoRecordModel.insertMotionRecord(channelIndex + 1, startTime,
                                               path, eventId, recordId, errorMessage)
        : videoRecordModel.insertRecord(channelIndex + 1, type, startTime,
                                         path, recordId, errorMessage);
    if (!saved) {
        videoWriters[channelIndex].release();
        QFile::remove(path);
        qWarning() << "新增录像记录失败：" << errorMessage;
        return false;
    }
    recordIds[channelIndex] = recordId;
    eventIds[channelIndex] = eventId;
    startTimes[channelIndex] = startTime;
    videoPaths[channelIndex] = path;
    motionModes[channelIndex] = motion;
    imageCounts[channelIndex] = 0;
    writtenFrames[channelIndex] = 0;
    lastFrames[channelIndex].release();
    nextRetryTimes[channelIndex] = QDateTime();
    segmentTimers[channelIndex].start();
    if (detectionService != nullptr)
        detectionService->setMotionActive(channelIndex, motion);
    qInfo() << "录制线程" << recordThreadIndex + 1 << "开始录制：" << path;
    return true;
}

void RecordTask::writeFrame(int channelIndex, const cv::Mat &frame)
{
    // 固定30fps输出，按真实经过的时间补帧，避免低帧率来源导致录像时长缩短。
    const int targetCount = qMin(SegmentLengthMs * RecordFps / 1000,
        static_cast<int>(segmentTimers[channelIndex].elapsed() * RecordFps / 1000) + 1);
    while (writtenFrames[channelIndex] < targetCount) {
        videoWriters[channelIndex].write(frame);
        ++writtenFrames[channelIndex];
    }
    lastFrames[channelIndex] = frame.clone();
}

void RecordTask::saveEventImage(int channelIndex, const cv::Mat &frame)
{
    // 在移动事件开始后的多个时间点保存截图并关联事件与录像记录。
    const int sequence = imageCounts[channelIndex];
    if (sequence >= 3 || segmentTimers[channelIndex].elapsed() < sequence * 500)
        return;
    cv::Mat rgbFrame;
    cv::cvtColor(frame, rgbFrame, cv::COLOR_BGR2RGB);
    const QImage image = QImage(rgbFrame.data, rgbFrame.cols, rgbFrame.rows,
        static_cast<int>(rgbFrame.step), QImage::Format_RGB888).copy();
    const qint64 offset = qMax(0, writtenFrames[channelIndex] - 1) * 1000LL / RecordFps;
    const QDateTime frameTime = startTimes[channelIndex].addMSecs(offset);
    const QString directory = QDir(outputDirectory).filePath(QStringLiteral("events"));
    QString errorMessage;
    if (snapshotService.saveEventSnapshot(image, channelIndex + 1, eventIds[channelIndex],
            recordIds[channelIndex], startTimes[channelIndex], frameTime, offset,
            sequence + 1, directory, errorMessage)) {
        ++imageCounts[channelIndex];
    } else {
        qWarning() << "保存异常图片失败：" << errorMessage;
    }
}

cv::Mat RecordTask::resizeRecordFrame(const cv::Mat &frame) const
{
    // 等比例缩放录像帧并以黑边补齐目标尺寸，避免画面变形或裁切。
    if (frame.empty())
        return cv::Mat();

    const double resizeScale =
        qMin(RecordWidth / static_cast<double>(frame.cols),
             RecordHeight / static_cast<double>(frame.rows));
    const int resizeWidth = qMax(1, cvRound(frame.cols * resizeScale));
    const int resizeHeight = qMax(1, cvRound(frame.rows * resizeScale));

    cv::Mat resizedFrame;
    cv::resize(frame, resizedFrame, cv::Size(resizeWidth, resizeHeight));
    cv::Mat targetFrame(
        cv::Size(RecordWidth, RecordHeight), frame.type(), cv::Scalar(0, 0, 0));
    const int left = (RecordWidth - resizeWidth) / 2;
    const int top = (RecordHeight - resizeHeight) / 2;
    resizedFrame.copyTo(targetFrame(cv::Rect(left, top, resizeWidth, resizeHeight)));
    return targetFrame;
}

void RecordTask::finishChannelRecord(int channelIndex, int status)
{
    // 关闭并校验录像文件，再回写数据库状态、时长和文件大小。
    if (channelIndex < 0 || channelIndex >= MaxCaptureThreadCount)
        return;
    const bool wasMotion = motionModes[channelIndex];
    if (videoWriters[channelIndex].isOpened()) {
        try {
            if (status == 1 && segmentTimers[channelIndex].elapsed() >= SegmentLengthMs
                && !lastFrames[channelIndex].empty()) {
                const int totalFrames = SegmentLengthMs * RecordFps / 1000;
                while (writtenFrames[channelIndex] < totalFrames) {
                    videoWriters[channelIndex].write(lastFrames[channelIndex]);
                    ++writtenFrames[channelIndex];
                }
            }
        } catch (const cv::Exception &exception) {
            status = 2;
            qWarning() << "补写录像帧失败：" << exception.what();
        }
        try {
            videoWriters[channelIndex].release();
        } catch (const cv::Exception &exception) {
            status = 2;
            qWarning() << "关闭录像文件失败：" << exception.what();
        }
    }
    if (recordIds[channelIndex] != 0) {
        QFileInfo videoFile(videoPaths[channelIndex]);
        qulonglong size = videoFile.exists()
            ? static_cast<qulonglong>(videoFile.size()) : 0;
        // 仅在正常结束且文件非空时重新打开读取首帧，过滤未正确封装的片段。
        if (size > 0 && status == 1) {
            bool playable = false;
            try {
                cv::VideoCapture verificationCapture;
                cv::Mat firstFrame;
                const QByteArray encodedPath =
                    QFile::encodeName(videoPaths[channelIndex]);
                playable = verificationCapture.open(
                               encodedPath.constData(), cv::CAP_FFMPEG)
                    && verificationCapture.read(firstFrame)
                    && !firstFrame.empty();
                verificationCapture.release();
            } catch (const cv::Exception &exception) {
                qWarning() << "校验录像片段失败：" << exception.what();
            }
            if (!playable) {
                status = 2;
                QFile::remove(videoPaths[channelIndex]);
                size = 0;
                qWarning() << "录像片段未能正确封装，已清理不可播放文件："
                           << videoPaths[channelIndex];
            }
        }
        if (size == 0)
            status = 2;
        QString errorMessage;
        if (!videoRecordModel.finishRecord(recordIds[channelIndex],
                QDateTime::currentDateTime(), size, status, errorMessage))
            qWarning() << "更新录像记录失败：" << errorMessage;
        if (wasMotion && imageCounts[channelIndex] < 3)
            qWarning() << "异常录像提前结束或图片保存失败，已保存图片数：" << imageCounts[channelIndex];
    }
    recordIds[channelIndex] = 0;
    eventIds[channelIndex] = 0;
    motionModes[channelIndex] = false;
    videoPaths[channelIndex].clear();
    lastFrames[channelIndex].release();
    if (detectionService != nullptr)
        detectionService->setMotionActive(channelIndex, false);
}

void RecordTask::setOutputDirectory(const QString &directory)
{
    // 切换保存目录前先结束现有片段，避免同一片段跨目录写入。
    if (QDir(directory).absolutePath() == QDir(outputDirectory).absolutePath())
        return;
    finishAllRecords();
    outputDirectory = directory;
    QDir().mkpath(outputDirectory);
}

void RecordTask::finishAllRecords()
{
    // 处理最后缓存帧后关闭所有通道的录像片段。
    processPendingFrames();
    for (int index = 0; index < MaxCaptureThreadCount; ++index)
        finishChannelRecord(index, 1);
}
