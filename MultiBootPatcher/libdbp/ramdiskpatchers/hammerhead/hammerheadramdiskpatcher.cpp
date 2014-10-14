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

#include "hammerheadramdiskpatcher.h"
#include "hammerheadramdiskpatcher_p.h"

#include "ramdiskpatchers/common/coreramdiskpatcher.h"
#include "ramdiskpatchers/qcom/qcomramdiskpatcher.h"


HammerheadBaseRamdiskPatcher::HammerheadBaseRamdiskPatcher(const PatcherPaths * const pp,
                                                           const FileInfo * const info,
                                                           CpioFile * const cpio) :
    d_ptr(new HammerheadBaseRamdiskPatcherPrivate())
{
    Q_D(HammerheadBaseRamdiskPatcher);

    d->pp = pp;
    d->info = info;
    d->cpio = cpio;
}

HammerheadBaseRamdiskPatcher::~HammerheadBaseRamdiskPatcher()
{
    // Destructor so d_ptr is destroyed
}

PatcherError::Error HammerheadBaseRamdiskPatcher::error() const
{
    Q_D(const HammerheadBaseRamdiskPatcher);

    return d->errorCode;
}

QString HammerheadBaseRamdiskPatcher::errorString() const
{
    Q_D(const HammerheadBaseRamdiskPatcher);

    return d->errorString;
}

////////////////////////////////////////////////////////////////////////////////

const QString HammerheadAOSPRamdiskPatcher::Id
        = QStringLiteral("hammerhead/AOSP/AOSP");

HammerheadAOSPRamdiskPatcher::HammerheadAOSPRamdiskPatcher(const PatcherPaths *const pp,
                                                           const FileInfo *const info,
                                                           CpioFile *const cpio)
    : HammerheadBaseRamdiskPatcher(pp, info, cpio)
{
}

QString HammerheadAOSPRamdiskPatcher::id() const
{
    return Id;
}

bool HammerheadAOSPRamdiskPatcher::patchRamdisk()
{
    Q_D(HammerheadBaseRamdiskPatcher);

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

    if (!qcomPatcher.modifyFstab()) {
        d->errorCode = qcomPatcher.error();
        d->errorString = qcomPatcher.errorString();
        return false;
    }

    if (!qcomPatcher.modifyInitTargetRc(
            QStringLiteral("init.hammerhead.rc"))) {
        d->errorCode = qcomPatcher.error();
        d->errorString = qcomPatcher.errorString();
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////

const QString HammerheadNoobdevRamdiskPatcher::Id
        = QStringLiteral("hammerhead/AOSP/cxl");

HammerheadNoobdevRamdiskPatcher::HammerheadNoobdevRamdiskPatcher(const PatcherPaths *const pp,
                                                                 const FileInfo *const info,
                                                                 CpioFile *const cpio)
    : HammerheadBaseRamdiskPatcher(pp, info, cpio)
{
}

QString HammerheadNoobdevRamdiskPatcher::id() const
{
    return Id;
}

bool HammerheadNoobdevRamdiskPatcher::patchRamdisk()
{
    Q_D(HammerheadBaseRamdiskPatcher);

    CoreRamdiskPatcher corePatcher(d->pp, d->info, d->cpio);
    QcomRamdiskPatcher qcomPatcher(d->pp, d->info, d->cpio);

    if (!corePatcher.patchRamdisk()) {
        d->errorCode = corePatcher.error();
        d->errorString = corePatcher.errorString();
        return false;
    }

    // /raw-cache needs to always be mounted rw so OpenDelta can write to
    // /cache/recovery
    QVariantMap args;
    args[QcomRamdiskPatcher::ArgForceCacheRw] = true;
    args[QcomRamdiskPatcher::ArgKeepMountPoints] = true;
    args[QcomRamdiskPatcher::ArgSystemMountPoint] =
            QStringLiteral("/raw-system");
    args[QcomRamdiskPatcher::ArgCacheMountPoint] =
            QStringLiteral("/raw-cache");
    args[QcomRamdiskPatcher::ArgDataMountPoint] =
            QStringLiteral("/raw-data");

    if (!qcomPatcher.modifyFstab(args)) {
        d->errorCode = qcomPatcher.error();
        d->errorString = qcomPatcher.errorString();
        return false;
    }

    return true;
}
