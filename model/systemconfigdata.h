#ifndef SYSTEMCONFIGDATA_H
#define SYSTEMCONFIGDATA_H

#include <QString>
#include <array>

struct ChannelConfig
{
    QString cameraName;
    QString channelName;
};

struct SystemConfigData
{
    static constexpr int ChannelCount = 4;

    QString videoSavePath;
    int segmentSeconds = 30;
    std::array<ChannelConfig, ChannelCount> channels;
};

#endif // SYSTEMCONFIGDATA_H
