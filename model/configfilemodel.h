#ifndef CONFIGFILEMODEL_H
#define CONFIGFILEMODEL_H

#include "systemconfigdata.h"
#include <QString>
class ConfigFileModel
{
public:
    QString configFilePath(const QString &username) const;

    bool ensureDirectory(const QString &directoryPath,
                         QString &errorMessage) const;

    bool save(const QString &filePath,
              const SystemConfigData &config,
              QString &errorMessage) const;

    bool load(const QString &filePath,
              SystemConfigData &config,
              QString &errorMessage) const;
};

#endif // CONFIGFILEMODEL_H
