#include "jflteramdiskplugin.h"
#include "jflteramdiskpatcher.h"


JflteRamdiskPlugin::JflteRamdiskPlugin(QObject *parent) :
    QObject(parent)
{
}

QStringList JflteRamdiskPlugin::ramdiskPatchers() const
{
    return QStringList()
            << JflteRamdiskPatcher::AOSP
            << JflteRamdiskPatcher::CXL
            << JflteRamdiskPatcher::GoogleEdition
            << JflteRamdiskPatcher::TouchWiz;
}

QSharedPointer<RamdiskPatcher> JflteRamdiskPlugin::createRamdiskPatcher(const PatcherPaths * const pp,
                                                                        const QString &id,
                                                                        const FileInfo * const info,
                                                                        CpioFile * const cpio) const
{
    return QSharedPointer<RamdiskPatcher>(new JflteRamdiskPatcher(pp, id, info, cpio));
}
