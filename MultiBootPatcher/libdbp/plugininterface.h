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

#ifndef PLUGININTERFACE_H
#define PLUGININTERFACE_H

#include "libdbp_global.h"

#include "cpiofile.h"
#include "fileinfo.h"
#include "partitionconfig.h"
#include "patcherpaths.h"
#include "patchinfo.h"

#include <QtCore/QtPlugin>

#include <QtCore/QStringList>


class LIBDBPSHARED_EXPORT Patcher : public QObject
{
    Q_OBJECT

public:
    explicit Patcher(QObject *parent = 0) : QObject(parent) {
    }

    virtual ~Patcher() {}

    // Error reporting
    virtual PatcherError::Error error() const = 0;
    virtual QString errorString() const = 0;

    // Patcher info
    virtual QString id() const = 0;
    virtual QString name() const = 0;
    virtual bool usesPatchInfo() const = 0;
    virtual QStringList supportedPartConfigIds() const = 0;

    // Patching
    virtual void setFileInfo(const FileInfo * const info) = 0;

    virtual QString newFilePath() = 0;

    virtual bool patchFile() = 0;

signals:
    void maxProgressUpdated(int max);
    void progressUpdated(int max);
    void detailsUpdated(const QString &details);
};

class IPatcherFactory
{
public:
    virtual ~IPatcherFactory() {}

    virtual QStringList patchers() const = 0;
    virtual QString patcherName(const QString &id) const = 0;

    virtual QList<PartitionConfig *> partConfigs() const = 0;

    virtual QSharedPointer<Patcher> createPatcher(const PatcherPaths * const pp,
                                                  const QString &id) const = 0;
};

class AutoPatcher
{
public:
    virtual ~AutoPatcher() {}

    virtual PatcherError::Error error() const = 0;
    virtual QString errorString() const = 0;

    virtual QString name() const = 0;

    virtual QStringList newFiles() const = 0; // Currently unimplemented
    virtual QStringList existingFiles() const = 0;

    virtual bool patchFile(const QString &file,
                           QByteArray * const contents,
                           const QStringList &bootImages) = 0;
};

class IAutoPatcherFactory
{
public:
    virtual ~IAutoPatcherFactory() {}

    virtual QStringList autoPatchers() const = 0;

    virtual QSharedPointer<AutoPatcher> createAutoPatcher(const PatcherPaths * const pp,
                                                          const QString &id,
                                                          const FileInfo * const info,
                                                          const PatchInfo::AutoPatcherArgs &args) const = 0;
};

class RamdiskPatcher
{
public:
    virtual ~RamdiskPatcher() {}

    virtual PatcherError::Error error() const = 0;
    virtual QString errorString() const = 0;

    virtual QString name() const = 0;

    virtual bool patchRamdisk() = 0;
};

class IRamdiskPatcherFactory
{
public:
    virtual ~IRamdiskPatcherFactory() {}

    virtual QStringList ramdiskPatchers() const = 0;

    virtual QSharedPointer<RamdiskPatcher> createRamdiskPatcher(const PatcherPaths * const pp,
                                                                const QString &id,
                                                                const FileInfo * const info,
                                                                CpioFile * const cpio) const = 0;
};

#define IPatcherFactory_iid "com.github.chenxiaolong.libdbp.IPatcherFactory/1.0"

Q_DECLARE_INTERFACE(IPatcherFactory, IPatcherFactory_iid)

#define IAutoPatcherFactory_iid "com.github.chenxiaolong.libdbp.IAutoPatcherFactory/1.0"

Q_DECLARE_INTERFACE(IAutoPatcherFactory, IAutoPatcherFactory_iid)

#define IRamdiskPatcherFactory_iid "com.github.chenxiaolong.libdbp.IRamdiskPatcherFactory/1.0"

Q_DECLARE_INTERFACE(IRamdiskPatcherFactory, IRamdiskPatcherFactory_iid)

#endif // PLUGININTERFACE_H
