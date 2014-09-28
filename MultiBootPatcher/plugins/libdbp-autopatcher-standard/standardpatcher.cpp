#include "standardpatcher.h"
#include "standardpatcher_p.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QString>


const QString StandardPatcher::Name =
        QStringLiteral("StandardPatcher");

const QString StandardPatcher::ArgLegacyScript =
        QStringLiteral("legacy-script");

const QString StandardPatcher::UpdaterScript =
        QStringLiteral("META-INF/com/google/android/updater-script");
static const QString Mount =
        QStringLiteral("run_program(\"/tmp/dualboot.sh\", \"mount-%1\");");
static const QString Unmount =
        QStringLiteral("run_program(\"/tmp/dualboot.sh\", \"unmount-%1\");");
static const QString Format =
        QStringLiteral("run_program(\"/tmp/dualboot.sh\", \"format-%1\");");

static const QString System = QStringLiteral("system");
static const QString Cache = QStringLiteral("cache");
static const QString Data = QStringLiteral("data");

static const QChar Newline = QLatin1Char('\n');


StandardPatcher::StandardPatcher(const PatcherPaths * const pp,
                                 const QString &id,
                                 const FileInfo * const info,
                                 const PatchInfo::AutoPatcherArgs &args) :
    d_ptr(new StandardPatcherPrivate())
{
    Q_D(StandardPatcher);

    d->pp = pp;
    d->id = id;
    d->info = info;

    if (args.contains(ArgLegacyScript)) {
        const QString &value = args[ArgLegacyScript];

        if (value.toLower() == QStringLiteral("true")) {
            d->legacyScript = true;
        }
    }
}

StandardPatcher::~StandardPatcher()
{
    // Destructor so d_ptr is destroyed
}

PatcherError::Error StandardPatcher::error() const
{
    return PatcherError::NoError;
}

QString StandardPatcher::errorString() const
{
    return PatcherError::errorString(PatcherError::NoError);
}

QString StandardPatcher::name() const
{
    return Name;
}

QStringList StandardPatcher::newFiles() const
{
    return QStringList();
}

QStringList StandardPatcher::existingFiles() const
{
    Q_D(const StandardPatcher);

    if (d->id == Name) {
        return QStringList() << UpdaterScript;
    }

    return QStringList();
}

bool StandardPatcher::patchFile(const QString &file,
                                QByteArray * const contents,
                                const QStringList &bootImages)
{
    Q_D(StandardPatcher);

    if (d->id == Name && file == UpdaterScript) {
        QStringList lines = QString::fromUtf8(*contents).split(Newline);

        insertDualBootSh(&lines, d->legacyScript);
        replaceMountLines(&lines, d->info->device());
        replaceUnmountLines(&lines, d->info->device());
        replaceFormatLines(&lines, d->info->device());

        if (!bootImages.isEmpty()) {
            for (const QString &bootImage : bootImages) {
                insertWriteKernel(&lines, bootImage);
            }
        }

        // Too many ROMs don't unmount partitions after installation
        insertUnmountEverything(lines.size(), &lines);

        // Remove device check if requested
        QString key = d->info->patchInfo()->keyFromFilename(
                d->info->filename());
        if (!d->info->patchInfo()->deviceCheck(key)) {
            removeDeviceChecks(&lines);
        }

        *contents = lines.join(Newline).toUtf8();

        return true;
    }

    return false;
}

void StandardPatcher::removeDeviceChecks(QStringList *lines)
{
    QString reLine = QStringLiteral(
            "^\\s*assert\\s*\\(.*getprop\\s*\\(.*(ro.product.device|ro.build.product)");

    for (QString &line : *lines) {
        if (line.contains(QRegularExpression(reLine))) {
            line.replace(QRegularExpression(QStringLiteral("^(\\s*assert\\s*\\()")),
                         QStringLiteral("\\1\"true\" == \"true\" || "));
        }
    }
}

void StandardPatcher::insertDualBootSh(QStringList *lines, bool legacyScript)
{
    int i = 0;
    lines->insert(i++, QStringLiteral(
            "package_extract_file(\"dualboot.sh\", \"/tmp/dualboot.sh\");"));
    i += insertSetPerms(i, lines, legacyScript,
            QStringLiteral("/tmp/dualboot.sh"), 0777);
    lines->insert(i++, QStringLiteral(
            "ui_print(\"NOT INSTALLING AS PRIMARY\");"));
    insertUnmountEverything(i++, lines);
}

void StandardPatcher::insertWriteKernel(QStringList *lines,
                                        const QString &bootImage)
{
    QString setKernelLine = QStringLiteral(
            "run_program(\"/tmp/dualboot.sh\", \"set-multi-kernel\");");
    QStringList searchItems;
    searchItems << QStringLiteral("loki.sh");
    searchItems << QStringLiteral("flash_kernel.sh");
    searchItems << bootImage;

    // Look for the last line containing the boot image string and insert
    // after that
    for (const QString &item : searchItems) {
        for (int i = lines->size() - 1; i > 0; i--) {
            if (lines->at(i).contains(item)) {
                // Statements can be on multiple lines, so insert after a
                // semicolon is found
                while (i < lines->size()) {
                    if (lines->at(i).contains(QStringLiteral(";"))) {
                        lines->insert(i + 1, setKernelLine);
                        return;
                    } else {
                        i++;
                    }
                }

                break;
            }
        }
    }
}

void StandardPatcher::replaceMountLines(QStringList *lines,
                                        Device *device)
{
    QString pSystem = device->partition(System);
    QString pCache = device->partition(Cache);
    QString pData = device->partition(Data);

    for (int i = 0; i < lines->size();) {
        if (lines->at(i).contains(QRegularExpression(QStringLiteral(
                "^\\s*mount\\s*\\(.*$")))
                || lines->at(i).contains(QRegularExpression(QStringLiteral(
                "^\\s*run_program\\s*\\(\\s*\"[^\"]*busybox\"\\s*,\\s*\"mount\".*$")))) {
            if (lines->at(i).contains(System)
                    || (!pSystem.isEmpty() && lines->at(i).contains(pSystem))) {
                lines->removeAt(i);
                i += insertMountSystem(i, lines);
            } else if (lines->at(i).contains(Cache)
                    || (!pCache.isEmpty() && lines->at(i).contains(pCache))) {
                lines->removeAt(i);
                i += insertMountCache(i, lines);
            } else if (lines->at(i).contains(Data)
                    || lines->at(i).contains(QStringLiteral("userdata"))
                    || (!pData.isEmpty() && lines->at(i).contains(pData))) {
                lines->removeAt(i);
                i += insertMountData(i, lines);
            } else {
                i++;
            }
        } else {
            i++;
        }
    }
}

void StandardPatcher::replaceUnmountLines(QStringList *lines,
                                          Device *device)
{
    QString pSystem = device->partition(System);
    QString pCache = device->partition(Cache);
    QString pData = device->partition(Data);

    for (int i = 0; i < lines->size();) {
        if (lines->at(i).contains(QRegularExpression(QStringLiteral(
                "^\\s*unmount\\s*\\(.*$")))
                || lines->at(i).contains(QRegularExpression(QStringLiteral(
                "^\\s*run_program\\s*\\(\\s*\"[^\"]*busybox\"\\s*,\\s*\"umount\".*$")))) {
            if (lines->at(i).contains(System)
                    || (!pSystem.isEmpty() && lines->at(i).contains(pSystem))) {
                lines->removeAt(i);
                i += insertUnmountSystem(i, lines);
            } else if (lines->at(i).contains(Cache)
                    || (!pCache.isEmpty() && lines->at(i).contains(pCache))) {
                lines->removeAt(i);
                i += insertUnmountCache(i, lines);
            } else if (lines->at(i).contains(Data)
                    || lines->at(i).contains(QStringLiteral("userdata"))
                    || (!pData.isEmpty() && lines->at(i).contains(pData))) {
                lines->removeAt(i);
                i += insertUnmountData(i, lines);
            } else {
                i++;
            }
        } else {
            i++;
        }
    }
}

void StandardPatcher::replaceFormatLines(QStringList *lines,
                                         Device *device)
{
    QString pSystem = device->partition(System);
    QString pCache = device->partition(Cache);
    QString pData = device->partition(Data);

    for (int i = 0; i < lines->size();) {
        if (lines->at(i).contains(QRegularExpression(
                QStringLiteral("^\\s*format\\s*\\(.*$")))) {
            if (lines->at(i).contains(System)
                    || (!pSystem.isEmpty() && lines->at(i).contains(pSystem))) {
                lines->removeAt(i);
                i += insertFormatSystem(i, lines);
            } else if (lines->at(i).contains(Cache)
                    || (!pCache.isEmpty() && lines->at(i).contains(pCache))) {
                lines->removeAt(i);
                i += insertFormatCache(i, lines);
            } else if (lines->at(i).contains(QStringLiteral("userdata"))
                    || (!pData.isEmpty() && lines->at(i).contains(pData))) {
                lines->removeAt(i);
                i += insertFormatData(i, lines);
            } else {
                i++;
            }
        } else if (lines->at(i).contains(QRegularExpression(
                QStringLiteral("delete_recursive\\s*\\([^\\)]*\"/system\"")))) {
            lines->removeAt(i);
            i += insertFormatSystem(i, lines);
        } else if (lines->at(i).contains(QRegularExpression(
                QStringLiteral("delete_recursive\\s*\\([^\\)]*\"/cache\"")))) {
            lines->removeAt(i);
            i += insertFormatCache(i, lines);
        } else {
            i++;
        }
    }
}

int StandardPatcher::insertMountSystem(int index, QStringList *lines)
{
    lines->insert(index, Mount.arg(System));
    return 1;
}

int StandardPatcher::insertMountCache(int index, QStringList *lines)
{
    lines->insert(index, Mount.arg(Cache));
    return 1;
}

int StandardPatcher::insertMountData(int index, QStringList *lines)
{
    lines->insert(index, Mount.arg(Data));
    return 1;
}

int StandardPatcher::insertUnmountSystem(int index, QStringList *lines)
{
    lines->insert(index, Unmount.arg(System));
    return 1;
}

int StandardPatcher::insertUnmountCache(int index, QStringList *lines)
{
    lines->insert(index, Unmount.arg(Cache));
    return 1;
}

int StandardPatcher::insertUnmountData(int index, QStringList *lines)
{
    lines->insert(index, Unmount.arg(Data));
    return 1;
}

int StandardPatcher::insertUnmountEverything(int index, QStringList *lines)
{
    lines->insert(index, Unmount.arg(QStringLiteral("everything")));
    return 1;
}

int StandardPatcher::insertFormatSystem(int index, QStringList *lines)
{
    lines->insert(index, Format.arg(System));
    return 1;
}

int StandardPatcher::insertFormatCache(int index, QStringList *lines)
{
    lines->insert(index, Format.arg(Cache));
    return 1;
}

int StandardPatcher::insertFormatData(int index, QStringList *lines)
{
    lines->insert(index, Format.arg(Data));
    return 1;
}

int StandardPatcher::insertSetPerms(int index, QStringList *lines,
                                    bool legacyScript, const QString &file,
                                    uint mode)
{
#if 0
    if (legacyScript) {
        lines->insert(index, QStringLiteral("set_perm(0, 0, %2, \"%1\");")
                .arg(file)
                .arg(mode, 5, 8, QLatin1Char('0')));
    } else {
        lines->insert(index, QStringLiteral(
                "set_metadata(\"%1\", \"uid\", 0, \"gid\", 0, \"mode\", 0%2);")
                .arg(file)
                .arg(mode, 0, 8, QLatin1Char('0')));
    }
#endif

    // Don't feel like updating all the patchinfos right now to account for
    // all cases
    lines->insert(index, QStringLiteral(
            "run_program(\"/sbin/busybox\", \"chmod\", \"0%2\", \"%1\");")
            .arg(file)
            .arg(mode, 0, 8, QLatin1Char('0')));

    return 1;
}