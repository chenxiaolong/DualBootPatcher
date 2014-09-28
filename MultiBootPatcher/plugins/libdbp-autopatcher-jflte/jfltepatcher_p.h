#ifndef JFLTEPATCHER_P_H
#define JFLTEPATCHER_P_H

#include <libdbp/fileinfo.h>


class JfltePatcherPrivate
{
public:
    const PatcherPaths *pp;
    QString id;
    const FileInfo *info;
};

#endif // JFLTEPATCHER_P_H
