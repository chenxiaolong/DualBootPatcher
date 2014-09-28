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

#include "multibootplugin.h"
#include "multibootplugin_p.h"
#include "multibootpatcher.h"


MultiBootPlugin::MultiBootPlugin(QObject *parent) :
    QObject(parent), d_ptr(new MultiBootPluginPrivate())
{
    Q_D(MultiBootPlugin);

    // Create supported partition configurations
    QString romInstalled = tr("ROM installed to %1");

    PartitionConfig *dual = new PartitionConfig();

    dual->setId(QStringLiteral("dual"));
    dual->setKernel(QStringLiteral("secondary"));
    dual->setName(tr("Dual Boot"));
    dual->setDescription(romInstalled.arg(QStringLiteral("/system/dual")));

    dual->setTargetSystem(QStringLiteral("/raw-system/dual"));
    dual->setTargetCache(QStringLiteral("/raw-cache/dual"));
    dual->setTargetData(QStringLiteral("/raw-data/dual"));

    dual->setTargetSystemPartition(PartitionConfig::System);
    dual->setTargetCachePartition(PartitionConfig::Cache);
    dual->setTargetDataPartition(PartitionConfig::Data);

    d->configs << dual;

    // Add multi-slots
    QString multiSlotId = QStringLiteral("multi-slot-%1");
    for (int i = 0; i < 3; i++) {
        PartitionConfig *multiSlot = new PartitionConfig();

        multiSlot->setId(multiSlotId.arg(i + 1));
        multiSlot->setKernel(multiSlotId.arg(i + 1));
        multiSlot->setName(tr("Multi Boot Slot %1").arg(i + 1));
        multiSlot->setDescription(romInstalled
                .arg(QStringLiteral("/cache/multi-slot-%1/system").arg(i + 1)));

        multiSlot->setTargetSystem(
                QStringLiteral("/raw-cache/multi-slot-%1/system").arg(i + 1));
        multiSlot->setTargetCache(
                QStringLiteral("/raw-system/multi-slot-%1/cache").arg(i + 1));
        multiSlot->setTargetData(
                QStringLiteral("/raw-data/multi-slot-%1").arg(i + 1));

        multiSlot->setTargetSystemPartition(PartitionConfig::Cache);
        multiSlot->setTargetCachePartition(PartitionConfig::System);
        multiSlot->setTargetDataPartition(PartitionConfig::Data);

        d->configs << multiSlot;
    }
}

MultiBootPlugin::~MultiBootPlugin()
{
}

QStringList MultiBootPlugin::patchers() const
{
    return QStringList() << MultiBootPatcher::Id;
}

QString MultiBootPlugin::patcherName(const QString &id) const
{
    if (id == MultiBootPatcher::Id) {
        return MultiBootPatcher::Name;
    }

    return QString();
}

QList<PartitionConfig *> MultiBootPlugin::partConfigs() const
{
    Q_D(const MultiBootPlugin);

    return d->configs;
}

QSharedPointer<Patcher> MultiBootPlugin::createPatcher(const PatcherPaths * const pp,
                                                       const QString &id) const
{
    return QSharedPointer<Patcher>(new MultiBootPatcher(pp, id));
}
