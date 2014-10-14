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

#include "noobdevpatcher.h"
#include "noobdevpatcher_p.h"


static const QString UpdaterScript =
        QStringLiteral("META-INF/com/google/android/updater-script");
static const QString DualBootSh =
        QStringLiteral("dualboot.sh");
static const QString BuildProp =
        QStringLiteral("system/build.prop");

static const QString PrintEmpty =
        QStringLiteral("ui_print(\"\");");

static const QChar Newline = QLatin1Char('\n');


NoobdevBasePatcher::NoobdevBasePatcher(const PatcherPaths * const pp,
                                       const FileInfo * const info)
    : d_ptr(new NoobdevBasePatcherPrivate())
{
    Q_D(NoobdevBasePatcher);

    d->pp = pp;
    d->info = info;
}

NoobdevBasePatcher::~NoobdevBasePatcher()
{
    // Destructor so d_ptr is destroyed
}

PatcherError::Error NoobdevBasePatcher::error() const
{
    return PatcherError::NoError;
}

QString NoobdevBasePatcher::errorString() const
{
    return PatcherError::errorString(PatcherError::NoError);
}

////////////////////////////////////////////////////////////////////////////////

const QString NoobdevMultiBoot::Id
        = QStringLiteral("NoobdevMultiBoot");

NoobdevMultiBoot::NoobdevMultiBoot(const PatcherPaths* const pp,
                                   const FileInfo* const info)
    : NoobdevBasePatcher(pp, info)
{
}

QString NoobdevMultiBoot::id() const
{
    return Id;
}

QStringList NoobdevMultiBoot::newFiles() const
{
    return QStringList();
}

QStringList NoobdevMultiBoot::existingFiles() const
{
    return QStringList() << UpdaterScript
                         << DualBootSh;
}

bool NoobdevMultiBoot::patchFile(const QString &file,
                                 QByteArray * const contents,
                                 const QStringList &bootImages)
{
    Q_UNUSED(bootImages);

    if (file == UpdaterScript) {
        // My ROM has built-in dual boot support, so we'll hack around that to
        // support triple, quadruple, ... boot
        QStringList lines = QString::fromUtf8(*contents).split(Newline);

        QMutableStringListIterator iter(lines);
        while (iter.hasNext()) {
            QString &line = iter.next();

            if (line.contains(QStringLiteral("system/bin/dualboot.sh"))) {
                // Remove existing dualboot.sh lines
                iter.remove();
            } else if (line.contains(QStringLiteral("boot installation is"))) {
                // Remove confusing messages, but need to keep at least one
                // statement in the if-block to keep update-binary happy
                line = PrintEmpty;
            } else if (line.contains(QStringLiteral("set-secondary"))) {
                line = PrintEmpty;
            }
        }

        *contents = lines.join(Newline).toUtf8();

        return true;
    } else if (file == DualBootSh) {
        QString noDualBoot = QStringLiteral("echo 'ro.dualboot=0' > /tmp/dualboot.prop\n");
        contents->append(noDualBoot.toUtf8());

        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////

const QString NoobdevSystemProp::Id
        = QStringLiteral("NoobdevSystemProp");

NoobdevSystemProp::NoobdevSystemProp(const PatcherPaths* const pp,
                                     const FileInfo* const info)
    : NoobdevBasePatcher(pp, info)
{
}

QString NoobdevSystemProp::id() const
{
    return Id;
}

QStringList NoobdevSystemProp::newFiles() const
{
    return QStringList();
}

QStringList NoobdevSystemProp::existingFiles() const
{
    return QStringList() << BuildProp;
}

bool NoobdevSystemProp::patchFile(const QString &file,
                                  QByteArray * const contents,
                                  const QStringList &bootImages)
{
    Q_UNUSED(bootImages);
    Q_D(NoobdevBasePatcher);

    if (file == BuildProp) {
        // The auto-updater in my ROM needs to know if the ROM has been patched
        QString prop = QStringLiteral("ro.chenxiaolong.patched=%1\n");
        contents->append(prop.arg(d->info->partConfig()->id()).toUtf8());

        return true;
    }

    return false;
}
