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

#ifndef CORERAMDISKPATCHER_H
#define CORERAMDISKPATCHER_H

#include <libdbp/cpiofile.h>
#include <libdbp/patcherinterface.h>


class CoreRamdiskPatcherPrivate;

class CoreRamdiskPatcher : public RamdiskPatcher
{
public:
    explicit CoreRamdiskPatcher(const PatcherPaths * const pp,
                                const FileInfo * const info,
                                CpioFile * const cpio);
    ~CoreRamdiskPatcher();

    static const QString FstabRegex;
    static const QString ExecMount;
    static const QString PropPartconfig;
    static const QString PropVersion;
    static const QString SyncdaemonService;

    virtual PatcherError::Error error() const override;
    virtual QString errorString() const override;

    virtual QString id() const override;

    virtual bool patchRamdisk() override;

    bool modifyDefaultProp();
    bool addSyncdaemon();
    bool removeAndRelinkBusybox();

private:
    const QScopedPointer<CoreRamdiskPatcherPrivate> d_ptr;
    Q_DECLARE_PRIVATE(CoreRamdiskPatcher)
};

#endif // CORERAMDISKPATCHER_H
