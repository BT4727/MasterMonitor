#ifndef MONITORTASK_H
#define MONITORTASK_H

#include <QObject>
#include <QString>
#include <QSemaphore>
#include <QMetaType>
#include <QMutex>
#include <opencv2/opencv.hpp>

class MonitorTask : public QObject
{
    Q_OBJECT
public:
    explicit MonitorTask(
        int channelIndex,
        int cameraIndex,
        const QString &videoPath,
        QSemaphore *frameSemaphore,
        QObject *parent = nullptr);
    void setRecordEnabled(bool enabled);

signals:
    void sendFrame(cv::Mat frame,const QString &status);
    void sendRecordFrame(int channelIndex,cv::Mat frame);
public slots:
    void startMonitor();

private:
    bool openCamera(cv::VideoCapture &camera);
    bool openVideo(cv::VideoCapture &video);
    bool readFrame(
        cv::VideoCapture &source,
        cv::Mat &frame);

    int getVideoInterval(cv::VideoCapture &video);
    void waitInterval(int milliseconds);

    int channelIndex;
    int cameraIndex;
    QString videoPath;

    QSemaphore *frameSemaphore;
    QMutex recordMutex;
    bool recordEnabled = false;
};

#endif // MONITORTASK_H
