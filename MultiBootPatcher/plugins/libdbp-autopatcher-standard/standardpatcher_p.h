#ifndef STANDARDPATCHER_P_H
#define STANDARDPATCHER_P_H

#include <libdbp/fileinfo.h>


class StandardPatcherPrivate
{
public:
    const PatcherPaths *pp;
    QString id;
    const FileInfo *info;

    bool legacyScript;
};

#endif // STANDARDPATCHER_P_H
