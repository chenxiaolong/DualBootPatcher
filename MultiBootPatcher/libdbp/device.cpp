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


const QString DevicePrivate::Unchanged = QStringLiteral("unchanged");

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

QString Device::name() const
{
    Q_D(const Device);

    return d->name;
}

QString Device::architecture() const
{
    Q_D(const Device);

    return d->architecture;
}

QString Device::selinux() const
{
    Q_D(const Device);

    if (d->selinux == DevicePrivate::Unchanged) {
        return QString();
    }

    return d->selinux;
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

QStringList Device::partitionTypes() const
{
    Q_D(const Device);

    return d->partitions.keys();
}
