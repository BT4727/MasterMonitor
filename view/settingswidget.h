#ifndef SETTINGSWIDGET_H
#define SETTINGSWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QCloseEvent>
#include <QString>
#include <array>
#include "../model/systemconfigdata.h"
#include "../service/systemconfigservice.h"
#include "../service/logservice.h"

class SettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsWidget(QWidget *parent = nullptr);
    bool openForUser(const QString &username);

signals:
    void settingsSaved();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    static constexpr int ChannelCount = 4;

    void initForm();
    void initButtons();
    void initConnect();
    void initQss();

    void refreshCameraDevices();
    void applyConfiguration(const SystemConfigData &configuration);
    SystemConfigData readConfiguration() const;
    SystemConfigData createDefaultConfiguration() const;
    QString resultMessage(SystemConfigService::Result result,
                          const QString &errorMessage) const;
    void returnToMain();

private slots:
    void onBrowseClicked();
    void onSaveClicked();
    void onCancelClicked();

private:
    QString currentUsername;
    SystemConfigData loadedConfiguration;
    SystemConfigService configService;
    LogService logService;

    QVBoxLayout *mainLayout = nullptr;
    QLabel *titleLabel = nullptr;

    // 表单区域
    QWidget *formWidget = nullptr;
    QVBoxLayout *formLayout = nullptr;

    // 保存路径
    QHBoxLayout *pathLayout = nullptr;
    QLabel *pathLabel = nullptr;
    QLineEdit *pathEdit = nullptr;
    QPushButton *browseButton = nullptr;

    // 间隔时间
    QHBoxLayout *intervalLayout = nullptr;
    QLabel *intervalLabel = nullptr;
    QComboBox *intervalComboBox = nullptr;

    // 四个通道
    QGridLayout *channelLayout = nullptr;
    std::array<QLineEdit *, ChannelCount> cameraEdits{};
    std::array<QLineEdit *, ChannelCount> channelNameEdits{};
    std::array<QLabel *, ChannelCount> channelLabels{};

    // 底部按钮
    QHBoxLayout *buttonLayout = nullptr;
    QPushButton *saveButton = nullptr;
    QPushButton *cancelButton = nullptr;
};

#endif // SETTINGSWIDGET_H
