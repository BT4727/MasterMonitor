#ifndef ADMINUSERSERVICE_H
#define ADMINUSERSERVICE_H
#include <QString>
class AdminUserService
{
public:
    AdminUserService();
    enum class LoginResult {
        Success,
        InvalidCredentials,
        DatabaseError,
        AccountLocked
    };
    LoginResult login(const QString &username,
                      const QString &password) const;

private:
    QString calculateMd5(const QString &text) const;
};

#endif // ADMINUSERSERVICE_H
