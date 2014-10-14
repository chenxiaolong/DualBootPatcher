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

#include "galaxyramdiskpatcher.h"
#include "galaxyramdiskpatcher_p.h"

#include "ramdiskpatchers/common/coreramdiskpatcher.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QRegularExpressionMatch>
#include <QtCore/QStringBuilder>
#include <QtCore/QVector>


/*! \brief Android Jelly Bean */
const QString GalaxyRamdiskPatcher::JellyBean = QStringLiteral("jb");
/*! \brief Android Kit Kat */
const QString GalaxyRamdiskPatcher::KitKat = QStringLiteral("kk");

static const QString InitRc = QStringLiteral("init.rc");
static const QString InitTargetRc = QStringLiteral("init.target.rc");
static const QString UeventdRc = QStringLiteral("ueventd.rc");
static const QString UeventdQcomRc = QStringLiteral("ueventd.qcom.rc");
static const QString Msm8960LpmRc = QStringLiteral("MSM8960_lpm.rc");

static const QChar Comment = QLatin1Char('#');
static const QChar Newline = QLatin1Char('\n');

/*!
    \class GalaxyRamdiskPatcher
    \brief Handles common ramdisk patching operations for Samsung Galaxy devices.
 */

/*!
    Constructs a ramdisk patcher associated with the ramdisk of a particular
    kernel image.

    The \a cpio is a pointer to a CpioFile on which all of the operations will
    be applied. If more than one ramdisk needs to be patched, create a new
    instance for each one. The \a version parameter specifies which Android
    version the ramdisk is from (GalaxyRamdiskPatcher::JellyBean,
    GalaxyRamdiskPatcher::KitKat, etc.).
 */
GalaxyRamdiskPatcher::GalaxyRamdiskPatcher(const PatcherPaths * const pp,
                                           const FileInfo * const info,
                                           CpioFile * const cpio,
                                           const QString &version) :
    d_ptr(new GalaxyRamdiskPatcherPrivate())
{
    Q_D(GalaxyRamdiskPatcher);

    d->pp = pp;
    d->info = info;
    d->cpio = cpio;
    d->version = version;
}

GalaxyRamdiskPatcher::~GalaxyRamdiskPatcher()
{
    // Destructor so d_ptr is destroyed
}

PatcherError::Error GalaxyRamdiskPatcher::error() const
{
    Q_D(const GalaxyRamdiskPatcher);

    return d->errorCode;
}

QString GalaxyRamdiskPatcher::errorString() const
{
    Q_D(const GalaxyRamdiskPatcher);

    return d->errorString;
}

QString GalaxyRamdiskPatcher::id() const
{
    return QString();
}

bool GalaxyRamdiskPatcher::patchRamdisk()
{
    return false;
}

static QString whitespace(const QString &str) {
    int index = 0;

    for (QChar c : str) {
        if (c.isSpace()) {
            index++;
        } else {
            break;
        }
    }

    return str.left(index);
}

/*!
    \brief Patches init.rc in Google Edition ramdisks

    This method performs the following modifications on the init.rc file:

    1. Finds the /system mounting line for the charger mode:

           mount /system

       and replaces it with lines to mount the multiboot paths:

           mount_all fstab.jgedlte
           exec /sbin/busybox-static sh /init.multiboot.mounting.sh

    \return Succeeded or not
 */
bool GalaxyRamdiskPatcher::geModifyInitRc()
{
    Q_D(GalaxyRamdiskPatcher);

    QByteArray contents = d->cpio->contents(InitRc);
    if (contents.isNull()) {
        d->errorCode = PatcherError::CpioFileNotExistError;
        d->errorString = PatcherError::errorString(d->errorCode).arg(InitRc);
        return false;
    }

    QString previousLine;

    QStringList lines = QString::fromUtf8(contents).split(Newline);
    QMutableStringListIterator iter(lines);
    while (iter.hasNext()) {
        QString &line = iter.next();

        if (line.contains(QRegularExpression(QStringLiteral("mount.*/system")))
                && previousLine.contains(QRegularExpression(
                        QStringLiteral("on\\s+charger")))) {
            iter.remove();
            iter.insert(QStringLiteral("    mount_all fstab.jgedlte"));
            iter.insert(QStringLiteral("    ") % CoreRamdiskPatcher::ExecMount);
        }

        previousLine = line;
    }

    d->cpio->setContents(InitRc, lines.join(Newline).toUtf8());

    return true;
}

/*!
    \brief Patches init.rc in TouchWiz ramdisks

    This method performs the following modifications on the init.rc file:

    1. Comments out lines that attempt to reload the SELinux policy (which fails
       in multi-booted TouchWiz ROMs.

           setprop selinux.reload_policy 1

    3. Removes calls to \c check_icd, which does not exist in the custom init
       implementation.

    4. If the version was set to GalaxyRamdiskPatcher::KitKat, then the
       \c /system/bin/mediaserver service will be modified such that it runs
       under the root user.

    \return Succeeded or not
 */
bool GalaxyRamdiskPatcher::twModifyInitRc()
{
    Q_D(GalaxyRamdiskPatcher);

    QByteArray contents = d->cpio->contents(InitRc);
    if (contents.isNull()) {
        d->errorCode = PatcherError::CpioFileNotExistError;
        d->errorString = PatcherError::errorString(d->errorCode).arg(InitRc);
        return false;
    }

    bool inMediaserver = false;

    QStringList lines = QString::fromUtf8(contents).split(Newline);
    for (QString &line : lines) {
        if (line.contains(QRegularExpression(
                QStringLiteral("^.*setprop.*selinux.reload_policy.*$")))) {
            line.insert(0, Comment);
        } else if (line.contains(QStringLiteral("check_icd"))) {
            line.insert(0, Comment);
        } else if (d->version == KitKat
                && line.startsWith(QStringLiteral("service"))) {
            inMediaserver = line.contains(QStringLiteral(
                    "/system/bin/mediaserver"));
        } else if (inMediaserver && line.contains(QRegularExpression(
                QStringLiteral("^\\s*user")))) {
            line = whitespace(line) % QStringLiteral("user root");
        }
    }

    d->cpio->setContents(InitRc, lines.join(Newline).toUtf8());

    return true;
}

/*!
    \brief Patches init.target.rc in TouchWiz ramdisks

    This method performs the following modifications on the init.target.rc file:

    1. If the version is Kit Kat, the following multiboot mounting lines:

           mount_all fstab.qcom
           exec /sbin/busybox-static sh /init.multiboot.mounting.sh

       will be added to the:

           on fs_selinux

       section.

    2. Comments out lines that attempt to reload the SELinux policy (which fails
       in multi-booted TouchWiz ROMs.

           setprop selinux.reload_policy 1

    3. If the version was set to GalaxyRamdiskPatcher::KitKat, then the
       \c qcamerasvr service will be modified such that it runs under the root
       user and group.

    \return Succeeded or not
 */
bool GalaxyRamdiskPatcher::twModifyInitTargetRc()
{
    Q_D(GalaxyRamdiskPatcher);

    QByteArray contents = d->cpio->contents(InitTargetRc);
    if (contents.isNull()) {
        d->errorCode = PatcherError::CpioFileNotExistError;
        d->errorString = PatcherError::errorString(d->errorCode)
        .arg(InitTargetRc);
        return false;
    }

    bool inQcam = false;

    QStringList lines = QString::fromUtf8(contents).split(Newline);
    QMutableStringListIterator iter(lines);
    while (iter.hasNext()) {
        QString &line = iter.next();

        if (d->version == KitKat && line.contains(QRegularExpression(
                QStringLiteral("^on\\s+fs_selinux\\s*$")))) {
            iter.insert(QStringLiteral("    mount_all fstab.qcom"));
            iter.insert(QStringLiteral("    ") % CoreRamdiskPatcher::ExecMount);
        } else if (line.contains(QRegularExpression(
                QStringLiteral("^.*setprop.*selinux.reload_policy.*$")))) {
            line.insert(0, Comment);
        } else if (d->version == KitKat
                && line.startsWith(QStringLiteral("service"))) {
            inQcam = line.contains(QStringLiteral("qcamerasvr"));
        }

        // This is not exactly safe, but it's the best we can do. TouchWiz is
        // doing some funny business where a process running under a certain
        // user or group (confirmed by /proc/<pid>/status) cannot access files
        // by that group. Not only that, mm-qcamera-daemon doesn't work if the
        // process has multiple groups and root is not the primary group. Oh
        // well, I'm done debugging proprietary binaries.

        else if (inQcam && line.contains(QRegularExpression(
                QStringLiteral("^\\s*user")))) {
            line = whitespace(line) % QStringLiteral("user root");
        } else if (inQcam && line.contains(QRegularExpression(
                QStringLiteral("^\\s*group")))) {
            line = whitespace(line) % QStringLiteral("group root");
        }
    }

    d->cpio->setContents(InitTargetRc, lines.join(Newline).toUtf8());

    return true;
}

/*!
    \brief Patches ueventd.rc in TouchWiz Kit Kat ramdisks

    \note The method does not do anything for non-Kit Kat ramdisks.

    This method changed the permissions of the \c /dev/snd/* entry in ueventd.rc
    from 0660 to 0666.

    \return Succeeded or not
 */
bool GalaxyRamdiskPatcher::twModifyUeventdRc()
{
    Q_D(GalaxyRamdiskPatcher);

    // Only needs to be patched for Kit Kat
    if (d->version != KitKat) {
        return true;
    }

    QByteArray contents = d->cpio->contents(UeventdRc);
    if (contents.isNull()) {
        d->errorCode = PatcherError::CpioFileNotExistError;
        d->errorString = PatcherError::errorString(d->errorCode).arg(UeventdRc);
        return false;
    }

    QStringList lines = QString::fromUtf8(contents).split(Newline);
    for (QString &line : lines) {
        if (line.contains(QStringLiteral("/dev/snd/*"))) {
            line.replace(QStringLiteral("0660"), QStringLiteral("0666"));
        }
    }

    d->cpio->setContents(UeventdRc, lines.join(Newline).toUtf8());

    return true;
}

/*!
    \brief Patches ueventd.qcom.rc in TouchWiz Kit Kat ramdisks

    \note The method does not do anything for non-Kit Kat ramdisks.

    This method changed the permissions from 0660 to 0666 in the following
    entries in \c ueventd.qcom.rc.

    - /dev/video*
    - /dev/media*
    - /dev/v4l-subdev*
    - /dev/msm_camera/*
    - /dev/msm_vidc_enc

    \return Succeeded or not
 */
bool GalaxyRamdiskPatcher::twModifyUeventdQcomRc()
{
    Q_D(GalaxyRamdiskPatcher);

    // Only needs to be patched for Kit Kat
    if (d->version != KitKat) {
        return true;
    }

    QByteArray contents = d->cpio->contents(UeventdQcomRc);
    if (contents.isNull()) {
        d->errorCode = PatcherError::CpioFileNotExistError;
        d->errorString = PatcherError::errorString(d->errorCode)
                .arg(UeventdQcomRc);
        return false;
    }

    QStringList lines = QString::fromUtf8(contents).split(Newline);
    for (QString &line : lines) {
        // More funny business: even with mm-qcamera-daemon running as root,
        // the daemon and the default camera application are unable access the
        // camera hardware unless it's writable to everyone. Baffles me...
        //
        // More, more funny business: chmod'ing the /dev/video* devices to
        // anything while mm-qcamera-daemon is running causes a kernel panic.
        // **Wonderful** /s
        if (line.contains(QStringLiteral("/dev/video*"))
                || line.contains(QStringLiteral("/dev/media*"))
                || line.contains(QStringLiteral("/dev/v4l-subdev*"))
                || line.contains(QStringLiteral("/dev/msm_camera/*"))
                || line.contains(QStringLiteral("/dev/msm_vidc_enc"))) {
            line.replace(QStringLiteral("0660"), QStringLiteral("0666"));
        }
    }

    lines << QStringLiteral("/dev/ion 0666 system system\n");

    d->cpio->setContents(UeventdQcomRc, lines.join(Newline).toUtf8());

    return true;
}

/*!
    \brief Patches MSM8960_lpm.rc in TouchWiz and Google Edition ramdisks

    \note The method does not do anything for Kit Kat ramdisks.

    This method comments out the line that mounts \c /cache in
    \c MSM8960_lpm.rc.

    \return Succeeded or not
 */
bool GalaxyRamdiskPatcher::getwModifyMsm8960LpmRc()
{
    Q_D(GalaxyRamdiskPatcher);

    // This file does not exist on Kit Kat ramdisks
    if (d->version == KitKat) {
        return true;
    }

    QByteArray contents = d->cpio->contents(Msm8960LpmRc);
    if (contents.isNull()) {
        d->errorCode = PatcherError::CpioFileNotExistError;
        d->errorString = PatcherError::errorString(d->errorCode)
                .arg(Msm8960LpmRc);
        return false;
    }

    QStringList lines = QString::fromUtf8(contents).split(Newline);
    for (QString &line : lines) {
        if (line.contains(QRegularExpression(
                QStringLiteral("^\\s+mount.*/cache.*$")))) {
            line.insert(0, Comment);
        }
    }

    d->cpio->setContents(Msm8960LpmRc, lines.join(Newline).toUtf8());

    return true;
}
