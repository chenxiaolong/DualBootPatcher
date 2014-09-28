#include "baconramdiskplugin.h"
#include "baconramdiskpatcher.h"


BaconRamdiskPlugin::BaconRamdiskPlugin(QObject *parent) :
    QObject(parent)
{
}

QStringList BaconRamdiskPlugin::ramdiskPatchers() const
{
    return QStringList() << BaconRamdiskPatcher::AOSP;
}

QSharedPointer<RamdiskPatcher> BaconRamdiskPlugin::createRamdiskPatcher(const PatcherPaths * const pp,
                                                                        const QString &id,
                                                                        const FileInfo * const info,
                                                                        CpioFile * const cpio) const
{
    return QSharedPointer<RamdiskPatcher>(new BaconRamdiskPatcher(pp, id, info, cpio));
}
