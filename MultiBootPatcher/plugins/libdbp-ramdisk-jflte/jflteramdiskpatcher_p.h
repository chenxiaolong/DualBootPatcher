#ifndef JFLTERAMDISKPATCHER_P_H
#define JFLTERAMDISKPATCHER_P_H

#include <libdbp/fileinfo.h>
#include <libdbp/patchererror.h>


class JflteRamdiskPatcherPrivate
{
public:
    const PatcherPaths *pp;
    QString id;
    const FileInfo *info;
    CpioFile *cpio;

    QString getwVersion;

    PatcherError::Error errorCode;
    QString errorString;
};

#endif // JFLTERAMDISKPATCHER_P_H
