#ifndef CPIOFILE_P_H
#define CPIOFILE_P_H

#include "cpiofile.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QPair>

// libarchive
#include <archive_entry.h>

class CpioFilePrivate
{
public:
    ~CpioFilePrivate();

    QList<QPair<archive_entry *, QByteArray>> files;

    PatcherError::Error errorCode;
    QString errorString;
};

#endif // CPIOFILE_P_H
