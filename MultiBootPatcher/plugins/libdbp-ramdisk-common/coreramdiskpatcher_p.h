#ifndef CORERAMDISKPATCHER_P_H
#define CORERAMDISKPATCHER_P_H

#include <libdbp/fileinfo.h>
#include <libdbp/patchererror.h>


class CoreRamdiskPatcherPrivate
{
public:
    const PatcherPaths *pp;
    const FileInfo *info;
    CpioFile *cpio;

    PatcherError::Error errorCode;
    QString errorString;
};

#endif // CORERAMDISKPATCHER_P_H
