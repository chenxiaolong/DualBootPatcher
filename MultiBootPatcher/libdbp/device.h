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

#ifndef DEVICE_H
#define DEVICE_H

#include "libdbp_global.h"

#include <QtCore/QScopedPointer>
#include <QtCore/QStringList>

class DevicePrivate;
class PatcherPaths;

class LIBDBPSHARED_EXPORT Device
{
public:
    static const QString SelinuxPermissive;
    static const QString SelinuxUnchanged;
    static const QString SystemPartition;
    static const QString CachePartition;
    static const QString DataPartition;

    Device();
    ~Device();

    QString codename() const;
    void setCodename(const QString &name);
    QString name() const;
    void setName(const QString &name);
    QString architecture() const;
    void setArchitecture(const QString &arch);
    QString selinux() const;
    void setSelinux(const QString &selinux);
    QString partition(const QString &which) const;
    void setPartition(const QString &which, const QString &partition);
    QStringList partitionTypes() const;

private:
    const QScopedPointer<DevicePrivate> d_ptr;
    Q_DECLARE_PRIVATE(Device)
};

#endif // DEVICE_H
