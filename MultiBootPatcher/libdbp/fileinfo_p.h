#ifndef FILEINFO_P_H
#define FILEINFO_P_H

#include "fileinfo.h"
#include "patchinfo.h"

class FileInfoPrivate
{
public:
    PatchInfo *patchinfo;
    Device *device;
    QString filename;
    PartitionConfig *partconfig;
};

#endif // FILEINFO_P_H
