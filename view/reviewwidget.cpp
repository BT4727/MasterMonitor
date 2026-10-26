#include "reviewwidget.h"
#include <QDateTime>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QTableWidgetItem>
#include <QSizePolicy>
#include <QFileInfo>
#include <QMessageBox>
#include <QUrl>
#include <QPainter>

ReviewWidget::ReviewWidget(QWidget *parent, PageRole role)
    : QWidget{parent}, pageRole(role)
{
    m_videoThumbnail.load(
        QStringLiteral(":/images/image-fill.png"));
    m_blackMonitorImage.load(
        QStringLiteral(":/images/black_monitor.jpeg"));

    if (m_videoThumbnail.isNull()) {
        m_videoThumbnail = QPixmap(1, 1);
        m_videoThumbnail.fill(Qt::gray);
    }

    if (m_blackMonitorImage.isNull()) {
        m_blackMonitorImage = QPixmap(1, 1);
        m_blackMonitorImage.fill(Qt::black);
    }

    m_mainLayout = new QHBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(12);

    initLeftArea();
    initRightArea();
    initConnect();

    // 整体左 : 右 = 3 : 7
    m_mainLayout->addWidget(m_leftWidget, 3);
    m_mainLayout->addWidget(m_rightWidget, 7);

    m_previewModeWidget->setVisible(pageRole == VideoReview);
    setPreviewMode(pageRole == SnapshotReview ? ImagePreview : VideoPreview);
    updateVideoPagination();
    updateImagePagination();
}

void ReviewWidget::initLeftArea()
{
    m_leftWidget = new QWidget(this);
    m_leftLayout = new QVBoxLayout(m_leftWidget);
    m_leftLayout->setContentsMargins(0, 0, 0, 0);
    m_leftLayout->setSpacing(8);

    m_filterWidget = new QWidget(m_leftWidget);
    m_filterLayout = new QGridLayout(m_filterWidget);
    m_filterLayout->setContentsMargins(0, 0, 0, 0);
    m_filterLayout->setHorizontalSpacing(6);
    m_filterLayout->setVerticalSpacing(2);

    m_dateTimeEdit = new QDateTimeEdit(
        QDateTime::currentDateTime(),
        m_filterWidget);
    m_dateTimeEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    m_dateTimeEdit->setCalendarPopup(true);

    startTimeEdit = new QTimeEdit(QTime(0, 0), m_filterWidget);
    startTimeEdit->setDisplayFormat(QStringLiteral("HH:mm:ss"));

    endTimeEdit = new QTimeEdit(QTime(23, 59, 59), m_filterWidget);
    endTimeEdit->setDisplayFormat(QStringLiteral("HH:mm:ss"));

    m_channelComboBox = new QComboBox(m_filterWidget);
    m_channelComboBox->addItem(QStringLiteral("全部通道"), 0);
    m_channelComboBox->addItem(QStringLiteral("1通道"), 1);
    m_channelComboBox->addItem(QStringLiteral("2通道"), 2);
    m_channelComboBox->addItem(QStringLiteral("3通道"), 3);
    m_channelComboBox->addItem(QStringLiteral("4通道"), 4);

    recordTypeComboBox = new QComboBox(m_filterWidget);
    recordTypeComboBox->addItem(QStringLiteral("普通录像"), QStringLiteral("normal"));
    recordTypeComboBox->addItem(QStringLiteral("移动目标录像"), QStringLiteral("motion"));
    recordTypeComboBox->setEnabled(false);

    searchButton = new QPushButton(QStringLiteral("查询"), m_filterWidget);
    searchButton->setToolTip(QStringLiteral("打开页面默认显示全部普通录像；点击查询后应用所选日期、时间和通道"));

    m_filterLayout->addWidget(new QLabel(QStringLiteral("日期"), m_filterWidget), 0, 0);
    m_filterLayout->addWidget(new QLabel(QStringLiteral("开始时间"), m_filterWidget), 0, 1);
    m_filterLayout->addWidget(new QLabel(QStringLiteral("结束时间"), m_filterWidget), 0, 2);
    m_filterLayout->addWidget(m_dateTimeEdit, 1, 0);
    m_filterLayout->addWidget(startTimeEdit, 1, 1);
    m_filterLayout->addWidget(endTimeEdit, 1, 2);

    m_filterLayout->addWidget(new QLabel(QStringLiteral("通道"), m_filterWidget), 2, 0);
    QLabel *recordTypeLabel = new QLabel(QStringLiteral("录像类型"), m_filterWidget);
    m_filterLayout->addWidget(recordTypeLabel, 2, 1);
    recordTypeLabel->setVisible(pageRole == VideoReview);
    recordTypeComboBox->setVisible(pageRole == VideoReview);
    m_filterLayout->addWidget(m_channelComboBox, 3, 0);
    m_filterLayout->addWidget(recordTypeComboBox, 3, 1);
    m_filterLayout->addWidget(searchButton, 3, 2);

    m_leftContentWidget = new QWidget(m_leftWidget);
    m_leftContentStack = new QStackedLayout(m_leftContentWidget);
    m_leftContentStack->setContentsMargins(0, 0, 0, 0);

    initVideoList();
    initTimeRanges();

    m_leftContentStack->addWidget(m_videoTable);
    m_leftContentStack->addWidget(m_timeRangeWidget);

    m_leftBottomWidget = new QWidget(m_leftWidget);
    m_leftBottomStack = new QStackedLayout(m_leftBottomWidget);
    m_leftBottomStack->setContentsMargins(0, 0, 0, 0);

    m_videoPagerWidget = new QWidget(m_leftBottomWidget);
    m_videoPagerLayout = new QHBoxLayout(m_videoPagerWidget);
    m_videoPagerLayout->setContentsMargins(0, 0, 0, 0);
    m_videoPagerLayout->setSpacing(8);

    m_videoPreviousButton = new QPushButton(
        QStringLiteral("上一页"),
        m_videoPagerWidget);

    m_videoPageLabel = new QLabel(m_videoPagerWidget);
    m_videoPageLabel->setObjectName(QStringLiteral("pageIndicator"));
    m_videoPageLabel->setAlignment(Qt::AlignCenter);

    m_videoNextButton = new QPushButton(
        QStringLiteral("下一页"),
        m_videoPagerWidget);

    m_videoPagerLayout->addWidget(m_videoPreviousButton);
    m_videoPagerLayout->addWidget(m_videoPageLabel, 1);
    m_videoPagerLayout->addWidget(m_videoNextButton);

    m_emptyLeftBottomWidget = new QWidget(m_leftBottomWidget);

    m_leftBottomStack->addWidget(m_videoPagerWidget);
    m_leftBottomStack->addWidget(m_emptyLeftBottomWidget);

    m_leftLayout->addWidget(m_filterWidget, 3);
    m_leftLayout->addWidget(m_leftContentWidget, 9);
    m_leftLayout->addWidget(m_leftBottomWidget, 1);
}

void ReviewWidget::initVideoList()
{
    m_videoTable = new QTableWidget(0, 2);

    m_videoTable->setHorizontalHeaderLabels({
        QStringLiteral("视频截图"),
        QStringLiteral("视频名称")
    });

    m_videoTable->horizontalHeader()->setVisible(false);
    m_videoTable->verticalHeader()->setVisible(false);
    m_videoTable->horizontalHeader()->setStretchLastSection(true);
    m_videoTable->horizontalHeader()->setMinimumSectionSize(0);
    m_videoTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_videoTable->viewport()->installEventFilter(this);

    m_videoTable->setSelectionBehavior(
        QAbstractItemView::SelectRows);
    m_videoTable->setSelectionMode(
        QAbstractItemView::SingleSelection);
    m_videoTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers);
}

void ReviewWidget::initTimeRanges()
{
    m_timeRangeWidget = new QWidget(m_leftContentWidget);
    m_timeRangeLayout =
        new QVBoxLayout(m_timeRangeWidget);
    m_timeRangeLayout->setContentsMargins(0, 0, 0, 0);
    m_timeRangeLayout->setSpacing(4);

    m_timeRangeGroup = new QButtonGroup(this);
    m_timeRangeGroup->setExclusive(true);

    // 00:00 到 16:00 共960分钟，平均分为15段，每段64分钟
    const QTime beginning(0, 0);
    constexpr int intervalMinutes = 64;

    for (int i = 0; i < TimeRangeCount; ++i) {
        const QTime startTime =
            beginning.addSecs(i * intervalMinutes * 60);
        const QTime endTime =
            beginning.addSecs((i + 1) * intervalMinutes * 60);

        QPushButton *button = new QPushButton(
            QStringLiteral("%1 - %2")
                .arg(startTime.toString("HH:mm"),
                     endTime.toString("HH:mm")),
            m_timeRangeWidget);

        button->setCheckable(true);
        button->setProperty(
            "startTime", startTime.toString("HH:mm"));
        button->setProperty(
            "endTime", endTime.toString("HH:mm"));

        m_timeRangeButtons[i] = button;
        m_timeRangeGroup->addButton(button, i);
        m_timeRangeLayout->addWidget(button);
    }

    m_timeRangeButtons[0]->setChecked(true);
}

void ReviewWidget::initRightArea()
{
    m_rightWidget = new QWidget(this);
    m_rightLayout = new QVBoxLayout(m_rightWidget);
    m_rightLayout->setContentsMargins(0, 0, 0, 0);
    m_rightLayout->setSpacing(8);

    // 视频预览和图片预览切换按钮
    m_previewModeWidget = new QWidget(m_rightWidget);
    m_previewModeLayout =
        new QHBoxLayout(m_previewModeWidget);
    m_previewModeLayout->setContentsMargins(0, 0, 0, 0);
    m_previewModeLayout->setSpacing(8);

    m_videoPreviewButton = new QPushButton(
        QStringLiteral("普通回看"), m_previewModeWidget);
    motionPreviewButton = new QPushButton(
        QStringLiteral("移动侦测回看"), m_previewModeWidget);

    m_videoPreviewButton->setCheckable(true);
    motionPreviewButton->setCheckable(true);

    m_previewModeGroup = new QButtonGroup(this);
    m_previewModeGroup->setExclusive(true);
    m_previewModeGroup->addButton(
        m_videoPreviewButton, VideoPreview);
    m_previewModeGroup->addButton(
        motionPreviewButton, MotionPreview);

    m_previewModeLayout->addStretch();
    m_previewModeLayout->addWidget(m_videoPreviewButton);
    m_previewModeLayout->addWidget(motionPreviewButton);
    m_previewModeLayout->addStretch();

    // 视频页面和图片页面的堆栈
    m_previewContentWidget = new QWidget(m_rightWidget);
    m_previewContentStack =
        new QStackedLayout(m_previewContentWidget);
    m_previewContentStack->setContentsMargins(0, 0, 0, 0);

    initVideoPreview();
    initImagePreview();

    m_previewContentStack->addWidget(m_videoPreviewPage);
    m_previewContentStack->addWidget(m_imagePreviewPage);

    // 顶部模式按钮 : 下部预览区域 = 1 : 10
    // 预览页内部再按9 : 1划分内容和操作区
    m_rightLayout->addWidget(m_previewModeWidget, 1);
    m_rightLayout->addWidget(m_previewContentWidget, 10);
}

void ReviewWidget::initVideoPreview()
{
    m_videoPreviewPage = new QWidget(m_previewContentWidget);
    m_videoPreviewLayout = new QVBoxLayout(m_videoPreviewPage);
    m_videoPreviewLayout->setContentsMargins(0, 0, 0, 0);
    m_videoPreviewLayout->setSpacing(8);

    videoWidget = new QVideoWidget(m_videoPreviewPage);
    videoWidget->setAspectRatioMode(Qt::KeepAspectRatio);

    mediaPlayer = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this);
    mediaPlayer->setAudioOutput(audioOutput);
    mediaPlayer->setVideoOutput(videoWidget);

    m_videoControlsWidget = new QWidget(m_videoPreviewPage);
    m_videoControlsLayout = new QVBoxLayout(m_videoControlsWidget);
    m_videoControlsLayout->setContentsMargins(0, 0, 0, 0);
    m_videoControlsLayout->setSpacing(6);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->setSpacing(8);

    playButton = new QPushButton(QStringLiteral("播放"), m_videoControlsWidget);
    m_pauseButton = new QPushButton(QStringLiteral("暂停"), m_videoControlsWidget);
    continueButton = new QPushButton(QStringLiteral("继续"), m_videoControlsWidget);
    stopButton = new QPushButton(QStringLiteral("停止"), m_videoControlsWidget);
    m_screenshotButton = new QPushButton(QStringLiteral("截图"), m_videoControlsWidget);

    m_speedComboBox = new QComboBox(m_videoControlsWidget);
    m_speedComboBox->addItem(QStringLiteral("1倍速"), 1.0);
    m_speedComboBox->addItem(QStringLiteral("2倍速"), 2.0);
    m_speedComboBox->addItem(QStringLiteral("4倍速"), 4.0);

    buttonLayout->addWidget(playButton);
    buttonLayout->addWidget(m_pauseButton);
    buttonLayout->addWidget(continueButton);
    buttonLayout->addWidget(stopButton);
    buttonLayout->addWidget(m_screenshotButton);
    buttonLayout->addWidget(m_speedComboBox);
    buttonLayout->addStretch();

    QHBoxLayout *progressLayout = new QHBoxLayout;
    positionLabel = new QLabel(QStringLiteral("00:00"), m_videoControlsWidget);
    progressSlider = new QSlider(Qt::Horizontal, m_videoControlsWidget);
    progressSlider->setRange(0, 0);
    durationLabel = new QLabel(QStringLiteral("00:00"), m_videoControlsWidget);

    progressLayout->addWidget(positionLabel);
    progressLayout->addWidget(progressSlider, 1);
    progressLayout->addWidget(durationLabel);

    m_videoControlsLayout->addLayout(buttonLayout);
    m_videoControlsLayout->addLayout(progressLayout);

    m_videoPreviewLayout->addWidget(videoWidget, 9);
    m_videoPreviewLayout->addWidget(m_videoControlsWidget, 2);
}

void ReviewWidget::initImagePreview()
{
    m_imagePreviewPage =
        new QWidget(m_previewContentWidget);
    m_imagePreviewLayout =
        new QVBoxLayout(m_imagePreviewPage);
    m_imagePreviewLayout->setContentsMargins(0, 0, 0, 0);
    m_imagePreviewLayout->setSpacing(8);

    m_imageGridWidget =
        new QWidget(m_imagePreviewPage);
    m_imageGridLayout =
        new QGridLayout(m_imageGridWidget);
    m_imageGridLayout->setContentsMargins(0, 0, 0, 0);
    m_imageGridLayout->setHorizontalSpacing(8);
    m_imageGridLayout->setVerticalSpacing(8);

    for (int row = 0; row < 3; ++row) {
        m_imageGridLayout->setRowStretch(row, 1);
        m_imageGridLayout->setColumnStretch(row, 1);
    }

    for (int i = 0; i < ImagePageSize; ++i) {
        QWidget *cellWidget = new QWidget(m_imageGridWidget);
        cellWidget->setMinimumSize(0, 0);
        cellWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        QVBoxLayout *cellLayout = new QVBoxLayout(cellWidget);
        cellLayout->setContentsMargins(2, 2, 2, 2);
        cellLayout->setSpacing(2);
        QLabel *imageLabel = new QLabel(cellWidget);
        imageLabel->setAlignment(Qt::AlignCenter);
        imageLabel->setScaledContents(false);
        imageLabel->setMinimumSize(0, 0);

        imageLabel->setSizePolicy(
            QSizePolicy::Ignored,
            QSizePolicy::Ignored);

        m_imageLabels[i] = imageLabel;
        QCheckBox *selectionCheck = new QCheckBox(cellWidget);
        selectionCheck->setEnabled(false);
        imageSelectionChecks[i] = selectionCheck;
        cellLayout->addWidget(imageLabel, 1);
        cellLayout->addWidget(selectionCheck);
        m_imageGridLayout->addWidget(
            cellWidget, i / 3, i % 3);
    }

    m_imagePagerWidget =
        new QWidget(m_imagePreviewPage);
    m_imagePagerLayout =
        new QHBoxLayout(m_imagePagerWidget);
    m_imagePagerLayout->setContentsMargins(0, 0, 0, 0);
    m_imagePagerLayout->setSpacing(8);

    m_imagePreviousButton = new QPushButton(
        QStringLiteral("上一页"), m_imagePagerWidget);
    m_imagePageLabel = new QLabel(m_imagePagerWidget);
    m_imagePageLabel->setObjectName(QStringLiteral("pageIndicator"));
    m_imagePageLabel->setAlignment(Qt::AlignCenter);
    m_imageNextButton = new QPushButton(
        QStringLiteral("下一页"), m_imagePagerWidget);

    m_imagePagerLayout->addStretch();
    m_imagePagerLayout->addWidget(m_imagePreviousButton);
    m_imagePagerLayout->addWidget(m_imagePageLabel);
    m_imagePagerLayout->addWidget(m_imageNextButton);
    m_imagePagerLayout->addStretch();
    viewSnapshotButton = new QPushButton(QStringLiteral("查看大图"), m_imagePagerWidget);
    deleteSnapshotsButton = new QPushButton(QStringLiteral("删除所选"), m_imagePagerWidget);
    exportSnapshotsButton = new QPushButton(QStringLiteral("导出所选"), m_imagePagerWidget);
    m_imagePagerLayout->addWidget(viewSnapshotButton);
    m_imagePagerLayout->addWidget(deleteSnapshotsButton);
    m_imagePagerLayout->addWidget(exportSnapshotsButton);

    // 图片网格 : 分页栏 = 9 : 1
    m_imagePreviewLayout->addWidget(m_imageGridWidget, 9);
    m_imagePreviewLayout->addWidget(m_imagePagerWidget, 1);

}

void ReviewWidget::initConnect()
{
    connect(m_videoPreviewButton, &QPushButton::clicked, this, &ReviewWidget::openOrdinaryPlayback);

    connect(motionPreviewButton, &QPushButton::clicked, this, &ReviewWidget::openMotionPlayback);

    connect(searchButton, &QPushButton::clicked, this, &ReviewWidget::searchCurrentContent);

    connect(m_videoTable, &QTableWidget::cellClicked, this, [this](int row, int) {
        playVideoAtRow(row);
    });

    connect(m_videoPreviousButton, &QPushButton::clicked, this, [this]() {
        if (m_videoCurrentPage > 1) {
            --m_videoCurrentPage;
            loadVideoPage();
        }
    });

    connect(m_videoNextButton, &QPushButton::clicked, this, [this]() {
        if (m_videoCurrentPage < m_videoTotalPages) {
            ++m_videoCurrentPage;
            loadVideoPage();
        }
    });

    connect(m_imagePreviousButton, &QPushButton::clicked, this, [this]() {
        if (m_imageCurrentPage > 1) {
            --m_imageCurrentPage;
            updateImagePagination();
        }
    });

    connect(m_imageNextButton, &QPushButton::clicked, this, [this]() {
        if (m_imageCurrentPage < m_imageTotalPages) {
            ++m_imageCurrentPage;
            updateImagePagination();
        }
    });

    connect(playButton, &QPushButton::clicked, this, [this]() {
        if (mediaPlayer->source().isEmpty()) {
            playVideoAtRow(m_videoTable->currentRow());
            return;
        }

        mediaPlayer->play();
    });

    connect(m_pauseButton, &QPushButton::clicked, mediaPlayer, &QMediaPlayer::pause);
    connect(continueButton, &QPushButton::clicked, mediaPlayer, &QMediaPlayer::play);
    connect(stopButton, &QPushButton::clicked, mediaPlayer, &QMediaPlayer::stop);

    connect(m_speedComboBox, &QComboBox::currentIndexChanged, this, [this](int) {
        mediaPlayer->setPlaybackRate(m_speedComboBox->currentData().toDouble());
    });

    connect(mediaPlayer, &QMediaPlayer::durationChanged, this, [this](qint64 duration) {
        progressSlider->setRange(0, static_cast<int>(duration));
        durationLabel->setText(formatPlaybackTime(duration));
        applyPendingSeek();
    });

    connect(mediaPlayer, &QMediaPlayer::positionChanged, this, [this](qint64 position) {
        if (!progressSlider->isSliderDown())
            progressSlider->setValue(static_cast<int>(position));

        positionLabel->setText(formatPlaybackTime(position));
    });

    connect(progressSlider, &QSlider::sliderMoved, this, [this](int position) {
        mediaPlayer->setPosition(position);
    });

    connect(progressSlider, &QSlider::sliderReleased, this, [this]() {
        mediaPlayer->setPosition(progressSlider->value());
    });

    connect(mediaPlayer, &QMediaPlayer::errorOccurred, this,
            [this](QMediaPlayer::Error, const QString &errorString) {
                QMessageBox::warning(this, QStringLiteral("播放失败"), errorString);
            });

    connect(m_screenshotButton, &QPushButton::clicked,
            this, &ReviewWidget::savePlaybackSnapshot);

    connect(viewSnapshotButton, &QPushButton::clicked, this, &ReviewWidget::viewSelectedSnapshot);
    connect(deleteSnapshotsButton, &QPushButton::clicked,
            this, &ReviewWidget::deleteSelectedSnapshots);
    connect(exportSnapshotsButton, &QPushButton::clicked,
            this, &ReviewWidget::exportSelectedSnapshots);

    for (int index = 0; index < ImagePageSize; ++index) {
        connect(imageSelectionChecks[index], &QCheckBox::toggled,
                this, [this, index](bool checked) {
            const int snapshotIndex = (m_imageCurrentPage - 1) * ImagePageSize + index;
            if (snapshotIndex >= snapshots.size())
                return;
            const qulonglong snapshotId = snapshots.at(snapshotIndex).id;
            if (checked)
                selectedSnapshotIds.insert(snapshotId);
            else
                selectedSnapshotIds.remove(snapshotId);
        });
    }

    connect(videoWidget->videoSink(), &QVideoSink::videoFrameChanged,
            this, [this](const QVideoFrame &frame) {
        if (!frame.isValid())
            return;
        playbackFrame = frame.toImage();
        playbackFramePosition = frame.startTime() >= 0
            ? frame.startTime() / 1000 : mediaPlayer->position();
    });
    connect(mediaPlayer, &QMediaPlayer::mediaStatusChanged,
            this, [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::BufferedMedia)
            applyPendingSeek();
    });
    connect(mediaPlayer, &QMediaPlayer::seekableChanged, this, [this](bool seekable) {
        if (seekable)
            applyPendingSeek();
    });

    for (int i = 0; i < TimeRangeCount; ++i) {
        connect(m_timeRangeButtons[i], &QPushButton::clicked, this, [this, i]() {
            const QTime beginning(0, 0);
            constexpr int intervalMinutes = 64;

            const QTime startTime = beginning.addSecs(i * intervalMinutes * 60);
            const QTime endTime = beginning.addSecs((i + 1) * intervalMinutes * 60);

            startTimeEdit->setTime(startTime);
            endTimeEdit->setTime(endTime.addSecs(-1));
            searchSnapshots();
            emit timeRangeSelected(startTime, endTime);
        });
    }
}

void ReviewWidget::setPreviewMode(PreviewMode mode)
{
    if (mode != m_previewMode)
        mediaPlayer->pause();
    m_previewMode = mode;

    const int pageIndex =
        mode == ImagePreview ? 1 : 0;

    m_leftContentStack->setCurrentIndex(pageIndex);
    m_leftBottomStack->setCurrentIndex(pageIndex);
    m_previewContentStack->setCurrentIndex(pageIndex);
    m_leftContentWidget->setVisible(true);
    m_leftBottomWidget->setVisible(true);
    m_filterWidget->setEnabled(true);

    m_videoPreviewButton->setChecked(
        mode == VideoPreview);
    motionPreviewButton->setChecked(
        mode == MotionPreview);
    recordTypeComboBox->setCurrentIndex(mode == MotionPreview ? 1 : 0);
    recordTypeComboBox->setEnabled(false);
    if (mode == ImagePreview && !currentUsername.isEmpty() && isVisible())
        searchSnapshots();
}

void ReviewWidget::openOrdinaryPlayback()
{
    setPreviewMode(VideoPreview);
    resetVideoQuery();
}

void ReviewWidget::openMotionPlayback()
{
    setPreviewMode(MotionPreview);
    resetVideoQuery();
}

void ReviewWidget::resetVideoQuery()
{
    videoQueryStart = QDateTime();
    videoQueryEnd = QDateTime();
    videoQueryChannel = 0;
    m_channelComboBox->setCurrentIndex(0);
    m_videoCurrentPage = 1;
    loadVideoPage();
}

void ReviewWidget::searchVideos()
{
    videoQueryStart = QDateTime(m_dateTimeEdit->date(), startTimeEdit->time());
    videoQueryEnd = QDateTime(m_dateTimeEdit->date(), endTimeEdit->time());
    if (endTimeEdit->time() < startTimeEdit->time())
        videoQueryEnd = videoQueryEnd.addDays(1);
    videoQueryEnd = videoQueryEnd.addSecs(1);
    videoQueryChannel = m_channelComboBox->currentData().toInt();
    m_videoCurrentPage = 1;
    loadVideoPage();
}

void ReviewWidget::loadVideoPage()
{
    if (currentUsername.isEmpty() || m_previewMode == ImagePreview)
        return;
    QVector<VideoRecordInfo> queriedRecords;
    QString errorMessage;

    int totalCount = 0;
    const bool queryResult = playbackService.searchPage(
        videoQueryStart, videoQueryEnd, videoQueryChannel, m_videoCurrentPage,
        m_videoPageSize, queriedRecords, totalCount, errorMessage,
        m_previewMode == MotionPreview ? QStringLiteral("motion") : QStringLiteral("normal"));

    if (!queryResult) {
        videoRecords.clear();
        videoTotalCount = 0;
        m_videoCurrentPage = 1;
        updateVideoPagination();
        QMessageBox::warning(
            this,
            QStringLiteral("查询失败"),
            errorMessage);
        return;
    }

    videoRecords = queriedRecords;
    videoTotalCount = totalCount;
    updateVideoPagination();
}

void ReviewWidget::playVideoAtRow(int row)
{
    if (row < 0)
        return;

    const int recordIndex = row;

    if (recordIndex < 0 || recordIndex >= videoRecords.size())
        return;

    const VideoRecordInfo &record = videoRecords.at(recordIndex);

    if (!QFileInfo::exists(record.videoPath)) {
        logService.recordOperation(
            currentUsername, record.channelNo, QStringLiteral("playback"),
            QStringLiteral("video"), record.id, false,
            QStringLiteral("回放失败，录像文件不存在：%1").arg(record.videoPath));
        QMessageBox::warning(
            this,
            QStringLiteral("录像文件缺失"),
            QStringLiteral("找不到录像文件：\n%1").arg(record.videoPath));
        return;
    }

    mediaPlayer->stop();
    currentVideo = record;
    playbackFrame = QImage();
    playbackFramePosition = 0;
    pendingSeekPosition = -1;
    mediaPlayer->setSource(QUrl::fromLocalFile(record.videoPath));
    mediaPlayer->play();
    logService.recordOperation(
        currentUsername, record.channelNo, QStringLiteral("playback"),
        QStringLiteral("video"), record.id, true,
        QStringLiteral("开始回放录像：%1").arg(record.videoPath));
}

void ReviewWidget::searchCurrentContent()
{
    if (m_previewMode == ImagePreview)
        searchSnapshots();
    else
        searchVideos();
}

void ReviewWidget::searchSnapshots()
{
    if (currentUsername.isEmpty())
        return;
    const QDate selectedDate = m_dateTimeEdit->date();
    const QDateTime rangeStart(selectedDate, startTimeEdit->time());
    QDateTime rangeEnd(selectedDate, endTimeEdit->time());
    if (endTimeEdit->time() < startTimeEdit->time())
        rangeEnd = rangeEnd.addDays(1);
    // 包含结束时间这一秒内的毫秒。
    rangeEnd = rangeEnd.addSecs(1);
    QVector<SnapshotInfo> queriedSnapshots;
    QString errorMessage;
    const bool searched = snapshotService.searchSnapshots(
        rangeStart, rangeEnd, m_channelComboBox->currentData().toInt(),
        queriedSnapshots, errorMessage);
    if (!searched) {
        QMessageBox::warning(this, QStringLiteral("图片查询失败"), errorMessage);
        return;
    }
    snapshots = queriedSnapshots;
    selectedSnapshotIds.clear();
    m_imageCurrentPage = 1;
    updateImagePagination();
}

void ReviewWidget::updateSnapshotGrid()
{
    const int firstIndex = (m_imageCurrentPage - 1) * ImagePageSize;
    for (int index = 0; index < ImagePageSize; ++index) {
        QLabel *imageLabel = m_imageLabels[index];
        QCheckBox *selectionCheck = imageSelectionChecks[index];
        const int snapshotIndex = firstIndex + index;
        const bool hasSnapshot = snapshotIndex < snapshots.size();
        selectionCheck->blockSignals(true);
        selectionCheck->setEnabled(hasSnapshot);
        if (!hasSnapshot) {
            QPixmap whitePixmap(16, 16);
            whitePixmap.fill(Qt::white);
            imageLabel->setScaledContents(true);
            imageLabel->setPixmap(whitePixmap);
            imageLabel->setToolTip(QString());
            selectionCheck->setText(QString());
            selectionCheck->setChecked(false);
        } else {
            const SnapshotInfo &snapshot = snapshots.at(snapshotIndex);
            const QPixmap imagePixmap(snapshot.snapshotPath);
            imageLabel->setScaledContents(false);
            imageLabel->clear();
            if (imagePixmap.isNull()) {
                imageLabel->setText(QStringLiteral("图片缺失或无法读取"));
            } else {
                const QSize targetSize = imageLabel->size().expandedTo(QSize(1, 1));
                imageLabel->setPixmap(imagePixmap.scaled(
                    targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
            imageLabel->setToolTip(snapshot.snapshotPath);
            selectionCheck->setText(QStringLiteral("通道%1 %2").arg(snapshot.channelNo)
                .arg(snapshot.frameTime.toString(QStringLiteral("HH:mm:ss"))));
            selectionCheck->setChecked(selectedSnapshotIds.contains(snapshot.id));
        }
        selectionCheck->blockSignals(false);
    }
}

QVector<SnapshotInfo> ReviewWidget::selectedSnapshots() const
{
    QVector<SnapshotInfo> selected;
    for (const SnapshotInfo &snapshot : snapshots) {
        if (selectedSnapshotIds.contains(snapshot.id))
            selected.append(snapshot);
    }
    return selected;
}

void ReviewWidget::savePlaybackSnapshot()
{
    if (currentVideo.id == 0 || playbackFrame.isNull()
        || mediaPlayer->playbackState() == QMediaPlayer::StoppedState) {
        logService.recordOperation(
            currentUsername, currentVideo.channelNo, QStringLiteral("snapshot"),
            QStringLiteral("snapshot"), 0, false,
            QStringLiteral("回放截图失败：没有正在播放或暂停的录像"));
        QMessageBox::warning(this, QStringLiteral("截图失败"),
                            QStringLiteral("请先播放或暂停一段录像，再进行截图"));
        return;
    }
    // 在打开对话框前固定截图与时间，避免播放进度变化后关联错位。
    const QImage image = playbackFrame;
    const VideoRecordInfo record = currentVideo;
    const qint64 offsetMs = qMax(qint64(0), playbackFramePosition);
    const QDateTime frameTime = record.startTime.addMSecs(offsetMs);
    const QString directory =
        QCoreApplication::applicationDirPath() + QStringLiteral("/snapshots");
    if (!QDir().mkpath(directory)) {
        logService.recordOperation(
            currentUsername, record.channelNo, QStringLiteral("snapshot"),
            QStringLiteral("snapshot"), 0, false,
            QStringLiteral("回放截图失败：无法创建默认截图目录"));
        QMessageBox::warning(this, QStringLiteral("截图失败"),
                            QStringLiteral("无法创建默认截图目录"));
        return;
    }
    const QString defaultName = QStringLiteral("回放_通道%1_%2.jpg").arg(record.channelNo)
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss_zzz")));
    QString filePath = QFileDialog::getSaveFileName(
        this, QStringLiteral("保存回放截图"), QDir(directory).filePath(defaultName),
        QStringLiteral("图片文件 (*.jpg *.jpeg *.png)"));
    if (filePath.isEmpty())
        return;
    if (QFileInfo(filePath).suffix().isEmpty())
        filePath += QStringLiteral(".jpg");
    SnapshotInfo snapshot;
    QString errorMessage;
    const bool saved = snapshotService.saveManualSnapshot(
        image, record.channelNo, filePath, frameTime,
        record.id, offsetMs, snapshot, errorMessage);
    if (!saved) {
        logService.recordOperation(
            currentUsername, record.channelNo, QStringLiteral("snapshot"),
            QStringLiteral("snapshot"), 0, false,
            QStringLiteral("回放截图失败：%1").arg(errorMessage));
        QMessageBox::warning(this, QStringLiteral("截图失败"), errorMessage);
        return;
    }
    logService.recordOperation(
        currentUsername, record.channelNo, QStringLiteral("snapshot"),
        QStringLiteral("snapshot"), snapshot.id, true,
        QStringLiteral("保存回放截图：%1").arg(snapshot.snapshotPath));
    QMessageBox::information(this, QStringLiteral("截图成功"),
                            QStringLiteral("图片已保存：\n%1").arg(snapshot.snapshotPath));
}

void ReviewWidget::deleteSelectedSnapshots()
{
    const QVector<SnapshotInfo> selected = selectedSnapshots();
    if (selected.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("删除图片"),
                                 QStringLiteral("请先勾选需要删除的图片"));
        return;
    }
    const QMessageBox::StandardButton result = QMessageBox::question(
        this, QStringLiteral("确认删除"),
        QStringLiteral("确定删除选中的%1张图片及其数据库记录吗？").arg(selected.size()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (result != QMessageBox::Yes)
        return;
    int deletedCount = 0;
    for (const SnapshotInfo &snapshot : selected) {
        QString errorMessage;
        if (!snapshotService.deleteSnapshot(snapshot, errorMessage)) {
            logService.recordOperation(
                currentUsername, 0, QStringLiteral("delete"),
                QStringLiteral("snapshot"), 0, false,
                QStringLiteral("删除图片失败，已删除%1张：%2")
                    .arg(deletedCount).arg(errorMessage));
            QMessageBox::warning(this, QStringLiteral("删除未全部完成"),
                QStringLiteral("已删除%1张。\n%2").arg(deletedCount).arg(errorMessage));
            searchSnapshots();
            return;
        }
        ++deletedCount;
    }
    logService.recordOperation(
        currentUsername, 0, QStringLiteral("delete"),
        QStringLiteral("snapshot"), 0, true,
        QStringLiteral("批量删除图片，共%1张").arg(deletedCount));
    searchSnapshots();
}

void ReviewWidget::exportSelectedSnapshots()
{
    const QVector<SnapshotInfo> selected = selectedSnapshots();
    if (selected.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("导出图片"),
                                 QStringLiteral("请先勾选需要导出的图片"));
        return;
    }
    const QString directory = QFileDialog::getExistingDirectory(
        this, QStringLiteral("选择导出目录或设备路径"));
    if (directory.isEmpty())
        return;
    QString errorMessage;
    if (!snapshotService.exportSnapshots(selected, directory, errorMessage)) {
        logService.recordOperation(
            currentUsername, 0, QStringLiteral("export"),
            QStringLiteral("snapshot"), 0, false,
            QStringLiteral("导出图片失败：%1").arg(errorMessage));
        QMessageBox::warning(this, QStringLiteral("导出未全部完成"), errorMessage);
        return;
    }
    logService.recordOperation(
        currentUsername, 0, QStringLiteral("export"),
        QStringLiteral("snapshot"), 0, true,
        QStringLiteral("导出%1张图片到：%2").arg(selected.size()).arg(directory));
    QMessageBox::information(this, QStringLiteral("导出成功"),
        QStringLiteral("已导出%1张图片到：\n%2").arg(selected.size()).arg(directory));
}

void ReviewWidget::viewSelectedSnapshot()
{
    largeSnapshotIndex = -1;
    for (int index = 0; index < snapshots.size(); ++index) {
        if (selectedSnapshotIds.contains(snapshots.at(index).id)) {
            largeSnapshotIndex = index;
            break;
        }
    }
    if (largeSnapshotIndex < 0) {
        QMessageBox::information(this, QStringLiteral("查看大图"),
                                 QStringLiteral("请先勾选一张图片"));
        return;
    }
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("图片预览"));
    dialog.resize(1000, 750);
    snapshotDialog = &dialog;
    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
    snapshotScene = new QGraphicsScene(&dialog);
    snapshotView = new QGraphicsView(snapshotScene, &dialog);
    snapshotView->setDragMode(QGraphicsView::ScrollHandDrag);
    snapshotView->setRenderHint(QPainter::SmoothPixmapTransform);
    snapshotView->setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    snapshotMetadataLabel = new QLabel(&dialog);
    snapshotMetadataLabel->setWordWrap(true);
    snapshotMetadataLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    QHBoxLayout *controlLayout = new QHBoxLayout;
    previousSnapshotButton = new QPushButton(QStringLiteral("上一张"), &dialog);
    nextSnapshotButton = new QPushButton(QStringLiteral("下一张"), &dialog);
    relatedVideoButton = new QPushButton(QStringLiteral("跳转关联录像"), &dialog);
    snapshotZoomSlider = new QSlider(Qt::Horizontal, &dialog);
    snapshotZoomSlider->setRange(50, 200);
    snapshotZoomSlider->setValue(100);
    QPushButton *closeButton = new QPushButton(QStringLiteral("关闭"), &dialog);
    controlLayout->addWidget(previousSnapshotButton);
    controlLayout->addWidget(nextSnapshotButton);
    controlLayout->addWidget(new QLabel(QStringLiteral("缩放 0.5—2倍"), &dialog));
    controlLayout->addWidget(snapshotZoomSlider, 1);
    controlLayout->addWidget(relatedVideoButton);
    controlLayout->addWidget(closeButton);
    mainLayout->addWidget(snapshotView, 1);
    mainLayout->addWidget(snapshotMetadataLabel);
    mainLayout->addLayout(controlLayout);
    connect(previousSnapshotButton, &QPushButton::clicked,
            this, &ReviewWidget::showPreviousSnapshot);
    connect(nextSnapshotButton, &QPushButton::clicked, this, &ReviewWidget::showNextSnapshot);
    connect(snapshotZoomSlider, &QSlider::valueChanged, this, &ReviewWidget::updateSnapshotZoom);
    connect(relatedVideoButton, &QPushButton::clicked, this, &ReviewWidget::playRelatedVideo);
    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    showLargeSnapshot();
    dialog.exec();
    snapshotDialog = nullptr;
    snapshotView = nullptr;
    snapshotScene = nullptr;
    snapshotZoomSlider = nullptr;
    snapshotMetadataLabel = nullptr;
    previousSnapshotButton = nullptr;
    nextSnapshotButton = nullptr;
    relatedVideoButton = nullptr;
}

void ReviewWidget::showLargeSnapshot()
{
    if (largeSnapshotIndex < 0 || largeSnapshotIndex >= snapshots.size())
        return;
    const SnapshotInfo &snapshot = snapshots.at(largeSnapshotIndex);
    const QPixmap image(snapshot.snapshotPath);
    const QFileInfo fileInfo(snapshot.snapshotPath);
    snapshotScene->clear();
    if (image.isNull()) {
        snapshotScene->addText(QStringLiteral("图片缺失或无法读取"));
        snapshotScene->setSceneRect(snapshotScene->itemsBoundingRect());
    } else {
        snapshotScene->addPixmap(image);
        snapshotScene->setSceneRect(QRectF(image.rect()));
    }
    snapshotZoomSlider->setValue(100);
    updateSnapshotZoom(100);
    snapshotView->centerOn(snapshotScene->sceneRect().center());
    const QString typeName = snapshot.snapshotType == QStringLiteral("auto")
        ? QStringLiteral("自动截图") : QStringLiteral("手动截图");
    QString metadata = QStringLiteral("图片 %1/%2 | ID：%3 | 通道：%4 | 类型：%5")
        .arg(largeSnapshotIndex + 1).arg(snapshots.size()).arg(snapshot.id)
        .arg(snapshot.channelNo).arg(typeName);
    metadata += QStringLiteral("\n画面时间：%1 | 保存时间：%2")
        .arg(snapshot.frameTime.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")))
        .arg(snapshot.createTime.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")));
    metadata += QStringLiteral("\n尺寸：%1×%2 | 大小：%3 KB | 事件ID：%4")
        .arg(image.width()).arg(image.height()).arg(fileInfo.size() / 1024.0, 0, 'f', 1)
        .arg(snapshot.eventId);
    metadata += QStringLiteral("\n关联录像ID：%1 | 偏移：%2 ms\n图片路径：%3")
        .arg(snapshot.videoId).arg(snapshot.offsetMs).arg(snapshot.snapshotPath);
    snapshotMetadataLabel->setText(metadata);
    previousSnapshotButton->setEnabled(largeSnapshotIndex > 0);
    nextSnapshotButton->setEnabled(largeSnapshotIndex + 1 < snapshots.size());
    relatedVideoButton->setEnabled(
        snapshot.videoId != 0 && snapshot.offsetMs >= 0 && !snapshot.videoPath.isEmpty());
}

void ReviewWidget::showPreviousSnapshot()
{
    if (largeSnapshotIndex > 0) {
        --largeSnapshotIndex;
        showLargeSnapshot();
    }
}

void ReviewWidget::showNextSnapshot()
{
    if (largeSnapshotIndex + 1 < snapshots.size()) {
        ++largeSnapshotIndex;
        showLargeSnapshot();
    }
}

void ReviewWidget::updateSnapshotZoom(int value)
{
    if (snapshotView == nullptr)
        return;
    const double scaleValue = qBound(50, value, 200) / 100.0;
    snapshotView->resetTransform();
    snapshotView->scale(scaleValue, scaleValue);
    snapshotView->setToolTip(
        QStringLiteral("当前缩放：%1%，按住鼠标左键拖动画面").arg(value));
}

void ReviewWidget::playRelatedVideo()
{
    if (largeSnapshotIndex < 0 || largeSnapshotIndex >= snapshots.size())
        return;
    const SnapshotInfo snapshot = snapshots.at(largeSnapshotIndex);
    if (snapshot.videoId == 0 || snapshot.offsetMs < 0 || snapshot.videoPath.isEmpty()) {
        QMessageBox::information(snapshotDialog, QStringLiteral("关联录像"),
                                 QStringLiteral("这张图片没有关联录像"));
        return;
    }
    if (!QFileInfo::exists(snapshot.videoPath)) {
        QMessageBox::warning(snapshotDialog, QStringLiteral("录像文件缺失"),
            QStringLiteral("找不到关联录像：\n%1").arg(snapshot.videoPath));
        return;
    }
    if (snapshotDialog != nullptr)
        snapshotDialog->accept();
    emit relatedVideoRequested(snapshot.videoId, snapshot.offsetMs);
}

void ReviewWidget::applyPendingSeek()
{
    if (pendingSeekPosition < 0 || !mediaPlayer->isSeekable() || mediaPlayer->duration() <= 0)
        return;
    const QMediaPlayer::MediaStatus status = mediaPlayer->mediaStatus();
    if (status != QMediaPlayer::LoadedMedia && status != QMediaPlayer::BufferingMedia
        && status != QMediaPlayer::BufferedMedia && status != QMediaPlayer::EndOfMedia)
        return;
    const qint64 targetPosition = qMin(pendingSeekPosition, mediaPlayer->duration() - 1);
    pendingSeekPosition = -1;
    mediaPlayer->setPosition(targetPosition);
}

QString ReviewWidget::formatPlaybackTime(qint64 milliseconds) const
{
    const qint64 totalSeconds = milliseconds / 1000;
    const qint64 minutes = totalSeconds / 60;
    const qint64 seconds = totalSeconds % 60;

    return QStringLiteral("%1:%2")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}

void ReviewWidget::updateVideoPagination()
{
    const int videoCount = videoTotalCount;

    m_videoTotalPages = qMax(
        1,
        (videoCount + m_videoPageSize - 1) / m_videoPageSize);

    if (m_videoCurrentPage > m_videoTotalPages)
        m_videoCurrentPage = m_videoTotalPages;

    m_videoPageLabel->setText(
        QStringLiteral("%1/%2").arg(m_videoCurrentPage).arg(m_videoTotalPages));

    m_videoPreviousButton->setEnabled(m_videoCurrentPage > 1);
    m_videoNextButton->setEnabled(m_videoCurrentPage < m_videoTotalPages);

    const int currentPageVideoCount = static_cast<int>(videoRecords.size());

    m_videoTable->setRowCount(0);
    m_videoTable->setRowCount(currentPageVideoCount);

    for (int row = 0; row < currentPageVideoCount; ++row) {
        const VideoRecordInfo &record = videoRecords.at(row);

        QLabel *thumbnailLabel = new QLabel(m_videoTable);
        thumbnailLabel->setAlignment(Qt::AlignCenter);
        thumbnailLabel->setScaledContents(true);
        thumbnailLabel->setMinimumSize(0, 0);
        thumbnailLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        thumbnailLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        thumbnailLabel->setPixmap(m_videoThumbnail);

        m_videoTable->setCellWidget(row, 0, thumbnailLabel);

        const QString typeName =
            record.recordType == QStringLiteral("motion")
                ? QStringLiteral("移动侦测")
                : QStringLiteral("普通录像");

        QTableWidgetItem *videoItem = new QTableWidgetItem(
            QStringLiteral("通道%1 | %2 | %3")
                .arg(record.channelNo)
                .arg(typeName)
                .arg(record.startTime.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))));

        videoItem->setToolTip(record.videoPath);
        m_videoTable->setItem(row, 1, videoItem);
        m_videoTable->setRowHeight(row, 90);
    }
}

void ReviewWidget::updateImagePagination()
{
    const int imageCount = static_cast<int>(snapshots.size());
    m_imageTotalPages = qMax(1, (imageCount + ImagePageSize - 1) / ImagePageSize);
    m_imageCurrentPage = qBound(1, m_imageCurrentPage, m_imageTotalPages);
    m_imagePageLabel->setText(
        QStringLiteral("%1/%2")
            .arg(m_imageCurrentPage)
            .arg(m_imageTotalPages));

    m_imagePreviousButton->setEnabled(
        m_imageCurrentPage > 1);
    m_imageNextButton->setEnabled(
        m_imageCurrentPage < m_imageTotalPages);

    updateSnapshotGrid();
}

bool ReviewWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_videoTable->viewport() && event->type() == QEvent::Resize)
        updateVideoColumnWidths();
    return QWidget::eventFilter(watched, event);
}

void ReviewWidget::updateVideoColumnWidths()
{
    // 截图列 : 名称列 = 2 : 8，跟随表格实际可用宽度调整。
    const int availableWidth = m_videoTable->viewport()->width();
    if (availableWidth > 0)
        m_videoTable->setColumnWidth(0, availableWidth * 2 / 10);
}

void ReviewWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    updateVideoColumnWidths();
    if (m_previewMode == ImagePreview)
        updateSnapshotGrid();
}

void ReviewWidget::setCurrentUser(const QString &username)
{
    currentUsername = username.trimmed();
    playbackService.setCurrentUser(currentUsername);
    snapshotService.setCurrentUser(currentUsername);
    mediaPlayer->stop();
    mediaPlayer->setSource(QUrl());
    pendingSeekPosition = -1;
    relatedPlaybackPending = false;
    playbackFrame = QImage();
    currentVideo = VideoRecordInfo();
    videoRecords.clear();
    videoTotalCount = 0;
    snapshots.clear();
    selectedSnapshotIds.clear();
    m_videoCurrentPage = 1;
    m_imageCurrentPage = 1;
    updateVideoPagination();
    updateImagePagination();
}

bool ReviewWidget::playRecord(qulonglong recordId, qint64 offsetMs)
{
    VideoRecordInfo record;
    QString errorMessage;
    if (!playbackService.findRecordById(recordId, record, errorMessage)) {
        logService.recordOperation(
            currentUsername, 0, QStringLiteral("playback"),
            QStringLiteral("video"), recordId, false,
            QStringLiteral("关联录像查询失败：%1").arg(errorMessage));
        QMessageBox::warning(this, QStringLiteral("关联录像"), errorMessage);
        return false;
    }
    if (!QFileInfo::exists(record.videoPath)) {
        logService.recordOperation(
            currentUsername, record.channelNo, QStringLiteral("playback"),
            QStringLiteral("video"), record.id, false,
            QStringLiteral("关联录像文件不存在：%1").arg(record.videoPath));
        QMessageBox::warning(this, QStringLiteral("录像文件缺失"), record.videoPath);
        return false;
    }
    if (record.recordStatus != 1
        || (record.recordType != QStringLiteral("normal") && record.recordType != QStringLiteral("motion"))) {
        logService.recordOperation(
            currentUsername, record.channelNo, QStringLiteral("playback"),
            QStringLiteral("video"), record.id, false,
            QStringLiteral("关联录像尚未完成或类型不支持"));
        QMessageBox::information(this, QStringLiteral("关联录像"),
                                 QStringLiteral("关联录像尚未完成或类型不支持"));
        return false;
    }
    setPreviewMode(record.recordType == QStringLiteral("motion") ? MotionPreview : VideoPreview);
    relatedPlaybackPending = !isVisible();
    mediaPlayer->stop();
    currentVideo = record;
    playbackFrame = QImage();
    playbackFramePosition = 0;
    pendingSeekPosition = qMax(qint64(0), offsetMs);
    mediaPlayer->setSource(QUrl::fromLocalFile(record.videoPath));
    mediaPlayer->play();
    logService.recordOperation(
        currentUsername, record.channelNo, QStringLiteral("playback"),
        QStringLiteral("video"), record.id, true,
        QStringLiteral("从关联记录回放录像：%1，偏移%2毫秒")
            .arg(record.videoPath).arg(pendingSeekPosition));
    return true;
}

void ReviewWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (currentUsername.isEmpty())
        return;
    if (pageRole == SnapshotReview) {
        searchSnapshots();
        return;
    }
    if (relatedPlaybackPending) {
        relatedPlaybackPending = false;
        return;
    }
    // 打开普通回看不使用筛选控件的当前值；查询按钮才应用这些条件。
    openOrdinaryPlayback();
}

void ReviewWidget::hideEvent(QHideEvent *event)
{
    mediaPlayer->pause();
    QWidget::hideEvent(event);
}
