#include "jfltepatcher.h"
#include "jfltepatcher_p.h"

#include <libdbp-autopatcher-standard/standardpatcher.h>

#include <QtCore/QRegularExpression>


const QString JfltePatcher::DalvikCachePatcher =
        QStringLiteral("DalvikCachePatcher");
const QString JfltePatcher::GoogleEditionPatcher =
        QStringLiteral("GoogleEditionPatcher");
const QString JfltePatcher::SlimAromaBundledMount =
        QStringLiteral("SlimAromaBundledMount");
const QString JfltePatcher::ImperiumPatcher =
        QStringLiteral("ImperiumPatcher");
const QString JfltePatcher::NegaliteNoWipeData =
        QStringLiteral("NegaliteNoWipeData");
const QString JfltePatcher::TriForceFixAroma =
        QStringLiteral("TriForceFixAroma");
const QString JfltePatcher::TriForceFixUpdate =
        QStringLiteral("TriForceFixUpdate");

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

JfltePatcher::JfltePatcher(const PatcherPaths * const pp,
                           const QString &id,
                           const FileInfo * const info) :
    d_ptr(new JfltePatcherPrivate())
{
    Q_D(JfltePatcher);

    d->pp = pp;
    d->id = id;
    d->info = info;
}

JfltePatcher::~JfltePatcher()
{
    // Destructor so d_ptr is destroyed
}

PatcherError::Error JfltePatcher::error() const
{
    return PatcherError::NoError;
}

QString JfltePatcher::errorString() const
{
    return PatcherError::errorString(PatcherError::NoError);
}

QString JfltePatcher::name() const
{
    Q_D(const JfltePatcher);

    return d->id;
}

QStringList JfltePatcher::newFiles() const
{
    return QStringList();
}

QStringList JfltePatcher::existingFiles() const
{
    Q_D(const JfltePatcher);

    if (d->id == DalvikCachePatcher) {
        return QStringList() << BuildProp;
    } else if (d->id == GoogleEditionPatcher) {
        return QStringList() << QcomAudioScript;
    } else if (d->id == SlimAromaBundledMount) {
        return QStringList() << StandardPatcher::UpdaterScript;
    } else if (d->id == ImperiumPatcher) {
        return QStringList() << StandardPatcher::UpdaterScript;
    } else if (d->id == NegaliteNoWipeData) {
        return QStringList() << StandardPatcher::UpdaterScript;
    } else if (d->id == TriForceFixAroma) {
        return QStringList() << AromaScript;
    } else if (d->id == TriForceFixUpdate) {
        return QStringList() << StandardPatcher::UpdaterScript;
    }

    return QStringList();
}

bool JfltePatcher::patchFile(const QString &file,
                             QByteArray * const contents,
                             const QStringList &bootImages)
{
    Q_D(JfltePatcher);
    Q_UNUSED(bootImages);

    if (d->id == DalvikCachePatcher && file == BuildProp) {
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
    } else if (d->id == GoogleEditionPatcher && file == QcomAudioScript) {
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
    } else if (d->id == SlimAromaBundledMount
            && file == StandardPatcher::UpdaterScript) {
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
    } else if (d->id == ImperiumPatcher
            && file == StandardPatcher::UpdaterScript) {
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
    } else if (d->id == NegaliteNoWipeData
            && file == StandardPatcher::UpdaterScript) {
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
    } else if (d->id == TriForceFixAroma && file == AromaScript) {
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
    } else if (d->id == TriForceFixUpdate
            && file == StandardPatcher::UpdaterScript) {
        QStringList lines = QString::fromUtf8(*contents).split(Newline);

        for (int i = 0; i < lines.size(); i++) {
            if (lines[i].contains(QRegularExpression(
                    QStringLiteral("getprop.+/system/build.prop")))) {
                i += StandardPatcher::insertMountSystem(i, &lines);
                i += StandardPatcher::insertMountCache(i, &lines);
                i += StandardPatcher::insertMountData(i, &lines);
                lines[i] = lines[i].replace(System,
                                            d->info->partConfig()->targetSystem());
            }
        }

        *contents = lines.join(Newline).toUtf8();

        return true;
    }

    return false;
}
