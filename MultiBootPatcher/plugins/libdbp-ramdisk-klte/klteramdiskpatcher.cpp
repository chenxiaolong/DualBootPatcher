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

#include "klteramdiskpatcher.h"
#include "klteramdiskpatcher_p.h"

#include <libdbp/patcherpaths.h>

#include <libdbp-ramdisk-common/coreramdiskpatcher.h>
#include <libdbp-ramdisk-galaxy/galaxyramdiskpatcher.h>
#include <libdbp-ramdisk-qcom/qcomramdiskpatcher.h>

#include <QtCore/QRegularExpression>
#include <QtCore/QStringBuilder>


/*! \brief AOSP ramdisk patcher */
const QString KlteRamdiskPatcher::AOSP
        = QStringLiteral("klte/AOSP/AOSP");
/*! \brief TouchWiz ramdisk patcher */
const QString KlteRamdiskPatcher::TouchWiz
        = QStringLiteral("klte/TouchWiz/TouchWiz");

static const QChar Comment = QLatin1Char('#');
static const QChar Newline = QLatin1Char('\n');


/*!
    \class KlteRamdiskPatcher
    \brief Handles common ramdisk patching operations for the Samsung Galaxy S 5

    This patcher handles the patching of ramdisks for the Samsung Galaxy S 5.
    The currently supported ramdisk types are:

    1. AOSP or AOSP-derived ramdisks
    2. TouchWiz ramdisks
 */


KlteRamdiskPatcher::KlteRamdiskPatcher(const PatcherPaths * const pp,
                                       const QString &id,
                                       const FileInfo * const info,
                                       CpioFile * const cpio) :
    d_ptr(new KlteRamdiskPatcherPrivate())
{
    Q_D(KlteRamdiskPatcher);

    d->pp = pp;
    d->id = id;
    d->info = info;
    d->cpio = cpio;
}

KlteRamdiskPatcher::~KlteRamdiskPatcher()
{
    // Destructor so d_ptr is destroyed
}

PatcherError::Error KlteRamdiskPatcher::error() const
{
    Q_D(const KlteRamdiskPatcher);

    return d->errorCode;
}

QString KlteRamdiskPatcher::errorString() const
{
    Q_D(const KlteRamdiskPatcher);

    return d->errorString;
}

QString KlteRamdiskPatcher::name() const
{
    Q_D(const KlteRamdiskPatcher);

    return d->id;
}

bool KlteRamdiskPatcher::patchRamdisk()
{
    Q_D(KlteRamdiskPatcher);

    if (d->id == AOSP) {
        return patchAOSP();
    } else if (d->id == TouchWiz) {
        return patchTouchWiz();
    }

    return false;
}

bool KlteRamdiskPatcher::patchAOSP()
{
    Q_D(KlteRamdiskPatcher);

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

    bool moveApnhlos;
    bool moveMdm;

    if (!qcomPatcher.modifyFstab(&moveApnhlos, &moveMdm)) {
        d->errorCode = qcomPatcher.error();
        d->errorString = qcomPatcher.errorString();
        return false;
    }

    if (!qcomPatcher.modifyInitTargetRc(moveApnhlos, moveMdm)) {
        d->errorCode = qcomPatcher.error();
        d->errorString = qcomPatcher.errorString();
        return false;
    }

    return true;
}

bool KlteRamdiskPatcher::patchTouchWiz()
{
    Q_D(KlteRamdiskPatcher);

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

    // Samsung's init binary is pretty screwed up
    d->cpio->remove(QStringLiteral("init"));

    QString newInit = d->pp->initsDirectory()
            % QStringLiteral("/jflte/tw44-init");
    d->cpio->addFile(newInit, QStringLiteral("init"), 0755);

    QString newAdbd = d->pp->initsDirectory()
            % QStringLiteral("/jflte/tw44-adbd");
    d->cpio->addFile(newAdbd, QStringLiteral("sbin/adbd"), 0755);

    QString mountScript = d->pp->scriptsDirectory()
            % QStringLiteral("/jflte/mount.modem.sh");
    d->cpio->addFile(mountScript, QStringLiteral("init.additional.sh"), 0755);

    return true;
}
