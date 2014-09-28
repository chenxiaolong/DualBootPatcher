#include "primaryupgradeplugin.h"
#include "primaryupgradeplugin_p.h"
#include "primaryupgradepatcher.h"


PrimaryUpgradePlugin::PrimaryUpgradePlugin(QObject *parent) :
    QObject(parent), d_ptr(new PrimaryUpgradePluginPrivate())
{
    Q_D(PrimaryUpgradePlugin);

    // Add primary upgrade partition configuration
    PartitionConfig *config = new PartitionConfig();
    config->setId(QStringLiteral("primaryupgrade"));
    config->setKernel(QStringLiteral("primary"));
    config->setName(tr("Primary ROM Upgrade"));
    config->setDescription(
            tr("Upgrade primary ROM without wiping other ROMs"));

    config->setTargetSystem(QStringLiteral("/system"));
    config->setTargetCache(QStringLiteral("/cache"));
    config->setTargetData(QStringLiteral("/data"));

    config->setTargetSystemPartition(PartitionConfig::System);
    config->setTargetCachePartition(PartitionConfig::Cache);
    config->setTargetDataPartition(PartitionConfig::Data);

    d->configs << config;
}

PrimaryUpgradePlugin::~PrimaryUpgradePlugin()
{
}

QStringList PrimaryUpgradePlugin::patchers() const
{
    return QStringList() << PrimaryUpgradePatcher::Id;
}

QString PrimaryUpgradePlugin::patcherName(const QString &id) const
{
    if (id == PrimaryUpgradePatcher::Id) {
        return PrimaryUpgradePatcher::Name;
    }

    return QString();
}

QList<PartitionConfig *> PrimaryUpgradePlugin::partConfigs() const
{
    Q_D(const PrimaryUpgradePlugin);

    return d->configs;
}

QSharedPointer<Patcher> PrimaryUpgradePlugin::createPatcher(const PatcherPaths * const pp,
                                                            const QString &id) const
{
    return QSharedPointer<Patcher>(new PrimaryUpgradePatcher(pp, id));
}
