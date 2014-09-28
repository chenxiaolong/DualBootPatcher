#ifndef PATCHERPATHS_P_H
#define PATCHERPATHS_P_H

#include "patcherpaths.h"

#include <QtCore/QPluginLoader>
#include <QtCore/QString>

class IPatcherFactory;
class IAutoPatcherFactory;
class IRamdiskPatcherFactory;

class PatcherPathsPrivate
{
public:
    ~PatcherPathsPrivate();

    // Files
    QString configFile;

    // Directories
    QString binariesDir;
    QString dataDir;
    QString initsDir;
    QString patchesDir;
    QString patchInfosDir;
    QString pluginsDir;
    QString scriptsDir;

    // Config
    QString version;
    QList<Device *> devices;
    QStringList patchinfoIncludeDirs;

    // PatchInfos
    QList<PatchInfo *> patchInfos;

    // Partition configurations
    QList<PartitionConfig *> partConfigs;

    // Plugins
    QList<QPluginLoader *> pluginLoaders;
    QList<IPatcherFactory *> patcherFactories;
    QList<IAutoPatcherFactory *> autoPatcherFactories;
    QList<IRamdiskPatcherFactory *> ramdiskPatcherFactories;

    bool loadedConfig;

    // Errors
    PatcherError::Error errorCode;
    QString errorString;
};

#endif // PATCHERPATHS_P_H
