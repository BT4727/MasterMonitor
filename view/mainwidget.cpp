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
    // 构建主界面并初始化预览、导航及定时更新功能。
    setWindowTitle(QStringLiteral("智能监控系统"));
    resize(1200, 800);
    setMinimumSize(1000, 650);

    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);

    initHeader();
    initPages();
    initConnections();

    // 整体上 : 下 = 3 : 7
    mainLayout->addWidget(headerWidget, 3);
    mainLayout->addWidget(contentWidget, 7);

    // 默认进入监控页面，未登录，四通道模式
    setCurrentUser(QString());
    pageStack->setCurrentIndex(MonitorPage);
    navigationButtons[MonitorPage]->setChecked(true);
    channelButtons[currentChannel]->setChecked(true);
    setSingleChannelMode(false);

    updateTime();
    clockTimer->start(1000);
    initQss();
    QTimer::singleShot(
        0,
        this,
        &MainWidget::initMonitorService);
}

void MainWidget::initHeader()
{
    // 创建标题、用户操作区和主页面导航按钮。
    headerWidget = new QWidget(this);
    headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(12);

    // 左侧标题
    titleLabel = new QLabel(
        QStringLiteral("智能监控系统"), headerWidget);

    titleLabel->setAlignment(Qt::AlignCenter);

    // 右侧区域
    headerRightWidget = new QWidget(headerWidget);
    headerRightLayout = new QVBoxLayout(headerRightWidget);
    headerRightLayout->setContentsMargins(2, 2, 2, 2);
    headerRightLayout->setSpacing(10);

    // 上部左 : 右 = 3 : 7
    headerLayout->addWidget(titleLabel, 3);
    headerLayout->addWidget(headerRightWidget, 7);

    // 时间及用户状态
    statusWidget = new QWidget(headerRightWidget);
    statusLayout = new QHBoxLayout(statusWidget);
    statusLayout->setContentsMargins(2, 2, 2, 2);
    statusLayout->setSpacing(12);

    timeLabel = new QLabel(statusWidget);
    timeLabel->setAlignment(Qt::AlignCenter);

    // 按钮下面显示用户名
    userLayout = new QVBoxLayout;
    userLayout->setContentsMargins(0, 0, 0, 0);
    userLayout->setSpacing(0);

    loginButton = new QPushButton(
        QStringLiteral("登录"), statusWidget);

    userLabel = new QLabel(statusWidget);
    userLabel->setAlignment(Qt::AlignCenter);

    userLayout->addWidget(loginButton);
    userLayout->addWidget(userLabel);

    statusLayout->addWidget(timeLabel, 1);
    statusLayout->addLayout(userLayout);

    // 导航按钮
    navigationWidget = new QWidget(headerRightWidget);

    // 外层纵向布局：上留白、按钮行、下留白
    QVBoxLayout *navigationOuterLayout = new QVBoxLayout(navigationWidget);
    navigationOuterLayout->setContentsMargins(2, 2, 2, 2);
    navigationOuterLayout->setSpacing(0);

    // 原来的横向布局继续用于排列五个按钮
    navigationLayout = new QHBoxLayout;
    navigationLayout->setContentsMargins(0, 0, 0, 0);
    navigationLayout->setSpacing(6);

    // 1 : 2 : 1，按钮行占导航区域高度的一半
    navigationOuterLayout->addStretch(1);
    navigationOuterLayout->addLayout(navigationLayout, 2);
    navigationOuterLayout->addStretch(1);

    navigationGroup = new QButtonGroup(this);
    navigationGroup->setExclusive(true);

    const std::array<QString, NavigationCount> names = {
        QStringLiteral("监控主页面"),
        QStringLiteral("回看"),
        QStringLiteral("图片查看"),
        QStringLiteral("日志查看"),
        QStringLiteral("系统设置")
    };

    for (int i = 0; i < NavigationCount; ++i) {
        QPushButton *button = new QPushButton(
            names[i], navigationWidget);

        button->setSizePolicy(
            QSizePolicy::Expanding,
            QSizePolicy::Preferred);

        // 前四个按钮对应内部页面
        // 系统设置按钮用于请求独立页面跳转
        if (i < PageCount) {
            button->setCheckable(true);
            navigationGroup->addButton(button, i);
        }

        navigationButtons[i] = button;
        navigationLayout->addWidget(button, 1);
    }

    // 顶部右侧上 : 下 = 1 : 1
    headerRightLayout->addWidget(statusWidget, 1);
    headerRightLayout->addWidget(navigationWidget, 1);

    clockTimer = new QTimer(this);
}

void MainWidget::initPages()
{
    // 创建主页面内容栈并登记各个功能页面。
    contentWidget = new QWidget(this);
    pageStack = new QStackedLayout(contentWidget);
    pageStack->setContentsMargins(2, 2, 2, 2);

    // 监控主页面
    pages[MonitorPage] =
        new QWidget(contentWidget);

    // 普通视频回看
    pages[PlaybackPage] =
        new ReviewWidget(contentWidget);

    // 移动侦测回看
    pages[SnapshotPage] =
        new SnapshotWidget(contentWidget);

    // 日志查看暂时保留占位文字
    pages[LogPage] =
        new LogWidget(contentWidget);

    for (QWidget *page : pages)
        pageStack->addWidget(page);

    initMonitorPage();
}

void MainWidget::initMonitorPage()
{
    // 建立监控通道列表、预览区域和画面模式控制区。
    QWidget *page = pages[MonitorPage];

    monitorLayout = new QHBoxLayout(page);
    monitorLayout->setContentsMargins(0, 0, 0, 0);
    monitorLayout->setSpacing(12);

    // 左侧通道列表
    channelWidget = new QWidget(page);
    channelLayout = new QVBoxLayout(channelWidget);
    channelLayout->setContentsMargins(0, 0, 0, 0);
    channelLayout->setSpacing(12);

    channelGroup = new QButtonGroup(this);
    channelGroup->setExclusive(true);

    for (int i = 0; i < ChannelCount; ++i) {
        QPushButton *button = new QPushButton(
            QStringLiteral("通道%1").arg(i + 1),
            channelWidget);

        button->setCheckable(true);

        channelButtons[i] = button;
        channelGroup->addButton(button, i);
        channelLayout->addWidget(button);
    }

    channelLayout->addStretch();

    // 右侧预览和操作区
    monitorRightWidget = new QWidget(page);
    monitorRightLayout = new QVBoxLayout(monitorRightWidget);
    monitorRightLayout->setContentsMargins(0, 0, 0, 0);
    monitorRightLayout->setSpacing(8);

    // 监控页面左 : 右 = 2 : 8
    monitorLayout->addWidget(channelWidget, 2);
    monitorLayout->addWidget(monitorRightWidget, 8);

    // 预览区
    previewWidget = new QWidget(monitorRightWidget);

    previewLayout = new QGridLayout(previewWidget);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    previewLayout->setSpacing(2);

    for (int i = 0; i < 2; ++i) {
        previewLayout->setRowStretch(i, 1);
        previewLayout->setColumnStretch(i, 1);
    }

    // 优先使用资源中的黑色图片
    // 没有该资源时，在内存中生成黑色图片
    QPixmap blackPixmap(QStringLiteral(":/images/back.jpg"));

    for (int i = 0; i < ChannelCount; ++i) {
        QLabel *label = new QLabel(previewWidget);

        label->setAlignment(Qt::AlignCenter);
        label->setMinimumSize(0, 0);
        label->setSizePolicy(
            QSizePolicy::Ignored,
            QSizePolicy::Ignored);
        label->setScaledContents(false);
        label->setPixmap(blackPixmap);

        previewLabels[i] = label;
        previewLayout->addWidget(label, i / 2, i % 2);
    }

    // 下方操作区
    controlsWidget = new QWidget(monitorRightWidget);
    controlsLayout = new QHBoxLayout(controlsWidget);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->setSpacing(16);

    singleChannelButton = new QPushButton(controlsWidget);
    singleChannelButton->setIcon(
        QIcon(QStringLiteral(":/images/onechannel.png")));
    singleChannelButton->setIconSize(QSize(28, 28));
    singleChannelButton->setToolTip(QStringLiteral("单通道"));
    singleChannelButton->setAccessibleName(
        QStringLiteral("单通道"));
    singleChannelButton->setCheckable(true);

    fourChannelButton = new QPushButton(controlsWidget);
    fourChannelButton->setIcon(
        QIcon(QStringLiteral(":/images/fourchannel.png")));
    fourChannelButton->setIconSize(QSize(28, 28));
    fourChannelButton->setToolTip(QStringLiteral("4通道"));
    fourChannelButton->setAccessibleName(
        QStringLiteral("4通道"));
    fourChannelButton->setCheckable(true);

    modeGroup = new QButtonGroup(this);
    modeGroup->setExclusive(true);
    modeGroup->addButton(singleChannelButton);
    modeGroup->addButton(fourChannelButton);

    motionCheckBox = new QCheckBox(
        QStringLiteral("移动侦测"), controlsWidget);
    monitorScreenshotButton = new QPushButton(
        QStringLiteral("截图当前通道"), controlsWidget);

    controlsLayout->addStretch();
    controlsLayout->addWidget(singleChannelButton);
    controlsLayout->addWidget(fourChannelButton);
    controlsLayout->addWidget(motionCheckBox);
    controlsLayout->addWidget(monitorScreenshotButton);
    controlsLayout->addStretch();

    // 预览 : 操作区 = 9 : 1
    monitorRightLayout->addWidget(previewWidget, 9);
    monitorRightLayout->addWidget(controlsWidget, 1);
}

void MainWidget::initConnections()
{
    // 统一连接页面导航、用户操作、截图及播放控制信号。
    connect(pageStack, &QStackedLayout::currentChanged, this, &MainWidget::syncDetectionPreview);
    connect(static_cast<ReviewWidget *>(pages[SnapshotPage]), &ReviewWidget::relatedVideoRequested,
            this, &MainWidget::openSnapshotVideo);
    connect(static_cast<LogWidget *>(pages[LogPage]), &LogWidget::relatedVideoRequested,
            this, &MainWidget::openSnapshotVideo);
    connect(monitorScreenshotButton, &QPushButton::clicked,
            this, &MainWidget::saveMonitorSnapshot);
    connect(clockTimer, &QTimer::timeout,
            this, &MainWidget::updateTime);

    // 前四个导航按钮切换内部页面
    for (int i = 0; i < PageCount; ++i) {
        connect(navigationButtons[i], &QPushButton::clicked,
                this, [this, i]() {
                    pageStack->setCurrentIndex(i);
                });
    }

    // 最后一个导航按钮请求打开独立设置页面
    connect(navigationButtons[NavigationCount - 1],
            &QPushButton::clicked,
            this, &MainWidget::settingsRequested);
    connect(this,
            &MainWidget::settingsRequested,
            this,
            &MainWidget::openSettingsPage);

    // 通道选择
    for (int i = 0; i < ChannelCount; ++i) {
        connect(channelButtons[i], &QPushButton::clicked,
                this, [this, i]() {
                    selectChannel(i);
                });
    }

    // 预览模式切换
    connect(singleChannelButton, &QPushButton::clicked,
            this, [this]() {
                setSingleChannelMode(true);
            });

    connect(fourChannelButton, &QPushButton::clicked,
            this, [this]() {
                setSingleChannelMode(false);
            });

    connect(motionCheckBox, &QCheckBox::toggled,
            this, &MainWidget::motionDetectionChanged);
    connect(motionCheckBox, &QCheckBox::toggled,
            this, &MainWidget::syncDetectionPreview);

    connect(loginButton,
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
    // 将当前选中通道的实时画面保存为用户手动截图。
    if (monitorService == nullptr)
        return;
    const int channelNo = currentChannel + 1;
    const QImage image = monitorService->getChannelImage(currentChannel);
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
    // 根据当前登录状态执行登录页面切换或退出确认流程。
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
    // 刷新主页面显示的本地实时时间。
    timeLabel->setText(
        QDateTime::currentDateTime().toString(
            QStringLiteral("yyyy-MM-dd HH:mm:ss")));
}

void MainWidget::openSettingsPage()
{
    // 打开当前用户的系统设置页面并加载其配置。
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
    connect(
        &manager->settingsWidget,
        &SettingsWidget::settingsSaved,
        this,
        &MainWidget::updateChannelNames,
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
    // 更新登录用户显示、页面访问权限和用户关联服务。
    if (monitorService != nullptr && currentUsername != username.trimmed())
        monitorService->stopRecord();
    currentUsername = username.trimmed();
    loggedIn = !currentUsername.isEmpty();
    updateChannelNames();
    snapshotService.setCurrentUser(currentUsername);
    static_cast<ReviewWidget *>(pages[PlaybackPage])->setCurrentUser(currentUsername);
    static_cast<ReviewWidget *>(pages[SnapshotPage])->setCurrentUser(currentUsername);
    static_cast<LogWidget *>(pages[LogPage])->setCurrentUser(currentUsername);

    userLabel->setText(
        loggedIn
            ? currentUsername
            : QStringLiteral("未登录"));

    loginButton->setText(
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
        pageStack->setCurrentIndex(MonitorPage);
        navigationButtons[MonitorPage]->setChecked(true);
    }
}

void MainWidget::updateNavigationAccess()
{
    // 根据登录状态启用或禁用需要认证的页面入口。
    // 监控主页面始终允许访问。
    navigationButtons[MonitorPage]->setEnabled(true);

    // 普通回看、移动侦测回看、日志查看需要登录。
    for (int pageIndex = PlaybackPage;
         pageIndex <= LogPage;
         ++pageIndex) {
        navigationButtons[pageIndex]->setEnabled(loggedIn);
    }

    // 系统设置按钮不在内容堆栈中，但同样需要登录。
    navigationButtons[NavigationCount - 1]
        ->setEnabled(loggedIn);
}

void MainWidget::selectChannel(int channelIndex)
{
    // 切换当前预览通道并同步单路检测预览状态。
    if (channelIndex < 0 || channelIndex >= ChannelCount)
        return;

    const bool changed = currentChannel != channelIndex;

    currentChannel = channelIndex;
    channelButtons[channelIndex]->setChecked(true);

    // 单通道模式下，显示新选择的通道
    if (singleChannelMode)
        setSingleChannelMode(true);

    if (changed)
        emit currentChannelChanged(channelIndex);
}

void MainWidget::setSingleChannelMode(bool enabled)
{
    // 切换单画面/四画面布局并更新检测开关可用状态。
    singleChannelMode = enabled;

    // 移除原有布局位置，保留预览控件
    for (QLabel *label : previewLabels) {
        previewLayout->removeWidget(label);
        label->hide();
    }

    if (enabled) {
        // 当前通道占满整个预览区
        QLabel *label = previewLabels[currentChannel];
        previewLayout->addWidget(label, 0, 0, 2, 2);
        label->show();
    } else {
        // 恢复四通道排列
        for (int i = 0; i < ChannelCount; ++i) {
            previewLayout->addWidget(
                previewLabels[i], i / 2, i % 2);
            previewLabels[i]->show();
        }
    }

    singleChannelButton->setChecked(enabled);
    fourChannelButton->setChecked(!enabled);

    // 四通道模式必须取消勾选并禁用移动侦测
    if (!enabled)
        motionCheckBox->setChecked(false);

    motionCheckBox->setEnabled(enabled);
    motionCheckBox->setToolTip(QStringLiteral("勾选后在当前单通道开启人体检测"));
    syncDetectionPreview();
}
MainWidget::~MainWidget()
{
    // 主页面销毁前停止监控线程和相关后台服务。
    stopMonitorService();
}

void MainWidget::initMonitorService()
{
    // 创建并启动监控采集、帧处理及存储空间监测服务。
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
    // 将当前画面模式和移动侦测复选框状态同步给监控服务。
    if (monitorService == nullptr)
        return;
    const bool previewActive = isVisible()
        && pageStack->currentIndex() == MonitorPage
        && singleChannelMode;
    const bool detectionEnabled =
        singleChannelMode && motionCheckBox->isChecked();
    monitorService->setDetectionPreview(
        previewActive, currentChannel, detectionEnabled);
}

void MainWidget::showEvent(QShowEvent *event)
{
    // 页面显示时恢复必要的预览状态和服务。
    QWidget::showEvent(event);
    syncDetectionPreview();
}

void MainWidget::hideEvent(QHideEvent *event)
{
    // 页面隐藏时暂停不应继续展示的监控资源。
    QWidget::hideEvent(event);
    if (monitorService != nullptr)
        monitorService->setDetectionPreview(
            false, currentChannel, false);
}

void MainWidget::updateRecordDirectory()
{
    // 从当前用户设置更新录像服务的输出目录。
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

void MainWidget::updateChannelNames()
{
    // 按当前用户的配置更新通道按钮，缺失或空名称时使用通道编号。
    SystemConfigData configuration;
    for (int index = 0; index < ChannelCount; ++index) {
        configuration.channels[index].channelName =
            QStringLiteral("通道%1").arg(index + 1);
    }

    if (loggedIn) {
        SystemConfigService configService;
        QString errorMessage;
        const SystemConfigService::Result result =
            configService.load(currentUsername, configuration, errorMessage);

        if (result != SystemConfigService::Result::Success) {
            for (int index = 0; index < ChannelCount; ++index) {
                configuration.channels[index].channelName =
                    QStringLiteral("通道%1").arg(index + 1);
            }
        }
    }

    for (int index = 0; index < ChannelCount; ++index) {
        QString channelName =
            configuration.channels[index].channelName.trimmed();
        if (channelName.isEmpty())
            channelName = QStringLiteral("通道%1").arg(index + 1);
        channelButtons[index]->setText(channelName);
    }
}

void MainWidget::openSnapshotVideo(qulonglong recordId, qint64 offsetMs)
{
    // 从图片关联信息打开对应录像并定位到截图时间点。
    if (!loggedIn)
        return;
    ReviewWidget *playbackPage = static_cast<ReviewWidget *>(pages[PlaybackPage]);
    if (!playbackPage->playRecord(recordId, offsetMs))
        return;
    pageStack->setCurrentIndex(PlaybackPage);
    navigationButtons[PlaybackPage]->setChecked(true);
}

void MainWidget::stopMonitorService()
{
    // 关闭监控和存储监测服务，释放相关线程资源。
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
    // 刷新可见预览控件中的最新通道画面。
    if (monitorService == nullptr) {
        return;
    }

    // 这里只跳过绘制，不会停止采集和处理。
    if (!isVisible()
        || pageStack->currentIndex() != MonitorPage) {
        return;
    }

    for (int index = 0; index < ChannelCount; index++) {
        if (singleChannelMode
            && index != currentChannel) {
            continue;
        }

        QLabel *previewLabel = previewLabels[index];
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
    // 加载并应用主页面样式表。
    QFile file(QStringLiteral(":/styles/mainwidget.qss"));

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setStyleSheet(QString::fromUtf8(file.readAll()));
    } else {
        qWarning() << "主页面样式加载失败：" << file.errorString();
    }
}

void MainWidget::initStorageMonitor(const QString &storagePath)
{
    // 初始化磁盘空间监测，并将告警反馈到界面。
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
