#ifndef PATCHERERROR_H
#define PATCHERERROR_H

#include "libdbp_global.h"

#include <QtCore/QObject>
#include <QtCore/QString>

class LIBDBPSHARED_EXPORT PatcherError : public QObject
{
    Q_OBJECT

public:
    enum Error {
        NoError,
        CustomError,
        UnknownError,
        ImplementationError,

        // Plugins
        PatcherCreateError,
        AutoPatcherCreateError,
        RamdiskPatcherCreateError,

        // File access
        FileOpenError,
        FileMapError,
        FileWriteError,
        DirectoryNotExistError,

        // Boot Image
        BootImageSmallerThanHeaderError,
        BootImageNoAndroidHeaderError,
        BootImageNoRamdiskGzipHeaderError,
        BootImageNoRamdiskSizeError,
        BootImageNoKernelSizeError,
        BootImageNoRamdiskAddressError,

        // cpio
        CpioFileAlreadyExistsError,
        CpioFileNotExistError,

        // libarchive
        LibArchiveReadOpenError,
        LibArchiveReadDataError,
        LibArchiveReadHeaderError,
        LibArchiveWriteOpenError,
        LibArchiveWriteDataError,
        LibArchiveWriteHeaderError,
        LibArchiveCloseError,
        LibArchiveFreeError,

        // XML
        XmlParseFileError,
    };

    static QString errorString(Error error);

private:
    PatcherError();
};

#endif // PATCHERERROR_H
