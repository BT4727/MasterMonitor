#include "snapshotservice.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QUuid>

void SnapshotService::setCurrentUser(const QString &username)
{
    snapshotModel.setCurrentUser(username);
}

bool SnapshotService::saveManualSnapshot(
    const QImage &image, int channelNo, const QString &filePath,
    const QDateTime &frameTime, qulonglong videoId, qint64 offsetMs,
    SnapshotInfo &snapshot, QString &errorMessage)
{
    errorMessage.clear();
    snapshot = SnapshotInfo();
    if (image.isNull()) {
        errorMessage = QStringLiteral("当前没有可以保存的画面");
        return false;
    }
    if (channelNo < 1 || channelNo > 4 || !frameTime.isValid()) {
        errorMessage = QStringLiteral("截图通道或画面时间无效");
        return false;
    }
    const QFileInfo fileInfo(filePath);
    const QString suffix = fileInfo.suffix().toLower();
    if (suffix != QStringLiteral("jpg") && suffix != QStringLiteral("jpeg")
        && suffix != QStringLiteral("png")) {
        errorMessage = QStringLiteral("图片格式仅支持 JPG、JPEG 和 PNG");
        return false;
    }
    if (fileInfo.exists()) {
        errorMessage = QStringLiteral("该文件已经存在，请修改名称后重新保存");
        return false;
    }
    if (!QDir().mkpath(fileInfo.absolutePath())) {
        errorMessage = QStringLiteral("无法创建图片保存目录");
        return false;
    }
    snapshot.channelNo = channelNo;
    snapshot.snapshotType = QStringLiteral("manual");
    snapshot.createTime = QDateTime::currentDateTime();
    snapshot.frameTime = frameTime;
    snapshot.snapshotPath = fileInfo.absoluteFilePath();
    snapshot.videoId = videoId;
    snapshot.offsetMs = videoId == 0 ? -1 : offsetMs;
    if (snapshot.videoId == 0) {
        if (!snapshotModel.findRelatedVideo(snapshot, errorMessage))
            return false;
    }
    QSaveFile imageFile(snapshot.snapshotPath);
    if (!imageFile.open(QIODevice::WriteOnly)) {
        errorMessage = imageFile.errorString();
        return false;
    }
    const char *imageFormat = suffix == QStringLiteral("png") ? "PNG" : "JPG";
    if (!image.save(&imageFile, imageFormat, 95)) {
        imageFile.cancelWriting();
        errorMessage = QStringLiteral("图片编码或写入失败");
        return false;
    }
    if (!imageFile.commit()) {
        errorMessage = imageFile.errorString();
        return false;
    }
    const bool inserted = snapshotModel.insertSnapshot(
        snapshot.channelNo, snapshot.snapshotType, snapshot.createTime,
        snapshot.frameTime, snapshot.snapshotPath, snapshot.videoId,
        0, snapshot.offsetMs, snapshot.id, errorMessage);
    if (!inserted) {
        const QString databaseError = errorMessage;
        if (!QFile::remove(snapshot.snapshotPath)) {
            errorMessage = QStringLiteral("图片记录写入失败：%1\n同时无法清理图片文件：%2")
                .arg(databaseError, snapshot.snapshotPath);
        }
        return false;
    }
    return true;
}

bool SnapshotService::saveEventSnapshots(
    const QVector<QImage> &images, const QVector<QDateTime> &frameTimes,
    int channelNo, qulonglong eventId, const QString &directory,
    QVector<SnapshotInfo> &snapshots, QString &errorMessage)
{
    Q_UNUSED(images);
    Q_UNUSED(frameTimes);
    Q_UNUSED(channelNo);
    Q_UNUSED(eventId);
    Q_UNUSED(directory);
    snapshots.clear();
    // 后续传入三张间隔帧和画面时间，按“异常_通道_时间_序号”命名，
    // snapshot_type 写入 auto，event_id 关联事件。当前仅预留接口。
    errorMessage = QStringLiteral("自动事件截图接口尚未接入移动侦测");
    return false;
}

bool SnapshotService::searchSnapshots(
    const QDateTime &startTime, const QDateTime &endTime, int channelNo,
    QVector<SnapshotInfo> &snapshots, QString &errorMessage)
{
    errorMessage.clear();
    if (!startTime.isValid() || !endTime.isValid() || endTime <= startTime) {
        errorMessage = QStringLiteral("图片查询时间范围无效");
        return false;
    }
    if (!snapshotModel.searchSnapshots(startTime, endTime, channelNo, snapshots, errorMessage))
        return false;
    for (SnapshotInfo &snapshot : snapshots) {
        if (snapshot.videoId == 0) {
            if (!snapshotModel.findRelatedVideo(snapshot, errorMessage))
                return false;
        }
    }
    return true;
}

bool SnapshotService::deleteSnapshot(const SnapshotInfo &snapshot, QString &errorMessage)
{
    errorMessage.clear();
    const bool fileExists = QFileInfo::exists(snapshot.snapshotPath);
    QString temporaryPath;
    // 暂存文件，数据库删除失败时恢复原文件。
    if (fileExists) {
        temporaryPath = snapshot.snapshotPath + QStringLiteral(".delete_")
            + QUuid::createUuid().toString(QUuid::WithoutBraces);
        if (!QFile::rename(snapshot.snapshotPath, temporaryPath)) {
            errorMessage = QStringLiteral("图片正在被占用或没有删除权限：\n%1")
                .arg(snapshot.snapshotPath);
            return false;
        }
    }
    if (!snapshotModel.deleteSnapshot(snapshot.id, errorMessage)) {
        if (fileExists && !QFile::rename(temporaryPath, snapshot.snapshotPath)) {
            errorMessage += QStringLiteral("\n恢复图片失败，文件暂存于：\n%1").arg(temporaryPath);
        }
        return false;
    }
    if (fileExists && !QFile::remove(temporaryPath)) {
        errorMessage = QStringLiteral("数据库记录已删除，但临时文件清理失败：\n%1")
            .arg(temporaryPath);
        return false;
    }
    return true;
}

bool SnapshotService::exportSnapshots(
    const QVector<SnapshotInfo> &snapshots, const QString &targetDirectory,
    QString &errorMessage)
{
    errorMessage.clear();
    if (snapshots.isEmpty()) {
        errorMessage = QStringLiteral("请先选择需要导出的图片");
        return false;
    }
    QDir outputDirectory(targetDirectory);
    if (!outputDirectory.exists() && !QDir().mkpath(targetDirectory)) {
        errorMessage = QStringLiteral("无法创建导出目录");
        return false;
    }
    int exportedCount = 0;
    for (const SnapshotInfo &snapshot : snapshots) {
        const QFileInfo sourceInfo(snapshot.snapshotPath);
        if (!sourceInfo.isFile()) {
            errorMessage = QStringLiteral("已导出%1张，以下图片不存在：\n%2")
                .arg(exportedCount).arg(snapshot.snapshotPath);
            return false;
        }
        const QString baseName = sourceInfo.completeBaseName();
        const QString suffix = sourceInfo.suffix();
        QString targetPath = outputDirectory.filePath(sourceInfo.fileName());
        int duplicateIndex = 1;
        while (QFileInfo::exists(targetPath)) {
            const QString fileName = QStringLiteral("%1_%2.%3")
                .arg(baseName).arg(duplicateIndex++).arg(suffix);
            targetPath = outputDirectory.filePath(fileName);
        }
        if (!QFile::copy(snapshot.snapshotPath, targetPath)) {
            errorMessage = QStringLiteral("已导出%1张，复制图片失败：\n%2")
                .arg(exportedCount).arg(targetPath);
            return false;
        }
        ++exportedCount;
    }
    return true;
}
