#include "hammerheadramdiskpatcher.h"
#include "hammerheadramdiskpatcher_p.h"

#include <libdbp-ramdisk-common/coreramdiskpatcher.h>
#include <libdbp-ramdisk-qcom/qcomramdiskpatcher.h>


const QString HammerheadRamdiskPatcher::AOSP =
        QStringLiteral("hammerhead/AOSP/AOSP");
const QString HammerheadRamdiskPatcher::CXL =
        QStringLiteral("hammerhead/AOSP/cxl");

HammerheadRamdiskPatcher::HammerheadRamdiskPatcher(const PatcherPaths * const pp,
                                                   const QString &id,
                                                   const FileInfo * const info,
                                                   CpioFile * const cpio) :
    d_ptr(new HammerheadRamdiskPatcherPrivate())
{
    Q_D(HammerheadRamdiskPatcher);

    d->pp = pp;
    d->id = id;
    d->info = info;
    d->cpio = cpio;
}

HammerheadRamdiskPatcher::~HammerheadRamdiskPatcher()
{
    // Destructor so d_ptr is destroyed
}

PatcherError::Error HammerheadRamdiskPatcher::error() const
{
    Q_D(const HammerheadRamdiskPatcher);

    return d->errorCode;
}

QString HammerheadRamdiskPatcher::errorString() const
{
    Q_D(const HammerheadRamdiskPatcher);

    return d->errorString;
}

QString HammerheadRamdiskPatcher::name() const
{
    Q_D(const HammerheadRamdiskPatcher);

    return d->id;
}

bool HammerheadRamdiskPatcher::patchRamdisk()
{
    Q_D(HammerheadRamdiskPatcher);

    if (d->id == AOSP) {
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

        if (!qcomPatcher.modifyFstab()) {
            d->errorCode = qcomPatcher.error();
            d->errorString = qcomPatcher.errorString();
            return false;
        }

        if (!qcomPatcher.modifyInitTargetRc(
                QStringLiteral("init.hammerhead.rc"))) {
            d->errorCode = qcomPatcher.error();
            d->errorString = qcomPatcher.errorString();
            return false;
        }

        return true;
    } else if (d->id == CXL) {
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

        return true;
    }

    return false;
}
