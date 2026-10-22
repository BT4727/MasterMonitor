#include "storagemonitorservice.h"

#include <QDebug>
#include <QStorageInfo>
#include <QtMath>

StorageMonitorService::StorageMonitorService(QObject *parent)
    : QObject(parent)
{
    checkTimer.setInterval(CheckIntervalMs);

    connect(
        &checkTimer,
        &QTimer::timeout,
        this,
        &StorageMonitorService::checkStorageSpace);
}

void StorageMonitorService::start(const QString &path)
{
    setStoragePath(path);

    if (!checkTimer.isActive())
        checkTimer.start();

    checkStorageSpace();
}

void StorageMonitorService::setStoragePath(const QString &path)
{
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
    checkTimer.stop();
}

void StorageMonitorService::checkStorageSpace()
{
    if (storagePath.isEmpty())
        return;

    QStorageInfo storageInfo(storagePath);
    storageInfo.refresh();

    if (!storageInfo.isValid()
        || !storageInfo.isReady()
        || storageInfo.bytesTotal() <= 0) {
        return;
    }

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

