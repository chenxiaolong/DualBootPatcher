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

#ifndef NOOBDEVPATCHER_H
#define NOOBDEVPATCHER_H

#include <libdbp/plugininterface.h>

#include <QtCore/QtPlugin>

class NoobdevPatcherPrivate;

class NoobdevPatcher : public AutoPatcher
{
public:
    explicit NoobdevPatcher(const PatcherPaths * const pp,
                            const QString &id,
                            const FileInfo * const info);
    ~NoobdevPatcher();

    static const QString NoobdevMultiBoot;
    static const QString NoobdevSystemProp;

    virtual PatcherError::Error error() const;
    virtual QString errorString() const;

    virtual QString name() const;

    virtual QStringList newFiles() const;
    virtual QStringList existingFiles() const;

    virtual bool patchFile(const QString &file,
                           QByteArray * const contents,
                           const QStringList &bootImages);

private:
    const QScopedPointer<NoobdevPatcherPrivate> d_ptr;
    Q_DECLARE_PRIVATE(NoobdevPatcher)
};

#endif // NOOBDEVPATCHER_H
