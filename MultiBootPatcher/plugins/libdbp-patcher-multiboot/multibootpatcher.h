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

#ifndef MULTIBOOTPATCHER_H
#define MULTIBOOTPATCHER_H

#include <libdbp/plugininterface.h>

// libarchive
#include <archive.h>

class MultiBootPatcherPrivate;

class MultiBootPatcher : public Patcher
{
    Q_OBJECT

public:
    explicit MultiBootPatcher(const PatcherPaths * const pp,
                              const QString &id,
                              QObject *parent = 0);
    ~MultiBootPatcher();

    static const QString Id;
    static const QString Name;

    virtual PatcherError::Error error() const;
    virtual QString errorString() const;

    // Patcher info
    virtual QString id() const;
    virtual QString name() const;
    virtual bool usesPatchInfo() const;
    virtual QStringList supportedPartConfigIds() const;

    // Patching
    virtual void setFileInfo(const FileInfo * const info);

    virtual QString newFilePath();

    virtual bool patchFile();

private:
    bool patchBootImage(QByteArray *data);
    bool patchZip();
    bool addFile(archive * const a,
                 const QString &path,
                 const QString &name);
    bool addFile(archive * const a,
                 const QByteArray &contents,
                 const QString &name);
    bool readToByteArray(archive *aInput,
                         QByteArray *output) const;
    bool copyData(archive *aInput, archive *aOutput) const;

    bool scanAndPatchBootImages(archive * const aOutput,
                                QStringList *bootImages);
    bool scanAndPatchRemaining(archive * const aOutput,
                               const QStringList &bootImages);

    int scanNumberOfFiles();

private:
    const QScopedPointer<MultiBootPatcherPrivate> d_ptr;
    Q_DECLARE_PRIVATE(MultiBootPatcher)
};

#endif // MULTIBOOTPATCHER_H
