#include "jflteramdiskpatcher.h"
#include "jflteramdiskpatcher_p.h"

#include <libdbp/patcherpaths.h>

#include <libdbp-ramdisk-common/coreramdiskpatcher.h>
#include <libdbp-ramdisk-qcom/qcomramdiskpatcher.h>

#include <QtCore/QRegularExpression>
#include <QtCore/QStringBuilder>


const QString JflteRamdiskPatcher::AOSP =
        QStringLiteral("jflte/AOSP/AOSP");
const QString JflteRamdiskPatcher::CXL =
        QStringLiteral("jflte/AOSP/cxl");
const QString JflteRamdiskPatcher::GoogleEdition =
        QStringLiteral("jflte/GoogleEdition/GoogleEdition");
const QString JflteRamdiskPatcher::TouchWiz =
        QStringLiteral("jflte/TouchWiz/TouchWiz");

static const QString JellyBean = QStringLiteral("jb43");
static const QString KitKat = QStringLiteral("kk44");

static const QString InitRc = QStringLiteral("init.rc");
static const QString InitTargetRc = QStringLiteral("init.target.rc");
static const QString UeventdRc = QStringLiteral("ueventd.rc");
static const QString UeventdQcomRc = QStringLiteral("ueventd.qcom.rc");
static const QString Msm8960LpmRc = QStringLiteral("MSM8960_lpm.rc");

static const QChar Comment = QLatin1Char('#');
static const QChar Newline = QLatin1Char('\n');

JflteRamdiskPatcher::JflteRamdiskPatcher(const PatcherPaths * const pp,
                                         const QString &id,
                                         const FileInfo * const info,
                                         CpioFile * const cpio) :
    d_ptr(new JflteRamdiskPatcherPrivate())
{
    Q_D(JflteRamdiskPatcher);

    d->pp = pp;
    d->id = id;
    d->info = info;
    d->cpio = cpio;

    if (d->id == GoogleEdition || d->id == TouchWiz) {
        if (d->cpio->exists(Msm8960LpmRc)) {
            d->getwVersion = JellyBean;
        } else {
            d->getwVersion = KitKat;
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

    if (!geModifyInitRc()) {
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

    if (!getwModifyMsm8960LpmRc()) {
        return false;
    }

    // Samsung's init binary is pretty screwed up
    if (d->getwVersion == KitKat) {
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

    if (!twModifyInitRc()) {
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

    if (!twModifyInitTargetRc()) {
        return false;
    }

    if (!getwModifyMsm8960LpmRc()) {
        return false;
    }

    if (!twModifyUeventdRc()) {
        return false;
    }

    if (!twModifyUeventdQcomRc()) {
        return false;
    }

    // Samsung's init binary is pretty screwed up
    if (d->getwVersion == KitKat) {
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

bool JflteRamdiskPatcher::geModifyInitRc()
{
    Q_D(JflteRamdiskPatcher);

    QByteArray contents = d->cpio->contents(InitRc);
    if (contents.isNull()) {
        d->errorCode = PatcherError::CpioFileNotExistError;
        d->errorString = PatcherError::errorString(d->errorCode)
                .arg(InitRc);
        return false;
    }

    QString previousLine;

    QStringList lines = QString::fromUtf8(contents).split(Newline);
    QMutableStringListIterator iter(lines);
    while (iter.hasNext()) {
        QString &line = iter.next();

        if (d->getwVersion == KitKat
                && line.contains(QRegularExpression(
                        QStringLiteral("mount.*/system")))
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

bool JflteRamdiskPatcher::twModifyInitRc()
{
    Q_D(JflteRamdiskPatcher);

    QByteArray contents = d->cpio->contents(InitRc);
    if (contents.isNull()) {
        d->errorCode = PatcherError::CpioFileNotExistError;
        d->errorString = PatcherError::errorString(d->errorCode)
                .arg(InitRc);
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
        } else if (d->getwVersion == KitKat
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

bool JflteRamdiskPatcher::twModifyInitTargetRc()
{
    Q_D(JflteRamdiskPatcher);

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

        if (d->getwVersion == KitKat
                && line.contains(QRegularExpression(
                        QStringLiteral("^on\\s+fs_selinux\\s*$")))) {
            iter.insert(QStringLiteral("    mount_all fstab.qcom"));
            iter.insert(QStringLiteral("    ") % CoreRamdiskPatcher::ExecMount);
        } else if (line.contains(QRegularExpression(
                QStringLiteral("^.*setprop.*selinux.reload_policy.*$")))) {
            line.insert(0, Comment);
        } else if (d->getwVersion == KitKat
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

bool JflteRamdiskPatcher::twModifyUeventdRc()
{
    Q_D(JflteRamdiskPatcher);

    // Only needs to be patched for Kit Kat
    if (d->getwVersion != KitKat) {
        return true;
    }

    QByteArray contents = d->cpio->contents(UeventdRc);
    if (contents.isNull()) {
        d->errorCode = PatcherError::CpioFileNotExistError;
        d->errorString = PatcherError::errorString(d->errorCode)
                .arg(UeventdRc);
        return false;
    }

    QStringList lines = QString::fromUtf8(contents).split(Newline);
    for (QString &line : lines) {
        if (line.contains(QStringLiteral("/dev/snd/*"))) {
            line.replace(QStringLiteral("0660"),
                         QStringLiteral("0666"));
        }
    }

    d->cpio->setContents(UeventdRc, lines.join(Newline).toUtf8());

    return true;
}

bool JflteRamdiskPatcher::twModifyUeventdQcomRc()
{
    Q_D(JflteRamdiskPatcher);

    // Only needs to be patched for Kit Kat
    if (d->getwVersion != KitKat) {
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
            line.replace(QStringLiteral("0660"),
                         QStringLiteral("0666"));
        }
    }

    lines << QStringLiteral("/dev/ion 0666 system system\n");

    d->cpio->setContents(UeventdQcomRc, lines.join(Newline).toUtf8());

    return true;
}

bool JflteRamdiskPatcher::getwModifyMsm8960LpmRc()
{
    Q_D(JflteRamdiskPatcher);

    // This file does not exist on Kit Kat ramdisks
    if (d->getwVersion == KitKat) {
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

    d->cpio->setContents(Msm8960LpmRc,
                         lines.join(Newline).toUtf8());

    return true;
}
