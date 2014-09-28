#ifndef FALCONRAMDISKPATCHER_P_H
#define FALCONRAMDISKPATCHER_P_H

#include <libdbp/fileinfo.h>
#include <libdbp/patchererror.h>


class FalconRamdiskPatcherPrivate
{
public:
    const PatcherPaths *pp;
    QString id;
    const FileInfo *info;
    CpioFile *cpio;

    PatcherError::Error errorCode;
    QString errorString;
};

#endif // FALCONRAMDISKPATCHER_P_H
