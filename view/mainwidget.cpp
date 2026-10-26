#include "mainwidget.h"
#include <QDateTime>
#include <QIcon>
#include <QPixmap>
#include <QSizePolicy>
#include <QCoreApplication>
#include <QPainter>
#include <QMessageBox>
#include "../util/winmanager.h"
#include "qmessagebox.h"
#include "reviewwidget.h"
#include "snapshotwidget.h"
#include "logwidget.h"
#include "../service/systemconfigservice.h"
#include "../model/systemconfigdata.h"
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>

MainWidget::MainWidget(QWidget *parent)
    : QWidget{parent}
{
    setWindowTitle(QStringLiteral("智能监控系统"));
    resize(1200, 800);
    setMinimumSize(1000, 650);

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(12, 12, 12, 12);
    m_mainLayout->setSpacing(10);

    initHeader();
    initPages();
    initConnections();

    // 整体上 : 下 = 3 : 7
    m_mainLayout->addWidget(m_headerWidget, 3);
    m_mainLayout->addWidget(m_contentWidget, 7);

    // 默认进入监控页面，未登录，四通道模式
    setCurrentUser(QString());
    m_pageStack->setCurrentIndex(MonitorPage);
    m_navigationButtons[MonitorPage]->setChecked(true);
    m_channelButtons[m_currentChannel]->setChecked(true);
    setSingleChannelMode(false);

    updateTime();
    m_clockTimer->start(1000);
    initQss();
    QTimer::singleShot(
        0,
        this,
        &MainWidget::initMonitorService);
}

void MainWidget::initHeader()
{
    m_headerWidget = new QWidget(this);
    m_headerLayout = new QHBoxLayout(m_headerWidget);
    m_headerLayout->setContentsMargins(0, 0, 0, 0);
    m_headerLayout->setSpacing(12);

    // 左侧标题
    m_titleLabel = new QLabel(
        QStringLiteral("智能监控系统"), m_headerWidget);

    m_titleLabel->setAlignment(Qt::AlignCenter);

    // 右侧区域
    m_headerRightWidget = new QWidget(m_headerWidget);
    m_headerRightLayout = new QVBoxLayout(m_headerRightWidget);
    m_headerRightLayout->setContentsMargins(2, 2, 2, 2);
    m_headerRightLayout->setSpacing(10);

    // 上部左 : 右 = 3 : 7
    m_headerLayout->addWidget(m_titleLabel, 3);
    m_headerLayout->addWidget(m_headerRightWidget, 7);

    // 时间及用户状态
    m_statusWidget = new QWidget(m_headerRightWidget);
    m_statusLayout = new QHBoxLayout(m_statusWidget);
    m_statusLayout->setContentsMargins(2, 2, 2, 2);
    m_statusLayout->setSpacing(12);

    m_timeLabel = new QLabel(m_statusWidget);
    m_timeLabel->setAlignment(Qt::AlignCenter);

    // 按钮下面显示用户名
    m_userLayout = new QVBoxLayout;
    m_userLayout->setContentsMargins(0, 0, 0, 0);
    m_userLayout->setSpacing(0);

    m_loginButton = new QPushButton(
        QStringLiteral("登录"), m_statusWidget);

    m_userLabel = new QLabel(m_statusWidget);
    m_userLabel->setAlignment(Qt::AlignCenter);

    m_userLayout->addWidget(m_loginButton);
    m_userLayout->addWidget(m_userLabel);

    m_statusLayout->addWidget(m_timeLabel, 1);
    m_statusLayout->addLayout(m_userLayout);

    // 导航按钮
    m_navigationWidget = new QWidget(m_headerRightWidget);

    // 外层纵向布局：上留白、按钮行、下留白
    QVBoxLayout *navigationOuterLayout = new QVBoxLayout(m_navigationWidget);
    navigationOuterLayout->setContentsMargins(2, 2, 2, 2);
    navigationOuterLayout->setSpacing(0);

    // 原来的横向布局继续用于排列五个按钮
    m_navigationLayout = new QHBoxLayout;
    m_navigationLayout->setContentsMargins(0, 0, 0, 0);
    m_navigationLayout->setSpacing(6);

    // 1 : 2 : 1，按钮行占导航区域高度的一半
    navigationOuterLayout->addStretch(1);
    navigationOuterLayout->addLayout(m_navigationLayout, 2);
    navigationOuterLayout->addStretch(1);

    m_navigationGroup = new QButtonGroup(this);
    m_navigationGroup->setExclusive(true);

    const std::array<QString, NavigationCount> names = {
        QStringLiteral("监控主页面"),
        QStringLiteral("回看"),
        QStringLiteral("图片查看"),
        QStringLiteral("日志查看"),
        QStringLiteral("系统设置")
    };

    for (int i = 0; i < NavigationCount; ++i) {
        QPushButton *button = new QPushButton(
            names[i], m_navigationWidget);

        button->setSizePolicy(
            QSizePolicy::Expanding,
            QSizePolicy::Preferred);

        // 前四个按钮对应内部页面
        // 系统设置按钮用于请求独立页面跳转
        if (i < PageCount) {
            button->setCheckable(true);
            m_navigationGroup->addButton(button, i);
        }

        m_navigationButtons[i] = button;
        m_navigationLayout->addWidget(button, 1);
    }

    // 顶部右侧上 : 下 = 1 : 1
    m_headerRightLayout->addWidget(m_statusWidget, 1);
    m_headerRightLayout->addWidget(m_navigationWidget, 1);

    m_clockTimer = new QTimer(this);
}

void MainWidget::initPages()
{
    m_contentWidget = new QWidget(this);
    m_pageStack = new QStackedLayout(m_contentWidget);
    m_pageStack->setContentsMargins(2, 2, 2, 2);

    // 监控主页面
    m_pages[MonitorPage] =
        new QWidget(m_contentWidget);

    // 普通视频回看
    m_pages[PlaybackPage] =
        new ReviewWidget(m_contentWidget);

    // 移动侦测回看
    m_pages[SnapshotPage] =
        new SnapshotWidget(m_contentWidget);

    // 日志查看暂时保留占位文字
    m_pages[LogPage] =
        new LogWidget(m_contentWidget);

    for (QWidget *page : m_pages)
        m_pageStack->addWidget(page);

    initMonitorPage();
}

void MainWidget::initMonitorPage()
{
    QWidget *page = m_pages[MonitorPage];

    m_monitorLayout = new QHBoxLayout(page);
    m_monitorLayout->setContentsMargins(0, 0, 0, 0);
    m_monitorLayout->setSpacing(12);

    // 左侧通道列表
    m_channelWidget = new QWidget(page);
    m_channelLayout = new QVBoxLayout(m_channelWidget);
    m_channelLayout->setContentsMargins(0, 0, 0, 0);
    m_channelLayout->setSpacing(12);

    m_channelGroup = new QButtonGroup(this);
    m_channelGroup->setExclusive(true);

    for (int i = 0; i < ChannelCount; ++i) {
        QPushButton *button = new QPushButton(
            QStringLiteral("监控通道 %1").arg(i + 1),
            m_channelWidget);

        button->setCheckable(true);

        m_channelButtons[i] = button;
        m_channelGroup->addButton(button, i);
        m_channelLayout->addWidget(button);
    }

    m_channelLayout->addStretch();

    // 右侧预览和操作区
    m_monitorRightWidget = new QWidget(page);
    m_monitorRightLayout = new QVBoxLayout(m_monitorRightWidget);
    m_monitorRightLayout->setContentsMargins(0, 0, 0, 0);
    m_monitorRightLayout->setSpacing(8);

    // 监控页面左 : 右 = 2 : 8
    m_monitorLayout->addWidget(m_channelWidget, 2);
    m_monitorLayout->addWidget(m_monitorRightWidget, 8);

    // 预览区
    m_previewWidget = new QWidget(m_monitorRightWidget);

    m_previewLayout = new QGridLayout(m_previewWidget);
    m_previewLayout->setContentsMargins(0, 0, 0, 0);
    m_previewLayout->setSpacing(2);

    for (int i = 0; i < 2; ++i) {
        m_previewLayout->setRowStretch(i, 1);
        m_previewLayout->setColumnStretch(i, 1);
    }

    // 优先使用资源中的黑色图片
    // 没有该资源时，在内存中生成黑色图片
    QPixmap blackPixmap(QStringLiteral(":/images/back.jpg"));

    for (int i = 0; i < ChannelCount; ++i) {
        QLabel *label = new QLabel(m_previewWidget);

        label->setAlignment(Qt::AlignCenter);
        label->setMinimumSize(0, 0);
        label->setSizePolicy(
            QSizePolicy::Ignored,
            QSizePolicy::Ignored);
        label->setScaledContents(false);
        label->setPixmap(blackPixmap);

        m_previewLabels[i] = label;
        m_previewLayout->addWidget(label, i / 2, i % 2);
    }

    // 下方操作区
    m_controlsWidget = new QWidget(m_monitorRightWidget);
    m_controlsLayout = new QHBoxLayout(m_controlsWidget);
    m_controlsLayout->setContentsMargins(0, 0, 0, 0);
    m_controlsLayout->setSpacing(16);

    m_singleChannelButton = new QPushButton(m_controlsWidget);
    m_singleChannelButton->setIcon(
        QIcon(QStringLiteral(":/images/onechannel.png")));
    m_singleChannelButton->setIconSize(QSize(28, 28));
    m_singleChannelButton->setToolTip(QStringLiteral("单通道"));
    m_singleChannelButton->setAccessibleName(
        QStringLiteral("单通道"));
    m_singleChannelButton->setCheckable(true);

    m_fourChannelButton = new QPushButton(m_controlsWidget);
    m_fourChannelButton->setIcon(
        QIcon(QStringLiteral(":/images/fourchannel.png")));
    m_fourChannelButton->setIconSize(QSize(28, 28));
    m_fourChannelButton->setToolTip(QStringLiteral("4通道"));
    m_fourChannelButton->setAccessibleName(
        QStringLiteral("4通道"));
    m_fourChannelButton->setCheckable(true);

    m_modeGroup = new QButtonGroup(this);
    m_modeGroup->setExclusive(true);
    m_modeGroup->addButton(m_singleChannelButton);
    m_modeGroup->addButton(m_fourChannelButton);

    m_motionCheckBox = new QCheckBox(
        QStringLiteral("移动侦测"), m_controlsWidget);
    monitorScreenshotButton = new QPushButton(
        QStringLiteral("截图当前通道"), m_controlsWidget);

    m_controlsLayout->addStretch();
    m_controlsLayout->addWidget(m_singleChannelButton);
    m_controlsLayout->addWidget(m_fourChannelButton);
    m_controlsLayout->addWidget(m_motionCheckBox);
    m_controlsLayout->addWidget(monitorScreenshotButton);
    m_controlsLayout->addStretch();

    // 预览 : 操作区 = 9 : 1
    m_monitorRightLayout->addWidget(m_previewWidget, 9);
    m_monitorRightLayout->addWidget(m_controlsWidget, 1);
}

void MainWidget::initConnections()
{
    connect(m_pageStack, &QStackedLayout::currentChanged, this, &MainWidget::syncDetectionPreview);
    connect(static_cast<ReviewWidget *>(m_pages[SnapshotPage]), &ReviewWidget::relatedVideoRequested,
            this, &MainWidget::openSnapshotVideo);
    connect(static_cast<LogWidget *>(m_pages[LogPage]), &LogWidget::relatedVideoRequested,
            this, &MainWidget::openSnapshotVideo);
    connect(monitorScreenshotButton, &QPushButton::clicked,
            this, &MainWidget::saveMonitorSnapshot);
    connect(m_clockTimer, &QTimer::timeout,
            this, &MainWidget::updateTime);

    // 前四个导航按钮切换内部页面
    for (int i = 0; i < PageCount; ++i) {
        connect(m_navigationButtons[i], &QPushButton::clicked,
                this, [this, i]() {
                    m_pageStack->setCurrentIndex(i);
                });
    }

    // 最后一个导航按钮请求打开独立设置页面
    connect(m_navigationButtons[NavigationCount - 1],
            &QPushButton::clicked,
            this, &MainWidget::settingsRequested);
    connect(this,
            &MainWidget::settingsRequested,
            this,
            &MainWidget::openSettingsPage);

    // 通道选择
    for (int i = 0; i < ChannelCount; ++i) {
        connect(m_channelButtons[i], &QPushButton::clicked,
                this, [this, i]() {
                    selectChannel(i);
                });
    }

    // 预览模式切换
    connect(m_singleChannelButton, &QPushButton::clicked,
            this, [this]() {
                setSingleChannelMode(true);
            });

    connect(m_fourChannelButton, &QPushButton::clicked,
            this, [this]() {
                setSingleChannelMode(false);
            });

    connect(m_motionCheckBox, &QCheckBox::toggled,
            this, &MainWidget::motionDetectionChanged);
    connect(m_motionCheckBox, &QCheckBox::toggled,
            this, &MainWidget::syncDetectionPreview);

    connect(m_loginButton,
            &QPushButton::clicked,
            this,
            &MainWidget::handleLoginButtonClicked);
    previewTimer = new QTimer(this);
    previewTimer->setInterval(40);
    connect(
        previewTimer,
        &QTimer::timeout,
        this,
        &MainWidget::refreshMonitorFrames);
    connect(
        QCoreApplication::instance(),
        &QCoreApplication::aboutToQuit,
        this,
        &MainWidget::stopMonitorService);

}

void MainWidget::saveMonitorSnapshot()
{
    if (monitorService == nullptr)
        return;
    const int channelNo = m_currentChannel + 1;
    const QImage image = monitorService->getChannelImage(m_currentChannel);
    const QDateTime frameTime = QDateTime::currentDateTime();
    if (image.isNull()) {
        logService.recordOperation(
            currentUsername, channelNo, QStringLiteral("snapshot"),
            QStringLiteral("snapshot"), 0, false,
            QStringLiteral("实时截图失败：当前通道没有可以保存的画面"));
        QMessageBox::warning(this, QStringLiteral("截图失败"),
                            QStringLiteral("当前通道没有可以保存的画面"));
        return;
    }
    const QString directory =
        QCoreApplication::applicationDirPath() + QStringLiteral("/snapshots");
    if (!QDir().mkpath(directory)) {
        logService.recordOperation(
            currentUsername, channelNo, QStringLiteral("snapshot"),
            QStringLiteral("snapshot"), 0, false,
            QStringLiteral("实时截图失败：无法创建默认截图目录"));
        QMessageBox::warning(this, QStringLiteral("截图失败"),
                            QStringLiteral("无法创建默认截图目录"));
        return;
    }
    const QString defaultName = QStringLiteral("实时_通道%1_%2.jpg").arg(channelNo)
        .arg(frameTime.toString(QStringLiteral("yyyyMMdd_HHmmss_zzz")));
    QString filePath = QFileDialog::getSaveFileName(
        this, QStringLiteral("保存实时截图"), QDir(directory).filePath(defaultName),
        QStringLiteral("图片文件 (*.jpg *.jpeg *.png)"));
    if (filePath.isEmpty())
        return;
    if (QFileInfo(filePath).suffix().isEmpty())
        filePath += QStringLiteral(".jpg");
    SnapshotInfo snapshot;
    QString errorMessage;
    const bool saved = snapshotService.saveManualSnapshot(
        image, channelNo, filePath, frameTime, 0, -1, snapshot, errorMessage);
    if (!saved) {
        logService.recordOperation(
            currentUsername, channelNo, QStringLiteral("snapshot"),
            QStringLiteral("snapshot"), 0, false,
            QStringLiteral("实时截图失败：%1").arg(errorMessage));
        QMessageBox::warning(this, QStringLiteral("截图失败"), errorMessage);
        return;
    }
    logService.recordOperation(
        currentUsername, channelNo, QStringLiteral("snapshot"),
        QStringLiteral("snapshot"), snapshot.id, true,
        QStringLiteral("保存实时截图：%1").arg(snapshot.snapshotPath));
    QMessageBox::information(this, QStringLiteral("截图成功"),
        QStringLiteral("通道%1截图已保存：\n%2").arg(channelNo).arg(snapshot.snapshotPath));
}

void MainWidget::handleLoginButtonClicked()
{
    if (!loggedIn) {
        WinManager::getInstance()
        ->loginWidget.show();

        hide();
        return;
    }

    const QMessageBox::StandardButton result =
        QMessageBox::question(
            this,
            QStringLiteral("退出登录"),
            QStringLiteral("确定要退出当前账号吗？"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);

    if (result != QMessageBox::Yes)
        return;

    const QString logoutUsername = currentUsername;
    logService.recordOperation(
        logoutUsername, 0, QStringLiteral("logout"),
        QStringLiteral("user"), 0, true,
        QStringLiteral("账号%1退出登录").arg(logoutUsername));
    setCurrentUser(QString());
}

void MainWidget::updateTime()
{
    m_timeLabel->setText(
        QDateTime::currentDateTime().toString(
            QStringLiteral("yyyy-MM-dd HH:mm:ss")));
}

void MainWidget::openSettingsPage()
{
    if (!loggedIn) {
        return;
    }

    WinManager *manager = WinManager::getInstance();

    connect(
        &manager->settingsWidget,
        &SettingsWidget::settingsSaved,
        this,
        &MainWidget::updateRecordDirectory,
        Qt::UniqueConnection);

    if (!manager->settingsWidget.openForUser(currentUsername)) {
        return;
    }

    manager->settingsWidget.show();
    manager->settingsWidget.raise();
    manager->settingsWidget.activateWindow();
    hide();
}

void MainWidget::setCurrentUser(const QString &username)
{
    if (monitorService != nullptr && currentUsername != username.trimmed())
        monitorService->stopRecord();
    currentUsername = username.trimmed();
    loggedIn = !currentUsername.isEmpty();
    snapshotService.setCurrentUser(currentUsername);
    static_cast<ReviewWidget *>(m_pages[PlaybackPage])->setCurrentUser(currentUsername);
    static_cast<ReviewWidget *>(m_pages[SnapshotPage])->setCurrentUser(currentUsername);
    static_cast<LogWidget *>(m_pages[LogPage])->setCurrentUser(currentUsername);

    m_userLabel->setText(
        loggedIn
            ? currentUsername
            : QStringLiteral("未登录"));

    m_loginButton->setText(
        loggedIn
            ? QStringLiteral("退出登录")
            : QStringLiteral("登录"));

    updateNavigationAccess();
    updateRecordDirectory();

    // 未登录时强制停留在监控主页面。
    if (!loggedIn) {
        if (monitorService != nullptr)
            monitorService->stopRecord();
        if (storageMonitorService != nullptr)
            storageMonitorService->stop();
        m_pageStack->setCurrentIndex(MonitorPage);
        m_navigationButtons[MonitorPage]->setChecked(true);
    }
}

void MainWidget::updateNavigationAccess()
{
    // 监控主页面始终允许访问。
    m_navigationButtons[MonitorPage]->setEnabled(true);

    // 普通回看、移动侦测回看、日志查看需要登录。
    for (int pageIndex = PlaybackPage;
         pageIndex <= LogPage;
         ++pageIndex) {
        m_navigationButtons[pageIndex]->setEnabled(loggedIn);
    }

    // 系统设置按钮不在内容堆栈中，但同样需要登录。
    m_navigationButtons[NavigationCount - 1]
        ->setEnabled(loggedIn);
}

void MainWidget::selectChannel(int channelIndex)
{
    if (channelIndex < 0 || channelIndex >= ChannelCount)
        return;

    const bool changed = m_currentChannel != channelIndex;

    m_currentChannel = channelIndex;
    m_channelButtons[channelIndex]->setChecked(true);

    // 单通道模式下，显示新选择的通道
    if (m_singleChannelMode)
        setSingleChannelMode(true);

    if (changed)
        emit currentChannelChanged(channelIndex);
}

void MainWidget::setSingleChannelMode(bool enabled)
{
    m_singleChannelMode = enabled;

    // 移除原有布局位置，保留预览控件
    for (QLabel *label : m_previewLabels) {
        m_previewLayout->removeWidget(label);
        label->hide();
    }

    if (enabled) {
        // 当前通道占满整个预览区
        QLabel *label = m_previewLabels[m_currentChannel];
        m_previewLayout->addWidget(label, 0, 0, 2, 2);
        label->show();
    } else {
        // 恢复四通道排列
        for (int i = 0; i < ChannelCount; ++i) {
            m_previewLayout->addWidget(
                m_previewLabels[i], i / 2, i % 2);
            m_previewLabels[i]->show();
        }
    }

    m_singleChannelButton->setChecked(enabled);
    m_fourChannelButton->setChecked(!enabled);

    // 四通道模式必须取消勾选并禁用移动侦测
    if (!enabled)
        m_motionCheckBox->setChecked(false);

    m_motionCheckBox->setEnabled(enabled);
    m_motionCheckBox->setToolTip(QStringLiteral("勾选后在当前单通道开启人体检测"));
    syncDetectionPreview();
}
MainWidget::~MainWidget()
{
    stopMonitorService();
}

void MainWidget::initMonitorService()
{
    monitorService = new MonitorService(this);
    const QString videoDirectory = QCoreApplication::applicationDirPath()
        + QStringLiteral("/video");
    monitorService->startMonitor(videoDirectory);
    if (loggedIn)
        updateRecordDirectory();
    previewTimer->start();
    syncDetectionPreview();
}

void MainWidget::syncDetectionPreview()
{
    if (monitorService == nullptr)
        return;
    const bool previewActive = isVisible()
        && m_pageStack->currentIndex() == MonitorPage
        && m_singleChannelMode;
    const bool detectionEnabled =
        m_singleChannelMode && m_motionCheckBox->isChecked();
    monitorService->setDetectionPreview(
        previewActive, m_currentChannel, detectionEnabled);
}

void MainWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    syncDetectionPreview();
}

void MainWidget::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    if (monitorService != nullptr)
        monitorService->setDetectionPreview(
            false, m_currentChannel, false);
}

void MainWidget::updateRecordDirectory()
{
    if (monitorService == nullptr
        || !loggedIn) {
        return;
    }

    SystemConfigService configService;
    SystemConfigData configuration;
    QString errorMessage;

    const SystemConfigService::Result result =
        configService.load(
            currentUsername,
            configuration,
            errorMessage);

    if (result != SystemConfigService::Result::Success && result != SystemConfigService::Result::NoConfig) {
        monitorService->stopRecord();
        QMessageBox::warning(this, QStringLiteral("无法开始录制"), errorMessage);
        return;
    }
    QString recordDirectory = configuration.videoSavePath.trimmed();
    if (recordDirectory.isEmpty()) {
        recordDirectory = QCoreApplication::applicationDirPath()
            + QStringLiteral("/videos/%1").arg(currentUsername);
    }
    if (!QDir().mkpath(recordDirectory)) {
        monitorService->stopRecord();
        QMessageBox::warning(this, QStringLiteral("无法开始录制"), QStringLiteral("无法创建录像保存目录"));
        return;
    }
    monitorService->startRecord(currentUsername, recordDirectory);
    if (storageMonitorService == nullptr)
        initStorageMonitor(recordDirectory);
    else
        storageMonitorService->start(recordDirectory);
}

void MainWidget::openSnapshotVideo(qulonglong recordId, qint64 offsetMs)
{
    if (!loggedIn)
        return;
    ReviewWidget *playbackPage = static_cast<ReviewWidget *>(m_pages[PlaybackPage]);
    if (!playbackPage->playRecord(recordId, offsetMs))
        return;
    m_pageStack->setCurrentIndex(PlaybackPage);
    m_navigationButtons[PlaybackPage]->setChecked(true);
}

void MainWidget::stopMonitorService()
{
    if (previewTimer != nullptr) {
        previewTimer->stop();
    }

    if (monitorService != nullptr) {
        monitorService->stopMonitor();
    }

    if (storageMonitorService != nullptr)
        storageMonitorService->stop();
}

void MainWidget::refreshMonitorFrames()
{
    if (monitorService == nullptr) {
        return;
    }

    // 这里只跳过绘制，不会停止采集和处理。
    if (!isVisible()
        || m_pageStack->currentIndex() != MonitorPage) {
        return;
    }

    for (int index = 0; index < ChannelCount; index++) {
        if (m_singleChannelMode
            && index != m_currentChannel) {
            continue;
        }

        QLabel *previewLabel = m_previewLabels[index];
        QSize previewSize =
            previewLabel->contentsRect().size();

        if (previewSize.isEmpty()) {
            continue;
        }

        QImage image =
            monitorService->getChannelImage(index);

        QPixmap canvas(previewSize);
        canvas.fill(Qt::black);

        // 使用独立作用域，确保绘制结束后再设置Pixmap。
        {
            QPainter painter(&canvas);

            if (image.isNull()) {
                painter.setPen(Qt::white);

                painter.drawText(
                    canvas.rect(),
                    Qt::AlignCenter,
                    QString("通道%1：正在连接")
                        .arg(index + 1));
            } else {
                QImage scaledImage = image.scaled(
                    previewSize,
                    Qt::KeepAspectRatio,
                    Qt::SmoothTransformation);

                int left =
                    (previewSize.width()
                     - scaledImage.width()) / 2;

                int top =
                    (previewSize.height()
                     - scaledImage.height()) / 2;

                painter.drawImage(
                    left, top, scaledImage);
            }
        }

        previewLabel->setPixmap(canvas);
    }
}
void MainWidget::initQss()
{
    QFile file(QStringLiteral(":/styles/mainwidget.qss"));

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setStyleSheet(QString::fromUtf8(file.readAll()));
    } else {
        qWarning() << "主页面样式加载失败：" << file.errorString();
    }
}

void MainWidget::initStorageMonitor(const QString &storagePath)
{
    if (storageMonitorService == nullptr) {
        storageMonitorService = new StorageMonitorService(this);

        connect(
            storageMonitorService,
            &StorageMonitorService::storageSpaceWarning,
            this,
            [this](const QString &message) {
                QMessageBox::warning(
                    this,
                    QStringLiteral("存储空间不足"),
                    message);
            });
    }

    storageMonitorService->start(storagePath);
}
