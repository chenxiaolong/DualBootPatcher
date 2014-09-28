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

#include "baconramdiskpatcher.h"
#include "baconramdiskpatcher_p.h"

#include <libdbp-ramdisk-common/coreramdiskpatcher.h>
#include <libdbp-ramdisk-qcom/qcomramdiskpatcher.h>

#include <QtCore/QDebug>


const QString BaconRamdiskPatcher::AOSP =
        QStringLiteral("bacon/AOSP/AOSP");

BaconRamdiskPatcher::BaconRamdiskPatcher(const PatcherPaths * const pp,
                                         const QString &id,
                                         const FileInfo * const info,
                                         CpioFile * const cpio) :
    d_ptr(new BaconRamdiskPatcherPrivate())
{
    Q_D(BaconRamdiskPatcher);

    d->pp = pp;
    d->id = id;
    d->info = info;
    d->cpio = cpio;
}

BaconRamdiskPatcher::~BaconRamdiskPatcher()
{
    // Destructor so d_ptr is destroyed
}

PatcherError::Error BaconRamdiskPatcher::error() const
{
    Q_D(const BaconRamdiskPatcher);

    return d->errorCode;
}

QString BaconRamdiskPatcher::errorString() const
{
    Q_D(const BaconRamdiskPatcher);

    return d->errorString;
}

QString BaconRamdiskPatcher::name() const
{
    Q_D(const BaconRamdiskPatcher);

    return d->id;
}

bool BaconRamdiskPatcher::patchRamdisk()
{
    Q_D(BaconRamdiskPatcher);

    if (d->id == AOSP) {
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

        if (!qcomPatcher.modifyFstab()) {
            d->errorCode = qcomPatcher.error();
            d->errorString = qcomPatcher.errorString();
            return false;
        }

        if (!qcomPatcher.modifyInitTargetRc(
                QStringLiteral("init.bacon.rc"))) {
            d->errorCode = qcomPatcher.error();
            d->errorString = qcomPatcher.errorString();
            return false;
        }

        return true;
    }

    qWarning() << "Invalid ID:" << d->id;
    d->errorCode = PatcherError::ImplementationError;
    d->errorString = PatcherError::errorString(d->errorCode);
    return false;
}
