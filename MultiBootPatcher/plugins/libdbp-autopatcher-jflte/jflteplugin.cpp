#include "jflteplugin.h"
#include "jfltepatcher.h"

JfltePlugin::JfltePlugin(QObject *parent) :
    QObject(parent)
{
}

QStringList JfltePlugin::autoPatchers() const
{
    return QStringList()
            << JfltePatcher::DalvikCachePatcher
            << JfltePatcher::GoogleEditionPatcher
            << JfltePatcher::SlimAromaBundledMount
            << JfltePatcher::ImperiumPatcher
            << JfltePatcher::NegaliteNoWipeData
            << JfltePatcher::TriForceFixAroma
            << JfltePatcher::TriForceFixUpdate;
}

QSharedPointer<AutoPatcher> JfltePlugin::createAutoPatcher(const PatcherPaths * const pp,
                                                           const QString &id,
                                                           const FileInfo * const info,
                                                           const PatchInfo::AutoPatcherArgs &args) const
{
    Q_UNUSED(args);
    return QSharedPointer<AutoPatcher>(new JfltePatcher(pp, id, info));
}
