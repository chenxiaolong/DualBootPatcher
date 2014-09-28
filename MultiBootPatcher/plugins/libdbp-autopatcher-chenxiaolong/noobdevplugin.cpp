#include "noobdevplugin.h"
#include "noobdevpatcher.h"


NoobdevPlugin::NoobdevPlugin(QObject *parent) :
    QObject(parent)
{
}

QStringList NoobdevPlugin::autoPatchers() const
{
    return QStringList()
            << NoobdevPatcher::NoobdevMultiBoot
            << NoobdevPatcher::NoobdevSystemProp;
}

QSharedPointer<AutoPatcher> NoobdevPlugin::createAutoPatcher(const PatcherPaths * const pp,
                                                             const QString &id,
                                                             const FileInfo * const info,
                                                             const PatchInfo::AutoPatcherArgs &args) const
{
    Q_UNUSED(args);
    return QSharedPointer<AutoPatcher>(new NoobdevPatcher(pp, id, info));
}
