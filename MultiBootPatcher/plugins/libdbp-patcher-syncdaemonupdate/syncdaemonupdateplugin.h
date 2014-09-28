#ifndef SYNCDAEMONUPDATEPLUGIN_H
#define SYNCDAEMONUPDATEPLUGIN_H

#include <libdbp/plugininterface.h>

#include <QtCore/QtPlugin>


class SyncdaemonUpdatePlugin : public QObject,
                               public IPatcherFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.github.chenxiaolong.libdbp.IPatcherFactory/1.0" FILE "libdbp-patcher-syncdaemonupdate.json")
    Q_INTERFACES(IPatcherFactory)

public:
    explicit SyncdaemonUpdatePlugin(QObject *parent = 0);

    virtual QStringList patchers() const;

    virtual QList<PartitionConfig *> partConfigs() const;
    virtual QString patcherName(const QString &id) const;

    virtual QSharedPointer<Patcher> createPatcher(const PatcherPaths * const pp,
                                                  const QString &id) const;
};

#endif // SYNCDAEMONUPDATEPLUGIN_H
