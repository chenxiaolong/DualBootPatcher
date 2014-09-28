#ifndef BACONRAMDISKPATCHER_P_H
#define BACONRAMDISKPATCHER_P_H

#include <libdbp/fileinfo.h>
#include <libdbp/patchererror.h>


class BaconRamdiskPatcherPrivate
{
public:
    const PatcherPaths *pp;
    QString id;
    const FileInfo *info;
    CpioFile *cpio;

    PatcherError::Error errorCode;
    QString errorString;
};

#endif // BACONRAMDISKPATCHER_P_H
