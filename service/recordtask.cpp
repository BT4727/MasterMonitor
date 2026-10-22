#include "recordtask.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>
RecordTask::RecordTask(
    int recordThreadIndex,
    const QString &outputDirectory,
    const QString &username,
    QObject *parent)
    : QObject(parent)
{
    this->recordThreadIndex = recordThreadIndex;
    this->outputDirectory = outputDirectory;
    videoRecordModel.setCurrentUser(username);

    for (int index = 0;
         index < MaxCaptureThreadCount;
         index++) {
        recordIds[index] = 0;
    }
}
void RecordTask::recordFrame(
    int channelIndex,
    cv::Mat frame)
{
    if (channelIndex < 0
        || channelIndex >= MaxCaptureThreadCount
        || frame.empty()) {
        return;
    }

    if (videoWriters[channelIndex].isOpened()
        && segmentTimers[channelIndex].elapsed()
               >= SegmentLengthMs) {
        finishChannelRecord(channelIndex, 1);
    }

    if (!videoWriters[channelIndex].isOpened()) {
        if (nextRetryTimes[channelIndex].isValid()
            && QDateTime::currentDateTime()
                   < nextRetryTimes[channelIndex]) {
            return;
        }

        startChannelRecord(channelIndex, frame);

        if (!videoWriters[channelIndex].isOpened()) {
            return;
        }
    }

    try {
        videoWriters[channelIndex].write(frame);
    } catch (const cv::Exception &exception) {
        qWarning() << "写入录像帧失败："
                   << exception.what();
        finishChannelRecord(channelIndex, 2);
    }
}

void RecordTask::startChannelRecord(
    int channelIndex,
    const cv::Mat &frame)
{
    QDir().mkpath(outputDirectory);

    videoPaths[channelIndex] =
        createVideoPath(channelIndex);

    startTimes[channelIndex] =
        QDateTime::currentDateTime();

    QString errorMessage;

    const bool databaseResult =
        videoRecordModel.insertRecord(
            channelIndex + 1,
            QStringLiteral("normal"),
            startTimes[channelIndex],
            QDir::toNativeSeparators(
                videoPaths[channelIndex]),
            recordIds[channelIndex],
            errorMessage);

    if (!databaseResult) {
        qWarning() << "新增录像记录失败："
                   << errorMessage;

        recordIds[channelIndex] = 0;
        nextRetryTimes[channelIndex] =
            QDateTime::currentDateTime().addSecs(3);
        return;
    }

    const int fourcc = cv::VideoWriter::fourcc(
        'H', '2', '6', '4');

    const bool writerOpened =
        videoWriters[channelIndex].open(
            videoPaths[channelIndex].toStdString(),
            cv::CAP_FFMPEG,
            fourcc,
            30.0,
            cv::Size(frame.cols, frame.rows),
            true);

    if (!writerOpened) {
        qWarning() << "H.264录像文件创建失败："
                   << videoPaths[channelIndex];

        finishChannelRecord(channelIndex, 2);

        nextRetryTimes[channelIndex] =
            QDateTime::currentDateTime().addSecs(3);
        return;
    }

    segmentTimers[channelIndex].start();
    nextRetryTimes[channelIndex] = QDateTime();
    qInfo() << "录制线程" << recordThreadIndex + 1
            << "开始录制通道" << channelIndex + 1
            << videoPaths[channelIndex];
}

void RecordTask::finishChannelRecord(
    int channelIndex,
    int status)
{
    if (channelIndex < 0
        || channelIndex >= MaxCaptureThreadCount) {
        return;
    }

    if (videoWriters[channelIndex].isOpened()) {
        videoWriters[channelIndex].release();
    }

    if (recordIds[channelIndex] == 0) {
        return;
    }

    const QFileInfo videoFile(videoPaths[channelIndex]);
    const qulonglong videoSize =
        videoFile.exists()
            ? static_cast<qulonglong>(videoFile.size())
            : 0;

    QString errorMessage;

    const bool updateResult =
        videoRecordModel.finishRecord(
            recordIds[channelIndex],
            QDateTime::currentDateTime(),
            videoSize,
            status,
            errorMessage);

    if (!updateResult) {
        qWarning() << "更新录像记录失败："
                   << errorMessage
                   << "录像ID：" << recordIds[channelIndex];
    }

    qInfo() << "通道" << channelIndex + 1
            << "录像结束，状态" << status
            << "大小" << videoSize << "字节";

    recordIds[channelIndex] = 0;
    videoPaths[channelIndex].clear();
}

QString RecordTask::createVideoPath(
    int channelIndex) const
{
    const QString fileName =
        QStringLiteral("channel%1_normal_%2.mp4")
            .arg(channelIndex + 1)
            .arg(QDateTime::currentDateTime()
                     .toString(
                         QStringLiteral("yyyyMMdd_HHmmss_zzz")));

    return QDir(outputDirectory).filePath(fileName);
}

void RecordTask::setOutputDirectory(
    const QString &directory)
{
    // 路径切换时先结束旧目录下的录像片段。
    for (int index = 0;
         index < MaxCaptureThreadCount;
         index++) {
        if (videoWriters[index].isOpened()) {
            finishChannelRecord(index, 1);
        }
    }

    outputDirectory = directory;
    QDir().mkpath(outputDirectory);
}

void RecordTask::finishAllRecords()
{
    for (int index = 0;
         index < MaxCaptureThreadCount;
         index++) {
        if (videoWriters[index].isOpened()) {
            finishChannelRecord(index, 1);
        }
    }
}
