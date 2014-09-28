#include "partitionconfig.h"
#include "partitionconfig_p.h"

#include <QtCore/QStringList>

const QString PartitionConfig::ReplaceLine =
        QStringLiteral("# PATCHER REPLACE ME - DO NOT REMOVE");

// Device and mount point for the system, cache, and data filesystems
const QString PartitionConfig::DevSystem =
        QStringLiteral("/dev/block/platform/msm_sdcc.1/by-name/system::/raw-system");
const QString PartitionConfig::DevCache =
        QStringLiteral("/dev/block/platform/msm_sdcc.1/by-name/cache::/raw-cache");
const QString PartitionConfig::DevData =
        QStringLiteral("/dev/block/platform/msm_sdcc.1/by-name/userdata::/raw-data");

const QString PartitionConfig::System = QStringLiteral("$DEV_SYSTEM");
const QString PartitionConfig::Cache = QStringLiteral("$DEV_CACHE");
const QString PartitionConfig::Data = QStringLiteral("$DEV_DATA");

PartitionConfig::PartitionConfig() : d_ptr(new PartitionConfigPrivate())
{
}

PartitionConfig::~PartitionConfig()
{
    // Destructor so d_ptr is destroyed
}

QString PartitionConfig::name() const
{
    Q_D(const PartitionConfig);

    return d->name;
}

void PartitionConfig::setName(const QString &name)
{
    Q_D(PartitionConfig);

    d->name = name;
}

QString PartitionConfig::description() const
{
    Q_D(const PartitionConfig);

    return d->description;
}

void PartitionConfig::setDescription(const QString &description)
{
    Q_D(PartitionConfig);

    d->description = description;
}

QString PartitionConfig::kernel() const
{
    Q_D(const PartitionConfig);

    return d->kernel;
}

void PartitionConfig::setKernel(const QString &kernel)
{
    Q_D(PartitionConfig);

    d->kernel = kernel;
}

QString PartitionConfig::id() const
{
    Q_D(const PartitionConfig);

    return d->id;
}

void PartitionConfig::setId(const QString &id)
{
    Q_D(PartitionConfig);

    d->id = id;
}

QString PartitionConfig::targetSystem() const
{
    Q_D(const PartitionConfig);

    return d->targetSystem;
}

void PartitionConfig::setTargetSystem(const QString &path)
{
    Q_D(PartitionConfig);

    d->targetSystem = path;
}

QString PartitionConfig::targetCache() const
{
    Q_D(const PartitionConfig);

    return d->targetCache;
}

void PartitionConfig::setTargetCache(const QString &path)
{
    Q_D(PartitionConfig);

    d->targetCache = path;
}

QString PartitionConfig::targetData() const
{
    Q_D(const PartitionConfig);

    return d->targetData;
}

void PartitionConfig::setTargetData(const QString &path)
{
    Q_D(PartitionConfig);

    d->targetData = path;
}

QString PartitionConfig::targetSystemPartition() const
{
    Q_D(const PartitionConfig);

    return d->targetSystemPartition;
}

void PartitionConfig::setTargetSystemPartition(const QString &partition)
{
    Q_D(PartitionConfig);

    d->targetSystemPartition = partition;
}

QString PartitionConfig::targetCachePartition() const
{
    Q_D(const PartitionConfig);

    return d->targetCachePartition;
}

void PartitionConfig::setTargetCachePartition(const QString &partition)
{
    Q_D(PartitionConfig);

    d->targetCachePartition = partition;
}

QString PartitionConfig::targetDataPartition() const
{
    Q_D(const PartitionConfig);

    return d->targetDataPartition;
}

void PartitionConfig::setTargetDataPartition(const QString &partition)
{
    Q_D(PartitionConfig);

    d->targetDataPartition = partition;
}

bool PartitionConfig::replaceShellLine(QByteArray *contents,
                                       bool targetPathOnly) const
{
    Q_D(const PartitionConfig);

    QStringList lines = QString::fromUtf8(*contents).split(QLatin1Char('\n'));

    QMutableStringListIterator iter(lines);
    while (iter.hasNext()) {
        const QString &line = iter.next();

        if (line.contains(ReplaceLine)) {
            iter.remove();

            if (!targetPathOnly) {
                iter.insert(QStringLiteral("KERNEL_NAME=\"%1\"")
                        .arg(d->kernel));

                iter.insert(QStringLiteral("DEV_SYSTEM=\"%1\"")
                        .arg(DevSystem));
                iter.insert(QStringLiteral("DEV_CACHE=\"%1\"")
                        .arg(DevCache));
                iter.insert(QStringLiteral("DEV_DATA=\"%1\"")
                        .arg(DevData));

                iter.insert(QStringLiteral("TARGET_SYSTEM_PARTITION=\"%1\"")
                        .arg(d->targetSystemPartition));
                iter.insert(QStringLiteral("TARGET_CACHE_PARTITION=\"%1\"")
                        .arg(d->targetCachePartition));
                iter.insert(QStringLiteral("TARGET_DATA_PARTITION=\"%1\"")
                        .arg(d->targetDataPartition));
            }

            iter.insert(QStringLiteral("TARGET_SYSTEM=\"%1\"")
                    .arg(d->targetSystem));
            iter.insert(QStringLiteral("TARGET_CACHE=\"%1\"")
                    .arg(d->targetCache));
            iter.insert(QStringLiteral("TARGET_DATA=\"%1\"")
                    .arg(d->targetData));
        }
    }

    *contents = lines.join(QLatin1Char('\n')).toUtf8();

    return true;
}
