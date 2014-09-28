#include "d800ramdiskplugin.h"
#include "d800ramdiskpatcher.h"


D800RamdiskPlugin::D800RamdiskPlugin(QObject *parent) :
    QObject(parent)
{
}

QStringList D800RamdiskPlugin::ramdiskPatchers() const
{
    return QStringList() << D800RamdiskPatcher::AOSP;
}

QSharedPointer<RamdiskPatcher> D800RamdiskPlugin::createRamdiskPatcher(const PatcherPaths * const pp,
                                                                       const QString &id,
                                                                       const FileInfo * const info,
                                                                       CpioFile * const cpio) const
{
    return QSharedPointer<RamdiskPatcher>(new D800RamdiskPatcher(pp, id, info, cpio));
}
