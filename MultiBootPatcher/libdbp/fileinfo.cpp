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

#include "fileinfo.h"
#include "fileinfo_p.h"
#include "patcherpaths.h"

#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>

FileInfo::FileInfo(QObject *parent) :
    QObject(parent), d_ptr(new FileInfoPrivate())
{
}

FileInfo::~FileInfo()
{
    // Destructor so d_ptr is destroyed
}

void FileInfo::setFilename(const QString &path)
{
    Q_D(FileInfo);

    d->filename = path;
}

void FileInfo::setPatchInfo(PatchInfo * const info)
{
    Q_D(FileInfo);

    d->patchinfo = info;
}

void FileInfo::setDevice(Device * const device)
{
    Q_D(FileInfo);

    d->device = device;
}

void FileInfo::setPartConfig(PartitionConfig * const config)
{
    Q_D(FileInfo);

    d->partconfig = config;
}

QString FileInfo::filename() const
{
    Q_D(const FileInfo);

    return d->filename;
}

PatchInfo * FileInfo::patchInfo() const
{
    Q_D(const FileInfo);

    return d->patchinfo;
}

Device * FileInfo::device() const
{
    Q_D(const FileInfo);

    return d->device;
}

PartitionConfig * FileInfo::partConfig() const
{
    Q_D(const FileInfo);

    return d->partconfig;
}
