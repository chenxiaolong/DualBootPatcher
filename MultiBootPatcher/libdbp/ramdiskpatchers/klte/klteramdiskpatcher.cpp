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

#include "ramdiskpatchers/common/coreramdiskpatcher.h"
#include "ramdiskpatchers/galaxy/galaxyramdiskpatcher.h"
#include "ramdiskpatchers/qcom/qcomramdiskpatcher.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QStringBuilder>


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


KlteBaseRamdiskPatcher::KlteBaseRamdiskPatcher(const PatcherPaths * const pp,
                                               const FileInfo * const info,
                                               CpioFile * const cpio) :
    d_ptr(new KlteBaseRamdiskPatcherPrivate())
{
    Q_D(KlteBaseRamdiskPatcher);

    d->pp = pp;
    d->info = info;
    d->cpio = cpio;
}

KlteBaseRamdiskPatcher::~KlteBaseRamdiskPatcher()
{
    // Destructor so d_ptr is destroyed
}

PatcherError::Error KlteBaseRamdiskPatcher::error() const
{
    Q_D(const KlteBaseRamdiskPatcher);

    return d->errorCode;
}

QString KlteBaseRamdiskPatcher::errorString() const
{
    Q_D(const KlteBaseRamdiskPatcher);

    return d->errorString;
}

////////////////////////////////////////////////////////////////////////////////

const QString KlteAOSPRamdiskPatcher::Id
        = QStringLiteral("klte/AOSP/AOSP");

KlteAOSPRamdiskPatcher::KlteAOSPRamdiskPatcher(const PatcherPaths *const pp,
                                               const FileInfo *const info,
                                               CpioFile *const cpio)
    : KlteBaseRamdiskPatcher(pp, info, cpio)
{
}

QString KlteAOSPRamdiskPatcher::id() const
{
    return Id;
}

bool KlteAOSPRamdiskPatcher::patchRamdisk()
{
    Q_D(KlteBaseRamdiskPatcher);

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

    if (d->cpio->exists(QStringLiteral("init.target.rc"))) {
        if (!qcomPatcher.modifyInitTargetRc()) {
            d->errorCode = qcomPatcher.error();
            d->errorString = qcomPatcher.errorString();
            return false;
        }
    } else {
        if (!qcomPatcher.modifyInitTargetRc(QStringLiteral("init.qcom.rc"))) {
            d->errorCode = qcomPatcher.error();
            d->errorString = qcomPatcher.errorString();
            return false;
        }
    }

    QString mountScript = d->pp->scriptsDirectory()
            % QStringLiteral("/klte/mount.modem.aosp.sh");
    d->cpio->addFile(mountScript, QStringLiteral("init.additional.sh"), 0755);

    return true;
}

////////////////////////////////////////////////////////////////////////////////

const QString KlteTouchWizRamdiskPatcher::Id
        = QStringLiteral("klte/TouchWiz/TouchWiz");

KlteTouchWizRamdiskPatcher::KlteTouchWizRamdiskPatcher(const PatcherPaths *const pp,
                                                       const FileInfo *const info,
                                                       CpioFile *const cpio)
    : KlteBaseRamdiskPatcher(pp, info, cpio)
{
}

QString KlteTouchWizRamdiskPatcher::id() const
{
    return Id;
}

bool KlteTouchWizRamdiskPatcher::patchRamdisk()
{
    Q_D(KlteBaseRamdiskPatcher);

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
            % QStringLiteral("/klte/mount.modem.touchwiz.sh");
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
