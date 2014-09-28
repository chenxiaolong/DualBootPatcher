#ifndef MULTIBOOTPLUGIN_P_H
#define MULTIBOOTPLUGIN_P_H

#include <libdbp/partitionconfig.h>


class MultiBootPluginPrivate
{
public:
    QList<PartitionConfig *> configs;
};

#endif // MULTIBOOTPLUGIN_P_H
