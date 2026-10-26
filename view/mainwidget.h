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
    int m_currentChannel = 0;
    bool m_singleChannelMode = false;
    bool loggedIn = false;

    // 整体纵向布局
    QVBoxLayout *m_mainLayout = nullptr;

    // 顶部横向布局
    QWidget *m_headerWidget = nullptr;
    QHBoxLayout *m_headerLayout = nullptr;
    QLabel *m_titleLabel = nullptr;

    // 顶部右侧纵向布局
    QWidget *m_headerRightWidget = nullptr;
    QVBoxLayout *m_headerRightLayout = nullptr;

    // 时间及登录状态区域
    QWidget *m_statusWidget = nullptr;
    QHBoxLayout *m_statusLayout = nullptr;
    QVBoxLayout *m_userLayout = nullptr;
    QLabel *m_timeLabel = nullptr;
    QPushButton *m_loginButton = nullptr;
    QLabel *m_userLabel = nullptr;
    QTimer *m_clockTimer = nullptr;

    // 五个导航按钮
    QWidget *m_navigationWidget = nullptr;
    QHBoxLayout *m_navigationLayout = nullptr;
    QButtonGroup *m_navigationGroup = nullptr;
    std::array<QPushButton *, NavigationCount> m_navigationButtons{};

    // 四个内部内容页面
    QWidget *m_contentWidget = nullptr;
    QStackedLayout *m_pageStack = nullptr;
    std::array<QWidget *, PageCount> m_pages{};

    // 监控页面整体布局
    QHBoxLayout *m_monitorLayout = nullptr;

    // 左侧通道列表
    QWidget *m_channelWidget = nullptr;
    QVBoxLayout *m_channelLayout = nullptr;
    QButtonGroup *m_channelGroup = nullptr;
    std::array<QPushButton *, ChannelCount> m_channelButtons{};

    // 右侧预览及操作区域
    QWidget *m_monitorRightWidget = nullptr;
    QVBoxLayout *m_monitorRightLayout = nullptr;

    // 预览区域
    QWidget *m_previewWidget = nullptr;
    QGridLayout *m_previewLayout = nullptr;
    std::array<QLabel *, ChannelCount> m_previewLabels{};

    // 操作区域
    QWidget *m_controlsWidget = nullptr;
    QHBoxLayout *m_controlsLayout = nullptr;
    QPushButton *m_singleChannelButton = nullptr;
    QPushButton *m_fourChannelButton = nullptr;
    QButtonGroup *m_modeGroup = nullptr;
    QCheckBox *m_motionCheckBox = nullptr;
private slots:
    void refreshMonitorFrames();
    void stopMonitorService();
};

#endif // MAINWIDGET_H
