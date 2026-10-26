#ifndef REVIEWWIDGET_H
#define REVIEWWIDGET_H

#include <QWidget>
#include <QDateTimeEdit>
#include <QComboBox>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QStackedLayout>
#include <QPixmap>
#include <QResizeEvent>
#include <QTime>
#include <array>
#include <QTimeEdit>
#include <QSlider>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoWidget>
#include <QVector>
#include "../service/playbackservice.h"
#include <QCheckBox>
#include "../service/snapshotservice.h"
#include "../service/logservice.h"
#include <QImage>
#include <QVideoSink>
#include <QVideoFrame>
#include <QFileDialog>
#include <QCoreApplication>
#include <QDir>
#include <QSet>
#include <QDialog>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QShowEvent>
#include <QHideEvent>

class ReviewWidget : public QWidget
{
    Q_OBJECT
public:
    enum PageRole { VideoReview, SnapshotReview };
    explicit ReviewWidget(QWidget *parent = nullptr, PageRole role = VideoReview);
    void setCurrentUser(const QString &username);
    bool playRecord(qulonglong recordId, qint64 offsetMs);
signals:
    void relatedVideoRequested(qulonglong recordId, qint64 offsetMs);
    void screenshotRequested();
    void timeRangeSelected(const QTime &startTime,
                           const QTime &endTime);
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    enum PreviewMode {
        VideoPreview = 0,
        ImagePreview,
        MotionPreview
    };

    static constexpr int TimeRangeCount = 15;
    static constexpr int ImagePageSize = 9;

    void initLeftArea();
    void initRightArea();
    void initVideoList();
    void updateVideoColumnWidths();
    void initTimeRanges();
    void initVideoPreview();
    void initImagePreview();
    void initConnect();

    void setPreviewMode(PreviewMode mode);
    void updateVideoPagination();
    void updateImagePagination();
    void searchVideos();
    void openOrdinaryPlayback();
    void openMotionPlayback();
    void resetVideoQuery();
    void loadVideoPage();
    void playVideoAtRow(int row);
    QString formatPlaybackTime(qint64 milliseconds) const;

    PreviewMode m_previewMode = VideoPreview;
    int m_videoCurrentPage = 1;
    int m_videoTotalPages = 1;
    int m_imageCurrentPage = 1;
    int m_imageTotalPages = 1;

    // 每页显示数量
    int m_videoPageSize = 4;

    QPixmap m_videoThumbnail;
    QPixmap m_blackMonitorImage;

    QHBoxLayout *m_mainLayout = nullptr;

    // 左侧区域
    QWidget *m_leftWidget = nullptr;
    QVBoxLayout *m_leftLayout = nullptr;

    QWidget *m_filterWidget = nullptr;
    QGridLayout *m_filterLayout = nullptr;
    QDateTimeEdit *m_dateTimeEdit = nullptr;
    QComboBox *m_channelComboBox = nullptr;
    QTimeEdit *startTimeEdit = nullptr;
    QTimeEdit *endTimeEdit = nullptr;
    QComboBox *recordTypeComboBox = nullptr;
    QPushButton *searchButton = nullptr;

    QWidget *m_leftContentWidget = nullptr;
    QStackedLayout *m_leftContentStack = nullptr;

    QTableWidget *m_videoTable = nullptr;

    QWidget *m_timeRangeWidget = nullptr;
    QVBoxLayout *m_timeRangeLayout = nullptr;
    QButtonGroup *m_timeRangeGroup = nullptr;
    std::array<QPushButton *, TimeRangeCount> m_timeRangeButtons{};

    QWidget *m_leftBottomWidget = nullptr;
    QStackedLayout *m_leftBottomStack = nullptr;

    QWidget *m_videoPagerWidget = nullptr;
    QHBoxLayout *m_videoPagerLayout = nullptr;
    QPushButton *m_videoPreviousButton = nullptr;
    QLabel *m_videoPageLabel = nullptr;
    QPushButton *m_videoNextButton = nullptr;

    QWidget *m_emptyLeftBottomWidget = nullptr;

    // 右侧区域
    QWidget *m_rightWidget = nullptr;
    QVBoxLayout *m_rightLayout = nullptr;

    QWidget *m_previewModeWidget = nullptr;
    QHBoxLayout *m_previewModeLayout = nullptr;
    QPushButton *m_videoPreviewButton = nullptr;
    QPushButton *motionPreviewButton = nullptr;
    QButtonGroup *m_previewModeGroup = nullptr;

    QWidget *m_previewContentWidget = nullptr;
    QStackedLayout *m_previewContentStack = nullptr;

    // 视频预览页
    QWidget *m_videoPreviewPage = nullptr;
    QVBoxLayout *m_videoPreviewLayout = nullptr;
    QLabel *m_videoDisplayLabel = nullptr;

    QWidget *m_videoControlsWidget = nullptr;
    QVBoxLayout *m_videoControlsLayout = nullptr;
    QPushButton *m_pauseButton = nullptr;
    QPushButton *m_screenshotButton = nullptr;
    QComboBox *m_speedComboBox = nullptr;

    // 图片预览页
    QWidget *m_imagePreviewPage = nullptr;
    QVBoxLayout *m_imagePreviewLayout = nullptr;

    QWidget *m_imageGridWidget = nullptr;
    QGridLayout *m_imageGridLayout = nullptr;
    std::array<QLabel *, ImagePageSize> m_imageLabels{};

    QWidget *m_imagePagerWidget = nullptr;
    QHBoxLayout *m_imagePagerLayout = nullptr;
    QPushButton *m_imagePreviousButton = nullptr;
    QLabel *m_imagePageLabel = nullptr;
    QPushButton *m_imageNextButton = nullptr;

    QMediaPlayer *mediaPlayer = nullptr;
    QAudioOutput *audioOutput = nullptr;
    QVideoWidget *videoWidget = nullptr;

    QPushButton *playButton = nullptr;
    QPushButton *continueButton = nullptr;
    QPushButton *stopButton = nullptr;

    QSlider *progressSlider = nullptr;
    QLabel *positionLabel = nullptr;
    QLabel *durationLabel = nullptr;

    PlaybackService playbackService;
    QVector<VideoRecordInfo> videoRecords;

    QVector<SnapshotInfo> snapshots;
    SnapshotService snapshotService;
    LogService logService;

    std::array<QCheckBox *, ImagePageSize> imageSelectionChecks{};

    QPushButton *viewSnapshotButton = nullptr;
    QPushButton *deleteSnapshotsButton = nullptr;
    QPushButton *exportSnapshotsButton = nullptr;

    void searchSnapshots();
    void updateSnapshotGrid();
    void viewSelectedSnapshot();
    void deleteSelectedSnapshots();
    void exportSelectedSnapshots();
    void savePlaybackSnapshot();
    void searchCurrentContent();
    QVector<SnapshotInfo> selectedSnapshots() const;
    void showLargeSnapshot();
    void showPreviousSnapshot();
    void showNextSnapshot();
    void updateSnapshotZoom(int value);
    void playRelatedVideo();
    void applyPendingSeek();

    QSet<qulonglong> selectedSnapshotIds;
    QImage playbackFrame;
    qint64 playbackFramePosition = 0;
    VideoRecordInfo currentVideo;
    qint64 pendingSeekPosition = -1;
    QDialog *snapshotDialog = nullptr;
    QGraphicsView *snapshotView = nullptr;
    QGraphicsScene *snapshotScene = nullptr;
    QSlider *snapshotZoomSlider = nullptr;
    QLabel *snapshotMetadataLabel = nullptr;
    QPushButton *previousSnapshotButton = nullptr;
    QPushButton *nextSnapshotButton = nullptr;
    QPushButton *relatedVideoButton = nullptr;
    int largeSnapshotIndex = -1;
    PageRole pageRole;
    QString currentUsername;
    int videoTotalCount = 0;
    QDateTime videoQueryStart;
    QDateTime videoQueryEnd;
    int videoQueryChannel = 0;
    bool relatedPlaybackPending = false;
};

#endif // REVIEWWIDGET_H
