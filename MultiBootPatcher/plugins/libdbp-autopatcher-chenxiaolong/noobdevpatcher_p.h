#ifndef NOOBDEVPATCHER_P_H
#define NOOBDEVPATCHER_P_H

#include <libdbp/fileinfo.h>


class NoobdevPatcherPrivate
{
public:
    const PatcherPaths *pp;
    QString id;
    const FileInfo *info;
};

#endif // NOOBDEVPATCHER_P_H
