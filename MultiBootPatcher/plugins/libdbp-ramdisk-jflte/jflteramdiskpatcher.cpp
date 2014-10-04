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

#include "jflteramdiskpatcher.h"
#include "jflteramdiskpatcher_p.h"

#include <libdbp/patcherpaths.h>

#include <libdbp-ramdisk-common/coreramdiskpatcher.h>
#include <libdbp-ramdisk-galaxy/galaxyramdiskpatcher.h>
#include <libdbp-ramdisk-qcom/qcomramdiskpatcher.h>

#include <QtCore/QRegularExpression>
#include <QtCore/QStringBuilder>


/*! \brief AOSP ramdisk patcher */
const QString JflteRamdiskPatcher::AOSP
        = QStringLiteral("jflte/AOSP/AOSP");
/*! \brief noobdev ramdisk patcher */
const QString JflteRamdiskPatcher::CXL
        = QStringLiteral("jflte/AOSP/cxl");
/*! \brief Google Edition ramdisk patcher */
const QString JflteRamdiskPatcher::GoogleEdition
        = QStringLiteral("jflte/GoogleEdition/GoogleEdition");
/*! \brief TouchWiz ramdisk patcher */
const QString JflteRamdiskPatcher::TouchWiz
        = QStringLiteral("jflte/TouchWiz/TouchWiz");

static const QString InitRc = QStringLiteral("init.rc");
static const QString InitTargetRc = QStringLiteral("init.target.rc");
static const QString UeventdRc = QStringLiteral("ueventd.rc");
static const QString UeventdQcomRc = QStringLiteral("ueventd.qcom.rc");
static const QString Msm8960LpmRc = QStringLiteral("MSM8960_lpm.rc");

static const QChar Comment = QLatin1Char('#');
static const QChar Newline = QLatin1Char('\n');


/*!
    \class JflteRamdiskPatcher
    \brief Handles common ramdisk patching operations for the Samsung Galaxy S 4

    This patcher handles the patching of ramdisks for the Samsung Galaxy S 4.
    The currently supported ramdisk types are:

    1. AOSP or AOSP-derived ramdisks
    2. Google Edition (Google Play Edition) ramdisks
    3. TouchWiz (Android 4.2-4.4) ramdisks
    4. noobdev (built-in dual booting) ramdisks
 */


JflteRamdiskPatcher::JflteRamdiskPatcher(const PatcherPaths * const pp,
                                         const QString &id,
                                         const FileInfo * const info,
                                         CpioFile * const cpio)
    : d_ptr(new JflteRamdiskPatcherPrivate())
{
    Q_D(JflteRamdiskPatcher);

    d->pp = pp;
    d->id = id;
    d->info = info;
    d->cpio = cpio;

    if (d->id == GoogleEdition || d->id == TouchWiz) {
        if (d->cpio->exists(Msm8960LpmRc)) {
            d->getwVersion = GalaxyRamdiskPatcher::JellyBean;
        } else {
            d->getwVersion = GalaxyRamdiskPatcher::KitKat;
        }
    }
}

JflteRamdiskPatcher::~JflteRamdiskPatcher()
{
    // Destructor so d_ptr is destroyed
}

PatcherError::Error JflteRamdiskPatcher::error() const
{
    Q_D(const JflteRamdiskPatcher);

    return d->errorCode;
}

QString JflteRamdiskPatcher::errorString() const
{
    Q_D(const JflteRamdiskPatcher);

    return d->errorString;
}

QString JflteRamdiskPatcher::name() const
{
    Q_D(const JflteRamdiskPatcher);

    return d->id;
}

bool JflteRamdiskPatcher::patchRamdisk()
{
    Q_D(JflteRamdiskPatcher);

    if (d->id == AOSP) {
        return patchAOSP();
    } else if (d->id == CXL) {
        return patchCXL();
    } else if (d->id == GoogleEdition) {
        return patchGoogleEdition();
    } else if (d->id == TouchWiz) {
        return patchTouchWiz();
    }

    return false;
}

bool JflteRamdiskPatcher::patchAOSP()
{
    Q_D(JflteRamdiskPatcher);

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

bool JflteRamdiskPatcher::patchCXL()
{
    Q_D(JflteRamdiskPatcher);

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

    if (!cxlModifyInitTargetRc()) {
        return false;
    }

    return true;
}

bool JflteRamdiskPatcher::patchGoogleEdition()
{
    Q_D(JflteRamdiskPatcher);

    CoreRamdiskPatcher corePatcher(d->pp, d->info, d->cpio);
    QcomRamdiskPatcher qcomPatcher(d->pp, d->info, d->cpio);
    GalaxyRamdiskPatcher galaxyPatcher(d->pp, d->info, d->cpio, d->getwVersion);

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

    if (!galaxyPatcher.geModifyInitRc()) {
        d->errorCode = galaxyPatcher.error();
        d->errorString = galaxyPatcher.errorString();
        return false;
    }

    if (!qcomPatcher.modifyInitQcomRc(
            QStringList() << QStringLiteral("init.jgedlte.rc"))) {
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

    if (!galaxyPatcher.getwModifyMsm8960LpmRc()) {
        d->errorCode = galaxyPatcher.error();
        d->errorString = galaxyPatcher.errorString();
        return false;
    }

    // Samsung's init binary is pretty screwed up
    if (d->getwVersion == GalaxyRamdiskPatcher::KitKat) {
        d->cpio->remove(QStringLiteral("init"));

        QString newInit = d->pp->initsDirectory()
                % QStringLiteral("/init-kk44");
        d->cpio->addFile(newInit, QStringLiteral("init"), 0755);
    }

    return true;
}

bool JflteRamdiskPatcher::patchTouchWiz()
{
    Q_D(JflteRamdiskPatcher);

    CoreRamdiskPatcher corePatcher(d->pp, d->info, d->cpio);
    QcomRamdiskPatcher qcomPatcher(d->pp, d->info, d->cpio);
    GalaxyRamdiskPatcher galaxyPatcher(d->pp, d->info, d->cpio, d->getwVersion);

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

    if (!galaxyPatcher.getwModifyMsm8960LpmRc()) {
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
    if (d->getwVersion == GalaxyRamdiskPatcher::KitKat) {
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
    }

    return true;
}

bool JflteRamdiskPatcher::cxlModifyInitTargetRc()
{
    Q_D(JflteRamdiskPatcher);

    QByteArray contents = d->cpio->contents(InitTargetRc);
    if (contents.isNull()) {
        d->errorCode = PatcherError::CpioFileNotExistError;
        d->errorString = PatcherError::errorString(d->errorCode)
                .arg(InitTargetRc);
        return false;
    }

    QString dualBootScript = QStringLiteral("init.dualboot.mounting.sh");
    QString multiBootScript = QStringLiteral("init.multiboot.mounting.sh");

    QStringList lines = QString::fromUtf8(contents).split(Newline);
    for (QString &line : lines) {
        if (line.contains(dualBootScript)) {
            line.replace(dualBootScript, multiBootScript);
        }
    }

    d->cpio->setContents(InitTargetRc, lines.join(Newline).toUtf8());

    return true;
}
