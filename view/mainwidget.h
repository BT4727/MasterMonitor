#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include <QString>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QStackedLayout>
#include <QButtonGroup>
#include <array>
#include <QFile>
#include <QDebug>
#include <QShowEvent>
#include <QHideEvent>
#include "../service/storagemonitorservice.h"
#include "../service/monitorservice.h"
#include "../service/snapshotservice.h"
#include "../service/logservice.h"

class MainWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MainWidget(QWidget *parent = nullptr);
    ~MainWidget() override;
    // 空字符串表示未登录
    void setCurrentUser(const QString &username);
protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
signals:
    // 交给页面管理类处理独立页面跳转
    void settingsRequested();

    // 通道编号使用 0～3
    void currentChannelChanged(int channelIndex);
    void motionDetectionChanged(bool enabled);
private:
    void initHeader();
    void initPages();
    void initMonitorPage();
    void initConnections();
    void syncDetectionPreview();
    void openSnapshotVideo(qulonglong recordId, qint64 offsetMs);
    void handleLoginButtonClicked();
    void openSettingsPage();
    void updateRecordDirectory();
    void updateChannelNames();
    void initStorageMonitor(const QString &storagePath);

    void updateTime();
    void setSingleChannelMode(bool enabled);
    void selectChannel(int channelIndex);
    void initQss();

    void updateNavigationAccess();
    void initMonitorService();
    void saveMonitorSnapshot();
    QPushButton *monitorScreenshotButton = nullptr;
    SnapshotService snapshotService;
    LogService logService;

    MonitorService *monitorService = nullptr;
    StorageMonitorService *storageMonitorService = nullptr;
    QTimer *previewTimer = nullptr;

    static constexpr int NavigationCount = 5;
    static constexpr int ChannelCount = 4;

    // 使用枚举增强表意性
    enum ContentPageIndex {
        MonitorPage = 0,
        PlaybackPage,
        SnapshotPage,
        LogPage,
        PageCount
    };

    // 当前状态
    QString currentUsername;
    int currentChannel = 0;
    bool singleChannelMode = false;
    bool loggedIn = false;

    // 整体纵向布局
    QVBoxLayout *mainLayout = nullptr;

    // 顶部横向布局
    QWidget *headerWidget = nullptr;
    QHBoxLayout *headerLayout = nullptr;
    QLabel *titleLabel = nullptr;

    // 顶部右侧纵向布局
    QWidget *headerRightWidget = nullptr;
    QVBoxLayout *headerRightLayout = nullptr;

    // 时间及登录状态区域
    QWidget *statusWidget = nullptr;
    QHBoxLayout *statusLayout = nullptr;
    QVBoxLayout *userLayout = nullptr;
    QLabel *timeLabel = nullptr;
    QPushButton *loginButton = nullptr;
    QLabel *userLabel = nullptr;
    QTimer *clockTimer = nullptr;

    // 五个导航按钮
    QWidget *navigationWidget = nullptr;
    QHBoxLayout *navigationLayout = nullptr;
    QButtonGroup *navigationGroup = nullptr;
    std::array<QPushButton *, NavigationCount> navigationButtons{};

    // 四个内部内容页面
    QWidget *contentWidget = nullptr;
    QStackedLayout *pageStack = nullptr;
    std::array<QWidget *, PageCount> pages{};

    // 监控页面整体布局
    QHBoxLayout *monitorLayout = nullptr;

    // 左侧通道列表
    QWidget *channelWidget = nullptr;
    QVBoxLayout *channelLayout = nullptr;
    QButtonGroup *channelGroup = nullptr;
    std::array<QPushButton *, ChannelCount> channelButtons{};

    // 右侧预览及操作区域
    QWidget *monitorRightWidget = nullptr;
    QVBoxLayout *monitorRightLayout = nullptr;

    // 预览区域
    QWidget *previewWidget = nullptr;
    QGridLayout *previewLayout = nullptr;
    std::array<QLabel *, ChannelCount> previewLabels{};

    // 操作区域
    QWidget *controlsWidget = nullptr;
    QHBoxLayout *controlsLayout = nullptr;
    QPushButton *singleChannelButton = nullptr;
    QPushButton *fourChannelButton = nullptr;
    QButtonGroup *modeGroup = nullptr;
    QCheckBox *motionCheckBox = nullptr;
private slots:
    void refreshMonitorFrames();
    void stopMonitorService();
};

#endif // MAINWIDGET_H
