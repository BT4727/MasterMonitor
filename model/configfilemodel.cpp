#include "configfilemodel.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>
#include <QFileInfo>

ConfigFileModel::ConfigFileModel() {}

QString ConfigFileModel::configFilePath(
    const QString &username) const
{
    const QString configDirectory =
        QCoreApplication::applicationDirPath()
        + QStringLiteral("/config");

    return QDir(configDirectory).filePath(
        username + QStringLiteral(".json"));
}

bool ConfigFileModel::ensureDirectory(
    const QString &directoryPath,
    QString &errorMessage) const
{
    QDir directory(directoryPath);

    if (directory.exists())
        return true;

    if (QDir().mkpath(directoryPath))
        return true;

    errorMessage = QStringLiteral("无法创建目录：%1")
                       .arg(directoryPath);
    return false;
}

bool ConfigFileModel::save(
    const QString &filePath,
    const SystemConfigData &config,
    QString &errorMessage) const
{
    const QString configDirectory =
        QFileInfo(filePath).absolutePath();

    if (!ensureDirectory(configDirectory, errorMessage))
        return false;

    QJsonArray channelArray;

    for (const ChannelConfig &channel : config.channels) {
        QJsonObject channelObject;
        channelObject.insert(
            QStringLiteral("cameraName"),
            channel.cameraName);
        channelObject.insert(
            QStringLiteral("channelName"),
            channel.channelName);

        channelArray.append(channelObject);
    }

    QJsonObject rootObject;
    rootObject.insert(
        QStringLiteral("videoSavePath"),
        config.videoSavePath);
    rootObject.insert(
        QStringLiteral("segmentSeconds"),
        config.segmentSeconds);
    rootObject.insert(
        QStringLiteral("channels"),
        channelArray);

    QSaveFile file(filePath);

    if (!file.open(QIODevice::WriteOnly)) {
        errorMessage = file.errorString();
        return false;
    }

    const QJsonDocument document(rootObject);

    if (file.write(document.toJson(QJsonDocument::Indented)) < 0) {
        errorMessage = file.errorString();
        file.cancelWriting();
        return false;
    }

    if (!file.commit()) {
        errorMessage = file.errorString();
        return false;
    }

    return true;
}

bool ConfigFileModel::load(
    const QString &filePath,
    SystemConfigData &config,
    QString &errorMessage) const
{
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        errorMessage = file.errorString();
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document =
        QJsonDocument::fromJson(
            file.readAll(), &parseError);

    if (parseError.error != QJsonParseError::NoError
        || !document.isObject()) {
        errorMessage = QStringLiteral("配置文件格式错误：%1")
                           .arg(parseError.errorString());
        return false;
    }

    const QJsonObject rootObject = document.object();

    config.videoSavePath =
        rootObject.value(
                      QStringLiteral("videoSavePath")).toString();

    config.segmentSeconds =
        rootObject.value(
                      QStringLiteral("segmentSeconds")).toInt(30);

    const QJsonArray channelArray =
        rootObject.value(
                      QStringLiteral("channels")).toArray();

    const int channelCount =
        qMin(channelArray.size(),
             SystemConfigData::ChannelCount);

    for (int index = 0; index < channelCount; ++index) {
        const QJsonObject channelObject =
            channelArray.at(index).toObject();

        config.channels[index].cameraName =
            channelObject.value(
                             QStringLiteral("cameraName")).toString();

        config.channels[index].channelName =
            channelObject.value(
                             QStringLiteral("channelName")).toString();
    }

    return true;
}