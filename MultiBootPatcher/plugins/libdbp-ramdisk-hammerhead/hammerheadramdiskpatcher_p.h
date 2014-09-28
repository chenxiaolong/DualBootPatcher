#ifndef HAMMERHEADRAMDISKPATCHER_P_H
#define HAMMERHEADRAMDISKPATCHER_P_H

#include <libdbp/fileinfo.h>
#include <libdbp/patchererror.h>


class HammerheadRamdiskPatcherPrivate
{
public:
    const PatcherPaths *pp;
    QString id;
    const FileInfo *info;
    CpioFile *cpio;

    PatcherError::Error errorCode;
    QString errorString;
};

#endif // HAMMERHEADRAMDISKPATCHER_P_H
