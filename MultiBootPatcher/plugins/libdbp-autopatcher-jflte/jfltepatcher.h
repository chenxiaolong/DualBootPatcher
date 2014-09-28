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

#ifndef JFLTEPATCHER_H
#define JFLTEPATCHER_H

#include <libdbp/plugininterface.h>

#include <QtCore/QtPlugin>

class JfltePatcherPrivate;

class JfltePatcher : public AutoPatcher
{
public:
    explicit JfltePatcher(const PatcherPaths * const pp,
                          const QString &id,
                          const FileInfo * const info);
    ~JfltePatcher();

    static const QString DalvikCachePatcher;
    static const QString GoogleEditionPatcher;
    static const QString SlimAromaBundledMount;
    static const QString ImperiumPatcher;
    static const QString NegaliteNoWipeData;
    static const QString TriForceFixAroma;
    static const QString TriForceFixUpdate;

    virtual PatcherError::Error error() const;
    virtual QString errorString() const;

    virtual QString name() const;

    virtual QStringList newFiles() const;
    virtual QStringList existingFiles() const;

    virtual bool patchFile(const QString &file,
                           QByteArray * const contents,
                           const QStringList &bootImages);

private:
    const QScopedPointer<JfltePatcherPrivate> d_ptr;
    Q_DECLARE_PRIVATE(JfltePatcher)
};

#endif // JFLTEPATCHER_H
