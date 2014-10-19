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

#include "device.h"
#include "device_p.h"

const QString Device::SelinuxPermissive = QStringLiteral("permissive");
const QString Device::SelinuxUnchanged = QStringLiteral("unchanged");
const QString Device::SystemPartition = QStringLiteral("system");
const QString Device::CachePartition = QStringLiteral("cache");
const QString Device::DataPartition = QStringLiteral("data");

Device::Device() : d_ptr(new DevicePrivate())
{
    Q_D(Device);

    // Other architectures currently aren't supported
    d->architecture = QStringLiteral("armeabi-v7a");
}

Device::~Device()
{
    // Destructor so d_ptr is destroyed
}

QString Device::codename() const
{
    Q_D(const Device);

    return d->codename;
}

void Device::setCodename(const QString& name)
{
    Q_D(Device);

    d->codename = name;
}

QString Device::name() const
{
    Q_D(const Device);

    return d->name;
}

void Device::setName(const QString& name)
{
    Q_D(Device);

    d->name = name;
}

QString Device::architecture() const
{
    Q_D(const Device);

    return d->architecture;
}

void Device::setArchitecture(const QString& arch)
{
    Q_D(Device);

    d->architecture = arch;
}

QString Device::selinux() const
{
    Q_D(const Device);

    if (d->selinux == SelinuxUnchanged) {
        return QString();
    }

    return d->selinux;
}

void Device::setSelinux(const QString& selinux)
{
    Q_D(Device);

    d->selinux = selinux;
}

QString Device::partition(const QString &which) const
{
    Q_D(const Device);

    if (d->partitions.contains(which)) {
        return d->partitions[which];
    } else {
        return QString();
    }
}

void Device::setPartition(const QString& which, const QString& partition)
{
    Q_D(Device);

    d->partitions[which] = partition;
}

QStringList Device::partitionTypes() const
{
    Q_D(const Device);

    return d->partitions.keys();
}
