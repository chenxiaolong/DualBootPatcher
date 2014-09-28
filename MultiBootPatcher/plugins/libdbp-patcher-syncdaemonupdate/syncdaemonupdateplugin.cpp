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

#include "syncdaemonupdateplugin.h"
#include "syncdaemonupdatepatcher.h"


SyncdaemonUpdatePlugin::SyncdaemonUpdatePlugin(QObject *parent) :
    QObject(parent)
{
}

QStringList SyncdaemonUpdatePlugin::patchers() const
{
    return QStringList() << SyncdaemonUpdatePatcher::Id;
}

QString SyncdaemonUpdatePlugin::patcherName(const QString &id) const
{
    if (id == SyncdaemonUpdatePatcher::Id) {
        return SyncdaemonUpdatePatcher::Name;
    }

    return QString();
}

QList<PartitionConfig *> SyncdaemonUpdatePlugin::partConfigs() const
{
    return QList<PartitionConfig *>();
}

QSharedPointer<Patcher> SyncdaemonUpdatePlugin::createPatcher(const PatcherPaths * const pp,
                                                              const QString &id) const
{
    return QSharedPointer<Patcher>(new SyncdaemonUpdatePatcher(pp, id));
}
