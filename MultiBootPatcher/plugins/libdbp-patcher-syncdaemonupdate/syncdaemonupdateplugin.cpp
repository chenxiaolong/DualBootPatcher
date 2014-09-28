#include "syncdaemonupdateplugin.h"
#include "syncdaemonupdatepatcher.h"


SyncdaemonUpdatePlugin::SyncdaemonUpdatePlugin(QObject *parent) :
    QObject(parent)
{
}

QStringList SyncdaemonUpdatePlugin::patchers() const
{
    return QStringList() << SyncdaemonUpdatePatcher::Id;
}

QString SyncdaemonUpdatePlugin::patcherName(const QString &id) const
{
    if (id == SyncdaemonUpdatePatcher::Id) {
        return SyncdaemonUpdatePatcher::Name;
    }

    return QString();
}

QList<PartitionConfig *> SyncdaemonUpdatePlugin::partConfigs() const
{
    return QList<PartitionConfig *>();
}

QSharedPointer<Patcher> SyncdaemonUpdatePlugin::createPatcher(const PatcherPaths * const pp,
                                                              const QString &id) const
{
    return QSharedPointer<Patcher>(new SyncdaemonUpdatePatcher(pp, id));
}
