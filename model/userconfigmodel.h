#ifndef USERCONFIGMODEL_H
#define USERCONFIGMODEL_H
#include <QString>
class UserConfigModel
{
public:
    enum class Result {
        Success,
        NotFound,
        DatabaseError
    };

    Result findConfigPath(const QString &username,
                          QString &configPath) const;

    Result saveConfigPath(const QString &username,
                          const QString &configPath) const;
};

#endif // USERCONFIGMODEL_H
