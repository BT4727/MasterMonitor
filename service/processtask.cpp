#include "processtask.h"

#include <QPainter>
#include <exception>
ProcessTask::ProcessTask(
    QSemaphore *frameSemaphore,
    QObject *parent)
    : QObject(parent)
{
    this->frameSemaphore = frameSemaphore;
}

void ProcessTask::processFrame(
    cv::Mat frame,
    const QString &status)
{
    QImage image;
    QString displayStatus = status;

    try {
        image = matToImage(frame);

        // 后续图像分析、移动侦测可以加在这里。
        // 隐藏通道也会继续执行本函数。
    } catch (const std::exception &) {
        image = QImage(
            640, 480, QImage::Format_RGB888);
        image.fill(Qt::black);

        displayStatus =
            QStringLiteral("视频帧处理失败");
    }

    if (!image.isNull()) {
        if (!displayStatus.isEmpty()) {
            drawStatus(image, displayStatus);
        }

        imageMutex.lock();
        channelImage = image;
        imageMutex.unlock();
    }

    // 当前帧处理结束，允许采集线程提交下一帧。
    frameSemaphore->release();
}

QImage ProcessTask::getImage()
{
    imageMutex.lock();
    QImage image = channelImage;
    imageMutex.unlock();

    return image;
}

QImage ProcessTask::matToImage(
    const cv::Mat &frame)
{
    if (frame.empty()) {
        return QImage();
    }

    cv::Mat rgbFrame;

    switch (frame.type()) {
    case CV_8UC1:
        return QImage(
                   frame.data,
                   frame.cols,
                   frame.rows,
                   static_cast<qsizetype>(frame.step),
                   QImage::Format_Grayscale8).copy();

    case CV_8UC3:
        cv::cvtColor(
            frame,
            rgbFrame,
            cv::COLOR_BGR2RGB);
        break;

    case CV_8UC4:
        cv::cvtColor(
            frame,
            rgbFrame,
            cv::COLOR_BGRA2RGB);
        break;

    default:
        return QImage();
    }

    // copy确保QImage不引用局部Mat的内存。
    return QImage(
               rgbFrame.data,
               rgbFrame.cols,
               rgbFrame.rows,
               static_cast<qsizetype>(rgbFrame.step),
               QImage::Format_RGB888).copy();
}

void ProcessTask::drawStatus(
    QImage &image,
    const QString &status)
{
    QPainter painter(&image);

    int height = qMin(48, image.height());

    QRect statusRect(
        0, 0, image.width(), height);

    painter.fillRect(
        statusRect,
        QColor(0, 0, 0, 180));

    painter.setPen(Qt::white);

    painter.drawText(
        statusRect.adjusted(8, 0, -8, 0),
        Qt::AlignLeft | Qt::AlignVCenter,
        status);
}

