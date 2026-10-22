#ifndef PROCESSTASK_H
#define PROCESSTASK_H

#include <QObject>
#include <QImage>
#include <QMutex>
#include <QSemaphore>
#include <QString>
#include <opencv2/opencv.hpp>

class ProcessTask : public QObject
{
    Q_OBJECT
public:
    explicit ProcessTask(
        QSemaphore *frameSemaphore,
        QObject *parent = nullptr);

    QImage getImage();

public slots:
    void processFrame(
        cv::Mat frame,
        const QString &status);

private:
    QImage matToImage(const cv::Mat &frame);
    void drawStatus(
        QImage &image,
        const QString &status);

    QSemaphore *frameSemaphore;

    QMutex imageMutex;
    QImage channelImage;
};

#endif // PROCESSTASK_H
