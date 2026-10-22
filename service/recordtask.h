#ifndef RECORDTASK_H
#define RECORDTASK_H

#include <QObject>
#include <QDateTime>
#include <QElapsedTimer>
#include <QString>
#include <opencv2/opencv.hpp>
#include "../model/videorecordmodel.h"

class RecordTask : public QObject
{
    Q_OBJECT
public:
    explicit RecordTask(
        int recordThreadIndex,
        const QString &outputDirectory,
        const QString &username,
        QObject *parent = nullptr);


public slots:
    void recordFrame(
        int channelIndex,
        cv::Mat frame);

    void setOutputDirectory(
        const QString &outputDirectory);

    void finishAllRecords();

private:
    void startChannelRecord(
        int channelIndex,
        const cv::Mat &frame);

    void finishChannelRecord(
        int channelIndex,
        int status);

    QString createVideoPath(int channelIndex) const;

    static constexpr int MaxCaptureThreadCount = 50;
    static constexpr int SegmentLengthMs = 60 * 1000;

    int recordThreadIndex;
    QString outputDirectory;

    cv::VideoWriter videoWriters[MaxCaptureThreadCount];
    qulonglong recordIds[MaxCaptureThreadCount];
    QDateTime startTimes[MaxCaptureThreadCount];
    QElapsedTimer segmentTimers[MaxCaptureThreadCount];
    QString videoPaths[MaxCaptureThreadCount];
    QDateTime nextRetryTimes[MaxCaptureThreadCount];

    VideoRecordModel videoRecordModel;
};

#endif // RECORDTASK_H
