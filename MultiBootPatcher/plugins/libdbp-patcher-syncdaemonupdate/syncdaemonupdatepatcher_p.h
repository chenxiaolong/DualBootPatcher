#ifndef SYNCDAEMONUPDATEPATCHER_P_H
#define SYNCDAEMONUPDATEPATCHER_P_H

#include <libdbp/fileinfo.h>
#include <libdbp/patchererror.h>


class SyncdaemonUpdatePatcherPrivate
{
public:
    const PatcherPaths *pp;
    QString id;
    const FileInfo *info;

    PatcherError::Error errorCode;
    QString errorString;
};

#endif // SYNCDAEMONUPDATEPATCHER_P_H
