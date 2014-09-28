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

#ifndef PATCHFILEPATCHER_H
#define PATCHFILEPATCHER_H

#include <libdbp/plugininterface.h>

#include <QtCore/QtPlugin>

class PatchFilePatcherPrivate;

class PatchFilePatcher : public QObject,
                         public AutoPatcher
{
    Q_OBJECT

public:
    explicit PatchFilePatcher(const PatcherPaths * const pp,
                              const QString &id,
                              const FileInfo * const info,
                              const PatchInfo::AutoPatcherArgs &args,
                              QObject *parent = 0);
    ~PatchFilePatcher();

    static const QString PatchFile;

    virtual PatcherError::Error error() const;
    virtual QString errorString() const;

    virtual QString name() const;

    virtual QStringList newFiles() const;
    virtual QStringList existingFiles() const;

    virtual bool patchFile(const QString &file,
                           QByteArray * const contents,
                           const QStringList &bootImages);

private:
    void skipNewlinesAndAdd(const QString &file,
                            const QString &contents,
                            int begin, int end);

    const QScopedPointer<PatchFilePatcherPrivate> d_ptr;
    Q_DECLARE_PRIVATE(PatchFilePatcher)
};

#endif // PATCHFILEPATCHER_H
