#include "adminuserservice.h"
#include "../model/adminusermodel.h"

#include <QCryptographicHash>
#include <QDateTime>
AdminUserService::AdminUserService() {}

QString AdminUserService::calculateMd5(const QString &text) const
{
    const QByteArray textBytes = text.toUtf8();

    const QByteArray hashBytes =
        QCryptographicHash::hash(
            textBytes,
            QCryptographicHash::Md5);

    return QString::fromLatin1(hashBytes.toHex());
}

AdminUserService::LoginResult AdminUserService::login(const QString &username, const QString &password) const
{
    constexpr int maxLoginErrorTimes = 1;
    constexpr int lockSeconds = 30;

    AdminUserModel userModel;
    AdminUserRecord userRecord;

    const AdminUserModel::QueryResult queryResult =
        userModel.findByUsername(
            username,
            userRecord);

    if (queryResult ==
        AdminUserModel::QueryResult::DatabaseError) {
        return LoginResult::DatabaseError;
    }

    if (queryResult ==
        AdminUserModel::QueryResult::NotFound) {
        // 用户名不存在时没有对应账户可累计次数。
        return LoginResult::InvalidCredentials;
    }

    const QDateTime currentTime =
        QDateTime::currentDateTime();

    if (userRecord.lockedUntil.isValid()
        && userRecord.lockedUntil > currentTime) {
        return LoginResult::AccountLocked;
    }

    // 锁定时间已过，清除历史失败状态后允许重新登录。
    if (userRecord.lockedUntil.isValid()
        && userRecord.lockedUntil <= currentTime) {
        const bool cleared =
            userModel.clearLoginFailures(username);

        if (!cleared)
            return LoginResult::DatabaseError;

        userRecord.errorTimes = 0;
        userRecord.lockedUntil = QDateTime();
    }

    const QString inputPasswordMd5 =
        calculateMd5(password);

    const bool passwordCorrect =
        inputPasswordMd5.compare(
            userRecord.passwordMd5,
            Qt::CaseInsensitive) == 0;

    if (!userRecord.enabled || !passwordCorrect) {
        const int failedTimes =
            userRecord.errorTimes + 1;

        QDateTime lockedUntil;

        if (failedTimes >= maxLoginErrorTimes) {
            lockedUntil =
                currentTime.addSecs(lockSeconds);
        }

        const bool updated =
            userModel.updateLoginFailure(
                username,
                failedTimes,
                lockedUntil);

        if (!updated)
            return LoginResult::DatabaseError;

        if (failedTimes >= maxLoginErrorTimes)
            return LoginResult::AccountLocked;

        return LoginResult::InvalidCredentials;
    }

    // 登录成功后清除历史失败次数。
    const bool cleared =
        userModel.clearLoginFailures(username);

    if (!cleared)
        return LoginResult::DatabaseError;

    return LoginResult::Success;
}
