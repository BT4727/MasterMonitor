#include "monitortask.h"

#include <QThread>
#include <QElapsedTimer>
#include <QFile>
#include <cmath>
#include <exception>
#include <QMutexLocker>
MonitorTask::MonitorTask(
    int channelIndex,
    int cameraIndex,
    const QString &videoPath,
    QSemaphore *frameSemaphore,
    QObject *parent)
    : QObject(parent)
{
    this->channelIndex = channelIndex;
    this->cameraIndex = cameraIndex;
    this->videoPath = videoPath;
    this->frameSemaphore = frameSemaphore;
}

void MonitorTask::startMonitor()
{
    // 在采集线程中创建、打开和释放。
    cv::VideoCapture camera;
    cv::VideoCapture video;

    QElapsedTimer reconnectTimer;
    reconnectTimer.start();

    qint64 nextCameraRetry = 0;
    qint64 nextVideoRetry = 0;

    QThread *currentThread = QThread::currentThread();

    while (!currentThread->isInterruptionRequested()) {
        QElapsedTimer frameTimer;
        frameTimer.start();

        cv::Mat frame;
        QString status;
        int interval = 33;

        // 摄像头打开失败后，每隔3秒重试。
        if (!camera.isOpened()
            && cameraIndex >= 0
            && reconnectTimer.elapsed() >= nextCameraRetry) {
            if (!openCamera(camera)) {
                nextCameraRetry =
                    reconnectTimer.elapsed() + 3000;
            }
        }

        bool cameraAvailable = false;

        if (camera.isOpened()) {
            cameraAvailable = readFrame(camera, frame);

            if (!cameraAvailable) {
                camera.release();

                nextCameraRetry =
                    reconnectTimer.elapsed() + 3000;
            }
        }

        if (cameraAvailable) {
            // 摄像头恢复后释放占位视频。
            if (video.isOpened()) {
                video.release();
            }
        } else {
            if (!video.isOpened()
                && !videoPath.isEmpty()
                && reconnectTimer.elapsed() >= nextVideoRetry) {
                if (!openVideo(video)) {
                    nextVideoRetry =
                        reconnectTimer.elapsed() + 3000;
                }
            }

            bool videoAvailable = false;

            if (video.isOpened()) {
                videoAvailable = readFrame(video, frame);

                if (!videoAvailable) {
                    // 播放完毕后重新打开，实现循环播放。
                    video.release();

                    if (openVideo(video)) {
                        videoAvailable =
                            readFrame(video, frame);
                    }
                }
            }

            if (videoAvailable) {
                status = QStringLiteral(
                             "通道%1：摄像头离线，播放占位视频")
                             .arg(channelIndex + 1);

                interval = getVideoInterval(video);
            } else {
                video.release();

                nextVideoRetry =
                    reconnectTimer.elapsed() + 3000;

                frame = cv::Mat::zeros(
                    480, 640, CV_8UC3);

                status = QStringLiteral(
                             "通道%1：离线，无可用占位视频")
                             .arg(channelIndex + 1);

                interval = 100;
            }
        }

        if (currentThread->isInterruptionRequested()) {
            break;
        }

        // 每个通道最多有一张帧正在排队或处理中。
        // 处理没完成时跳过当前帧，防止事件队列积压。
        if (frameSemaphore->tryAcquire()) {
            emit sendFrame(frame.clone(), status);
        }

        // 使用阻塞队列连接录制线程，
        // 使采集端不会无限堆积待录制帧。
        {
            QMutexLocker<QMutex> locker(&recordMutex);
            if (recordEnabled)
                emit sendRecordFrame(channelIndex, frame.clone());
        }
        const int remaining =
            interval - static_cast<int>(frameTimer.elapsed());

        if (remaining > 0) {
            waitInterval(remaining);
        }
    }

    camera.release();
    video.release();
}

void MonitorTask::setRecordEnabled(bool enabled)
{
    // 等正在发送的录制帧处理完毕，再切换录制状态。
    QMutexLocker<QMutex> locker(&recordMutex);
    recordEnabled = enabled;
}

bool MonitorTask::openCamera(
    cv::VideoCapture &camera)
{
    try {
#ifdef Q_OS_WIN
        bool opened =
            camera.open(cameraIndex, cv::CAP_DSHOW);
#else
        bool opened =
            camera.open(cameraIndex, cv::CAP_ANY);
#endif

        if (!opened) {
            camera.release();
            return false;
        }

        camera.set(cv::CAP_PROP_FRAME_WIDTH, 640);
        camera.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

        return true;
    } catch (const std::exception &) {
        camera.release();
        return false;
    }
}

bool MonitorTask::openVideo(
    cv::VideoCapture &video)
{
    if (videoPath.isEmpty()) {
        return false;
    }

    try {
        const QByteArray fileName =
            QFile::encodeName(videoPath);

        bool opened = video.open(
            fileName.constData(),
            cv::CAP_FFMPEG);

        if (!opened) {
            video.release();
        }

        return opened;
    } catch (const std::exception &) {
        video.release();
        return false;
    }
}

bool MonitorTask::readFrame(
    cv::VideoCapture &source,
    cv::Mat &frame)
{
    try {
        return source.read(frame) && !frame.empty();
    } catch (const std::exception &) {
        return false;
    }
}

int MonitorTask::getVideoInterval(
    cv::VideoCapture &video)
{
    double fps = video.get(cv::CAP_PROP_FPS);

    if (!std::isfinite(fps) || fps <= 0) {
        return 40;
    }

    return qBound(
        1,
        static_cast<int>(1000.0 / fps),
        1000);
}

void MonitorTask::waitInterval(int milliseconds)
{
    QThread *currentThread = QThread::currentThread();

    while (milliseconds > 0
           && !currentThread->isInterruptionRequested()) {
        int duration = qMin(milliseconds, 10);

        QThread::msleep(
            static_cast<unsigned long>(duration));

        milliseconds -= duration;
    }
}
