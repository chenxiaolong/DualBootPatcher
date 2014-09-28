#include "falconramdiskpatcher.h"
#include "falconramdiskpatcher_p.h"

#include <libdbp-ramdisk-common/coreramdiskpatcher.h>
#include <libdbp-ramdisk-qcom/qcomramdiskpatcher.h>


const QString FalconRamdiskPatcher::AOSP =
        QStringLiteral("falcon/AOSP/AOSP");

FalconRamdiskPatcher::FalconRamdiskPatcher(const PatcherPaths * const pp,
                                       const QString &id,
                                       const FileInfo * const info,
                                       CpioFile * const cpio) :
    d_ptr(new FalconRamdiskPatcherPrivate())
{
    Q_D(FalconRamdiskPatcher);

    d->pp = pp;
    d->id = id;
    d->info = info;
    d->cpio = cpio;
}

FalconRamdiskPatcher::~FalconRamdiskPatcher()
{
    // Destructor so d_ptr is destroyed
}

PatcherError::Error FalconRamdiskPatcher::error() const
{
    Q_D(const FalconRamdiskPatcher);

    return d->errorCode;
}

QString FalconRamdiskPatcher::errorString() const
{
    Q_D(const FalconRamdiskPatcher);

    return d->errorString;
}

QString FalconRamdiskPatcher::name() const
{
    Q_D(const FalconRamdiskPatcher);

    return d->id;
}

bool FalconRamdiskPatcher::patchRamdisk()
{
    Q_D(FalconRamdiskPatcher);

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

        if (!qcomPatcher.modifyInitQcomRc()) {
            d->errorCode = qcomPatcher.error();
            d->errorString = qcomPatcher.errorString();
            return false;
        }

        QVariantMap args;
        args[QcomRamdiskPatcher::ArgAdditionalFstabs] =
                QStringList() << QStringLiteral("gpe-fstab.qcom");

        if (!qcomPatcher.modifyFstab(args)) {
            d->errorCode = qcomPatcher.error();
            d->errorString = qcomPatcher.errorString();
            return false;
        }

        if (!qcomPatcher.modifyInitTargetRc()) {
            d->errorCode = qcomPatcher.error();
            d->errorString = qcomPatcher.errorString();
            return false;
        }

        return true;
    }

    return false;
}
