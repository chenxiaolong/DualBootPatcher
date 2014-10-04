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

#ifndef QCOMRAMDISKPATCHER_H
#define QCOMRAMDISKPATCHER_H

#include "libdbp-ramdisk-qcom_global.h"

#include <libdbp/cpiofile.h>
#include <libdbp/plugininterface.h>

#include <QtCore/QObject>
#include <QtCore/QVariant>

class QcomRamdiskPatcherPrivate;

class LIBDBPRAMDISKQCOMSHARED_EXPORT QcomRamdiskPatcher : public RamdiskPatcher
{
public:
    static const QString SystemPartition;
    static const QString CachePartition;
    static const QString DataPartition;

    static const QString ArgAdditionalFstabs;
    static const QString ArgForceSystemRw;
    static const QString ArgForceCacheRw;
    static const QString ArgForceDataRw;
    static const QString ArgKeepMountPoints;
    static const QString ArgSystemMountPoint;
    static const QString ArgCacheMountPoint;
    static const QString ArgDataMountPoint;
    static const QString ArgDefaultSystemMountArgs;
    static const QString ArgDefaultSystemVoldArgs;
    static const QString ArgDefaultCacheMountArgs;
    static const QString ArgDefaultCacheVoldArgs;

    explicit QcomRamdiskPatcher(const PatcherPaths * const pp,
                                const FileInfo * const info,
                                CpioFile * const cpio);
    ~QcomRamdiskPatcher();

    virtual PatcherError::Error error() const;
    virtual QString errorString() const;

    virtual QString name() const;

    virtual bool patchRamdisk();

    bool modifyInitRc();
    bool modifyInitQcomRc(QStringList additionalFiles = QStringList());

    bool modifyFstab(// Optional
                     bool removeModemMounts = false);
    bool modifyFstab(QVariantMap args,
                     // Optional
                     bool removeModemMounts = false);
    bool modifyFstab(bool removeModemMounts,
                     const QStringList &additionalFstabs,
                     bool forceSystemRw,
                     bool forceCacheRw,
                     bool forceDataRw,
                     bool keepMountPoints,
                     const QString &systemMountPoint,
                     const QString &cacheMountPoint,
                     const QString &dataMountPoint,
                     const QString &defaultSystemMountArgs,
                     const QString &defaultSystemVoldArgs,
                     const QString &defaultCacheMountArgs,
                     const QString &defaultCacheVoldArgs);

    bool modifyInitTargetRc();
    bool modifyInitTargetRc(QString filename);

private:
    static QString makeWritable(const QString &mountArgs);

    const QScopedPointer<QcomRamdiskPatcherPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QcomRamdiskPatcher)
};

#endif // QCOMRAMDISKPATCHER_H
