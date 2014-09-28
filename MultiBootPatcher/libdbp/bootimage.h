#ifndef BOOTIMAGE_H
#define BOOTIMAGE_H

#include "libdbp_global.h"
#include "patchererror.h"

#include <QtCore/QFile>
#include <QtCore/QScopedPointer>

class BootImagePrivate;
struct LokiHeader;

class LIBDBPSHARED_EXPORT BootImage
{
public:
    Q_PROPERTY(QString boardName READ boardName WRITE setBoardName RESET resetBoardName)
    Q_PROPERTY(QString kernelCmdline READ kernelCmdline WRITE setKernelCmdline RESET resetKernelCmdline)
    Q_PROPERTY(uint kernelAddress READ kernelAddress WRITE setKernelAddress RESET resetKernelAddress)
    Q_PROPERTY(uint ramdiskAddress READ ramdiskAddress WRITE setRamdiskAddress RESET resetRamdiskAddress)
    Q_PROPERTY(uint secondBootloaderAddress READ secondBootloaderAddress WRITE setSecondBootloaderAddress RESET resetSecondBootloaderAddress)
    Q_PROPERTY(uint kernelTagsAddress READ kernelTagsAddress WRITE setKernelTagsAddress RESET resetKernelTagsAddress)
    Q_PROPERTY(uint pageSize READ pageSize WRITE setPageSize RESET resetPageSize)

    BootImage();
    ~BootImage();

    PatcherError::Error error() const;
    QString errorString() const;

    bool load(const char * const data, int size);
    bool load(const QByteArray &data);
    bool load(const QString &filename);
    QByteArray create() const;
    bool createFile(const QString &path);
    bool extract(const QString &directory, const QString &prefix);

    QString boardName() const;
    void setBoardName(QString name);
    void resetBoardName();

    QString kernelCmdline() const;
    void setKernelCmdline(QString cmdline);
    void resetKernelCmdline();

    uint pageSize() const;
    void setPageSize(uint size);
    void resetPageSize();

    uint kernelAddress() const;
    void setKernelAddress(uint address);
    void resetKernelAddress();

    uint ramdiskAddress() const;
    void setRamdiskAddress(uint address);
    void resetRamdiskAddress();

    uint secondBootloaderAddress() const;
    void setSecondBootloaderAddress(uint address);
    void resetSecondBootloaderAddress();

    uint kernelTagsAddress() const;
    void setKernelTagsAddress(uint address);
    void resetKernelTagsAddress();

    // Set addresses using a base and offsets
    void setAddresses(uint base, uint kernelOffset, uint ramdiskOffset,
                      uint secondBootloaderOffset, uint kernelTagsOffset);

    // For setting the various images

    QByteArray kernelImage() const;
    void setKernelImage(const QByteArray &data);

    QByteArray ramdiskImage() const;
    void setRamdiskImage(const QByteArray &data);

    QByteArray secondBootloaderImage() const;
    void setSecondBootloaderImage(const QByteArray &data);

    QByteArray deviceTreeImage() const;
    void setDeviceTreeImage(const QByteArray &data);


private:
    bool loadAndroidHeader(const char * const data,
                           int size,
                           const int headerIndex,
                           const bool isLoki);
    bool loadLokiHeader(const char * const data,
                        int size,
                        const int headerIndex,
                        const bool isLoki);
    int lokiFindGzipOffset(const char * const data,
                           int size) const;
    int lokiFindRamdiskSize(const char * const data,
                            int size,
                            const LokiHeader *loki,
                            const int &ramdiskOffset) const;
    int lokiFindKernelSize(const char * const data,
                           const LokiHeader *loki) const;
    uint lokiFindRamdiskAddress(const char * const data,
                                int size,
                                const LokiHeader *loki) const;
    int skipPadding(const int &itemSize,
                    const int &pageSize) const;
    void updateSHA1Hash();

    void dumpHeader() const;

    const QScopedPointer<BootImagePrivate> d_ptr;
    Q_DECLARE_PRIVATE(BootImage)
};

#endif // BOOTIMAGE_H
