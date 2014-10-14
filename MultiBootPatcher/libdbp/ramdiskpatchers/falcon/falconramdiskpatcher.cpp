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

#include "falconramdiskpatcher.h"
#include "falconramdiskpatcher_p.h"

#include "ramdiskpatchers/common/coreramdiskpatcher.h"
#include "ramdiskpatchers/qcom/qcomramdiskpatcher.h"


const QString FalconRamdiskPatcher::Id =
        QStringLiteral("falcon/AOSP/AOSP");

FalconRamdiskPatcher::FalconRamdiskPatcher(const PatcherPaths * const pp,
                                       const FileInfo * const info,
                                       CpioFile * const cpio)
    : d_ptr(new FalconRamdiskPatcherPrivate())
{
    Q_D(FalconRamdiskPatcher);

    d->pp = pp;
    d->info = info;
    d->cpio = cpio;
}

FalconRamdiskPatcher::~FalconRamdiskPatcher()
{
    // Destructor so d_ptr is destroyed
}

PatcherError::Error FalconRamdiskPatcher::error() const
{
    Q_D(const FalconRamdiskPatcher);

    return d->errorCode;
}

QString FalconRamdiskPatcher::errorString() const
{
    Q_D(const FalconRamdiskPatcher);

    return d->errorString;
}

QString FalconRamdiskPatcher::id() const
{
    return Id;
}

bool FalconRamdiskPatcher::patchRamdisk()
{
    Q_D(FalconRamdiskPatcher);

    CoreRamdiskPatcher corePatcher(d->pp, d->info, d->cpio);
    QcomRamdiskPatcher qcomPatcher(d->pp, d->info, d->cpio);

    if (!corePatcher.patchRamdisk()) {
        d->errorCode = corePatcher.error();
        d->errorString = corePatcher.errorString();
        return false;
    }

    if (!qcomPatcher.modifyInitRc()) {
        d->errorCode = qcomPatcher.error();
        d->errorString = qcomPatcher.errorString();
        return false;
    }

    if (!qcomPatcher.modifyInitQcomRc()) {
        d->errorCode = qcomPatcher.error();
        d->errorString = qcomPatcher.errorString();
        return false;
    }

    QVariantMap args;
    args[QcomRamdiskPatcher::ArgAdditionalFstabs] =
            QStringList() << QStringLiteral("gpe-fstab.qcom");

    if (!qcomPatcher.modifyFstab(args)) {
        d->errorCode = qcomPatcher.error();
        d->errorString = qcomPatcher.errorString();
        return false;
    }

    if (!qcomPatcher.modifyInitTargetRc()) {
        d->errorCode = qcomPatcher.error();
        d->errorString = qcomPatcher.errorString();
        return false;
    }

    return true;
}
