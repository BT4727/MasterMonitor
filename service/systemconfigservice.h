#ifndef SYSTEMCONFIGSERVICE_H
#define SYSTEMCONFIGSERVICE_H
#include "../model/systemconfigdata.h"
#include <QString>
class SystemConfigService
{
public:
    SystemConfigService();
    enum class Result {
        Success,
        NoConfig,
        InvalidConfig,
        FileError,
        DatabaseError,
        UserNotFound
    };

    Result load(const QString &username,
                SystemConfigData &config,
                QString &errorMessage) const;

    Result save(const QString &username,
                const SystemConfigData &config,
                QString &errorMessage) const;
};

#endif // SYSTEMCONFIGSERVICE_H
