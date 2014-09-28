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

#ifndef NOOBDEVPLUGIN_H
#define NOOBDEVPLUGIN_H

#include <libdbp/plugininterface.h>

#include <QtCore/QtPlugin>


class NoobdevPlugin : public QObject,
                                public IAutoPatcherFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.github.chenxiaolong.libdbp.IAutoPatcherFactory/1.0" FILE "libdbp-autopatcher-chenxiaolong.json")
    Q_INTERFACES(IAutoPatcherFactory)

public:
    NoobdevPlugin(QObject *parent = 0);

    virtual QStringList autoPatchers() const;

    virtual QSharedPointer<AutoPatcher> createAutoPatcher(const PatcherPaths * const pp,
                                                          const QString &id,
                                                          const FileInfo * const info,
                                                          const PatchInfo::AutoPatcherArgs &args) const;
};

#endif // NOOBDEVPLUGIN_H
