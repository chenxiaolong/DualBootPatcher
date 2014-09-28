#ifndef PATCHFILEPATCHER_P_H
#define PATCHFILEPATCHER_P_H

#include <libdbp/fileinfo.h>
#include <libdbp/patchererror.h>


class PatchFilePatcherPrivate
{
public:
    const PatcherPaths *pp;
    QString id;
    const FileInfo *info;
    PatchInfo::AutoPatcherArgs args;

    QHash<QString, QByteArray> patches;

    PatcherError::Error errorCode;
    QString errorString;
};

#endif // PATCHFILEPATCHER_P_H
