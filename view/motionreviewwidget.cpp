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

    // 整体左侧 : 右侧 = 3 : 7
    m_mainLayout->addWidget(m_leftWidget, 3);
    m_mainLayout->addWidget(m_rightWidget, 7);

    updateVideoPagination();
}

void MotionReviewWidget::initLeftArea()
{
    m_leftWidget = new QWidget(this);
    m_leftLayout = new QVBoxLayout(m_leftWidget);
    m_leftLayout->setContentsMargins(0, 0, 0, 0);
    m_leftLayout->setSpacing(8);

    // 日期时间和通道选择
    m_filterWidget = new QWidget(m_leftWidget);
    m_filterLayout = new QHBoxLayout(m_filterWidget);
    m_filterLayout->setContentsMargins(0, 0, 0, 0);
    m_filterLayout->setSpacing(8);

    m_dateTimeEdit = new QDateTimeEdit(
        QDateTime::currentDateTime(), m_filterWidget);

    m_dateTimeEdit->setDisplayFormat(
        QStringLiteral("yyyy-MM-dd HH:mm"));
    m_dateTimeEdit->setCalendarPopup(true);

    m_channelComboBox = new QComboBox(m_filterWidget);
    m_channelComboBox->addItems({
        QStringLiteral("1通道"),
        QStringLiteral("2通道"),
        QStringLiteral("3通道"),
        QStringLiteral("4通道")
    });

    m_filterLayout->addWidget(m_dateTimeEdit, 2);
    m_filterLayout->addWidget(m_channelComboBox, 1);

    initVideoList();

    // 视频列表分页
    m_videoPagerWidget = new QWidget(m_leftWidget);
    m_videoPagerLayout =
        new QHBoxLayout(m_videoPagerWidget);
    m_videoPagerLayout->setContentsMargins(0, 0, 0, 0);
    m_videoPagerLayout->setSpacing(8);

    m_videoPreviousButton = new QPushButton(
        QStringLiteral("上一页"), m_videoPagerWidget);

    m_videoPageLabel = new QLabel(m_videoPagerWidget);
    m_videoPageLabel->setObjectName(
        QStringLiteral("pageIndicator"));
    m_videoPageLabel->setAlignment(Qt::AlignCenter);

    m_videoNextButton = new QPushButton(
        QStringLiteral("下一页"), m_videoPagerWidget);

    m_videoPagerLayout->addWidget(m_videoPreviousButton);
    m_videoPagerLayout->addWidget(m_videoPageLabel, 1);
    m_videoPagerLayout->addWidget(m_videoNextButton);

    // 筛选 : 列表 : 分页 = 1 : 9 : 1
    m_leftLayout->addWidget(m_filterWidget, 1);
    m_leftLayout->addWidget(m_videoTable, 9);
    m_leftLayout->addWidget(m_videoPagerWidget, 1);
}

void MotionReviewWidget::initVideoList()
{
    m_videoTable = new QTableWidget(0, 2, m_leftWidget);

    m_videoTable->setHorizontalHeaderLabels({
        QStringLiteral("视频截图"),
        QStringLiteral("视频名称")
    });

    m_videoTable->horizontalHeader()->setVisible(false);
    m_videoTable->verticalHeader()->setVisible(false);

    // 第一列由代码控制，第二列占据剩余宽度
    m_videoTable->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::Fixed);
    m_videoTable->horizontalHeader()->setStretchLastSection(true);

    m_videoTable->setSelectionBehavior(
        QAbstractItemView::SelectRows);
    m_videoTable->setSelectionMode(
        QAbstractItemView::SingleSelection);
    m_videoTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers);

    // 监听列表实际可显示区域尺寸变化，
    // 包括页面显示、窗口缩放和滚动条出现等情况
    m_videoTable->viewport()->installEventFilter(this);
}

void MotionReviewWidget::initRightArea()
{
    m_rightWidget = new QWidget(this);
    m_rightLayout = new QVBoxLayout(m_rightWidget);
    m_rightLayout->setContentsMargins(0, 0, 0, 0);
    m_rightLayout->setSpacing(8);

    // 视频预览占位图
    m_videoDisplayLabel = new QLabel(m_rightWidget);
    m_videoDisplayLabel->setAlignment(Qt::AlignCenter);
    m_videoDisplayLabel->setMinimumSize(0, 0);
    m_videoDisplayLabel->setSizePolicy(
        QSizePolicy::Ignored,
        QSizePolicy::Ignored);
    m_videoDisplayLabel->setScaledContents(true);
    m_videoDisplayLabel->setPixmap(m_blackMonitorImage);

    // 视频操作区域
    m_videoControlsWidget = new QWidget(m_rightWidget);
    m_videoControlsLayout =
        new QHBoxLayout(m_videoControlsWidget);
    m_videoControlsLayout->setContentsMargins(0, 0, 0, 0);
    m_videoControlsLayout->setSpacing(12);

    m_pauseButton = new QPushButton(
        QStringLiteral("暂停"), m_videoControlsWidget);

    m_screenshotButton = new QPushButton(
        QStringLiteral("截图"), m_videoControlsWidget);

    m_speedComboBox = new QComboBox(m_videoControlsWidget);
    m_speedComboBox->addItems({
        QStringLiteral("0.5倍速"),
        QStringLiteral("1倍速"),
        QStringLiteral("2倍速"),
        QStringLiteral("4倍速")
    });
    m_speedComboBox->setCurrentIndex(1);

    m_videoControlsLayout->addStretch();
    m_videoControlsLayout->addWidget(m_pauseButton);
    m_videoControlsLayout->addWidget(m_screenshotButton);
    m_videoControlsLayout->addWidget(m_speedComboBox);
    m_videoControlsLayout->addStretch();

    // 视频预览 : 操作区域 = 9 : 1
    m_rightLayout->addWidget(m_videoDisplayLabel, 9);
    m_rightLayout->addWidget(m_videoControlsWidget, 1);
}

void MotionReviewWidget::initConnect()
{
    connect(m_videoPreviousButton,
            &QPushButton::clicked,
            this,
            [this]() {
                if (m_videoCurrentPage > 1) {
                    --m_videoCurrentPage;
                    updateVideoPagination();
                }
            });

    connect(m_videoNextButton,
            &QPushButton::clicked,
            this,
            [this]() {
                if (m_videoCurrentPage < m_videoTotalPages) {
                    ++m_videoCurrentPage;
                    updateVideoPagination();
                }
            });

    connect(m_screenshotButton,
            &QPushButton::clicked,
            this,
            &MotionReviewWidget::screenshotRequested);

    // 暂停、倍速、筛选和选择视频的实际功能，
    // 后续在这里补充连接。
}

void MotionReviewWidget::updateVideoPagination()
{
    // 避免容量为零或总量为负
    m_videoPageSize = qMax(1, m_videoPageSize);
    m_totalVideoCount = qMax(0, m_totalVideoCount);

    // 总页数向上取整，空列表仍显示 1/1
    m_videoTotalPages = qMax(
        1,
        m_totalVideoCount / m_videoPageSize
            + (m_totalVideoCount % m_videoPageSize != 0));

    m_videoCurrentPage = qBound(
        1, m_videoCurrentPage, m_videoTotalPages);

    m_videoPageLabel->setText(
        QStringLiteral("%1/%2")
            .arg(m_videoCurrentPage)
            .arg(m_videoTotalPages));

    m_videoPreviousButton->setEnabled(
        m_videoCurrentPage > 1);
    m_videoNextButton->setEnabled(
        m_videoCurrentPage < m_videoTotalPages);

    const int firstVideoIndex =
        (m_videoCurrentPage - 1) * m_videoPageSize;

    const int currentPageVideoCount = qMax(
        0,
        qMin(m_videoPageSize,
             m_totalVideoCount - firstVideoIndex));

    // 清除上一页，再创建本页内容
    m_videoTable->setRowCount(0);
    m_videoTable->setRowCount(currentPageVideoCount);

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
        thumbnailLabel->setPixmap(m_videoThumbnail);

        // 表格接管缩略图控件的所有权
        m_videoTable->setCellWidget(row, 0, thumbnailLabel);

        // 模拟数据：后续替换为数据库中的视频名称
        m_videoTable->setItem(
            row,
            1,
            new QTableWidgetItem(
                QStringLiteral("视频%1").arg(videoIndex + 1)));

        m_videoTable->setRowHeight(row, 90);
    }

    updateVideoColumnWidths();
}

void MotionReviewWidget::updateVideoColumnWidths()
{
    const int availableWidth =
        m_videoTable->viewport()->width();

    // 截图列 : 名称列 = 2 : 8
    if (availableWidth > 0) {
        m_videoTable->setColumnWidth(
            0, availableWidth * 2 / 10);
    }
}

bool MotionReviewWidget::eventFilter(
    QObject *watched, QEvent *event)
{
    if (watched == m_videoTable->viewport()
        && event->type() == QEvent::Resize) {
        updateVideoColumnWidths();
    }

    return QWidget::eventFilter(watched, event);
}
