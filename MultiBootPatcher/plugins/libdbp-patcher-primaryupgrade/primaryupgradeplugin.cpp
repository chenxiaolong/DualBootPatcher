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

#include "primaryupgradeplugin.h"
#include "primaryupgradeplugin_p.h"
#include "primaryupgradepatcher.h"


PrimaryUpgradePlugin::PrimaryUpgradePlugin(QObject *parent) :
    QObject(parent), d_ptr(new PrimaryUpgradePluginPrivate())
{
    Q_D(PrimaryUpgradePlugin);

    // Add primary upgrade partition configuration
    PartitionConfig *config = new PartitionConfig();
    config->setId(QStringLiteral("primaryupgrade"));
    config->setKernel(QStringLiteral("primary"));
    config->setName(tr("Primary ROM Upgrade"));
    config->setDescription(
            tr("Upgrade primary ROM without wiping other ROMs"));

    config->setTargetSystem(QStringLiteral("/system"));
    config->setTargetCache(QStringLiteral("/cache"));
    config->setTargetData(QStringLiteral("/data"));

    config->setTargetSystemPartition(PartitionConfig::System);
    config->setTargetCachePartition(PartitionConfig::Cache);
    config->setTargetDataPartition(PartitionConfig::Data);

    d->configs << config;
}

PrimaryUpgradePlugin::~PrimaryUpgradePlugin()
{
}

QStringList PrimaryUpgradePlugin::patchers() const
{
    return QStringList() << PrimaryUpgradePatcher::Id;
}

QString PrimaryUpgradePlugin::patcherName(const QString &id) const
{
    if (id == PrimaryUpgradePatcher::Id) {
        return PrimaryUpgradePatcher::Name;
    }

    return QString();
}

QList<PartitionConfig *> PrimaryUpgradePlugin::partConfigs() const
{
    Q_D(const PrimaryUpgradePlugin);

    return d->configs;
}

QSharedPointer<Patcher> PrimaryUpgradePlugin::createPatcher(const PatcherPaths * const pp,
                                                            const QString &id) const
{
    return QSharedPointer<Patcher>(new PrimaryUpgradePatcher(pp, id));
}
