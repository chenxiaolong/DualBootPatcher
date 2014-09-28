#include "hammerheadramdiskplugin.h"
#include "hammerheadramdiskpatcher.h"


HammerheadRamdiskPlugin::HammerheadRamdiskPlugin(QObject *parent) :
    QObject(parent)
{
}

QStringList HammerheadRamdiskPlugin::ramdiskPatchers() const
{
    return QStringList()
            << HammerheadRamdiskPatcher::AOSP
            << HammerheadRamdiskPatcher::CXL;
}

QSharedPointer<RamdiskPatcher> HammerheadRamdiskPlugin::createRamdiskPatcher(const PatcherPaths * const pp,
                                                                             const QString &id,
                                                                             const FileInfo * const info,
                                                                             CpioFile * const cpio) const
{
    return QSharedPointer<RamdiskPatcher>(new HammerheadRamdiskPatcher(pp, id, info, cpio));
}
