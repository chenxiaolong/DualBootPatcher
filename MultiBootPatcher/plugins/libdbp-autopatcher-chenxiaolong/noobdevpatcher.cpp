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

const QString NoobdevPatcher::NoobdevMultiBoot =
        QStringLiteral("NoobdevMultiBoot");
const QString NoobdevPatcher::NoobdevSystemProp =
        QStringLiteral("NoobdevSystemProp");

static const QString UpdaterScript =
        QStringLiteral("META-INF/com/google/android/updater-script");
static const QString DualBootSh =
        QStringLiteral("dualboot.sh");
static const QString BuildProp =
        QStringLiteral("system/build.prop");

static const QString PrintEmpty =
        QStringLiteral("ui_print(\"\");");

static const QChar Newline = QLatin1Char('\n');


NoobdevPatcher::NoobdevPatcher(const PatcherPaths * const pp,
                               const QString &id,
                               const FileInfo * const info) :
    d_ptr(new NoobdevPatcherPrivate())
{
    Q_D(NoobdevPatcher);

    d->pp = pp;
    d->id = id;
    d->info = info;
}

NoobdevPatcher::~NoobdevPatcher()
{
    // Destructor so d_ptr is destroyed
}

PatcherError::Error NoobdevPatcher::error() const
{
    return PatcherError::NoError;
}

QString NoobdevPatcher::errorString() const
{
    return PatcherError::errorString(PatcherError::NoError);
}

QString NoobdevPatcher::name() const
{
    Q_D(const NoobdevPatcher);

    return d->id;
}

QStringList NoobdevPatcher::newFiles() const
{
    return QStringList();
}

QStringList NoobdevPatcher::existingFiles() const
{
    Q_D(const NoobdevPatcher);

    if (d->id == NoobdevMultiBoot) {
        return QStringList() << UpdaterScript
                             << DualBootSh;
    } else if (d->id == NoobdevSystemProp) {
        return QStringList() << BuildProp;
    }

    return QStringList();
}

bool NoobdevPatcher::patchFile(const QString &file,
                               QByteArray * const contents,
                               const QStringList &bootImages)
{
    Q_D(NoobdevPatcher);
    Q_UNUSED(bootImages);

    if (d->id == NoobdevMultiBoot && file == UpdaterScript) {
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
    } else if (d->id == NoobdevMultiBoot && file == DualBootSh) {
        QString noDualBoot = QStringLiteral("echo 'ro.dualboot=0' > /tmp/dualboot.prop\n");
        contents->append(noDualBoot.toUtf8());

        return true;
    } else if (d->id == NoobdevSystemProp && file == BuildProp) {
        // The auto-updater in my ROM needs to know if the ROM has been patched
        QString prop = QStringLiteral("ro.chenxiaolong.patched=%1\n");
        contents->append(prop.arg(d->info->partConfig()->id()).toUtf8());

        return true;
    }

    return false;
}
