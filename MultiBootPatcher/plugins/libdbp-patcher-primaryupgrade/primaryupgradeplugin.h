#ifndef PRIMARYUPGRADEPLUGIN_H
#define PRIMARYUPGRADEPLUGIN_H

#include <libdbp/plugininterface.h>

#include <QtCore/QtPlugin>


class PrimaryUpgradePluginPrivate;

class PrimaryUpgradePlugin : public QObject,
                             public IPatcherFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.github.chenxiaolong.libdbp.IPatcherFactory/1.0" FILE "libdbp-patcher-primaryupgrade.json")
    Q_INTERFACES(IPatcherFactory)

public:
    explicit PrimaryUpgradePlugin(QObject *parent = 0);
    ~PrimaryUpgradePlugin();

    virtual QStringList patchers() const;
    virtual QString patcherName(const QString &id) const;

    virtual QList<PartitionConfig *> partConfigs() const;

    virtual QSharedPointer<Patcher> createPatcher(const PatcherPaths * const pp,
                                                  const QString &id) const;

private:
    const QScopedPointer<PrimaryUpgradePluginPrivate> d_ptr;
    Q_DECLARE_PRIVATE(PrimaryUpgradePlugin)
};

#endif // PRIMARYUPGRADEPLUGIN_H
