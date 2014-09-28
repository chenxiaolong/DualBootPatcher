#ifndef MULTIBOOTPLUGIN_H
#define MULTIBOOTPLUGIN_H

#include <libdbp/plugininterface.h>

#include <QtCore/QtPlugin>


class MultiBootPluginPrivate;

class MultiBootPlugin : public QObject,
                        public IPatcherFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.github.chenxiaolong.libdbp.IPatcherFactory/1.0" FILE "libdbp-patcher-multiboot.json")
    Q_INTERFACES(IPatcherFactory)

public:
    explicit MultiBootPlugin(QObject *parent = 0);
    ~MultiBootPlugin();

    virtual QStringList patchers() const;

    virtual QList<PartitionConfig *> partConfigs() const;
    virtual QString patcherName(const QString &id) const;

    virtual QSharedPointer<Patcher> createPatcher(const PatcherPaths * const pp,
                                                  const QString &id) const;

private:
    const QScopedPointer<MultiBootPluginPrivate> d_ptr;
    Q_DECLARE_PRIVATE(MultiBootPlugin)
};

#endif // MULTIBOOTPLUGIN_H
