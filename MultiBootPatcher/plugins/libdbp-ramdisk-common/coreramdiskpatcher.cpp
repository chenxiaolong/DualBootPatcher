#include "coreramdiskpatcher.h"
#include "coreramdiskpatcher_p.h"

#include <libdbp/patcherpaths.h>


const QString CoreRamdiskPatcher::FstabRegex =
        QStringLiteral("^(#.+)?(/dev/\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)");
const QString CoreRamdiskPatcher::ExecMount =
        QStringLiteral("exec /sbin/busybox-static sh /init.multiboot.mounting.sh");
const QString CoreRamdiskPatcher::PropPartconfig =
        QStringLiteral("ro.patcher.patched=%1\n");
const QString CoreRamdiskPatcher::PropVersion =
        QStringLiteral("ro.patcher.version=%1\n");
const QString CoreRamdiskPatcher::SyncdaemonService =
        QStringLiteral("\nservice syncdaemon /sbin/syncdaemon\n")
        + QStringLiteral("    class main\n")
        + QStringLiteral("    user root\n");

static const QString DefaultProp =
        QStringLiteral("default.prop");
static const QString InitRc =
        QStringLiteral("init.rc");
static const QString Busybox =
        QStringLiteral("sbin/busybox");

CoreRamdiskPatcher::CoreRamdiskPatcher(const PatcherPaths * const pp,
                                       const FileInfo * const info,
                                       CpioFile * const cpio) :
    d_ptr(new CoreRamdiskPatcherPrivate())
{
    Q_D(CoreRamdiskPatcher);

    d->pp = pp;
    d->info = info;
    d->cpio = cpio;
}

CoreRamdiskPatcher::~CoreRamdiskPatcher()
{
    // Destructor so d_ptr is destroyed
}

PatcherError::Error CoreRamdiskPatcher::error() const
{
    Q_D(const CoreRamdiskPatcher);

    return d->errorCode;
}

QString CoreRamdiskPatcher::errorString() const
{
    Q_D(const CoreRamdiskPatcher);

    return d->errorString;
}

QString CoreRamdiskPatcher::name() const
{
    return QString();
}

bool CoreRamdiskPatcher::patchRamdisk()
{
    if (!modifyDefaultProp()) {
        return false;
    }
    if (!addSyncdaemon()) {
        return false;
    }
    if (!removeAndRelinkBusybox()) {
        return false;
    }
    return true;
}

bool CoreRamdiskPatcher::modifyDefaultProp()
{
    Q_D(CoreRamdiskPatcher);

    QByteArray defaultProp = d->cpio->contents(DefaultProp);
    if (defaultProp.isNull()) {
        d->errorCode = PatcherError::CpioFileNotExistError;
        d->errorString = PatcherError::errorString(d->errorCode)
                .arg(DefaultProp);
        return false;
    }

    defaultProp.append('\n');
    defaultProp.append(CoreRamdiskPatcher::PropPartconfig
            .arg(d->info->partConfig()->id()).toUtf8());
    defaultProp.append(CoreRamdiskPatcher::PropVersion
            .arg(d->pp->version()).toUtf8());

    d->cpio->setContents(DefaultProp, defaultProp);

    return true;
}

bool CoreRamdiskPatcher::addSyncdaemon()
{
    Q_D(CoreRamdiskPatcher);

    QByteArray initRc = d->cpio->contents(InitRc);
    if (initRc.isNull()) {
        d->errorCode = PatcherError::CpioFileNotExistError;
        d->errorString = PatcherError::errorString(d->errorCode)
                .arg(InitRc);
        return false;
    }

    initRc.append(SyncdaemonService.toUtf8());

    d->cpio->setContents(InitRc, initRc);

    return true;
}

bool CoreRamdiskPatcher::removeAndRelinkBusybox()
{
    Q_D(CoreRamdiskPatcher);

    if (d->cpio->exists(Busybox)) {
        d->cpio->remove(Busybox);
        bool ret = d->cpio->addSymlink(
                QStringLiteral("busybox-static"), Busybox);

        if (!ret) {
            d->errorCode = d->cpio->error();
            d->errorString = d->cpio->errorString();
        }

        return ret;
    }

    return true;
}
