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

#ifndef FILEINFO_H
#define FILEINFO_H

#include "libdbp_global.h"

#include "device.h"
#include "patchinfo.h"
#include "partitionconfig.h"

#include <QtCore/QObject>
#include <QtCore/QScopedPointer>

class FileInfoPrivate;

class LIBDBPSHARED_EXPORT FileInfo : public QObject
{
    Q_OBJECT
public:
    explicit FileInfo(QObject *parent = 0);
    ~FileInfo();

    void setFilename(const QString &path);
    void setPatchInfo(PatchInfo * const info);
    void setDevice(Device * const device);
    void setPartConfig(PartitionConfig * const config);

    QString filename() const;
    PatchInfo * patchInfo() const;
    Device * device() const;
    PartitionConfig * partConfig() const;

private:
    const QScopedPointer<FileInfoPrivate> d_ptr;
    Q_DECLARE_PRIVATE(FileInfo)
};

#endif // FILEINFO_H
