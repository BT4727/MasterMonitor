#include "snapshotservice.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QUuid>

void SnapshotService::setCurrentUser(const QString &username)
{
    // 将图片数据操作绑定到当前登录用户。
    snapshotModel.setCurrentUser(username);
}

bool SnapshotService::saveManualSnapshot(
    const QImage &image, int channelNo, const QString &filePath,
    const QDateTime &frameTime, qulonglong videoId, qint64 offsetMs,
    SnapshotInfo &snapshot, QString &errorMessage)
{
    // 校验手动截图参数，先安全写入图片，再创建数据库记录。
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

bool SnapshotService::saveEventSnapshot(
    const QImage &image, int channelNo, qulonglong eventId,
    qulonglong videoId, const QDateTime &eventTime,
    const QDateTime &frameTime, qint64 offsetMs, int sequence,
    const QString &directory, QString &errorMessage)
{
    // 保存移动事件截图，并关联异常事件、通道和录像时间点。
    errorMessage.clear();
    if (image.isNull() || eventId == 0 || videoId == 0 || offsetMs < 0
        || sequence < 1 || sequence > 3) {
        errorMessage = QStringLiteral("事件截图参数无效");
        return false;
    }
    if (!QDir().mkpath(directory)) {
        errorMessage = QStringLiteral("无法创建事件图片目录");
        return false;
    }
    const QString fileName = QStringLiteral("异常_通道%1_%2_%3.jpg")
        .arg(channelNo).arg(eventTime.toString(QStringLiteral("yyyyMMdd_HHmmss_zzz"))).arg(sequence);
    const QString filePath = QDir(directory).filePath(fileName);
    if (QFileInfo::exists(filePath)) {
        errorMessage = QStringLiteral("事件图片已经存在：%1").arg(filePath);
        return false;
    }
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        errorMessage = file.errorString();
        return false;
    }
    if (!image.save(&file, "JPG", 95)) {
        file.cancelWriting();
        errorMessage = QStringLiteral("事件图片编码失败");
        return false;
    }
    if (!file.commit()) {
        errorMessage = file.errorString();
        return false;
    }
    qulonglong snapshotId = 0;
    const bool inserted = snapshotModel.insertSnapshot(
        channelNo, QStringLiteral("auto"), QDateTime::currentDateTime(),
        frameTime, filePath, videoId, eventId, offsetMs, snapshotId, errorMessage);
    if (!inserted) {
        if (!QFile::remove(filePath))
            errorMessage += QStringLiteral("\n未能清理图片：%1").arg(filePath);
        return false;
    }
    return true;
}

bool SnapshotService::searchSnapshots(
    const QDateTime &startTime, const QDateTime &endTime, int channelNo,
    QVector<SnapshotInfo> &snapshots, QString &errorMessage)
{
    // 按时间和通道查询图片，并补齐可跳转的关联录像信息。
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
    // 暂存图片后删除数据库记录，失败时恢复文件以保持数据一致。
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
    // 将选中的图片批量复制到目标目录，并为重名文件生成新名称。
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
