#ifndef QCOMRAMDISKPATCHER_P_H
#define QCOMRAMDISKPATCHER_P_H

#include <libdbp/fileinfo.h>
#include <libdbp/patchererror.h>


class QcomRamdiskPatcherPrivate
{
public:
    const PatcherPaths *pp;
    const FileInfo *info;
    CpioFile *cpio;

    PatcherError::Error errorCode;
    QString errorString;
};

#endif // QCOMRAMDISKPATCHER_P_H
