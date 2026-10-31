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
#include <QWheelEvent>

ReviewWidget::ReviewWidget(QWidget *parent, PageRole role)
    : QWidget{parent}, pageRole(role)
{
    // 按页面角色创建普通回放或移动侦测回放界面。
    videoThumbnail.load(
        QStringLiteral(":/images/image-fill.png"));
    blackMonitorImage.load(
        QStringLiteral(":/images/black_monitor.jpeg"));

    if (videoThumbnail.isNull()) {
        videoThumbnail = QPixmap(1, 1);
        videoThumbnail.fill(Qt::gray);
    }

    if (blackMonitorImage.isNull()) {
        blackMonitorImage = QPixmap(1, 1);
        blackMonitorImage.fill(Qt::black);
    }

    mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(12);

    initLeftArea();
    initRightArea();
    initConnect();

    // 整体左 : 右 = 3 : 7
    mainLayout->addWidget(leftWidget, 3);
    mainLayout->addWidget(rightWidget, 7);

    previewModeWidget->setVisible(pageRole == VideoReview);
    setPreviewMode(pageRole == SnapshotReview ? ImagePreview : VideoPreview);
    updateVideoPagination();
    updateImagePagination();
}

void ReviewWidget::initLeftArea()
{
    // 创建回放筛选项、视频列表或图片时间段列表区域。
    leftWidget = new QWidget(this);
    leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);

    filterWidget = new QWidget(leftWidget);
    filterLayout = new QGridLayout(filterWidget);
    filterLayout->setContentsMargins(0, 0, 0, 0);
    filterLayout->setHorizontalSpacing(6);
    filterLayout->setVerticalSpacing(2);

    dateTimeEdit = new QDateTimeEdit(
        QDateTime::currentDateTime(),
        filterWidget);
    dateTimeEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    dateTimeEdit->setCalendarPopup(true);

    startTimeEdit = new QTimeEdit(QTime(0, 0), filterWidget);
    startTimeEdit->setDisplayFormat(QStringLiteral("HH:mm:ss"));

    endTimeEdit = new QTimeEdit(QTime(23, 59, 59), filterWidget);
    endTimeEdit->setDisplayFormat(QStringLiteral("HH:mm:ss"));

    channelComboBox = new QComboBox(filterWidget);
    channelComboBox->addItem(QStringLiteral("全部通道"), 0);
    channelComboBox->addItem(QStringLiteral("1通道"), 1);
    channelComboBox->addItem(QStringLiteral("2通道"), 2);
    channelComboBox->addItem(QStringLiteral("3通道"), 3);
    channelComboBox->addItem(QStringLiteral("4通道"), 4);

    recordTypeComboBox = new QComboBox(filterWidget);
    recordTypeComboBox->addItem(QStringLiteral("普通录像"), QStringLiteral("normal"));
    recordTypeComboBox->addItem(QStringLiteral("移动目标录像"), QStringLiteral("motion"));
    recordTypeComboBox->setEnabled(false);

    searchButton = new QPushButton(QStringLiteral("查询"), filterWidget);
    searchButton->setToolTip(QStringLiteral("打开页面默认显示全部普通录像；点击查询后应用所选日期、时间和通道"));

    filterLayout->addWidget(new QLabel(QStringLiteral("日期"), filterWidget), 0, 0);
    filterLayout->addWidget(new QLabel(QStringLiteral("开始时间"), filterWidget), 0, 1);
    filterLayout->addWidget(new QLabel(QStringLiteral("结束时间"), filterWidget), 0, 2);
    filterLayout->addWidget(dateTimeEdit, 1, 0);
    filterLayout->addWidget(startTimeEdit, 1, 1);
    filterLayout->addWidget(endTimeEdit, 1, 2);

    filterLayout->addWidget(new QLabel(QStringLiteral("通道"), filterWidget), 2, 0);
    QLabel *recordTypeLabel = new QLabel(QStringLiteral("录像类型"), filterWidget);
    filterLayout->addWidget(recordTypeLabel, 2, 1);
    recordTypeLabel->setVisible(pageRole == VideoReview);
    recordTypeComboBox->setVisible(pageRole == VideoReview);
    filterLayout->addWidget(channelComboBox, 3, 0);
    filterLayout->addWidget(recordTypeComboBox, 3, 1);
    filterLayout->addWidget(searchButton, 3, 2);

    leftContentWidget = new QWidget(leftWidget);
    leftContentStack = new QStackedLayout(leftContentWidget);
    leftContentStack->setContentsMargins(0, 0, 0, 0);

    initVideoList();
    initTimeRanges();

    leftContentStack->addWidget(videoTable);
    leftContentStack->addWidget(timeRangeWidget);

    leftBottomWidget = new QWidget(leftWidget);
    leftBottomStack = new QStackedLayout(leftBottomWidget);
    leftBottomStack->setContentsMargins(0, 0, 0, 0);

    videoPagerWidget = new QWidget(leftBottomWidget);
    videoPagerLayout = new QHBoxLayout(videoPagerWidget);
    videoPagerLayout->setContentsMargins(0, 0, 0, 0);
    videoPagerLayout->setSpacing(8);

    videoPreviousButton = new QPushButton(
        QStringLiteral("上一页"),
        videoPagerWidget);

    videoPageLabel = new QLabel(videoPagerWidget);
    videoPageLabel->setObjectName(QStringLiteral("pageIndicator"));
    videoPageLabel->setAlignment(Qt::AlignCenter);

    videoNextButton = new QPushButton(
        QStringLiteral("下一页"),
        videoPagerWidget);

    videoPagerLayout->addWidget(videoPreviousButton);
    videoPagerLayout->addWidget(videoPageLabel, 1);
    videoPagerLayout->addWidget(videoNextButton);

    emptyLeftBottomWidget = new QWidget(leftBottomWidget);

    leftBottomStack->addWidget(videoPagerWidget);
    leftBottomStack->addWidget(emptyLeftBottomWidget);

    leftLayout->addWidget(filterWidget, 3);
    leftLayout->addWidget(leftContentWidget, 9);
    leftLayout->addWidget(leftBottomWidget, 1);
}

void ReviewWidget::initVideoList()
{
    // 设置录像列表列结构及当前页分页控件。
    videoTable = new QTableWidget(0, 2);

    videoTable->setHorizontalHeaderLabels({
        QStringLiteral("视频截图"),
        QStringLiteral("视频名称")
    });

    videoTable->horizontalHeader()->setVisible(false);
    videoTable->verticalHeader()->setVisible(false);
    videoTable->horizontalHeader()->setStretchLastSection(true);
    videoTable->horizontalHeader()->setMinimumSectionSize(0);
    videoTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    videoTable->viewport()->installEventFilter(this);

    videoTable->setSelectionBehavior(
        QAbstractItemView::SelectRows);
    videoTable->setSelectionMode(
        QAbstractItemView::SingleSelection);
    videoTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers);
}

void ReviewWidget::initTimeRanges()
{
    // 建立图片预览使用的时间段筛选按钮。
    timeRangeWidget = new QWidget(leftContentWidget);
    timeRangeLayout =
        new QVBoxLayout(timeRangeWidget);
    timeRangeLayout->setContentsMargins(0, 0, 0, 0);
    timeRangeLayout->setSpacing(4);

    timeRangeGroup = new QButtonGroup(this);
    timeRangeGroup->setExclusive(true);
    timeRangeEmptyLabel = new QLabel(
        QStringLiteral("当前条件下没有图片"), timeRangeWidget);
    timeRangeEmptyLabel->setObjectName(
        QStringLiteral("snapshotTimeRangeEmpty"));
    timeRangeEmptyLabel->setAlignment(Qt::AlignCenter);
    timeRangeLayout->addWidget(timeRangeEmptyLabel);
}

void ReviewWidget::updateTimeRangeButtons()
{
    // 根据当前查询结果显示包含图片的时间段。
    for (QPushButton *button : timeRangeButtons) {
        timeRangeGroup->removeButton(button);
        timeRangeLayout->removeWidget(button);
        delete button;
    }
    timeRangeButtons.clear();

    std::array<int, TimeRangeCount> rangeCounts{};
    for (const SnapshotInfo &snapshot : daySnapshots) {
        const int rangeIndex = snapshot.frameTime.time().hour() / 2;
        if (rangeIndex >= 0 && rangeIndex < TimeRangeCount)
            ++rangeCounts[static_cast<size_t>(rangeIndex)];
    }

    timeRangeEmptyLabel->setVisible(daySnapshots.isEmpty());

    for (int rangeIndex = 0; rangeIndex < TimeRangeCount; ++rangeIndex) {
        if (rangeCounts[static_cast<size_t>(rangeIndex)] == 0)
            continue;

        const QTime startTime(rangeIndex * 2, 0);
        const QTime endTime((rangeIndex + 1) * 2, 0);
        QPushButton *button = new QPushButton(
            QStringLiteral("%1 - %2  (%3)")
                .arg(startTime.toString(QStringLiteral("HH:mm")))
                .arg(endTime.toString(QStringLiteral("HH:mm")))
                .arg(rangeCounts[static_cast<size_t>(rangeIndex)]),
            timeRangeWidget);

        button->setObjectName(QStringLiteral("snapshotTimeRangeButton"));
        button->setCheckable(true);
        button->setProperty("rangeIndex", rangeIndex);
        timeRangeButtons.append(button);
        timeRangeGroup->addButton(button, rangeIndex);
        timeRangeLayout->insertWidget(
            timeRangeLayout->count() - 1, button);
    }

}

void ReviewWidget::showTimeRange(int rangeIndex)
{
    // 选择时间段后更新图片查询的起止时间。
    if (rangeIndex < 0 || rangeIndex >= TimeRangeCount)
        return;

    const QTime startTime(rangeIndex * 2, 0);
    const QTime endTime((rangeIndex + 1) * 2, 0);
    startTimeEdit->setTime(startTime);
    endTimeEdit->setTime(endTime.addSecs(-1));

    snapshots.clear();
    for (const SnapshotInfo &snapshot : daySnapshots) {
        if (snapshot.frameTime.time() >= startTime
            && snapshot.frameTime.time() < endTime) {
            snapshots.append(snapshot);
        }
    }

    selectedSnapshotIds.clear();
    imageCurrentPage = 1;
    updateImagePagination();
    emit timeRangeSelected(startTime, endTime);
}

void ReviewWidget::initRightArea()
{
    // 创建右侧视频/图片预览及其播放控制区。
    rightWidget = new QWidget(this);
    rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);

    // 视频预览和图片预览切换按钮
    previewModeWidget = new QWidget(rightWidget);
    previewModeLayout =
        new QHBoxLayout(previewModeWidget);
    previewModeLayout->setContentsMargins(0, 0, 0, 0);
    previewModeLayout->setSpacing(8);

    videoPreviewButton = new QPushButton(
        QStringLiteral("普通回看"), previewModeWidget);
    motionPreviewButton = new QPushButton(
        QStringLiteral("移动侦测回看"), previewModeWidget);

    videoPreviewButton->setCheckable(true);
    motionPreviewButton->setCheckable(true);

    previewModeGroup = new QButtonGroup(this);
    previewModeGroup->setExclusive(true);
    previewModeGroup->addButton(
        videoPreviewButton, VideoPreview);
    previewModeGroup->addButton(
        motionPreviewButton, MotionPreview);

    previewModeLayout->addStretch();
    previewModeLayout->addWidget(videoPreviewButton);
    previewModeLayout->addWidget(motionPreviewButton);
    previewModeLayout->addStretch();

    // 视频页面和图片页面的堆栈
    previewContentWidget = new QWidget(rightWidget);
    previewContentStack =
        new QStackedLayout(previewContentWidget);
    previewContentStack->setContentsMargins(0, 0, 0, 0);

    initVideoPreview();
    initImagePreview();

    previewContentStack->addWidget(videoPreviewPage);
    previewContentStack->addWidget(imagePreviewPage);

    // 顶部模式按钮 : 下部预览区域 = 1 : 10
    // 预览页内部再按9 : 1划分内容和操作区
    rightLayout->addWidget(previewModeWidget, 1);
    rightLayout->addWidget(previewContentWidget, 10);
}

void ReviewWidget::initVideoPreview()
{
    // 初始化视频显示控件、播放状态和进度控制。
    videoPreviewPage = new QWidget(previewContentWidget);
    videoPreviewLayout = new QVBoxLayout(videoPreviewPage);
    videoPreviewLayout->setContentsMargins(0, 0, 0, 0);
    videoPreviewLayout->setSpacing(8);

    videoWidget = new QVideoWidget(videoPreviewPage);
    videoWidget->setAspectRatioMode(Qt::KeepAspectRatio);

    mediaPlayer = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this);
    mediaPlayer->setAudioOutput(audioOutput);
    mediaPlayer->setVideoOutput(videoWidget);

    videoControlsWidget = new QWidget(videoPreviewPage);
    videoControlsLayout = new QVBoxLayout(videoControlsWidget);
    videoControlsLayout->setContentsMargins(0, 0, 0, 0);
    videoControlsLayout->setSpacing(6);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(8);

    QWidget *videoOperationGroup = new QWidget(videoControlsWidget);
    QHBoxLayout *videoOperationLayout = new QHBoxLayout(videoOperationGroup);
    videoOperationLayout->setContentsMargins(0, 0, 0, 0);
    videoOperationLayout->setSpacing(8);

    playPauseButton = new QPushButton(QStringLiteral("播放"), videoOperationGroup);
    stopButton = new QPushButton(QStringLiteral("停止"), videoOperationGroup);
    speedComboBox = new QComboBox(videoOperationGroup);
    speedComboBox->addItem(QStringLiteral("1倍速"), 1.0);
    speedComboBox->addItem(QStringLiteral("2倍速"), 2.0);
    speedComboBox->addItem(QStringLiteral("4倍速"), 4.0);

    videoOperationLayout->addWidget(playPauseButton);
    videoOperationLayout->addWidget(stopButton);
    videoOperationLayout->addWidget(speedComboBox);

    QWidget *otherOperationGroup = new QWidget(videoControlsWidget);
    QHBoxLayout *otherOperationLayout = new QHBoxLayout(otherOperationGroup);
    otherOperationLayout->setContentsMargins(0, 0, 0, 0);
    otherOperationLayout->setSpacing(8);
    screenshotButton = new QPushButton(QStringLiteral("截图"), otherOperationGroup);
    otherOperationLayout->addWidget(screenshotButton);

    buttonLayout->addWidget(videoOperationGroup);
    buttonLayout->addStretch();
    buttonLayout->addWidget(otherOperationGroup);

    QHBoxLayout *progressLayout = new QHBoxLayout;
    positionLabel = new QLabel(QStringLiteral("00:00"), videoControlsWidget);
    progressSlider = new QSlider(Qt::Horizontal, videoControlsWidget);
    progressSlider->setRange(0, 0);
    durationLabel = new QLabel(QStringLiteral("00:00"), videoControlsWidget);

    progressLayout->addWidget(positionLabel);
    progressLayout->addWidget(progressSlider, 1);
    progressLayout->addWidget(durationLabel);

    videoControlsLayout->addLayout(buttonLayout);
    videoControlsLayout->addLayout(progressLayout);

    videoPreviewLayout->addWidget(videoWidget, 9);
    videoPreviewLayout->addWidget(videoControlsWidget, 2);
}

void ReviewWidget::initImagePreview()
{
    // 初始化3×3缩略图、分页和大图预览控件。
    imagePreviewPage =
        new QWidget(previewContentWidget);
    imagePreviewLayout =
        new QVBoxLayout(imagePreviewPage);
    imagePreviewLayout->setContentsMargins(0, 0, 0, 0);
    imagePreviewLayout->setSpacing(8);

    imageGridWidget =
        new QWidget(imagePreviewPage);
    imageGridLayout =
        new QGridLayout(imageGridWidget);
    imageGridLayout->setContentsMargins(0, 0, 0, 0);
    imageGridLayout->setHorizontalSpacing(8);
    imageGridLayout->setVerticalSpacing(8);

    for (int row = 0; row < 3; ++row) {
        imageGridLayout->setRowStretch(row, 1);
        imageGridLayout->setColumnStretch(row, 1);
    }

    for (int i = 0; i < ImagePageSize; ++i) {
        QWidget *cellWidget = new QWidget(imageGridWidget);
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

        imageLabels[i] = imageLabel;
        QCheckBox *selectionCheck = new QCheckBox(cellWidget);
        selectionCheck->setEnabled(false);
        selectionCheck->setVisible(false);
        imageSelectionChecks[i] = selectionCheck;
        cellLayout->addWidget(imageLabel, 1);
        cellLayout->addWidget(selectionCheck);
        imageGridLayout->addWidget(
            cellWidget, i / 3, i % 3);
    }

    imagePagerWidget =
        new QWidget(imagePreviewPage);
    imagePagerLayout =
        new QHBoxLayout(imagePagerWidget);
    imagePagerLayout->setContentsMargins(0, 0, 0, 0);
    imagePagerLayout->setSpacing(8);

    imagePreviousButton = new QPushButton(
        QStringLiteral("上一页"), imagePagerWidget);
    imagePageLabel = new QLabel(imagePagerWidget);
    imagePageLabel->setObjectName(QStringLiteral("pageIndicator"));
    imagePageLabel->setAlignment(Qt::AlignCenter);
    imageNextButton = new QPushButton(
        QStringLiteral("下一页"), imagePagerWidget);

    imagePagerLayout->addStretch();
    imagePagerLayout->addWidget(imagePreviousButton);
    imagePagerLayout->addWidget(imagePageLabel);
    imagePagerLayout->addWidget(imageNextButton);
    imagePagerLayout->addStretch();
    viewSnapshotButton = new QPushButton(QStringLiteral("查看大图"), imagePagerWidget);
    deleteSnapshotsButton = new QPushButton(QStringLiteral("删除所选"), imagePagerWidget);
    exportSnapshotsButton = new QPushButton(QStringLiteral("导出所选"), imagePagerWidget);
    imagePagerLayout->addWidget(viewSnapshotButton);
    imagePagerLayout->addWidget(deleteSnapshotsButton);
    imagePagerLayout->addWidget(exportSnapshotsButton);

    // 图片网格 : 分页栏 = 9 : 1
    imagePreviewLayout->addWidget(imageGridWidget, 9);
    imagePreviewLayout->addWidget(imagePagerWidget, 1);

}

void ReviewWidget::initConnect()
{
    // 连接查询、分页、预览、截图、删除和导出交互。
    connect(videoPreviewButton, &QPushButton::clicked, this, &ReviewWidget::openOrdinaryPlayback);

    connect(motionPreviewButton, &QPushButton::clicked, this, &ReviewWidget::openMotionPlayback);

    connect(searchButton, &QPushButton::clicked, this, &ReviewWidget::searchCurrentContent);

    connect(videoTable, &QTableWidget::cellClicked, this, [this](int row, int) {
        playVideoAtRow(row);
    });

    connect(videoPreviousButton, &QPushButton::clicked, this, [this]() {
        if (videoCurrentPage > 1) {
            --videoCurrentPage;
            loadVideoPage();
        }
    });

    connect(videoNextButton, &QPushButton::clicked, this, [this]() {
        if (videoCurrentPage < videoTotalPages) {
            ++videoCurrentPage;
            loadVideoPage();
        }
    });

    connect(imagePreviousButton, &QPushButton::clicked, this, [this]() {
        if (imageCurrentPage > 1) {
            --imageCurrentPage;
            updateImagePagination();
        }
    });

    connect(imageNextButton, &QPushButton::clicked, this, [this]() {
        if (imageCurrentPage < imageTotalPages) {
            ++imageCurrentPage;
            updateImagePagination();
        }
    });

    connect(playPauseButton, &QPushButton::clicked, this, [this]() {
        if (mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
            mediaPlayer->pause();
        } else if (mediaPlayer->source().isEmpty()) {
            playVideoAtRow(videoTable->currentRow());
        } else {
            mediaPlayer->play();
        }
    });

    connect(stopButton, &QPushButton::clicked, mediaPlayer, &QMediaPlayer::stop);
    connect(mediaPlayer, &QMediaPlayer::playbackStateChanged, this,
            [this](QMediaPlayer::PlaybackState state) {
        playPauseButton->setText(
            state == QMediaPlayer::PlayingState
                ? QStringLiteral("暂停")
                : QStringLiteral("播放"));
    });

    connect(speedComboBox, &QComboBox::currentIndexChanged, this, [this](int) {
        mediaPlayer->setPlaybackRate(speedComboBox->currentData().toDouble());
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

    connect(screenshotButton, &QPushButton::clicked,
            this, &ReviewWidget::savePlaybackSnapshot);

    connect(viewSnapshotButton, &QPushButton::clicked, this, &ReviewWidget::viewSelectedSnapshot);
    connect(deleteSnapshotsButton, &QPushButton::clicked,
            this, &ReviewWidget::deleteSelectedSnapshots);
    connect(exportSnapshotsButton, &QPushButton::clicked,
            this, &ReviewWidget::exportSelectedSnapshots);

    for (int index = 0; index < ImagePageSize; ++index) {
        connect(imageSelectionChecks[index], &QCheckBox::toggled,
                this, [this, index](bool checked) {
            const int snapshotIndex = (imageCurrentPage - 1) * ImagePageSize + index;
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

    connect(timeRangeGroup, &QButtonGroup::idClicked,
            this, &ReviewWidget::showTimeRange);
}

void ReviewWidget::setPreviewMode(PreviewMode mode)
{
    // 在视频预览与图片预览页面间切换并更新对应查询。
    if (mode != previewMode)
        mediaPlayer->pause();
    previewMode = mode;

    const int pageIndex =
        mode == ImagePreview ? 1 : 0;

    leftContentStack->setCurrentIndex(pageIndex);
    leftBottomStack->setCurrentIndex(pageIndex);
    previewContentStack->setCurrentIndex(pageIndex);
    leftContentWidget->setVisible(true);
    leftBottomWidget->setVisible(true);
    filterWidget->setEnabled(true);

    videoPreviewButton->setChecked(
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
    // 打开普通回放时执行默认分页查询。
    setPreviewMode(VideoPreview);
    resetVideoQuery();
}

void ReviewWidget::openMotionPlayback()
{
    // 打开移动侦测回放并查询移动目标录像。
    setPreviewMode(MotionPreview);
    resetVideoQuery();
}

void ReviewWidget::resetVideoQuery()
{
    // 重置视频筛选条件和分页状态。
    videoQueryStart = QDateTime();
    videoQueryEnd = QDateTime();
    videoQueryChannel = 0;
    channelComboBox->setCurrentIndex(0);
    videoCurrentPage = 1;
    loadVideoPage();
}

void ReviewWidget::searchVideos()
{
    // 校验筛选条件并开始查询当前用户的录像记录。
    videoQueryStart = QDateTime(dateTimeEdit->date(), startTimeEdit->time());
    videoQueryEnd = QDateTime(dateTimeEdit->date(), endTimeEdit->time());
    if (endTimeEdit->time() < startTimeEdit->time())
        videoQueryEnd = videoQueryEnd.addDays(1);
    videoQueryEnd = videoQueryEnd.addSecs(1);
    videoQueryChannel = channelComboBox->currentData().toInt();
    videoCurrentPage = 1;
    loadVideoPage();
}

void ReviewWidget::loadVideoPage()
{
    // 读取当前页录像并刷新列表和分页状态。
    if (currentUsername.isEmpty() || previewMode == ImagePreview)
        return;
    QVector<VideoRecordInfo> queriedRecords;
    QString errorMessage;

    int totalCount = 0;
    const bool queryResult = playbackService.searchPage(
        videoQueryStart, videoQueryEnd, videoQueryChannel, videoCurrentPage,
        videoPageSize, queriedRecords, totalCount, errorMessage,
        previewMode == MotionPreview ? QStringLiteral("motion") : QStringLiteral("normal"));

    if (!queryResult) {
        videoRecords.clear();
        videoTotalCount = 0;
        videoCurrentPage = 1;
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
    // 播放列表中指定行对应的录像文件。
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
    // 按当前预览模式执行视频或图片查询。
    if (previewMode == ImagePreview)
        searchSnapshots();
    else
        searchVideos();
}

void ReviewWidget::searchSnapshots()
{
    // 按日期、时间范围和通道查询图片缩略图。
    if (currentUsername.isEmpty())
        return;
    const QDate selectedDate = dateTimeEdit->date();
    const QDateTime rangeStart(selectedDate, QTime(0, 0));
    const QDateTime rangeEnd(selectedDate.addDays(1), QTime(0, 0));
    QVector<SnapshotInfo> queriedSnapshots;
    QString errorMessage;
    const bool searched = snapshotService.searchSnapshots(
        rangeStart, rangeEnd, channelComboBox->currentData().toInt(),
        queriedSnapshots, errorMessage);
    if (!searched) {
        QMessageBox::warning(this, QStringLiteral("图片查询失败"), errorMessage);
        return;
    }
    daySnapshots = queriedSnapshots;
    snapshots = daySnapshots;
    updateTimeRangeButtons();
    selectedSnapshotIds.clear();
    imageCurrentPage = 1;
    updateImagePagination();
}

void ReviewWidget::updateSnapshotGrid()
{
    // 将当前图片页填入固定3×3网格并补齐空白位置。
    const int firstIndex = (imageCurrentPage - 1) * ImagePageSize;
    for (int index = 0; index < ImagePageSize; ++index) {
        QLabel *imageLabel = imageLabels[index];
        QCheckBox *selectionCheck = imageSelectionChecks[index];
        const int snapshotIndex = firstIndex + index;
        const bool hasSnapshot = snapshotIndex < snapshots.size();
        selectionCheck->blockSignals(true);
        if (!hasSnapshot) {
            QPixmap whitePixmap(16, 16);
            whitePixmap.fill(Qt::white);
            imageLabel->setScaledContents(true);
            imageLabel->setPixmap(whitePixmap);
            imageLabel->setToolTip(QString());
            selectionCheck->setText(QString());
            selectionCheck->setChecked(false);
            selectionCheck->setEnabled(false);
            selectionCheck->setVisible(false);
        } else {
            const SnapshotInfo &snapshot = snapshots.at(snapshotIndex);
            const QPixmap imagePixmap(snapshot.snapshotPath);
            imageLabel->setScaledContents(false);
            imageLabel->clear();
            if (imagePixmap.isNull()) {
                imageLabel->setText(QStringLiteral("图片缺失或无法读取"));
                selectedSnapshotIds.remove(snapshot.id);
                selectionCheck->setText(QString());
                selectionCheck->setChecked(false);
                selectionCheck->setEnabled(false);
                selectionCheck->setVisible(false);
            } else {
                const QSize targetSize = imageLabel->size().expandedTo(QSize(1, 1));
                imageLabel->setPixmap(imagePixmap.scaled(
                    targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                selectionCheck->setText(QStringLiteral("通道%1 %2").arg(snapshot.channelNo)
                    .arg(snapshot.frameTime.toString(QStringLiteral("HH:mm:ss"))));
                selectionCheck->setChecked(selectedSnapshotIds.contains(snapshot.id));
                selectionCheck->setEnabled(true);
                selectionCheck->setVisible(true);
            }
            imageLabel->setToolTip(snapshot.snapshotPath);
        }
        selectionCheck->blockSignals(false);
    }
}

QVector<SnapshotInfo> ReviewWidget::selectedSnapshots() const
{
    // 收集缩略图网格中用户选中的图片记录。
    QVector<SnapshotInfo> selected;
    for (const SnapshotInfo &snapshot : snapshots) {
        if (selectedSnapshotIds.contains(snapshot.id))
            selected.append(snapshot);
    }
    return selected;
}

void ReviewWidget::savePlaybackSnapshot()
{
    // 从当前回放帧创建手动截图并保存用户选择的文件。
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
    // 二次确认后删除选中的图片及其数据库元数据。
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
    // 将选中图片批量导出到用户指定目录。
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
    // 打开所选图片的大图预览。
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
    snapshotView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    snapshotView->viewport()->installEventFilter(this);
    snapshotMetadataLabel = new QLabel(&dialog);
    snapshotMetadataLabel->setWordWrap(true);
    snapshotMetadataLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    QHBoxLayout *controlLayout = new QHBoxLayout;
    previousSnapshotButton = new QPushButton(QStringLiteral("上一张"), &dialog);
    nextSnapshotButton = new QPushButton(QStringLiteral("下一张"), &dialog);
    relatedVideoButton = new QPushButton(QStringLiteral("跳转关联录像"), &dialog);
    QPushButton *closeButton = new QPushButton(QStringLiteral("关闭"), &dialog);
    controlLayout->addWidget(previousSnapshotButton);
    controlLayout->addWidget(nextSnapshotButton);
    controlLayout->addWidget(new QLabel(
        QStringLiteral("滚轮缩放：上滚放大，下滚缩小（0.5—2倍）"), &dialog));
    controlLayout->addWidget(relatedVideoButton);
    controlLayout->addWidget(closeButton);
    mainLayout->addWidget(snapshotView, 1);
    mainLayout->addWidget(snapshotMetadataLabel);
    mainLayout->addLayout(controlLayout);
    connect(previousSnapshotButton, &QPushButton::clicked,
            this, &ReviewWidget::showPreviousSnapshot);
    connect(nextSnapshotButton, &QPushButton::clicked, this, &ReviewWidget::showNextSnapshot);
    connect(relatedVideoButton, &QPushButton::clicked, this, &ReviewWidget::playRelatedVideo);
    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    showLargeSnapshot();
    dialog.exec();
    snapshotDialog = nullptr;
    snapshotView = nullptr;
    snapshotScene = nullptr;
    snapshotMetadataLabel = nullptr;
    previousSnapshotButton = nullptr;
    nextSnapshotButton = nullptr;
    relatedVideoButton = nullptr;
}

void ReviewWidget::showLargeSnapshot()
{
    // 展示当前大图并同步元数据与关联视频信息。
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
    snapshotWheelRemainder = 0;
    updateSnapshotZoom(1.0);
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
    // 切换到当前筛选结果中的上一张图片。
    if (largeSnapshotIndex > 0) {
        --largeSnapshotIndex;
        showLargeSnapshot();
    }
}

void ReviewWidget::showNextSnapshot()
{
    // 切换到当前筛选结果中的下一张图片。
    if (largeSnapshotIndex + 1 < snapshots.size()) {
        ++largeSnapshotIndex;
        showLargeSnapshot();
    }
}

void ReviewWidget::updateSnapshotZoom(double scaleValue)
{
    // 应用限定在0.5至2倍之间的大图缩放比例。
    if (snapshotView == nullptr)
        return;
    snapshotScale = qBound(0.5, scaleValue, 2.0);
    snapshotView->resetTransform();
    snapshotView->scale(snapshotScale, snapshotScale);
    snapshotView->setToolTip(
        QStringLiteral("当前缩放：%1%，滚轮缩放，按住鼠标左键拖动画面")
            .arg(qRound(snapshotScale * 100.0)));
}

void ReviewWidget::playRelatedVideo()
{
    // 打开图片关联录像并定位到对应截图时间点。
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
    // 在播放器载入录像后应用待执行的时间偏移。
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
    // 将毫秒播放位置格式化为界面显示的时间文本。
    const qint64 totalSeconds = milliseconds / 1000;
    const qint64 minutes = totalSeconds / 60;
    const qint64 seconds = totalSeconds % 60;

    return QStringLiteral("%1:%2")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}

void ReviewWidget::updateVideoPagination()
{
    // 更新录像列表页码并同步上一页/下一页按钮状态。
    const int videoCount = videoTotalCount;

    videoTotalPages = qMax(
        1,
        (videoCount + videoPageSize - 1) / videoPageSize);

    if (videoCurrentPage > videoTotalPages)
        videoCurrentPage = videoTotalPages;

    videoPageLabel->setText(
        QStringLiteral("%1/%2").arg(videoCurrentPage).arg(videoTotalPages));

    videoPreviousButton->setEnabled(videoCurrentPage > 1);
    videoNextButton->setEnabled(videoCurrentPage < videoTotalPages);

    const int currentPageVideoCount = static_cast<int>(videoRecords.size());

    videoTable->setRowCount(0);
    videoTable->setRowCount(currentPageVideoCount);

    for (int row = 0; row < currentPageVideoCount; ++row) {
        const VideoRecordInfo &record = videoRecords.at(row);

        QLabel *thumbnailLabel = new QLabel(videoTable);
        thumbnailLabel->setAlignment(Qt::AlignCenter);
        thumbnailLabel->setScaledContents(true);
        thumbnailLabel->setMinimumSize(0, 0);
        thumbnailLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        thumbnailLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        thumbnailLabel->setPixmap(videoThumbnail);

        videoTable->setCellWidget(row, 0, thumbnailLabel);

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
        videoTable->setItem(row, 1, videoItem);
        videoTable->setRowHeight(row, 90);
    }
}

void ReviewWidget::updateImagePagination()
{
    // 更新图片缩略图页码及翻页按钮状态。
    const int imageCount = static_cast<int>(snapshots.size());
    imageTotalPages = qMax(1, (imageCount + ImagePageSize - 1) / ImagePageSize);
    imageCurrentPage = qBound(1, imageCurrentPage, imageTotalPages);
    imagePageLabel->setText(
        QStringLiteral("%1/%2")
            .arg(imageCurrentPage)
            .arg(imageTotalPages));

    imagePreviousButton->setEnabled(
        imageCurrentPage > 1);
    imageNextButton->setEnabled(
        imageCurrentPage < imageTotalPages);

    updateSnapshotGrid();
}

bool ReviewWidget::eventFilter(QObject *watched, QEvent *event)
{
    // 处理大图滚轮缩放及视频列表尺寸变化。
    if (watched == videoTable->viewport() && event->type() == QEvent::Resize)
        updateVideoColumnWidths();
    if (snapshotView != nullptr
        && watched == snapshotView->viewport()
        && event->type() == QEvent::Wheel) {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent *>(event);
        snapshotWheelRemainder += wheelEvent->angleDelta().y();
        while (snapshotWheelRemainder >= 120) {
            updateSnapshotZoom(snapshotScale * 1.15);
            snapshotWheelRemainder -= 120;
        }
        while (snapshotWheelRemainder <= -120) {
            updateSnapshotZoom(snapshotScale / 1.15);
            snapshotWheelRemainder += 120;
        }
        wheelEvent->accept();
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void ReviewWidget::updateVideoColumnWidths()
{
    // 按当前列表宽度更新截图列和名称列的比例。
    // 截图列 : 名称列 = 2 : 8，跟随表格实际可用宽度调整。
    const int availableWidth = videoTable->viewport()->width();
    if (availableWidth > 0)
        videoTable->setColumnWidth(0, availableWidth * 2 / 10);
}

void ReviewWidget::resizeEvent(QResizeEvent *event)
{
    // 窗口尺寸变化时重新调整视频列表列宽。
    QWidget::resizeEvent(event);

    updateVideoColumnWidths();
    if (previewMode == ImagePreview)
        updateSnapshotGrid();
}

void ReviewWidget::setCurrentUser(const QString &username)
{
    // 更新回放及图片服务所使用的当前登录用户。
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
    daySnapshots.clear();
    selectedSnapshotIds.clear();
    videoCurrentPage = 1;
    imageCurrentPage = 1;
    updateVideoPagination();
    updateImagePagination();
    updateTimeRangeButtons();
}

bool ReviewWidget::playRecord(qulonglong recordId, qint64 offsetMs)
{
    // 校验录像文件并从指定偏移位置开始播放。
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
    // 页面显示时恢复对应回放内容并启动必要的查询。
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
    // 页面隐藏时停止播放器并释放当前媒体资源。
    mediaPlayer->pause();
    QWidget::hideEvent(event);
}
