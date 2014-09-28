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

#include "qcomramdiskpatcher.h"
#include "qcomramdiskpatcher_p.h"

#include <libdbp-ramdisk-common/coreramdiskpatcher.h>

#include <QtCore/QRegularExpression>
#include <QtCore/QRegularExpressionMatch>
#include <QtCore/QStringBuilder>
#include <QtCore/QVector>

const QString QcomRamdiskPatcher::SystemPartition =
        QStringLiteral("/dev/block/platform/msm_sdcc.1/by-name/system");
const QString QcomRamdiskPatcher::CachePartition =
        QStringLiteral("/dev/block/platform/msm_sdcc.1/by-name/cache");
const QString QcomRamdiskPatcher::DataPartition =
        QStringLiteral("/dev/block/platform/msm_sdcc.1/by-name/userdata");
const QString QcomRamdiskPatcher::ApnhlosPartition =
        QStringLiteral("/dev/block/platform/msm_sdcc.1/by-name/apnhlos");
const QString QcomRamdiskPatcher::MdmPartition =
        QStringLiteral("/dev/block/platform/msm_sdcc.1/by-name/mdm");

const QString QcomRamdiskPatcher::ArgAdditionalFstabs =
        QStringLiteral("AdditionalFstabs");
const QString QcomRamdiskPatcher::ArgForceSystemRw =
        QStringLiteral("ForceSystemRw");
const QString QcomRamdiskPatcher::ArgForceCacheRw =
        QStringLiteral("ForceCacheRw");
const QString QcomRamdiskPatcher::ArgForceDataRw =
        QStringLiteral("ForceDataRw");
const QString QcomRamdiskPatcher::ArgKeepMountPoints =
        QStringLiteral("KeepMountPoints");
const QString QcomRamdiskPatcher::ArgSystemMountPoint =
        QStringLiteral("SystemMountPoint");
const QString QcomRamdiskPatcher::ArgCacheMountPoint =
        QStringLiteral("CacheMountPoint");
const QString QcomRamdiskPatcher::ArgDataMountPoint =
        QStringLiteral("DataMountPoint");
const QString QcomRamdiskPatcher::ArgDefaultSystemMountArgs =
        QStringLiteral("DefaultSystemMountArgs");
const QString QcomRamdiskPatcher::ArgDefaultSystemVoldArgs =
        QStringLiteral("DefaultSystemVoldArgs");
const QString QcomRamdiskPatcher::ArgDefaultCacheMountArgs =
        QStringLiteral("DefaultCacheMountArgs");
const QString QcomRamdiskPatcher::ArgDefaultCacheVoldArgs =
        QStringLiteral("DefaultDataMountArgs");

static const QString RawSystem = QStringLiteral("/raw-system");
static const QString RawCache = QStringLiteral("/raw-cache");
static const QString RawData = QStringLiteral("/raw-data");
static const QString System = QStringLiteral("/system");
static const QString Cache = QStringLiteral("/cache");
static const QString Data = QStringLiteral("/data");

static const QChar Newline = QLatin1Char('\n');

QcomRamdiskPatcher::QcomRamdiskPatcher(const PatcherPaths * const pp,
                                       const FileInfo * const info,
                                       CpioFile * const cpio) :
    d_ptr(new QcomRamdiskPatcherPrivate())
{
    Q_D(QcomRamdiskPatcher);

    d->pp = pp;
    d->info = info;
    d->cpio = cpio;
}

QcomRamdiskPatcher::~QcomRamdiskPatcher()
{
    // Destructor so d_ptr is destroyed
}

PatcherError::Error QcomRamdiskPatcher::error() const
{
    Q_D(const QcomRamdiskPatcher);

    return d->errorCode;
}

QString QcomRamdiskPatcher::errorString() const
{
    Q_D(const QcomRamdiskPatcher);

    return d->errorString;
}

QString QcomRamdiskPatcher::name() const
{
    return QString();
}

bool QcomRamdiskPatcher::patchRamdisk()
{
    return false;
}

bool QcomRamdiskPatcher::modifyInitRc()
{
    Q_D(QcomRamdiskPatcher);

    static const QString initRc = QStringLiteral("init.rc");

    QByteArray contents = d->cpio->contents(initRc);
    if (contents.isNull()) {
        d->errorCode = PatcherError::CpioFileNotExistError;
        d->errorString = PatcherError::errorString(d->errorCode)
                .arg(initRc);
        return false;
    }

    QStringList lines = QString::fromUtf8(contents).split(Newline);
    QMutableStringListIterator iter(lines);
    while (iter.hasNext()) {
        QString &line = iter.next();

        if (line.contains(QRegularExpression(
                QStringLiteral("mkdir /system(\\s|$)")))) {
            iter.insert(QString(line).replace(System, RawSystem));
        } else if (line.contains(QRegularExpression(
                QStringLiteral("mkdir /cache(\\s|$)")))) {
            iter.insert(QString(line).replace(Cache, RawCache));
        } else if (line.contains(QRegularExpression(
                QStringLiteral("mkdir /data(\\s|$)")))) {
            iter.insert(QString(line).replace(Data, RawData));
        } else if (line.contains(QStringLiteral("yaffs2"))) {
            line.insert(0, QLatin1Char('#'));
        }
    }

    d->cpio->setContents(initRc, lines.join(Newline).toUtf8());

    return true;
}

bool QcomRamdiskPatcher::modifyInitQcomRc(QStringList additionalFiles)
{
    Q_D(QcomRamdiskPatcher);

    QString qcomRc(QStringLiteral("init.qcom.rc"));
    QString qcomCommonRc(QStringLiteral("init.qcom-common.rc"));

    QStringList files;
    if (d->cpio->exists(qcomRc)) {
        files << qcomRc;
    } else if (d->cpio->exists(qcomCommonRc)) {
        files << qcomCommonRc;
    } else {
        d->errorCode = PatcherError::CpioFileNotExistError;
        d->errorString = PatcherError::errorString(d->errorCode)
                .arg(qcomRc % QStringLiteral(", ") % qcomCommonRc);
        return false;
    }
    files << additionalFiles;

    for (const QString &file : files) {
        QByteArray contents = d->cpio->contents(file);
        if (contents.isNull()) {
            d->errorCode = PatcherError::CpioFileNotExistError;
            d->errorString = PatcherError::errorString(d->errorCode)
                    .arg(file);
            return false;
        }

        QStringList lines = QString::fromUtf8(contents).split(Newline);
        QMutableStringListIterator iter(lines);
        while (iter.hasNext()) {
            QString &line = iter.next();

            if (line.contains(QRegularExpression(
                    QStringLiteral("/data/media(\\s|$)")))) {
                line.replace(QStringLiteral("/data/media"),
                             QStringLiteral("/raw-data/media"));
            }
        }

        d->cpio->setContents(file, lines.join(Newline).toUtf8());
    }

    return true;
}

// Qt port of the C++ Levenshtein implementation from WikiBooks:
// http://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Levenshtein_distance
template<class T>
static uint levenshteinDistance(const T &s1, const T &s2)
{
    const size_t len1 = s1.size();
    const size_t len2 = s2.size();

    QVector<uint> col(len2 + 1);
    QVector<uint> prevCol(len2 + 1);

    for (int i = 0; i < prevCol.size(); i++) {
        prevCol[i] = i;
    }

    for (uint i = 0; i < len1; i++) {
        col[0] = i + 1;
        for (uint j = 0; j < len2; j++)
            col[j + 1] = qMin(qMin(prevCol[1 + j] + 1, col[j] + 1),
                              prevCol[j] + (s1[i] == s2[j] ? 0 : 1));
        col.swap(prevCol);
    }

    return prevCol[len2];
}

static QString closestMatch(const QString &searchTerm, const QStringList &list)
{
    int index = 0;
    uint min = levenshteinDistance(searchTerm, list[0]);

    for (int i = 1; i < list.size(); i++) {
        uint distance = levenshteinDistance(searchTerm, list[i]);
        if (distance < min) {
            min = distance;
            index = i;
        }
    }

    return list[index];
}

bool QcomRamdiskPatcher::modifyFstab(bool *moveApnhlosMount,
                                     bool *moveMdmMount)
{
    return modifyFstab(QVariantMap(),
                       moveApnhlosMount, moveMdmMount);
}

bool QcomRamdiskPatcher::modifyFstab(QVariantMap args,
                                     bool *moveApnhlosMount,
                                     bool *moveMdmMount)
{
    QStringList additionalFstabs;
    bool forceSystemRw = false;
    bool forceCacheRw = false;
    bool forceDataRw = false;
    bool keepMountPoints = false;
    QString systemMountPoint = System;
    QString cacheMountPoint = Cache;
    QString dataMountPoint = Data;
    QString defaultSystemMountArgs = QStringLiteral("ro,barrier=1,errors=panic");
    QString defaultSystemVoldArgs = QStringLiteral("wait");
    QString defaultCacheMountArgs = QStringLiteral("nosuid,nodev,barrier=1");
    QString defaultCacheVoldArgs = QStringLiteral("wait,check");

    if (args.contains(ArgAdditionalFstabs)) {
        additionalFstabs = args[ArgAdditionalFstabs].toStringList();
    }

    if (args.contains(ArgForceSystemRw)) {
        forceSystemRw = args[ArgForceSystemRw].toBool();
    }

    if (args.contains(ArgForceCacheRw)) {
        forceCacheRw = args[ArgForceCacheRw].toBool();
    }

    if (args.contains(ArgForceDataRw)) {
        forceDataRw = args[ArgForceDataRw].toBool();
    }

    if (args.contains(ArgKeepMountPoints)) {
        keepMountPoints = args[ArgKeepMountPoints].toBool();
    }

    if (args.contains(ArgSystemMountPoint)) {
        systemMountPoint = args[ArgSystemMountPoint].toString();
    }

    if (args.contains(ArgCacheMountPoint)) {
        cacheMountPoint = args[ArgCacheMountPoint].toString();
    }

    if (args.contains(ArgDataMountPoint)) {
        dataMountPoint = args[ArgDataMountPoint].toString();
    }

    if (args.contains(ArgDefaultSystemMountArgs)) {
        defaultSystemMountArgs = args[ArgDefaultSystemMountArgs].toString();
    }

    if (args.contains(ArgDefaultSystemVoldArgs)) {
        defaultSystemVoldArgs = args[ArgDefaultSystemVoldArgs].toString();
    }

    if (args.contains(ArgDefaultCacheMountArgs)) {
        defaultCacheMountArgs = args[ArgDefaultCacheMountArgs].toString();
    }

    if (args.contains(ArgDefaultCacheVoldArgs)) {
        defaultCacheVoldArgs = args[ArgDefaultCacheVoldArgs].toString();
    }

    return modifyFstab(moveApnhlosMount,
                       moveMdmMount,
                       additionalFstabs,
                       forceSystemRw,
                       forceCacheRw,
                       forceDataRw,
                       keepMountPoints,
                       systemMountPoint,
                       cacheMountPoint,
                       dataMountPoint,
                       defaultSystemMountArgs,
                       defaultSystemVoldArgs,
                       defaultCacheMountArgs,
                       defaultCacheVoldArgs);
}

bool QcomRamdiskPatcher::modifyFstab(bool *moveApnhlosMount,
                                     bool *moveMdmMount,
                                     const QStringList &additionalFstabs,
                                     bool forceSystemRw,
                                     bool forceCacheRw,
                                     bool forceDataRw,
                                     bool keepMountPoints,
                                     const QString &systemMountPoint,
                                     const QString &cacheMountPoint,
                                     const QString &dataMountPoint,
                                     const QString &defaultSystemMountArgs,
                                     const QString &defaultSystemVoldArgs,
                                     const QString &defaultCacheMountArgs,
                                     const QString &defaultCacheVoldArgs)
{
    Q_D(QcomRamdiskPatcher);

    QStringList fstabs;
    for (const QString &file : d->cpio->filenames()) {
        if (file.startsWith(QStringLiteral("fstab."))) {
            fstabs << file;
        }
    }

    fstabs << additionalFstabs;

    for (const QString &fstab : fstabs) {
        QByteArray contents = d->cpio->contents(fstab);
        if (contents.isNull()) {
            d->errorCode = PatcherError::CpioFileNotExistError;
            d->errorString = PatcherError::errorString(d->errorCode)
                    .arg(fstab);
            return false;
        }

        QStringList lines = QString::fromUtf8(contents).split(Newline);
        QString mountLine = QStringLiteral("%1%2 %3 %4 %5 %6");

        // Some Android 4.2 ROMs mount the cache partition in the init
        // scripts, so the fstab has no cache line
        bool hasCacheLine = false;

        // Find the mount and vold arguments for the system and cache
        // partitions
        QHash<QString, QString> systemMountArgs;
        QHash<QString, QString> systemVoldArgs;
        QHash<QString, QString> cacheMountArgs;
        QHash<QString, QString> cacheVoldArgs;

        for (const QString &line : lines) {
            QRegularExpressionMatch match;

            if (line.contains(QRegularExpression(
                    CoreRamdiskPatcher::FstabRegex), &match)) {
                QString comment = match.captured(1);
                QString blockDev = match.captured(2);
                //QString mountPoint = match.captured(3);
                //QString fsType = match.captured(4);
                QString mountArgs = match.captured(5);
                QString voldArgs = match.captured(6);

                if (blockDev == SystemPartition) {
                    systemMountArgs[comment] = mountArgs;
                    systemVoldArgs[comment] = voldArgs;
                } else if (blockDev == CachePartition) {
                    cacheMountArgs[comment] = mountArgs;
                    cacheVoldArgs[comment] = voldArgs;
                }
            }
        }

        // In the unlikely case that we couldn't parse the mount arguments,
        // we'll just use the defaults
        if (systemMountArgs.isEmpty()) {
            systemMountArgs[QString()] = defaultSystemMountArgs;
        }
        if (systemVoldArgs.isEmpty()) {
            systemVoldArgs[QString()] = defaultSystemVoldArgs;
        }
        if (cacheMountArgs.isEmpty()) {
            cacheMountArgs[QString()] = defaultCacheMountArgs;
        }
        if (cacheVoldArgs.isEmpty()) {
            cacheVoldArgs[QString()] = defaultCacheVoldArgs;
        }

        QMutableStringListIterator iter(lines);
        while (iter.hasNext()) {
            QString &line = iter.next();
            QString comment;
            QString blockDev;
            QString mountPoint;
            QString fsType;
            QString mountArgs;
            QString voldArgs;

            QRegularExpressionMatch match = QRegularExpression(
                    CoreRamdiskPatcher::FstabRegex).match(line);

            if (match.hasMatch()) {
                comment = match.captured(1);
                blockDev = match.captured(2);
                mountPoint = match.captured(3);
                fsType = match.captured(4);
                mountArgs = match.captured(5);
                voldArgs = match.captured(6);
            }

            if (match.hasMatch() && blockDev == SystemPartition
                    && mountPoint == systemMountPoint) {
                if (!keepMountPoints) {
                    mountPoint = RawSystem;
                }

                if (d->info->partConfig()->targetCache().contains(
                        RawSystem)) {
                    QString cacheComment =
                            closestMatch(comment, cacheMountArgs.keys());
                    mountArgs = cacheMountArgs[cacheComment];
                    voldArgs = cacheVoldArgs[cacheComment];
                }

                if (forceSystemRw) {
                    mountArgs = makeWritable(mountArgs);
                }

                line = mountLine.arg(comment).arg(blockDev).arg(mountPoint)
                        .arg(fsType).arg(mountArgs).arg(voldArgs);
            } else if (match.hasMatch() && blockDev == CachePartition
                    && mountPoint == cacheMountPoint) {
                if (!keepMountPoints) {
                    mountPoint = RawCache;
                }

                hasCacheLine = true;

                if (d->info->partConfig()->targetSystem().contains(
                        RawCache)) {
                    QString systemComment =
                            closestMatch(comment, systemMountArgs.keys());
                    mountArgs = systemMountArgs[systemComment];
                    voldArgs = systemVoldArgs[systemComment];
                }

                if (forceCacheRw) {
                    mountArgs = makeWritable(mountArgs);
                }

                line = mountLine.arg(comment).arg(blockDev).arg(mountPoint)
                        .arg(fsType).arg(mountArgs).arg(voldArgs);
            } else if (match.hasMatch() && blockDev == DataPartition
                    && mountPoint == dataMountPoint) {
                if (!keepMountPoints) {
                    mountPoint = RawData;
                }

                if (d->info->partConfig()->targetSystem().contains(
                        RawData)) {
                    QString systemComment =
                            closestMatch(comment, systemMountArgs.keys());
                    mountArgs = systemMountArgs[systemComment];
                    voldArgs = systemVoldArgs[systemComment];
                }

                if (forceDataRw) {
                    mountArgs = makeWritable(mountArgs);
                }

                line = mountLine.arg(comment).arg(blockDev).arg(mountPoint)
                        .arg(fsType).arg(mountArgs).arg(voldArgs);
            } else if (match.hasMatch() && blockDev == ApnhlosPartition) {
                iter.remove();

                if (moveApnhlosMount != nullptr) {
                    *moveApnhlosMount = true;
                }
            } else if (match.hasMatch() && blockDev == MdmPartition) {
                iter.remove();

                if (moveMdmMount != nullptr) {
                    *moveMdmMount = true;
                }
            }
        }

        if (!hasCacheLine) {
            QString cacheLine = QStringLiteral("%1 /raw-cache ext4 %2 %3");
            QString mountArgs;
            QString voldArgs;

            if (d->info->partConfig()->targetSystem().contains(
                    RawCache)) {
                mountArgs = systemMountArgs[QString()];
                voldArgs = systemVoldArgs[QString()];
            } else {
                mountArgs = systemMountArgs[QString()];
                voldArgs = systemVoldArgs[QString()];
            }

            lines << cacheLine.arg(CachePartition).arg(mountArgs).arg(voldArgs);
        }

        d->cpio->setContents(fstab, lines.join(Newline).toUtf8());
    }

    return true;
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

bool QcomRamdiskPatcher::modifyInitTargetRc(bool insertApnhlos,
                                            bool insertMdm)
{
    return modifyInitTargetRc(QStringLiteral("init.target.rc"),
                              insertApnhlos, insertMdm);
}

bool QcomRamdiskPatcher::modifyInitTargetRc(QString filename,
                                            bool insertApnhlos,
                                            bool insertMdm)
{
    Q_D(QcomRamdiskPatcher);

    QByteArray contents = d->cpio->contents(filename);
    if (contents.isNull()) {
        d->errorCode = PatcherError::CpioFileNotExistError;
        d->errorString = PatcherError::errorString(d->errorCode)
                .arg(filename);
        return false;
    }

    QStringList lines = QString::fromUtf8(contents).split(Newline);
    QMutableStringListIterator iter(lines);
    while (iter.hasNext()) {
        QString &line = iter.next();

        if (line.contains(QRegularExpression(
                QStringLiteral("^\\s+wait\\s+/dev/.*/cache.*$")))
                || line.contains(QRegularExpression(
                QStringLiteral("^\\s+check_fs\\s+/dev/.*/cache.*$")))
                || line.contains(QRegularExpression(
                QStringLiteral("^\\s+mount\\s+ext4\\s+/dev/.*/cache.*$")))) {
            // Remove lines that mount /cache
            line.insert(0, QLatin1Char('#'));
        } else if (line.contains(QRegularExpression(
                QStringLiteral("^\\s+mount_all\\s+(\\./)?fstab\\..*$")))) {
            // Add command for our mount script
            iter.insert(whitespace(line) % CoreRamdiskPatcher::ExecMount);
        } else if (line.contains(QRegularExpression(
                QStringLiteral("/data/media(\\s|$)")))) {
            line.replace(QStringLiteral("/data/media"),
                         QStringLiteral("/raw-data/media"));
        } else if (line.contains(QRegularExpression(
                QStringLiteral("^\\s+setprop\\s+ro.crypto.fuse_sdcard\\s+true")))) {
            QString voldArgs = QStringLiteral(
                    "shortname=lower,uid=1000,gid=1000,dmask=227,fmask=337");

            QString mountVfat = QStringLiteral("mount vfat %1 %2 ro %3");
            QString wait = QStringLiteral("wait ");

            if (insertApnhlos) {
                iter.insert(whitespace(line)
                        % wait % ApnhlosPartition);
                iter.insert(whitespace(line)
                        % mountVfat.arg(ApnhlosPartition)
                                .arg(QStringLiteral("/firmware"))
                                .arg(voldArgs));
            }

            if (insertMdm) {
                iter.insert(whitespace(line)
                        % wait % MdmPartition);
                iter.insert(whitespace(line)
                        % mountVfat.arg(MdmPartition)
                                .arg(QStringLiteral("/firmware-mdm"))
                                .arg(voldArgs));
            }
        }
    }


    d->cpio->setContents(filename, lines.join(Newline).toUtf8());

    return true;
}

QString QcomRamdiskPatcher::makeWritable(const QString &mountArgs)
{
    static const QString PostRo = QStringLiteral("ro,");
    static const QString PostRw = QStringLiteral("rw,");
    static const QString PreRo = QStringLiteral(",ro");
    static const QString PreRw = QStringLiteral(",rw");

    if (mountArgs.contains(PostRo)) {
        return QString(mountArgs).replace(PostRo, PostRw);
    } else if (mountArgs.contains(PreRo)) {
        return QString(mountArgs).replace(PreRo, PreRw);
    } else {
        return PreRw % QString(mountArgs);
    }
}
