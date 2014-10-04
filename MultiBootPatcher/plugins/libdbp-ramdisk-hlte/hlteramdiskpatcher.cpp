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

#include "hlteramdiskpatcher.h"
#include "hlteramdiskpatcher_p.h"

#include <libdbp/patcherpaths.h>

#include <libdbp-ramdisk-common/coreramdiskpatcher.h>
#include <libdbp-ramdisk-galaxy/galaxyramdiskpatcher.h>
#include <libdbp-ramdisk-qcom/qcomramdiskpatcher.h>

#include <QtCore/QRegularExpression>
#include <QtCore/QStringBuilder>


/*! \brief AOSP ramdisk patcher */
const QString HlteRamdiskPatcher::AOSP
        = QStringLiteral("hlte/AOSP/AOSP");
/*! \brief TouchWiz ramdisk patcher */
const QString HlteRamdiskPatcher::TouchWiz
        = QStringLiteral("hlte/TouchWiz/TouchWiz");

static const QChar Comment = QLatin1Char('#');
static const QChar Newline = QLatin1Char('\n');


/*!
    \class HlteRamdiskPatcher
    \brief Handles common ramdisk patching operations for the Samsung Galaxy
           Note 3

    This patcher handles the patching of ramdisks for the Samsung Galaxy Note 3.
    The currently supported ramdisk types are:

    1. AOSP or AOSP-derived ramdisks
    2. TouchWiz ramdisks
 */


HlteRamdiskPatcher::HlteRamdiskPatcher(const PatcherPaths * const pp,
                                       const QString &id,
                                       const FileInfo * const info,
                                       CpioFile * const cpio) :
    d_ptr(new HlteRamdiskPatcherPrivate())
{
    Q_D(HlteRamdiskPatcher);

    d->pp = pp;
    d->id = id;
    d->info = info;
    d->cpio = cpio;
}

HlteRamdiskPatcher::~HlteRamdiskPatcher()
{
    // Destructor so d_ptr is destroyed
}

PatcherError::Error HlteRamdiskPatcher::error() const
{
    Q_D(const HlteRamdiskPatcher);

    return d->errorCode;
}

QString HlteRamdiskPatcher::errorString() const
{
    Q_D(const HlteRamdiskPatcher);

    return d->errorString;
}

QString HlteRamdiskPatcher::name() const
{
    Q_D(const HlteRamdiskPatcher);

    return d->id;
}

bool HlteRamdiskPatcher::patchRamdisk()
{
    Q_D(HlteRamdiskPatcher);

    if (d->id == AOSP) {
        return patchAOSP();
    } else if (d->id == TouchWiz) {
        return patchTouchWiz();
    }

    return false;
}

bool HlteRamdiskPatcher::patchAOSP()
{
    Q_D(HlteRamdiskPatcher);

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

    if (!qcomPatcher.modifyFstab(true)) {
        d->errorCode = qcomPatcher.error();
        d->errorString = qcomPatcher.errorString();
        return false;
    }

    if (!qcomPatcher.modifyInitTargetRc()) {
        d->errorCode = qcomPatcher.error();
        d->errorString = qcomPatcher.errorString();
        return false;
    }

    QString mountScript = d->pp->scriptsDirectory()
            % QStringLiteral("/hlte/mount.modem.sh");
    d->cpio->addFile(mountScript, QStringLiteral("init.additional.sh"), 0755);

    return true;
}

bool HlteRamdiskPatcher::patchTouchWiz()
{
    Q_D(HlteRamdiskPatcher);

    CoreRamdiskPatcher corePatcher(d->pp, d->info, d->cpio);
    QcomRamdiskPatcher qcomPatcher(d->pp, d->info, d->cpio);
    GalaxyRamdiskPatcher galaxyPatcher(d->pp, d->info, d->cpio,
                                       GalaxyRamdiskPatcher::KitKat);

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

    if (!galaxyPatcher.twModifyInitRc()) {
        d->errorCode = galaxyPatcher.error();
        d->errorString = galaxyPatcher.errorString();
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

    if (!qcomPatcher.modifyInitTargetRc()) {
        d->errorCode = qcomPatcher.error();
        d->errorString = qcomPatcher.errorString();
        return false;
    }

    if (!galaxyPatcher.twModifyInitTargetRc()) {
        d->errorCode = galaxyPatcher.error();
        d->errorString = galaxyPatcher.errorString();
        return false;
    }

    if (!galaxyPatcher.twModifyUeventdRc()) {
        d->errorCode = galaxyPatcher.error();
        d->errorString = galaxyPatcher.errorString();
        return false;
    }

    if (!galaxyPatcher.twModifyUeventdQcomRc()) {
        d->errorCode = galaxyPatcher.error();
        d->errorString = galaxyPatcher.errorString();
        return false;
    }

    QString mountScript = d->pp->scriptsDirectory()
            % QStringLiteral("/hlte/mount.modem.sh");
    d->cpio->addFile(mountScript, QStringLiteral("init.additional.sh"), 0755);

    // Samsung's init binary is pretty screwed up
    d->cpio->remove(QStringLiteral("init"));

    QString newInit = d->pp->initsDirectory()
            % QStringLiteral("/jflte/tw44-init");
    d->cpio->addFile(newInit, QStringLiteral("init"), 0755);

    QString newAdbd = d->pp->initsDirectory()
            % QStringLiteral("/jflte/tw44-adbd");
    d->cpio->addFile(newAdbd, QStringLiteral("sbin/adbd"), 0755);

    return true;
}
