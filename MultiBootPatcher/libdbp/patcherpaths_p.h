/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

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
