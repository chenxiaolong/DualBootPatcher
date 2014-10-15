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

#include <libdbp/patcherinterface.h>

// libarchive
#include <archive.h>


class MultiBootPatcherPrivate;

class MultiBootPatcher : public Patcher
{
    Q_OBJECT

public:
    explicit MultiBootPatcher(const PatcherPaths * const pp,
                              QObject *parent = 0);
    ~MultiBootPatcher();

    static const QString Id;
    static const QString Name;

    static QList<PartitionConfig *> partConfigs();

    virtual PatcherError::Error error() const override;
    virtual QString errorString() const override;

    // Patcher info
    virtual QString id() const override;
    virtual QString name() const override;
    virtual bool usesPatchInfo() const override;
    virtual QStringList supportedPartConfigIds() const override;

    // Patching
    virtual void setFileInfo(const FileInfo * const info) override;

    virtual QString newFilePath() override;

    virtual bool patchFile() override;

    virtual void cancelPatching() override;

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
