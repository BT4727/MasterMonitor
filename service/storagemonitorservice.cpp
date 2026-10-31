#include "storagemonitorservice.h"

#include <QDebug>
#include <QStorageInfo>
#include <QtMath>

StorageMonitorService::StorageMonitorService(QObject *parent)
    : QObject(parent)
{
    // 配置周期检测，并在检测到低空间时通知界面。
    checkTimer.setInterval(CheckIntervalMs);

    connect(
        &checkTimer,
        &QTimer::timeout,
        this,
        &StorageMonitorService::checkStorageSpace);
}

void StorageMonitorService::start(const QString &path)
{
    // 设置监测路径并立即执行一次磁盘空间检查。
    setStoragePath(path);

    if (!checkTimer.isActive())
        checkTimer.start();

    checkStorageSpace();
}

void StorageMonitorService::setStoragePath(const QString &path)
{
    // 更新磁盘监测目标，避免相同路径重复触发状态重置。
    const QString normalizedPath = path.trimmed();

    if (storagePath == normalizedPath)
        return;

    storagePath = normalizedPath;
    warningActive = false;

    if (checkTimer.isActive())
        checkStorageSpace();
}

void StorageMonitorService::stop()
{
    // 停止磁盘空间定时检查。
    checkTimer.stop();
}

void StorageMonitorService::checkStorageSpace()
{
    // 计算剩余容量比例，在首次低于阈值时记录并发出告警。
    if (storagePath.isEmpty())
        return;

    QStorageInfo storageInfo(storagePath);
    storageInfo.refresh();

    if (!storageInfo.isValid()
        || !storageInfo.isReady()
        || storageInfo.bytesTotal() <= 0) {
        return;
    }

    // 使用可用字节与总字节的比例判断阈值，避免依赖格式化字符串。
    const double remainingRatio =
        static_cast<double>(storageInfo.bytesAvailable())
        / static_cast<double>(storageInfo.bytesTotal());

    if (remainingRatio > WarningRatio) {
        warningActive = false;
        return;
    }

    if (warningActive)
        return;

    warningActive = true;

    const int remainingPercent = qFloor(remainingRatio * 100.0);
    const QString message =
        QStringLiteral("录像存储空间仅剩 %1%，请及时清理磁盘或更改录像保存路径。\n%2")
            .arg(remainingPercent)
            .arg(storagePath);

    QString errorMessage;

    if (!eventRecordModel.insertDiskLowEvent(
            remainingPercent,
            storagePath,
            errorMessage)) {
        qWarning() << "写入磁盘空间告警日志失败：" << errorMessage;
    }

    emit storageSpaceWarning(message);
}

