#include "coreramdiskplugin.h"

// This plugin is meant to be used by other plugins. It should not
// be used directly.

CoreRamdiskPlugin::CoreRamdiskPlugin(QObject *parent) :
    QObject(parent)
{
}

QStringList CoreRamdiskPlugin::ramdiskPatchers() const
{
    return QStringList();
}

QSharedPointer<RamdiskPatcher> CoreRamdiskPlugin::createRamdiskPatcher(const PatcherPaths * const pp,
                                                                       const QString &id,
                                                                       const FileInfo * const info,
                                                                       CpioFile * const cpio) const
{
    Q_UNUSED(pp);
    Q_UNUSED(id);
    Q_UNUSED(info);
    Q_UNUSED(cpio);
    return QSharedPointer<RamdiskPatcher>();
}
