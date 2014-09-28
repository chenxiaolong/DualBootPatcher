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

#ifndef PRIMARYUPGRADEPATCHER_H
#define PRIMARYUPGRADEPATCHER_H

#include <libdbp/plugininterface.h>

// libarchive
#include <archive.h>

class PrimaryUpgradePatcherPrivate;

class PrimaryUpgradePatcher : public Patcher
{
    Q_OBJECT

public:
    explicit PrimaryUpgradePatcher(const PatcherPaths * const pp,
                                   const QString &id,
                                   QObject *parent = 0);
    ~PrimaryUpgradePatcher();

    static const QString Id;
    static const QString Name;

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

private:
    static const QString Format;
    static const QString Mount;
    static const QString Unmount;
    static const QString TempDir;
    static const QString DualBootTool;
    static const QString SetFacl;
    static const QString GetFacl;
    static const QString SetFattr;
    static const QString GetFattr;
    static const QString PermTool;
    static const QString PermsBackup;
    static const QString PermsRestore;
    static const QString Extract;
    static const QString MakeExecutable;
    static const QString SetKernel;

    bool patchZip();
    bool patchUpdaterScript(QByteArray *contents);

    int insertFormatSystem(int index, QStringList *lines, bool mount) const;
    int insertFormatCache(int index, QStringList *lines, bool mount) const;

    bool addFile(archive * const a,
                 const QString &path,
                 const QString &name);
    bool addFile(archive * const a,
                 const QByteArray &contents,
                 const QString &name);
    bool readToByteArray(archive *aInput,
                         QByteArray *output) const;
    bool copyData(archive *aInput, archive *aOutput) const;

    int scanNumberOfFiles();

    bool ignoreFile(const QString &file) const;

private:
    const QScopedPointer<PrimaryUpgradePatcherPrivate> d_ptr;
    Q_DECLARE_PRIVATE(PrimaryUpgradePatcher)
};

#endif // PRIMARYUPGRADEPATCHER_H
