#ifndef PERSONDETECTION_H
#define PERSONDETECTION_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QElapsedTimer>
#include <QString>
#include <QVector>
#include <opencv2/core.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/video/tracking.hpp>

class PersonTrackAxis
{
public:
    double position = 0.0;
    double velocity = 0.0;
    double p00 = 16.0;
    double p01 = 0.0;
    double p11 = 10000.0;

    void advance(double seconds);
    void correct(double measurement);
};

class PersonTrack
{
public:
    cv::Rect detection;
    qint64 measuredTime = -1;
    PersonTrackAxis axes[4];
};

class PersonTracker
{
public:
    static constexpr qint64 MaxPredictionMs = 1000;
    static constexpr qint64 TrackLifetimeMs = 1500;

    void clear();
    void update(const QVector<cv::Rect> &boxes, qint64 frameTime);
    QVector<cv::Rect> predict(qint64 frameTime, const cv::Size &frameSize) const;

private:
    QVector<PersonTrack> tracks;
    qint64 stateTime = -1;
};

class PersonModel
{
public:
    void load(const QString &modelPath);
    QVector<cv::Rect> detect(const cv::Mat &frame);

private:
    QVector<cv::Rect> detectYolo(const cv::Mat &frame);
    QVector<cv::Rect> detectHog(const cv::Mat &frame);
    float decodeDistance(const float *outputData, int gridArea,
                         int sideIndex, int position) const;

    cv::dnn::Net network;
    cv::HOGDescriptor hog;
    bool yoloAvailable = false;
};

// 每通道最多保留一张待检帧，结果和跟踪状态也按通道隔离。
class PersonChannel
{
public:
    cv::Mat pendingFrame;
    qint64 pendingTime = 0;
    bool hasPending = false;
    qint64 revision = 0;
    qint64 resultTime = -1;
    qint64 deliveredTime = -1;
    bool personPresent = false;
    bool motionActive = false;
    PersonTracker tracker;
};

class DetectionService;

class DetectionWorker : public QThread
{
public:
    DetectionWorker(int workerIndex, const QString &modelPath, DetectionService *service);

protected:
    void run() override;

private:
    int workerIndex;
    QString modelPath;
    DetectionService *service;
};

class DetectionService : public QObject
{
public:
    explicit DetectionService(QObject *parent = nullptr);
    ~DetectionService() override;

    void start(int channelCount, const QString &modelPath);
    void stop();
    void setPreview(bool singleChannel, int channelIndex, bool detectionEnabled);
    void setMotionActive(int channelIndex, bool active);
    void resetRecordingState();
    bool canTrigger(int channelIndex);
    bool isMotionDetectionEnabled();

    // personState：-1无新结果，0无人，1有人；预测框不作为真实检测结果。
    void prepareFrame(int channelIndex, cv::Mat &frame, int &personState, bool &allowTrigger);
    bool takeFrame(int workerIndex, int &channelIndex, cv::Mat &frame,
                   qint64 &frameTime, qint64 &revision);
    void publishResult(int channelIndex, const QVector<cv::Rect> &boxes,
                       qint64 frameTime, qint64 revision);

private:
    void clearChannel(int channelIndex);

    QMutex stateMutex;
    QWaitCondition frameReady;
    QElapsedTimer clock;
    QVector<PersonChannel> channels;
    QVector<DetectionWorker *> workers;
    QVector<int> nextChannels;
    int workerCount = 0;
    int selectedChannel = -1;
    bool singleChannel = false;
    bool detectionEnabled = false;
    bool running = false;
};

#endif
