#include "systemconfigservice.h"

#include "../model/configfilemodel.h"
#include "../model/userconfigmodel.h"

#include <QDir>
SystemConfigService::Result SystemConfigService::load(
    const QString &username,
    SystemConfigData &config,
    QString &errorMessage) const
{
    // 先从数据库取得用户配置文件路径，再读取本地配置内容。
    UserConfigModel userConfigModel;
    QString configPath;

    const UserConfigModel::Result pathResult =
        userConfigModel.findConfigPath(
            username, configPath);

    if (pathResult == UserConfigModel::Result::DatabaseError)
        return Result::DatabaseError;

    if (pathResult == UserConfigModel::Result::NotFound)
        return Result::NoConfig;

    ConfigFileModel fileModel;

    if (!fileModel.load(configPath, config, errorMessage))
        return Result::FileError;

    return Result::Success;
}

SystemConfigService::Result SystemConfigService::save(
    const QString &username,
    const SystemConfigData &config,
    QString &errorMessage) const
{
    // 校验配置后写入本地文件，并将配置文件路径保存到数据库。
    if (username.trimmed().isEmpty()) {
        errorMessage = QStringLiteral("当前用户无效");
        return Result::UserNotFound;
    }

    if (config.videoSavePath.trimmed().isEmpty()) {
        errorMessage = QStringLiteral("视频保存路径不能为空");
        return Result::InvalidConfig;
    }

    const bool intervalValid =
        config.segmentSeconds == 30
        || config.segmentSeconds == 60
        || config.segmentSeconds == 90
        || config.segmentSeconds == 120;

    if (!intervalValid) {
        errorMessage = QStringLiteral("视频切片长度无效");
        return Result::InvalidConfig;
    }

    for (const ChannelConfig &channel : config.channels) {
        if (channel.channelName.trimmed().isEmpty()) {
            errorMessage = QStringLiteral("通道名称不能为空");
            return Result::InvalidConfig;
        }
    }

    ConfigFileModel fileModel;

    if (!fileModel.ensureDirectory(
            config.videoSavePath, errorMessage)) {
        return Result::FileError;
    }

    const QString configPath =
        fileModel.configFilePath(username);

    if (!fileModel.save(
            configPath, config, errorMessage)) {
        return Result::FileError;
    }

    UserConfigModel userConfigModel;

    const UserConfigModel::Result databaseResult =
        userConfigModel.saveConfigPath(
            username,
            QDir::toNativeSeparators(configPath));

    if (databaseResult ==
        UserConfigModel::Result::NotFound) {
        return Result::UserNotFound;
    }

    if (databaseResult ==
        UserConfigModel::Result::DatabaseError) {
        return Result::DatabaseError;
    }

    return Result::Success;
}
