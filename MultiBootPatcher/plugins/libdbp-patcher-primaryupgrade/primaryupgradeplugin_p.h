#ifndef PRIMARYUPGRADEPLUGIN_P_H
#define PRIMARYUPGRADEPLUGIN_P_H

#include <libdbp/partitionconfig.h>


class PrimaryUpgradePluginPrivate
{
public:
    QList<PartitionConfig *> configs;
};

#endif // PRIMARYUPGRADEPLUGIN_P_H
