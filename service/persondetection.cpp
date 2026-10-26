#include "persondetection.h"

#include <QFile>
#include <QDebug>
#include <QMutexLocker>
#include <cmath>
#include <exception>
#include <algorithm>
#include <limits>
#include <vector>

void PersonTrackAxis::advance(double seconds)
{
    constexpr double accelerationNoise = 2500.0;
    const double secondsSquared = seconds * seconds;
    position += velocity * seconds;
    p00 += 2.0 * seconds * p01
        + secondsSquared * p11
        + accelerationNoise * secondsSquared * secondsSquared / 4.0;
    p01 += seconds * p11
        + accelerationNoise * secondsSquared * seconds / 2.0;
    p11 += accelerationNoise * secondsSquared;
}

void PersonTrackAxis::correct(double measurement)
{
    const double innovation = measurement - position;
    const double denominator = p00 + 16.0;
    const double positionGain = p00 / denominator;
    const double velocityGain = p01 / denominator;
    position += positionGain * innovation;
    velocity += velocityGain * innovation;
    const double oldPositionVelocityCovariance = p01;
    p00 *= 1.0 - positionGain;
    p01 *= 1.0 - positionGain;
    p11 -= velocityGain * oldPositionVelocityCovariance;
}

void PersonTracker::clear()
{
    tracks.clear();
    stateTime = -1;
}

void PersonTracker::update(const QVector<cv::Rect> &boxes, qint64 frameTime)
{
    if (frameTime <= stateTime)
        return;

    for (int index = tracks.size() - 1; index >= 0; --index) {
        if (frameTime - tracks[index].measuredTime > TrackLifetimeMs)
            tracks.removeAt(index);
    }

    // Track state is advanced at detection timestamps; display prediction remains read-only.
    for (PersonTrack &track : tracks) {
        const qint64 predictionLimit =
            track.measuredTime + MaxPredictionMs;
        const qint64 fromTime = qMin(stateTime, predictionLimit);
        const qint64 toTime = qMin(frameTime, predictionLimit);
        const double seconds =
            qMax(qint64(0), toTime - fromTime) / 1000.0;
        for (PersonTrackAxis &axis : track.axes)
            axis.advance(seconds);
    }
    stateTime = frameTime;

    struct TrackMatch {
        int trackIndex;
        int boxIndex;
        double cost;
    };
    std::vector<TrackMatch> candidates;

    for (int trackIndex = 0; trackIndex < tracks.size(); ++trackIndex) {
        const PersonTrack &track = tracks.at(trackIndex);
        const double centerX = track.axes[0].position;
        const double centerY = track.axes[1].position;
        const double width = qMax(1.0, track.axes[2].position);
        const double height = qMax(1.0, track.axes[3].position);
        const cv::Rect2d predicted(
            centerX - width / 2.0,
            centerY - height / 2.0,
            width,
            height);

        for (int boxIndex = 0; boxIndex < boxes.size(); ++boxIndex) {
            const cv::Rect &box = boxes.at(boxIndex);
            if (box.area() <= 0)
                continue;

            const cv::Rect2d observed(box);
            const double intersection = (predicted & observed).area();
            const double unionArea = predicted.area() + observed.area() - intersection;
            const double overlap = unionArea > 0.0 ? intersection / unionArea : 0.0;
            const double deltaX =
                predicted.x + predicted.width / 2.0 - box.x - box.width / 2.0,
            deltaY =
                predicted.y + predicted.height / 2.0 - box.y - box.height / 2.0;
            const double relativeDistance =
                std::hypot(deltaX, deltaY)
                / qMax(1.0, std::hypot(box.width, box.height));
            if (overlap < 0.1 && relativeDistance > 0.75)
                continue;

            candidates.push_back({
                trackIndex,
                boxIndex,
                1.0 - overlap + relativeDistance
            });
        }
    }

    std::sort(candidates.begin(), candidates.end(),
              [](const TrackMatch &left, const TrackMatch &right) {
                  return left.cost < right.cost;
              });

    std::vector<bool> usedTracks(static_cast<size_t>(tracks.size()), false);
    std::vector<bool> usedBoxes(static_cast<size_t>(boxes.size()), false);
    for (const TrackMatch &match : candidates) {
        if (usedTracks[static_cast<size_t>(match.trackIndex)]
            || usedBoxes[static_cast<size_t>(match.boxIndex)]) {
            continue;
        }

        const cv::Rect &box = boxes.at(match.boxIndex);
        const double measurements[4] = {
            box.x + box.width / 2.0,
            box.y + box.height / 2.0,
            static_cast<double>(box.width),
            static_cast<double>(box.height)
        };
        PersonTrack &track = tracks[match.trackIndex];
        for (int axisIndex = 0; axisIndex < 4; ++axisIndex)
            track.axes[axisIndex].correct(measurements[axisIndex]);
        track.detection = box;
        track.measuredTime = frameTime;
        usedTracks[static_cast<size_t>(match.trackIndex)] = true;
        usedBoxes[static_cast<size_t>(match.boxIndex)] = true;
    }

    for (int boxIndex = 0; boxIndex < boxes.size(); ++boxIndex) {
        const cv::Rect &box = boxes.at(boxIndex);
        if (usedBoxes[static_cast<size_t>(boxIndex)] || box.area() <= 0)
            continue;

        PersonTrack track;
        track.axes[0].position = box.x + box.width / 2.0;
        track.axes[1].position = box.y + box.height / 2.0;
        track.axes[2].position = box.width;
        track.axes[3].position = box.height;
        track.detection = box;
        track.measuredTime = frameTime;
        tracks.append(track);
    }
}

QVector<cv::Rect> PersonTracker::predict(qint64 frameTime, const cv::Size &frameSize) const
{
    QVector<cv::Rect> result;
    if (stateTime < 0 || frameSize.empty())
        return result;

    for (const PersonTrack &track : tracks) {
        if (frameTime - track.measuredTime > TrackLifetimeMs)
            continue;
        const qint64 predictionTime =
            qMin(frameTime, track.measuredTime + MaxPredictionMs);
        const double seconds =
            qMax(qint64(0), predictionTime - stateTime) / 1000.0;
        const double centerX =
            track.axes[0].position + track.axes[0].velocity * seconds;
        const double centerY =
            track.axes[1].position + track.axes[1].velocity * seconds;
        const double width =
            qMax(1.0, track.axes[2].position + track.axes[2].velocity * seconds);
        const double height =
            qMax(1.0, track.axes[3].position + track.axes[3].velocity * seconds);

        const cv::Rect2d predicted(
            centerX - width / 2.0,
            centerY - height / 2.0,
            width,
            height);
        const double left = qBound(0.0, predicted.x, static_cast<double>(frameSize.width));
        const double top = qBound(0.0, predicted.y, static_cast<double>(frameSize.height));
        const double right = qBound(left, predicted.x + predicted.width,
                                    static_cast<double>(frameSize.width));
        const double bottom = qBound(top, predicted.y + predicted.height,
                                     static_cast<double>(frameSize.height));
        const cv::Rect rectangle(
            cvRound(left), cvRound(top),
            cvRound(right) - cvRound(left),
            cvRound(bottom) - cvRound(top));
        if (rectangle.area() > 0)
            result.append(rectangle);
    }
    return result;
}

void PersonModel::load(const QString &modelPath)
{
    hog.setSVMDetector(cv::HOGDescriptor::getDefaultPeopleDetector());
    try {
        network = cv::dnn::readNetFromONNX(QFile::encodeName(modelPath).constData());
        network.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        network.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        yoloAvailable = !network.empty();
    } catch (const cv::Exception &error) {
        yoloAvailable = false;
        qWarning() << "YOLO加载失败，使用HOG人体检测：" << error.what();
    }
}

QVector<cv::Rect> PersonModel::detect(const cv::Mat &frame)
{
    if (frame.empty())
        return QVector<cv::Rect>();
    if (yoloAvailable) {
        try {
            return detectYolo(frame);
        } catch (const cv::Exception &error) {
            yoloAvailable = false;
            qWarning() << "YOLO推理失败，使用HOG人体检测：" << error.what();
        }
    }
    return detectHog(frame);
}

QVector<cv::Rect> PersonModel::detectYolo(const cv::Mat &frame)
{
    const int inputSize = 640;
    const float resizeScale = qMin(inputSize / static_cast<float>(frame.cols),
                                   inputSize / static_cast<float>(frame.rows));
    const int resizeWidth = qBound(1, cvRound(frame.cols * resizeScale), inputSize);
    const int resizeHeight = qBound(1, cvRound(frame.rows * resizeScale), inputSize);
    const int leftPadding = (inputSize - resizeWidth) / 2;
    const int topPadding = (inputSize - resizeHeight) / 2;

    cv::Mat resizedFrame;
    cv::resize(frame, resizedFrame, cv::Size(resizeWidth, resizeHeight));
    cv::Mat inputFrame(inputSize, inputSize, CV_8UC3, cv::Scalar(114, 114, 114));
    resizedFrame.copyTo(inputFrame(cv::Rect(
        leftPadding, topPadding, resizeWidth, resizeHeight)));

    cv::Mat blob = cv::dnn::blobFromImage(
        inputFrame, 1.0 / 255.0, cv::Size(inputSize, inputSize), cv::Scalar(), true, false);
    network.setInput(blob);

    std::vector<cv::Mat> outputTensors;
    const std::vector<std::string> outputLayerNames =
        network.getUnconnectedOutLayersNames();
    network.forward(outputTensors, outputLayerNames);

    if (outputTensors.empty())
        CV_Error(cv::Error::StsBadArg, "YOLO has no output layer");

    std::vector<cv::Rect> detectedBoxes;
    std::vector<float> detectedScores;
    const cv::Rect sourceRectangle(0, 0, frame.cols, frame.rows);

    for (const cv::Mat &sourceOutput : outputTensors) {
        if (sourceOutput.dims != 4 || sourceOutput.size[0] != 1
            || sourceOutput.size[1] < 65) {
            CV_Error(cv::Error::StsBadArg, "Unsupported YOLO raw output shape");
        }

        const cv::Mat output = sourceOutput.isContinuous()
            ? sourceOutput : sourceOutput.clone();
        const int outputChannelCount = output.size[1];
        const int gridHeight = output.size[2];
        const int gridWidth = output.size[3];
        const int gridArea = gridWidth * gridHeight;
        const int classCount = outputChannelCount - 64;
        if (classCount <= 0)
            continue;

        const float strideX = inputSize / static_cast<float>(gridWidth);
        const float strideY = inputSize / static_cast<float>(gridHeight);
        const float *outputData = output.ptr<float>();

        for (int gridY = 0; gridY < gridHeight; ++gridY) {
            for (int gridX = 0; gridX < gridWidth; ++gridX) {
                const int position = gridY * gridWidth + gridX;

                // COCO类别0为person。
                const float personLogit = outputData[64 * gridArea + position];
                const float personScore = 1.0f / (1.0f + std::exp(-personLogit));
                if (!std::isfinite(personScore) || personScore < 0.25f)
                    continue;

                const float leftDistance =
                    decodeDistance(outputData, gridArea, 0, position);
                const float topDistance =
                    decodeDistance(outputData, gridArea, 1, position);
                const float rightDistance =
                    decodeDistance(outputData, gridArea, 2, position);
                const float bottomDistance =
                    decodeDistance(outputData, gridArea, 3, position);

                const float centerX = (gridX + 0.5f) * strideX;
                const float centerY = (gridY + 0.5f) * strideY;
                const float modelLeft = centerX - leftDistance * strideX;
                const float modelTop = centerY - topDistance * strideY;
                const float modelRight = centerX + rightDistance * strideX;
                const float modelBottom = centerY + bottomDistance * strideY;

                const float sourceLeft = (modelLeft - leftPadding) / resizeScale;
                const float sourceTop = (modelTop - topPadding) / resizeScale;
                const float sourceRight = (modelRight - leftPadding) / resizeScale;
                const float sourceBottom = (modelBottom - topPadding) / resizeScale;

                cv::Rect detectedRectangle(
                    cvRound(sourceLeft),
                    cvRound(sourceTop),
                    cvRound(sourceRight - sourceLeft),
                    cvRound(sourceBottom - sourceTop));
                detectedRectangle &= sourceRectangle;
                if (detectedRectangle.area() <= 0)
                    continue;

                detectedBoxes.push_back(detectedRectangle);
                detectedScores.push_back(personScore);
            }
        }
    }

    std::vector<int> retained;
    cv::dnn::NMSBoxes(detectedBoxes, detectedScores, 0.25f, 0.45f, retained);
    QVector<cv::Rect> result;
    for (int index : retained)
        result.append(detectedBoxes[index]);
    return result;
}

float PersonModel::decodeDistance(
    const float *outputData, int gridArea, int sideIndex, int position) const
{
    float values[16];
    float maximumValue = -std::numeric_limits<float>::max();

    for (int binIndex = 0; binIndex < 16; ++binIndex) {
        const int channelIndex = sideIndex * 16 + binIndex;
        values[binIndex] = outputData[channelIndex * gridArea + position];
        if (values[binIndex] > maximumValue)
            maximumValue = values[binIndex];
    }

    float totalWeight = 0.0f;
    float distance = 0.0f;
    for (int binIndex = 0; binIndex < 16; ++binIndex) {
        const float weight = std::exp(values[binIndex] - maximumValue);
        totalWeight += weight;
        distance += binIndex * weight;
    }

    if (totalWeight <= 0.0f)
        return 0.0f;
    return distance / totalWeight;
}

QVector<cv::Rect> PersonModel::detectHog(const cv::Mat &frame)
{
    const double scale = qMin(1.0, 640.0 / frame.cols);
    cv::Mat input;
    cv::resize(frame, input, cv::Size(), scale, scale);
    if (input.cols < 64 || input.rows < 128)
        return QVector<cv::Rect>();
    std::vector<cv::Rect> rectangles;
    hog.detectMultiScale(input, rectangles, 0, cv::Size(8, 8), cv::Size(8, 8), 1.05, 2);
    QVector<cv::Rect> result;
    for (const cv::Rect &rectangle : rectangles) {
        result.append(cv::Rect(cvRound(rectangle.x / scale), cvRound(rectangle.y / scale),
                              cvRound(rectangle.width / scale), cvRound(rectangle.height / scale)));
    }
    return result;
}

DetectionWorker::DetectionWorker(
    int workerIndex, const QString &modelPath, DetectionService *service)
    : QThread(service), workerIndex(workerIndex), modelPath(modelPath), service(service)
{
}

void DetectionWorker::run()
{
    try {
        PersonModel detector;
        detector.load(modelPath);
        int channelIndex = -1;
        cv::Mat frame;
        qint64 frameTime = 0;
        qint64 revision = 0;
        while (service->takeFrame(workerIndex, channelIndex, frame, frameTime, revision)) {
            QVector<cv::Rect> boxes;
            try {
                boxes = detector.detect(frame);
            } catch (const cv::Exception &error) {
                qWarning() << "人体检测失败：" << error.what();
                continue;
            }
            service->publishResult(channelIndex, boxes, frameTime, revision);
        }
    } catch (const std::exception &error) {
        qWarning() << "检测线程退出：" << error.what();
    }
}

DetectionService::DetectionService(QObject *parent)
    : QObject(parent)
{
}

DetectionService::~DetectionService()
{
    stop();
}

void DetectionService::start(int channelCount, const QString &modelPath)
{
    stop();
    channelCount = qBound(1, channelCount, 50);
    channels = QVector<PersonChannel>(channelCount);
    workerCount = qMax(2, (channelCount + 3) / 4);
    nextChannels = QVector<int>(workerCount, 0);
    clock.start();
    {
        QMutexLocker<QMutex> locker(&stateMutex);
        running = true;
    }
    for (int index = 0; index < workerCount; ++index) {
        DetectionWorker *worker = new DetectionWorker(index, modelPath, this);
        workers.append(worker);
        worker->start();
    }
}

void DetectionService::stop()
{
    {
        QMutexLocker<QMutex> locker(&stateMutex);
        running = false;
        frameReady.wakeAll();
    }
    for (DetectionWorker *worker : workers) {
        worker->wait();
        delete worker;
    }
    workers.clear();
    channels.clear();
    nextChannels.clear();
    workerCount = 0;
}

void DetectionService::clearChannel(int channelIndex)
{
    PersonChannel &channel = channels[channelIndex];
    ++channel.revision;
    channel.pendingFrame.release();
    channel.hasPending = false;
    channel.resultTime = -1;
    channel.deliveredTime = -1;
    channel.personPresent = false;
    channel.tracker.clear();
}

void DetectionService::setPreview(
    bool enabled, int channelIndex, bool detectionEnabled)
{
    QMutexLocker<QMutex> locker(&stateMutex);
    if (singleChannel == enabled
        && selectedChannel == channelIndex
        && this->detectionEnabled == detectionEnabled) {
        return;
    }
    singleChannel = enabled;
    selectedChannel = channelIndex;
    this->detectionEnabled = detectionEnabled;
    for (int index = 0; index < channels.size(); ++index) {
        if (!channels[index].motionActive)
            clearChannel(index);
    }
}

bool DetectionService::canTrigger(int channelIndex)
{
    QMutexLocker<QMutex> locker(&stateMutex);
    return running && singleChannel && detectionEnabled
        && selectedChannel == channelIndex;
}

bool DetectionService::isMotionDetectionEnabled()
{
    QMutexLocker<QMutex> locker(&stateMutex);
    return running && detectionEnabled;
}

void DetectionService::setMotionActive(int channelIndex, bool active)
{
    QMutexLocker<QMutex> locker(&stateMutex);
    if (channelIndex < 0 || channelIndex >= channels.size())
        return;
    channels[channelIndex].motionActive = active;
    if (!active && (!singleChannel || channelIndex != selectedChannel))
        clearChannel(channelIndex);
}

void DetectionService::resetRecordingState()
{
    QMutexLocker<QMutex> locker(&stateMutex);
    for (int index = 0; index < channels.size(); ++index) {
        channels[index].motionActive = false;
        clearChannel(index);
    }
}

void DetectionService::prepareFrame(
    int channelIndex, cv::Mat &frame, int &personState, bool &allowTrigger)
{
    personState = -1;
    allowTrigger = false;
    QMutexLocker<QMutex> locker(&stateMutex);
    if (!running || channelIndex < 0 || channelIndex >= channels.size() || frame.empty())
        return;
    PersonChannel &channel = channels[channelIndex];
    allowTrigger = singleChannel && detectionEnabled
        && channelIndex == selectedChannel;
    if (!allowTrigger && (!channel.motionActive || !detectionEnabled))
        return;
    const qint64 now = clock.elapsed();
    channel.pendingFrame = frame.clone();
    channel.pendingTime = now;
    channel.hasPending = true;
    frameReady.wakeAll();
    if (channel.resultTime >= 0) {
        if (channel.deliveredTime != channel.resultTime) {
            if (now - channel.resultTime <= 1000)
                personState = channel.personPresent ? 1 : 0;
            channel.deliveredTime = channel.resultTime;
        }
        const QVector<cv::Rect> boxes = channel.tracker.predict(now, frame.size());
        for (const cv::Rect &rectangle : boxes) {
            cv::rectangle(frame, rectangle, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
            cv::putText(frame, "person", cv::Point(rectangle.x, qMax(18, rectangle.y - 5)),
                        cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 2);
        }
    }
}

bool DetectionService::takeFrame(
    int workerIndex, int &channelIndex, cv::Mat &frame, qint64 &frameTime, qint64 &revision)
{
    QMutexLocker<QMutex> locker(&stateMutex);
    while (running) {
        const int count = static_cast<int>(channels.size());
        for (int offset = 0; offset < count; ++offset) {
            const int index = (nextChannels[workerIndex] + offset) % count;
            if (index % workerCount != workerIndex || !channels[index].hasPending)
                continue;
            PersonChannel &channel = channels[index];
            channelIndex = index;
            frame = channel.pendingFrame;
            frameTime = channel.pendingTime;
            revision = channel.revision;
            channel.pendingFrame.release();
            channel.hasPending = false;
            nextChannels[workerIndex] = (index + 1) % count;
            return true;
        }
        frameReady.wait(&stateMutex);
    }
    return false;
}

void DetectionService::publishResult(
    int channelIndex, const QVector<cv::Rect> &boxes, qint64 frameTime, qint64 revision)
{
    QMutexLocker<QMutex> locker(&stateMutex);
    if (!running || channelIndex < 0 || channelIndex >= channels.size())
        return;
    PersonChannel &channel = channels[channelIndex];
    if (channel.revision != revision || frameTime <= channel.resultTime)
        return;
    channel.tracker.update(boxes, frameTime);
    channel.personPresent = !boxes.isEmpty();
    channel.resultTime = frameTime;
}
