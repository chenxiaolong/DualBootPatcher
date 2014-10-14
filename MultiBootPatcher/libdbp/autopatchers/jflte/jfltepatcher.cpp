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

#include "jfltepatcher.h"
#include "jfltepatcher_p.h"

#include "autopatchers/standard/standardpatcher.h"

#include <QtCore/QRegularExpression>


static const QString AromaScript =
        QStringLiteral("META-INF/com/google/android/aroma-config");
static const QString BuildProp =
        QStringLiteral("system/build.prop");
static const QString QcomAudioScript =
        QStringLiteral("system/etc/init.qcom.audio.sh");

static const QString System = QStringLiteral("/system");
static const QString Cache = QStringLiteral("/cache");
static const QString Data = QStringLiteral("/data");

static const QChar Newline = QLatin1Char('\n');

JflteBasePatcher::JflteBasePatcher(const PatcherPaths * const pp,
                                   const FileInfo * const info)
    : d_ptr(new JflteBasePatcherPrivate())
{
    Q_D(JflteBasePatcher);

    d->pp = pp;
    d->info = info;
}

JflteBasePatcher::~JflteBasePatcher()
{
    // Destructor so d_ptr is destroyed
}

PatcherError::Error JflteBasePatcher::error() const
{
    return PatcherError::NoError;
}

QString JflteBasePatcher::errorString() const
{
    return PatcherError::errorString(PatcherError::NoError);
}

////////////////////////////////////////////////////////////////////////////////

const QString JflteDalvikCachePatcher::Id
        = QStringLiteral("DalvikCachePatcher");

JflteDalvikCachePatcher::JflteDalvikCachePatcher(const PatcherPaths* const pp,
                                                 const FileInfo* const info)
    : JflteBasePatcher(pp, info)
{
}

QString JflteDalvikCachePatcher::id() const
{
    return Id;
}

QStringList JflteDalvikCachePatcher::newFiles() const
{
    return QStringList();
}

QStringList JflteDalvikCachePatcher::existingFiles() const
{
    return QStringList() << BuildProp;
}

bool JflteDalvikCachePatcher::patchFile(const QString &file,
                                        QByteArray * const contents,
                                        const QStringList &bootImages)
{
    Q_UNUSED(bootImages);

    if (file == BuildProp) {
        QStringList lines = QString::fromUtf8(*contents).split(Newline);

        QMutableStringListIterator iter(lines);
        while (iter.hasNext()) {
            QString &line = iter.next();

            if (line.contains(QStringLiteral("dalvik.vm.dexopt-data-only"))) {
                iter.remove();
                break;
            }
        }

        *contents = lines.join(Newline).toUtf8();

        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////

const QString JflteGoogleEditionPatcher::Id
        = QStringLiteral("GoogleEditionPatcher");

JflteGoogleEditionPatcher::JflteGoogleEditionPatcher(const PatcherPaths* const pp,
                                                     const FileInfo* const info)
    : JflteBasePatcher(pp, info)
{
}

QString JflteGoogleEditionPatcher::id() const
{
    return Id;
}

QStringList JflteGoogleEditionPatcher::newFiles() const
{
    return QStringList();
}

QStringList JflteGoogleEditionPatcher::existingFiles() const
{
    return QStringList() << QcomAudioScript;
}

bool JflteGoogleEditionPatcher::patchFile(const QString &file,
                                          QByteArray * const contents,
                                          const QStringList &bootImages)
{
    Q_UNUSED(bootImages);

    if (file == QcomAudioScript) {
        QStringList lines = QString::fromUtf8(*contents).split(Newline);

        QMutableStringListIterator iter(lines);
        while (iter.hasNext()) {
            QString &line = iter.next();

            if (line.contains(QStringLiteral("snd_soc_msm_2x_Fusion3_auxpcm"))) {
                iter.remove();
            }
        }

        *contents = lines.join(Newline).toUtf8();

        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////

const QString JflteSlimAromaBundledMount::Id
        = QStringLiteral("SlimAromaBundledMount");

JflteSlimAromaBundledMount::JflteSlimAromaBundledMount(const PatcherPaths* const pp,
                                                       const FileInfo* const info)
    : JflteBasePatcher(pp, info)
{
}

QString JflteSlimAromaBundledMount::id() const
{
    return Id;
}

QStringList JflteSlimAromaBundledMount::newFiles() const
{
    return QStringList();
}

QStringList JflteSlimAromaBundledMount::existingFiles() const
{
    return QStringList() << StandardPatcher::UpdaterScript;
}

bool JflteSlimAromaBundledMount::patchFile(const QString &file,
                                           QByteArray * const contents,
                                           const QStringList &bootImages)
{
    Q_UNUSED(bootImages);

    if (file == StandardPatcher::UpdaterScript) {
        QStringList lines = QString::fromUtf8(*contents).split(Newline);

        for (int i = 0; i < lines.size();) {
            if (lines[i].contains(QRegularExpression(
                    QStringLiteral("/tmp/mount.*/system")))) {
                lines.removeAt(i);
                i += StandardPatcher::insertMountSystem(i, &lines);
            } else if (lines[i].contains(QRegularExpression(
                    QStringLiteral("/tmp/mount.*/cache")))) {
                lines.removeAt(i);
                i += StandardPatcher::insertMountCache(i, &lines);
            } else if (lines[i].contains(QRegularExpression(
                    QStringLiteral("/tmp/mount.*/data")))) {
                lines.removeAt(i);
                i += StandardPatcher::insertMountData(i, &lines);
            } else {
                i++;
            }
        }

        *contents = lines.join(Newline).toUtf8();

        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////

const QString JflteImperiumPatcher::Id
        = QStringLiteral("ImperiumPatcher");

JflteImperiumPatcher::JflteImperiumPatcher(const PatcherPaths* const pp,
                                           const FileInfo* const info)
    : JflteBasePatcher(pp, info)
{
}

QString JflteImperiumPatcher::id() const
{
    return Id;
}

QStringList JflteImperiumPatcher::newFiles() const
{
    return QStringList();
}

QStringList JflteImperiumPatcher::existingFiles() const
{
    return QStringList() << StandardPatcher::UpdaterScript;
}

bool JflteImperiumPatcher::patchFile(const QString &file,
                                     QByteArray * const contents,
                                     const QStringList &bootImages)
{
    Q_UNUSED(bootImages);

    Q_D(JflteBasePatcher);

    if (file == StandardPatcher::UpdaterScript) {
        QStringList lines = QString::fromUtf8(*contents).split(Newline);

        StandardPatcher::insertDualBootSh(&lines, true);
        StandardPatcher::replaceMountLines(&lines, d->info->device());
        StandardPatcher::replaceUnmountLines(&lines, d->info->device());
        StandardPatcher::replaceFormatLines(&lines, d->info->device());
        StandardPatcher::insertUnmountEverything(lines.size(), &lines);

        // Insert set kernel line
        QString setKernelLine = QStringLiteral(
                "run_program(\"/tmp/dualboot.sh\", \"set-multi-kernel\");");
        for (int i = lines.size() - 1; i >= 0; i--) {
            if (lines[i].contains(QStringLiteral("Umounting Partitions"))) {
                lines.insert(i + 1, setKernelLine);
                break;
            }
        }

        *contents = lines.join(Newline).toUtf8();

        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////

const QString JflteNegaliteNoWipeData::Id
        = QStringLiteral("NegaliteNoWipeData");

JflteNegaliteNoWipeData::JflteNegaliteNoWipeData(const PatcherPaths* const pp,
                                                 const FileInfo* const info)
    : JflteBasePatcher(pp, info)
{
}

QString JflteNegaliteNoWipeData::id() const
{
    return Id;
}

QStringList JflteNegaliteNoWipeData::newFiles() const
{
    return QStringList();
}

QStringList JflteNegaliteNoWipeData::existingFiles() const
{
    return QStringList() << StandardPatcher::UpdaterScript;
}

bool JflteNegaliteNoWipeData::patchFile(const QString &file,
                                        QByteArray * const contents,
                                        const QStringList &bootImages)
{
    Q_UNUSED(bootImages);

    if (file == StandardPatcher::UpdaterScript) {
        QStringList lines = QString::fromUtf8(*contents).split(Newline);

        for (int i = 0; i < lines.size(); i++) {
            if (lines[i].contains(QRegularExpression(
                    QStringLiteral("run_program.*/tmp/wipedata.sh")))) {
                lines.removeAt(i);
                StandardPatcher::insertFormatData(i, &lines);
                break;
            }
        }

        *contents = lines.join(Newline).toUtf8();

        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////

const QString JflteTriForceFixAroma::Id
        = QStringLiteral("TriForceFixAroma");

JflteTriForceFixAroma::JflteTriForceFixAroma(const PatcherPaths* const pp,
                                             const FileInfo* const info)
    : JflteBasePatcher(pp, info)
{
}

QString JflteTriForceFixAroma::id() const
{
    return Id;
}

QStringList JflteTriForceFixAroma::newFiles() const
{
    return QStringList();
}

QStringList JflteTriForceFixAroma::existingFiles() const
{
    return QStringList() << AromaScript;
}

bool JflteTriForceFixAroma::patchFile(const QString &file,
                                      QByteArray * const contents,
                                      const QStringList &bootImages)
{
    Q_UNUSED(bootImages);
    Q_D(JflteBasePatcher);

    if (file == AromaScript) {
        QStringList lines = QString::fromUtf8(*contents).split(Newline);

        for (int i = 0; i < lines.size(); i++) {
            if (lines[i].contains(BuildProp)) {
                // Remove 'raw-' since aroma mounts the partitions directly
                QString targetDir = d->info->partConfig()->targetSystem()
                        .replace(QStringLiteral("raw-"), QStringLiteral(""));
                lines[i].replace(System, targetDir);
            } else if (lines[i].contains(QRegularExpression(
                    QStringLiteral("/sbin/mount.+/system")))) {
                lines.insert(i + 1, lines[i].replace(System, Cache));
                lines.insert(i + 2, lines[i].replace(System, Data));
                i += 2;
            }
        }

        *contents = lines.join(Newline).toUtf8();

        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////

const QString JflteTriForceFixUpdate::Id
        = QStringLiteral("TriForceFixUpdate");

JflteTriForceFixUpdate::JflteTriForceFixUpdate(const PatcherPaths* const pp,
                                               const FileInfo* const info)
    : JflteBasePatcher(pp, info)
{
}

QString JflteTriForceFixUpdate::id() const
{
    return Id;
}

QStringList JflteTriForceFixUpdate::newFiles() const
{
    return QStringList();
}

QStringList JflteTriForceFixUpdate::existingFiles() const
{
    return QStringList() << StandardPatcher::UpdaterScript;
}

bool JflteTriForceFixUpdate::patchFile(const QString &file,
                                       QByteArray * const contents,
                                       const QStringList &bootImages)
{
    Q_UNUSED(bootImages);
    Q_D(JflteBasePatcher);

    if (file == StandardPatcher::UpdaterScript) {
        QStringList lines = QString::fromUtf8(*contents).split(Newline);

        for (int i = 0; i < lines.size(); i++) {
            if (lines[i].contains(QRegularExpression(
                    QStringLiteral("getprop.+/system/build.prop")))) {
                i += StandardPatcher::insertMountSystem(i, &lines);
                i += StandardPatcher::insertMountCache(i, &lines);
                i += StandardPatcher::insertMountData(i, &lines);
                lines[i] = lines[i].replace(
                        System, d->info->partConfig()->targetSystem());
            }
        }

        *contents = lines.join(Newline).toUtf8();

        return true;
    }

    return false;
}
