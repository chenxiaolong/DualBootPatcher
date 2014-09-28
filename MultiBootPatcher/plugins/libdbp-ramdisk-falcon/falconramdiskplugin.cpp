#include "falconramdiskplugin.h"
#include "falconramdiskpatcher.h"


FalconRamdiskPlugin::FalconRamdiskPlugin(QObject *parent) :
    QObject(parent)
{
}

QStringList FalconRamdiskPlugin::ramdiskPatchers() const
{
    return QStringList() << FalconRamdiskPatcher::AOSP;
}

QSharedPointer<RamdiskPatcher> FalconRamdiskPlugin::createRamdiskPatcher(const PatcherPaths * const pp,
                                                                         const QString &id,
                                                                         const FileInfo * const info,
                                                                         CpioFile * const cpio) const
{
    return QSharedPointer<RamdiskPatcher>(new FalconRamdiskPatcher(pp, id, info, cpio));
}
