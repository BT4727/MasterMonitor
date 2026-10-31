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

    static constexpr int TimeRangeCount = 12;
    static constexpr int ImagePageSize = 9;

    void initLeftArea();
    void initRightArea();
    void initVideoList();
    void updateVideoColumnWidths();
    void initTimeRanges();
    void updateTimeRangeButtons();
    void showTimeRange(int rangeIndex);
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

    PreviewMode previewMode = VideoPreview;
    int videoCurrentPage = 1;
    int videoTotalPages = 1;
    int imageCurrentPage = 1;
    int imageTotalPages = 1;

    // 每页显示数量
    int videoPageSize = 4;

    QPixmap videoThumbnail;
    QPixmap blackMonitorImage;

    QHBoxLayout *mainLayout = nullptr;

    // 左侧区域
    QWidget *leftWidget = nullptr;
    QVBoxLayout *leftLayout = nullptr;

    QWidget *filterWidget = nullptr;
    QGridLayout *filterLayout = nullptr;
    QDateTimeEdit *dateTimeEdit = nullptr;
    QComboBox *channelComboBox = nullptr;
    QTimeEdit *startTimeEdit = nullptr;
    QTimeEdit *endTimeEdit = nullptr;
    QComboBox *recordTypeComboBox = nullptr;
    QPushButton *searchButton = nullptr;

    QWidget *leftContentWidget = nullptr;
    QStackedLayout *leftContentStack = nullptr;

    QTableWidget *videoTable = nullptr;

    QWidget *timeRangeWidget = nullptr;
    QVBoxLayout *timeRangeLayout = nullptr;
    QButtonGroup *timeRangeGroup = nullptr;
    QVector<QPushButton *> timeRangeButtons;
    QLabel *timeRangeEmptyLabel = nullptr;

    QWidget *leftBottomWidget = nullptr;
    QStackedLayout *leftBottomStack = nullptr;

    QWidget *videoPagerWidget = nullptr;
    QHBoxLayout *videoPagerLayout = nullptr;
    QPushButton *videoPreviousButton = nullptr;
    QLabel *videoPageLabel = nullptr;
    QPushButton *videoNextButton = nullptr;

    QWidget *emptyLeftBottomWidget = nullptr;

    // 右侧区域
    QWidget *rightWidget = nullptr;
    QVBoxLayout *rightLayout = nullptr;

    QWidget *previewModeWidget = nullptr;
    QHBoxLayout *previewModeLayout = nullptr;
    QPushButton *videoPreviewButton = nullptr;
    QPushButton *motionPreviewButton = nullptr;
    QButtonGroup *previewModeGroup = nullptr;

    QWidget *previewContentWidget = nullptr;
    QStackedLayout *previewContentStack = nullptr;

    // 视频预览页
    QWidget *videoPreviewPage = nullptr;
    QVBoxLayout *videoPreviewLayout = nullptr;
    QLabel *videoDisplayLabel = nullptr;

    QWidget *videoControlsWidget = nullptr;
    QVBoxLayout *videoControlsLayout = nullptr;
    QPushButton *screenshotButton = nullptr;
    QComboBox *speedComboBox = nullptr;

    // 图片预览页
    QWidget *imagePreviewPage = nullptr;
    QVBoxLayout *imagePreviewLayout = nullptr;

    QWidget *imageGridWidget = nullptr;
    QGridLayout *imageGridLayout = nullptr;
    std::array<QLabel *, ImagePageSize> imageLabels{};

    QWidget *imagePagerWidget = nullptr;
    QHBoxLayout *imagePagerLayout = nullptr;
    QPushButton *imagePreviousButton = nullptr;
    QLabel *imagePageLabel = nullptr;
    QPushButton *imageNextButton = nullptr;

    QMediaPlayer *mediaPlayer = nullptr;
    QAudioOutput *audioOutput = nullptr;
    QVideoWidget *videoWidget = nullptr;

    QPushButton *playPauseButton = nullptr;
    QPushButton *stopButton = nullptr;

    QSlider *progressSlider = nullptr;
    QLabel *positionLabel = nullptr;
    QLabel *durationLabel = nullptr;

    PlaybackService playbackService;
    QVector<VideoRecordInfo> videoRecords;

    QVector<SnapshotInfo> snapshots;
    QVector<SnapshotInfo> daySnapshots;
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
    void updateSnapshotZoom(double scaleValue);
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
    double snapshotScale = 1.0;
    int snapshotWheelRemainder = 0;
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
