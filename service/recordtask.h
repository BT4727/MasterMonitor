#ifndef RECORDTASK_H
#define RECORDTASK_H

#include <QObject>
#include <QDateTime>
#include <QElapsedTimer>
#include <QMutex>
#include <QString>
#include <opencv2/opencv.hpp>
#include "../model/videorecordmodel.h"
#include "snapshotservice.h"
#include "persondetection.h"

class RecordTask : public QObject
{
    Q_OBJECT
public:
    explicit RecordTask(int recordThreadIndex, const QString &outputDirectory,
                        const QString &username, QObject *parent = nullptr);
    void setDetectionService(DetectionService *service);

public slots:
    void submitFrame(int channelIndex, cv::Mat rawFrame, cv::Mat markedFrame,
                     int personState, bool allowTrigger);
    void processPendingFrames();
    void setOutputDirectory(const QString &directory);
    void finishAllRecords();

private:
    void recordFrame(int channelIndex, const cv::Mat &rawFrame,
                     const cv::Mat &markedFrame, int personState, bool allowTrigger);
    bool startChannelRecord(int channelIndex, const cv::Mat &frame, bool motion);
    void finishChannelRecord(int channelIndex, int status);
    void writeFrame(int channelIndex, const cv::Mat &frame);
    void saveEventImage(int channelIndex, const cv::Mat &frame);
    cv::Mat resizeRecordFrame(const cv::Mat &frame) const;

    static constexpr int MaxCaptureThreadCount = 50;
    static constexpr int SegmentLengthMs = 60 * 1000;
    static constexpr int RecordFps = 30;
    static constexpr int RecordWidth = 640;
    static constexpr int RecordHeight = 480;

    int recordThreadIndex;
    QString outputDirectory;
    DetectionService *detectionService = nullptr;
    cv::VideoWriter videoWriters[MaxCaptureThreadCount];
    qulonglong recordIds[MaxCaptureThreadCount] = {};
    qulonglong eventIds[MaxCaptureThreadCount] = {};
    bool motionModes[MaxCaptureThreadCount] = {};
    bool triggerArmed[MaxCaptureThreadCount] = {};
    int imageCounts[MaxCaptureThreadCount] = {};
    int writtenFrames[MaxCaptureThreadCount] = {};
    QDateTime startTimes[MaxCaptureThreadCount];
    QElapsedTimer segmentTimers[MaxCaptureThreadCount];
    QDateTime nextRetryTimes[MaxCaptureThreadCount];
    QString videoPaths[MaxCaptureThreadCount];
    cv::Mat lastFrames[MaxCaptureThreadCount];
    QMutex pendingFrameMutex;
    cv::Mat pendingRawFrames[MaxCaptureThreadCount];
    cv::Mat pendingMarkedFrames[MaxCaptureThreadCount];
    int pendingPersonStates[MaxCaptureThreadCount] = {};
    bool pendingAllowTriggers[MaxCaptureThreadCount] = {};
    bool hasPendingFrames[MaxCaptureThreadCount] = {};
    bool processScheduled = false;
    VideoRecordModel videoRecordModel;
    SnapshotService snapshotService;
};

#endif
