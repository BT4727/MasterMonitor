#include "motionreviewwidget.h"
#include <QDateTime>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QTableWidgetItem>
#include <QSizePolicy>
#include <QtGlobal>
MotionReviewWidget::MotionReviewWidget(QWidget *parent)
    : QWidget{parent}
{
    // 创建移动侦测回看页面并准备列表和预览占位图。
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

    // 整体左侧 : 右侧 = 3 : 7
    mainLayout->addWidget(leftWidget, 3);
    mainLayout->addWidget(rightWidget, 7);

    updateVideoPagination();
}

void MotionReviewWidget::initLeftArea()
{
    // 创建日期、通道筛选、录像列表和分页区。
    leftWidget = new QWidget(this);
    leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);

    // 日期时间和通道选择
    filterWidget = new QWidget(leftWidget);
    filterLayout = new QHBoxLayout(filterWidget);
    filterLayout->setContentsMargins(0, 0, 0, 0);
    filterLayout->setSpacing(8);

    dateTimeEdit = new QDateTimeEdit(
        QDateTime::currentDateTime(), filterWidget);

    dateTimeEdit->setDisplayFormat(
        QStringLiteral("yyyy-MM-dd HH:mm"));
    dateTimeEdit->setCalendarPopup(true);

    channelComboBox = new QComboBox(filterWidget);
    channelComboBox->addItems({
        QStringLiteral("1通道"),
        QStringLiteral("2通道"),
        QStringLiteral("3通道"),
        QStringLiteral("4通道")
    });

    filterLayout->addWidget(dateTimeEdit, 2);
    filterLayout->addWidget(channelComboBox, 1);

    initVideoList();

    // 视频列表分页
    videoPagerWidget = new QWidget(leftWidget);
    videoPagerLayout =
        new QHBoxLayout(videoPagerWidget);
    videoPagerLayout->setContentsMargins(0, 0, 0, 0);
    videoPagerLayout->setSpacing(8);

    videoPreviousButton = new QPushButton(
        QStringLiteral("上一页"), videoPagerWidget);

    videoPageLabel = new QLabel(videoPagerWidget);
    videoPageLabel->setObjectName(
        QStringLiteral("pageIndicator"));
    videoPageLabel->setAlignment(Qt::AlignCenter);

    videoNextButton = new QPushButton(
        QStringLiteral("下一页"), videoPagerWidget);

    videoPagerLayout->addWidget(videoPreviousButton);
    videoPagerLayout->addWidget(videoPageLabel, 1);
    videoPagerLayout->addWidget(videoNextButton);

    // 筛选 : 列表 : 分页 = 1 : 9 : 1
    leftLayout->addWidget(filterWidget, 1);
    leftLayout->addWidget(videoTable, 9);
    leftLayout->addWidget(videoPagerWidget, 1);
}

void MotionReviewWidget::initVideoList()
{
    // 初始化录像缩略图列表及自适应列宽监听。
    videoTable = new QTableWidget(0, 2, leftWidget);

    videoTable->setHorizontalHeaderLabels({
        QStringLiteral("视频截图"),
        QStringLiteral("视频名称")
    });

    videoTable->horizontalHeader()->setVisible(false);
    videoTable->verticalHeader()->setVisible(false);

    // 第一列由代码控制，第二列占据剩余宽度
    videoTable->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::Fixed);
    videoTable->horizontalHeader()->setStretchLastSection(true);

    videoTable->setSelectionBehavior(
        QAbstractItemView::SelectRows);
    videoTable->setSelectionMode(
        QAbstractItemView::SingleSelection);
    videoTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers);

    // 监听列表实际可显示区域尺寸变化，
    // 包括页面显示、窗口缩放和滚动条出现等情况
    videoTable->viewport()->installEventFilter(this);
}

void MotionReviewWidget::initRightArea()
{
    // 创建移动侦测录像预览及其控制区域。
    rightWidget = new QWidget(this);
    rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);

    // 视频预览占位图
    videoDisplayLabel = new QLabel(rightWidget);
    videoDisplayLabel->setAlignment(Qt::AlignCenter);
    videoDisplayLabel->setMinimumSize(0, 0);
    videoDisplayLabel->setSizePolicy(
        QSizePolicy::Ignored,
        QSizePolicy::Ignored);
    videoDisplayLabel->setScaledContents(true);
    videoDisplayLabel->setPixmap(blackMonitorImage);

    // 视频操作区域
    videoControlsWidget = new QWidget(rightWidget);
    videoControlsLayout =
        new QHBoxLayout(videoControlsWidget);
    videoControlsLayout->setContentsMargins(0, 0, 0, 0);
    videoControlsLayout->setSpacing(12);

    pauseButton = new QPushButton(
        QStringLiteral("暂停"), videoControlsWidget);

    screenshotButton = new QPushButton(
        QStringLiteral("截图"), videoControlsWidget);

    speedComboBox = new QComboBox(videoControlsWidget);
    speedComboBox->addItems({
        QStringLiteral("0.5倍速"),
        QStringLiteral("1倍速"),
        QStringLiteral("2倍速"),
        QStringLiteral("4倍速")
    });
    speedComboBox->setCurrentIndex(1);

    videoControlsLayout->addStretch();
    videoControlsLayout->addWidget(pauseButton);
    videoControlsLayout->addWidget(screenshotButton);
    videoControlsLayout->addWidget(speedComboBox);
    videoControlsLayout->addStretch();

    // 视频预览 : 操作区域 = 9 : 1
    rightLayout->addWidget(videoDisplayLabel, 9);
    rightLayout->addWidget(videoControlsWidget, 1);
}

void MotionReviewWidget::initConnect()
{
    // 连接列表分页和手动截图请求。
    connect(videoPreviousButton,
            &QPushButton::clicked,
            this,
            [this]() {
                if (videoCurrentPage > 1) {
                    --videoCurrentPage;
                    updateVideoPagination();
                }
            });

    connect(videoNextButton,
            &QPushButton::clicked,
            this,
            [this]() {
                if (videoCurrentPage < videoTotalPages) {
                    ++videoCurrentPage;
                    updateVideoPagination();
                }
            });

    connect(screenshotButton,
            &QPushButton::clicked,
            this,
            &MotionReviewWidget::screenshotRequested);

    // 暂停、倍速、筛选和选择视频的实际功能，
    // 后续在这里补充连接。
}

void MotionReviewWidget::updateVideoPagination()
{
    // 根据模拟录像总量刷新本页列表和翻页状态。
    // 避免容量为零或总量为负
    videoPageSize = qMax(1, videoPageSize);
    totalVideoCount = qMax(0, totalVideoCount);

    // 总页数向上取整，空列表仍显示 1/1
    videoTotalPages = qMax(
        1,
        totalVideoCount / videoPageSize
            + (totalVideoCount % videoPageSize != 0));

    videoCurrentPage = qBound(
        1, videoCurrentPage, videoTotalPages);

    videoPageLabel->setText(
        QStringLiteral("%1/%2")
            .arg(videoCurrentPage)
            .arg(videoTotalPages));

    videoPreviousButton->setEnabled(
        videoCurrentPage > 1);
    videoNextButton->setEnabled(
        videoCurrentPage < videoTotalPages);

    const int firstVideoIndex =
        (videoCurrentPage - 1) * videoPageSize;

    const int currentPageVideoCount = qMax(
        0,
        qMin(videoPageSize,
             totalVideoCount - firstVideoIndex));

    // 清除上一页，再创建本页内容
    videoTable->setRowCount(0);
    videoTable->setRowCount(currentPageVideoCount);

    for (int row = 0; row < currentPageVideoCount; ++row) {
        const int videoIndex = firstVideoIndex + row;

        QLabel *thumbnailLabel = new QLabel;
        thumbnailLabel->setObjectName(
            QStringLiteral("videoThumbnail"));
        thumbnailLabel->setAlignment(Qt::AlignCenter);
        thumbnailLabel->setMinimumSize(0, 0);
        thumbnailLabel->setSizePolicy(
            QSizePolicy::Ignored,
            QSizePolicy::Ignored);
        thumbnailLabel->setScaledContents(true);
        thumbnailLabel->setPixmap(videoThumbnail);

        // 表格接管缩略图控件的所有权
        videoTable->setCellWidget(row, 0, thumbnailLabel);

        // 模拟数据：后续替换为数据库中的视频名称
        videoTable->setItem(
            row,
            1,
            new QTableWidgetItem(
                QStringLiteral("视频%1").arg(videoIndex + 1)));

        videoTable->setRowHeight(row, 90);
    }

    updateVideoColumnWidths();
}

void MotionReviewWidget::updateVideoColumnWidths()
{
    // 按2:8比例调整缩略图列和视频名称列。
    const int availableWidth =
        videoTable->viewport()->width();

    // 截图列 : 名称列 = 2 : 8
    if (availableWidth > 0) {
        videoTable->setColumnWidth(
            0, availableWidth * 2 / 10);
    }
}

bool MotionReviewWidget::eventFilter(
    QObject *watched, QEvent *event)
{
    // 监听列表视口尺寸变化并更新列宽。
    if (watched == videoTable->viewport()
        && event->type() == QEvent::Resize) {
        updateVideoColumnWidths();
    }

    return QWidget::eventFilter(watched, event);
}
