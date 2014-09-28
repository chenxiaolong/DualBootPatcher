#ifndef PARTITIONCONFIG_P_H
#define PARTITIONCONFIG_P_H

#include "partitionconfig.h"

#include <QtCore/QString>

class PartitionConfigPrivate
{
public:
    QString name;
    QString description;
    QString kernel;
    QString id;

    QString targetSystem;
    QString targetCache;
    QString targetData;

    QString targetSystemPartition;
    QString targetCachePartition;
    QString targetDataPartition;
};

#endif // PARTITIONCONFIG_P_H
