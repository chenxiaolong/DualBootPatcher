#include "standardplugin.h"
#include "standardpatcher.h"


StandardPlugin::StandardPlugin(QObject *parent) :
    QObject(parent)
{
}

QStringList StandardPlugin::autoPatchers() const
{
    return QStringList() << StandardPatcher::Name;
}

QSharedPointer<AutoPatcher> StandardPlugin::createAutoPatcher(const PatcherPaths * const pp,
                                                              const QString &id,
                                                              const FileInfo * const info,
                                                              const PatchInfo::AutoPatcherArgs &args) const
{
    return QSharedPointer<AutoPatcher>(new StandardPatcher(pp, id, info, args));
}
