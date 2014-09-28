#ifndef PRIMARYUPGRADEPATCHER_P_H
#define PRIMARYUPGRADEPATCHER_P_H

#include <libdbp/fileinfo.h>
#include <libdbp/patchererror.h>


class PrimaryUpgradePatcherPrivate
{
public:
    const PatcherPaths *pp;
    QString id;
    const FileInfo *info;

    int progress;

    PatcherError::Error errorCode;
    QString errorString;
};

#endif // PRIMARYUPGRADEPATCHER_P_H
