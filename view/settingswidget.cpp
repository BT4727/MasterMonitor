#include "settingswidget.h"
#include "../util/winmanager.h"

#include <QFileDialog>
#include <QCameraDevice>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QMediaDevices>
#include <QMessageBox>

SettingsWidget::SettingsWidget(QWidget *parent)
    : QWidget{parent}
{
    // 创建系统配置表单并加载默认显示内容。
    setWindowTitle(QStringLiteral("智能监控系统设置页"));
    resize(900, 650);
    setMinimumSize(700, 720);

    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 32, 40, 32);
    mainLayout->setSpacing(0);

    titleLabel = new QLabel(
        QStringLiteral("智能监控系统设置页"), this);
    titleLabel->setObjectName(
        QStringLiteral("settingsTitle"));
    titleLabel->setAlignment(Qt::AlignCenter);

    mainLayout->addWidget(titleLabel);
    mainLayout->addSpacing(56);

    initForm();
    initButtons();
    initConnect();

    mainLayout->addWidget(formWidget);
    mainLayout->addStretch(1);
    mainLayout->addSpacing(24);
    mainLayout->addLayout(buttonLayout);

    loadedConfiguration = createDefaultConfiguration();
    applyConfiguration(loadedConfiguration);
    initQss();
}

bool SettingsWidget::openForUser(const QString &username)
{
    // 打开设置时读取用户配置并刷新本机摄像头名称。
    const QString normalizedUsername = username.trimmed();

    if (normalizedUsername.isEmpty()) {
        QMessageBox::warning(
            this,
            QStringLiteral("无法打开设置"),
            QStringLiteral("请先登录后再打开系统设置"));
        return false;
    }

    SystemConfigData configuration = createDefaultConfiguration();
    QString errorMessage;
    const SystemConfigService::Result result =
        configService.load(
            normalizedUsername,
            configuration,
            errorMessage);

    if (result != SystemConfigService::Result::Success
        && result != SystemConfigService::Result::NoConfig) {
        QMessageBox::warning(
            this,
            QStringLiteral("配置加载失败"),
            resultMessage(result, errorMessage));
        return false;
    }

    currentUsername = normalizedUsername;
    applyConfiguration(configuration);
    refreshCameraDevices();
    loadedConfiguration = readConfiguration();
    return true;
}

void SettingsWidget::initForm()
{
    // 创建保存路径、切片长度和通道配置表单。
    formWidget = new QWidget(this);
    formLayout = new QVBoxLayout(formWidget);
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setSpacing(24);

    // 保存路径：文字 + 输入框 + 浏览按钮
    pathLayout = new QHBoxLayout;
    pathLayout->setContentsMargins(0, 0, 0, 0);
    pathLayout->setSpacing(8);

    pathLabel = new QLabel(
        QStringLiteral("保存路径："), formWidget);

    pathEdit = new QLineEdit(formWidget);
    pathEdit->setPlaceholderText(
        QStringLiteral("请选择保存目录"));

    browseButton = new QPushButton(
        QStringLiteral("浏览"), formWidget);

    pathLayout->addWidget(pathLabel);
    pathLayout->addWidget(pathEdit, 1);
    pathLayout->addWidget(browseButton);

    // 间隔时间
    intervalLayout = new QHBoxLayout;
    intervalLayout->setContentsMargins(0, 0, 0, 0);
    intervalLayout->setSpacing(8);

    intervalLabel = new QLabel(
        QStringLiteral("间隔时间："), formWidget);

    intervalComboBox = new QComboBox(formWidget);

    // 文本用于显示，附加数据用于后续读取秒数
    for (int seconds : {30, 60, 90, 120}) {
        intervalComboBox->addItem(
            QStringLiteral("%1s").arg(seconds),
            seconds);
    }


    intervalComboBox->setFixedWidth(110);

    intervalLayout->addWidget(intervalLabel);
    intervalLayout->addWidget(intervalComboBox);
    intervalLayout->addStretch();

    // 通道配置：摄像头名称、通道名称、通道编号
    channelLayout = new QGridLayout;
    channelLayout->setContentsMargins(0, 0, 0, 0);

    // 上下间隔稍大，左右间隔较小
    channelLayout->setVerticalSpacing(24);
    channelLayout->setHorizontalSpacing(8);

    channelLayout->setColumnStretch(0, 0);
    channelLayout->setColumnStretch(1, 0);
    channelLayout->setColumnStretch(2, 0);

    // 剩余空间全部放在通道编号右侧
    channelLayout->setColumnStretch(3, 1);

    for (int i = 0; i < ChannelCount; ++i) {
        cameraEdits[i] = new QLineEdit(formWidget);
        cameraEdits[i]->setReadOnly(true);
        cameraEdits[i]->setFocusPolicy(Qt::NoFocus);
        cameraEdits[i]->setPlaceholderText(
            QStringLiteral("未检测到摄像头"));
        cameraEdits[i]->setAccessibleName(
            QStringLiteral("通道%1摄像头名称").arg(i + 1));

        channelNameEdits[i] =
            new QLineEdit(formWidget);
        channelNameEdits[i]->setAccessibleName(
            QStringLiteral("通道%1名称").arg(i + 1));

        cameraEdits[i]->setFixedWidth(220);
        channelNameEdits[i]->setFixedWidth(140);

        channelLabels[i] = new QLabel(
            QStringLiteral("通道%1").arg(i + 1),
            formWidget);

        channelLayout->addWidget(
            cameraEdits[i], i, 0);
        channelLayout->addWidget(
            channelNameEdits[i], i, 1);
        channelLayout->addWidget(
            channelLabels[i], i, 2);
    }

    formLayout->addLayout(pathLayout);
    formLayout->addLayout(intervalLayout);

    formLayout->addSpacing(24);

    formLayout->addLayout(channelLayout);
}

void SettingsWidget::initButtons()
{
    // 创建保存和取消操作按钮。
    buttonLayout = new QHBoxLayout;
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(24);

    saveButton = new QPushButton(
        QStringLiteral("保存"), this);
    saveButton->setObjectName(
        QStringLiteral("saveSettingsButton"));

    cancelButton = new QPushButton(
        QStringLiteral("取消"), this);

    buttonLayout->addStretch();
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addStretch();
}

void SettingsWidget::initConnect()
{
    // 连接浏览目录、保存配置和取消操作。
    connect(browseButton,
            &QPushButton::clicked,
            this,
            &SettingsWidget::onBrowseClicked);

    connect(saveButton,
            &QPushButton::clicked,
            this,
            &SettingsWidget::onSaveClicked);

    connect(cancelButton,
            &QPushButton::clicked,
            this,
            &SettingsWidget::onCancelClicked);
}

void SettingsWidget::initQss()
{
    // 从Qt资源加载系统设置页面样式。
    QFile file(QStringLiteral(":/styles/settingswidget.qss"));

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setStyleSheet(QString::fromUtf8(file.readAll()));
    } else {
        qWarning() << "设置页样式加载失败："
                   << file.errorString();
    }
}

void SettingsWidget::refreshCameraDevices()
{
    // 枚举本机摄像头设备并更新只读设备名称字段。
    const QList<QCameraDevice> cameraDevices =
        QMediaDevices::videoInputs();

    for (int index = 0; index < ChannelCount; ++index) {
        const QString cameraName =
            index < cameraDevices.size()
                ? cameraDevices.at(index).description()
                : QString();

        cameraEdits[index]->setText(cameraName);
    }
}

void SettingsWidget::applyConfiguration(
    const SystemConfigData &configuration)
{
    // 将配置数据映射到对应表单控件。
    pathEdit->setText(configuration.videoSavePath);

    const int intervalIndex =
        intervalComboBox->findData(configuration.segmentSeconds);

    intervalComboBox->setCurrentIndex(
        intervalIndex >= 0 ? intervalIndex : 0);

    for (int index = 0; index < ChannelCount; ++index) {
        cameraEdits[index]->setText(
            configuration.channels[index].cameraName);
        channelNameEdits[index]->setText(
            configuration.channels[index].channelName);
    }
}

SystemConfigData SettingsWidget::readConfiguration() const
{
    // 从表单控件读取当前系统配置。
    SystemConfigData configuration;
    configuration.videoSavePath = pathEdit->text().trimmed();
    configuration.segmentSeconds =
        intervalComboBox->currentData().toInt();

    for (int index = 0; index < ChannelCount; ++index) {
        configuration.channels[index].cameraName =
            cameraEdits[index]->text().trimmed();
        configuration.channels[index].channelName =
            channelNameEdits[index]->text().trimmed();
    }

    return configuration;
}

SystemConfigData SettingsWidget::createDefaultConfiguration() const
{
    // 生成未保存配置时使用的默认路径和通道名称。
    SystemConfigData configuration;
    configuration.videoSavePath =
        QDir(QCoreApplication::applicationDirPath())
            .filePath(QStringLiteral("videos"));
    configuration.segmentSeconds = 30;

    for (int index = 0; index < ChannelCount; ++index) {
        configuration.channels[index].channelName =
            QStringLiteral("测试通道");
    }

    return configuration;
}

QString SettingsWidget::resultMessage(
    SystemConfigService::Result result,
    const QString &errorMessage) const
{
    // 将配置服务结果转换为用户可读的提示文本。
    if (!errorMessage.trimmed().isEmpty()) {
        return errorMessage;
    }

    switch (result) {
    case SystemConfigService::Result::InvalidConfig:
        return QStringLiteral("配置信息不完整或格式不正确");
    case SystemConfigService::Result::FileError:
        return QStringLiteral("配置文件读取或写入失败");
    case SystemConfigService::Result::DatabaseError:
        return QStringLiteral("数据库查询或保存失败");
    case SystemConfigService::Result::UserNotFound:
        return QStringLiteral("当前登录用户不存在");
    case SystemConfigService::Result::NoConfig:
        return QStringLiteral("当前用户尚未保存配置");
    case SystemConfigService::Result::Success:
        return QString();
    }

    return QStringLiteral("未知配置错误");
}

void SettingsWidget::onBrowseClicked()
{
    // 选择并显示录像文件保存目录。
    const QString currentPath = pathEdit->text().trimmed();
    const QString directory =
        QFileDialog::getExistingDirectory(
            this,
            QStringLiteral("选择保存目录"),
            currentPath.isEmpty()
                ? QDir::homePath()
                : currentPath);

    if (!directory.isEmpty()) {
        pathEdit->setText(
            QDir::toNativeSeparators(directory));
    }
}

void SettingsWidget::onSaveClicked()
{
    // 保存当前用户配置并反馈文件或数据库写入结果。
    const SystemConfigData configuration = readConfiguration();
    QString errorMessage;
    const SystemConfigService::Result result =
        configService.save(
            currentUsername,
            configuration,
            errorMessage);

    const bool success = result == SystemConfigService::Result::Success;
    const QString logDescription = success
        ? QStringLiteral("系统配置保存成功，录像目录：%1，切片时长：%2秒")
              .arg(configuration.videoSavePath)
              .arg(configuration.segmentSeconds)
        : QStringLiteral("系统配置保存失败：%1")
              .arg(resultMessage(result, errorMessage));
    logService.recordOperation(
        currentUsername, 0, QStringLiteral("config"),
        QStringLiteral("config"), 0, success, logDescription);

    if (result != SystemConfigService::Result::Success) {
        QMessageBox::warning(
            this,
            QStringLiteral("配置保存失败"),
            resultMessage(result, errorMessage));
        return;
    }

    loadedConfiguration = configuration;
    QMessageBox::information(
        this,
        QStringLiteral("保存成功"),
        QStringLiteral("系统配置已保存"));
    emit settingsSaved();
    returnToMain();
}

void SettingsWidget::onCancelClicked()
{
    // 放弃本次表单修改并恢复上次加载的配置。
    applyConfiguration(loadedConfiguration);
    refreshCameraDevices();
    returnToMain();
}

void SettingsWidget::returnToMain()
{
    // 隐藏设置页并返回主页面。
    WinManager::getInstance()->mainWidget.show();
    hide();
}

void SettingsWidget::closeEvent(QCloseEvent *event)
{
    // 将窗口关闭操作按取消处理，避免页面管理器持有对象被销毁。
    // 点击窗口右上角关闭时，按取消处理
    applyConfiguration(loadedConfiguration);
    refreshCameraDevices();
    returnToMain();

    // 页面由管理工具持有，隐藏即可
    event->ignore();
}

