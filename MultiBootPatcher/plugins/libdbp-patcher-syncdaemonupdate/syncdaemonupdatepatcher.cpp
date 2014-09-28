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

#include "syncdaemonupdatepatcher.h"
#include "syncdaemonupdatepatcher_p.h"

#include <libdbp/bootimage.h>
#include <libdbp/patcherpaths.h>
#include <libdbp-ramdisk-common/coreramdiskpatcher.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QStringBuilder>


const QString SyncdaemonUpdatePatcher::Id =
        QStringLiteral("SyncdaemonUpdatePatcher");
const QString SyncdaemonUpdatePatcher::Name =
        tr("Update syncdaemon");

static const QChar Sep = QLatin1Char('/');

SyncdaemonUpdatePatcher::SyncdaemonUpdatePatcher(const PatcherPaths * const pp,
                                                 const QString &id) :
    d_ptr(new SyncdaemonUpdatePatcherPrivate())
{
    Q_D(SyncdaemonUpdatePatcher);

    d->pp = pp;
    d->id = id;
}

SyncdaemonUpdatePatcher::~SyncdaemonUpdatePatcher()
{
    // Destructor so d_ptr is destroyed
}

PatcherError::Error SyncdaemonUpdatePatcher::error() const
{
    Q_D(const SyncdaemonUpdatePatcher);

    return d->errorCode;
}

QString SyncdaemonUpdatePatcher::errorString() const
{
    Q_D(const SyncdaemonUpdatePatcher);

    return d->errorString;
}

QString SyncdaemonUpdatePatcher::id() const
{
    Q_D(const SyncdaemonUpdatePatcher);

    return d->id;
}

QString SyncdaemonUpdatePatcher::name() const
{
    Q_D(const SyncdaemonUpdatePatcher);

    if (d->id == Id) {
        return Name;
    }

    return QString();
}

bool SyncdaemonUpdatePatcher::usesPatchInfo() const
{
    return false;
}

QStringList SyncdaemonUpdatePatcher::supportedPartConfigIds() const
{
    return QStringList();
}

void SyncdaemonUpdatePatcher::setFileInfo(const FileInfo * const info)
{
    Q_D(SyncdaemonUpdatePatcher);

    d->info = info;
}

QString SyncdaemonUpdatePatcher::newFilePath()
{
    Q_D(SyncdaemonUpdatePatcher);

    if (d->info == nullptr) {
        qWarning() << "d->info is null!";
        d->errorCode = PatcherError::ImplementationError;
        d->errorString = PatcherError::errorString(d->errorCode);
        return QString();
    }

    QFileInfo fi(d->info->filename());
    return fi.dir().filePath(fi.completeBaseName()
            % QStringLiteral("_syncdaemon.") % fi.suffix());
}

bool SyncdaemonUpdatePatcher::patchFile()
{
    Q_D(SyncdaemonUpdatePatcher);

    if (d->info == nullptr) {
        qWarning() << "d->info is null!";
        d->errorCode = PatcherError::ImplementationError;
        d->errorString = PatcherError::errorString(d->errorCode);
        return false;
    }

    if (d->id == Name) {
        return patchImage();
    }

    return false;
}

bool SyncdaemonUpdatePatcher::patchImage()
{
    Q_D(SyncdaemonUpdatePatcher);

    BootImage bi;
    if (!bi.load(d->info->filename())) {
        d->errorCode = bi.error();
        d->errorString = bi.errorString();
        return false;
    }

    CpioFile cpio;
    cpio.load(bi.ramdiskImage());

    QString partConfigId = findPartConfigId(&cpio);
    if (!partConfigId.isNull()) {
        PartitionConfig *config = nullptr;

        for (PartitionConfig *c : d->pp->partitionConfigs()) {
            if (c->id() == partConfigId) {
                config = c;
                break;
            }
        }

        QString mountScript = QStringLiteral("init.multiboot.mounting.sh");
        if (config != nullptr && cpio.exists(mountScript)) {
            QFile mountScriptFile(d->pp->scriptsDirectory()
                    % QLatin1Char('/') % mountScript);
            if (!mountScriptFile.open(QFile::ReadOnly)) {
                d->errorCode = PatcherError::FileOpenError;
                d->errorString = PatcherError::errorString(d->errorCode)
                        .arg(mountScriptFile.fileName());
                return false;
            }

            QByteArray mountScriptContents = mountScriptFile.readAll();
            mountScriptFile.close();

            config->replaceShellLine(&mountScriptContents, true);

            if (cpio.exists(mountScript)) {
                cpio.remove(mountScript);
            }

            cpio.addFile(mountScriptContents, mountScript, 0750);
        }
    }

    // Add syncdaemon to init.rc if it doesn't already exist
    if (!cpio.exists(QStringLiteral("sbin/syncdaemon"))) {
        CoreRamdiskPatcher crp(d->pp, d->info, &cpio);
        crp.addSyncdaemon();
    } else {
        // Make sure 'oneshot' (added by previous versions) is removed
        QByteArray initRc = cpio.contents(QStringLiteral("init.rc"));
        QStringList lines = QString::fromUtf8(initRc).split(QLatin1Char('\n'));

        bool inSyncdaemon = false;

        QMutableStringListIterator iter(lines);
        while (iter.hasNext()) {
            QString &line = iter.next();

            if (line.startsWith(QStringLiteral("service"))) {
                inSyncdaemon = line.contains(
                        QStringLiteral("/sbin/syncdaemon"));
            } else if (inSyncdaemon
                    && line.contains(QStringLiteral("oneshot"))) {
                iter.remove();
            }
        }

        cpio.setContents(QStringLiteral("init.rc"),
                         lines.join(QLatin1Char('\n')).toUtf8());
    }

    QString syncdaemon = QStringLiteral("sbin/syncdaemon");

    if (cpio.exists(syncdaemon)) {
        cpio.remove(syncdaemon);
    }

    cpio.addFile(d->pp->binariesDirectory() % Sep
            % QStringLiteral("android") % Sep
            % d->info->device()->architecture() % Sep
            % QStringLiteral("syncdaemon"), syncdaemon, 0750);

    bi.setRamdiskImage(cpio.createData(true));

    QFile file(newFilePath());
    if (!file.open(QFile::WriteOnly)) {
        d->errorCode = PatcherError::FileOpenError;
        d->errorString = PatcherError::errorString(d->errorCode)
                .arg(file.fileName());
        return false;
    }

    file.write(bi.create());
    file.close();

    return true;
}

QString SyncdaemonUpdatePatcher::findPartConfigId(const CpioFile * const cpio) const
{
    QByteArray defaultProp = cpio->contents(QStringLiteral("default.prop"));
    if (!defaultProp.isNull()) {
        QStringList lines = QString::fromUtf8(defaultProp).split(QLatin1Char('\n'));

        for (const QString &line : lines) {
            if (line.startsWith(QStringLiteral("ro.patcher.patched"))) {
                return line.split(QLatin1Char('='))[1];
            }
        }
    }

    QByteArray mountScript = cpio->contents(
            QStringLiteral("init.multiboot.mounting.sh"));
    if (!mountScript.isNull()) {
        QStringList lines = QString::fromUtf8(mountScript).split(QLatin1Char('\n'));

        for (const QString &line : lines) {
            if (line.startsWith(QStringLiteral("TARGET_DATA="))) {
                QRegularExpressionMatch match = QRegularExpression(
                        QStringLiteral("/raw-data/([^/\"]+)")).match(line);
                if (match.hasMatch()) {
                    return match.captured(1);
                }
            }
        }
    }

    return QString();
}
