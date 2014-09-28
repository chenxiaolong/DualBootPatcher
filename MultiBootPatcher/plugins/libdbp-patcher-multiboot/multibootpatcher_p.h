#ifndef MULTIBOOTPATCHER_P_H
#define MULTIBOOTPATCHER_P_H

#include <libdbp/fileinfo.h>
#include <libdbp/patchererror.h>


class MultiBootPatcherPrivate
{
public:
    const PatcherPaths *pp;
    QString id;
    const FileInfo *info;

    int progress;

    PatcherError::Error errorCode;
    QString errorString;
};

#endif // MULTIBOOTPATCHER_P_H
