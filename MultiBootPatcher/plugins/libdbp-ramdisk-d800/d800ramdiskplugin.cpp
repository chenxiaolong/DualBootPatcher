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

#include "d800ramdiskplugin.h"
#include "d800ramdiskpatcher.h"


D800RamdiskPlugin::D800RamdiskPlugin(QObject *parent) :
    QObject(parent)
{
}

QStringList D800RamdiskPlugin::ramdiskPatchers() const
{
    return QStringList() << D800RamdiskPatcher::AOSP;
}

QSharedPointer<RamdiskPatcher> D800RamdiskPlugin::createRamdiskPatcher(const PatcherPaths * const pp,
                                                                       const QString &id,
                                                                       const FileInfo * const info,
                                                                       CpioFile * const cpio) const
{
    return QSharedPointer<RamdiskPatcher>(new D800RamdiskPatcher(pp, id, info, cpio));
}
