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

#ifndef SYNCDAEMONUPDATEPATCHER_H
#define SYNCDAEMONUPDATEPATCHER_H

#include <libdbp/patcherinterface.h>


class SyncdaemonUpdatePatcherPrivate;

class SyncdaemonUpdatePatcher : public Patcher
{
    Q_OBJECT

public:
    explicit SyncdaemonUpdatePatcher(const PatcherPaths * const pp);
    ~SyncdaemonUpdatePatcher();

    static const QString Id;
    static const QString Name;

    static QList<PartitionConfig *> partConfigs();

    // Error reporting
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

    virtual void cancelPatching();

private:
    bool patchImage();
    QString findPartConfigId(const CpioFile * const cpio) const;

private:
    const QScopedPointer<SyncdaemonUpdatePatcherPrivate> d_ptr;
    Q_DECLARE_PRIVATE(SyncdaemonUpdatePatcher)
};
#endif // SYNCDAEMONUPDATEPATCHER_H
