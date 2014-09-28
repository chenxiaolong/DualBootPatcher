#include "bootimage.h"
#include "bootimage_p.h"

#include <QtCore/QByteArray>
#include <QtCore/QCryptographicHash>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QStringBuilder>
#include <QtCore/QTextStream>

#include <cstring>


BootImage::BootImage() : d_ptr(new BootImagePrivate())
{
}

BootImage::~BootImage()
{
    // Destructor so d_ptr is destroyed
}

PatcherError::Error BootImage::error() const
{
    Q_D(const BootImage);

    return d->errorCode;
}

QString BootImage::errorString() const
{
    Q_D(const BootImage);

    return d->errorString;
}

bool BootImage::load(const char * const data, int size)
{
    Q_D(BootImage);

    // Check that the size of the boot image is okay
    if (size < (qint64) (512 + sizeof(BootImageHeader))) {
        d->errorCode = PatcherError::BootImageSmallerThanHeaderError;
        d->errorString = PatcherError::errorString(d->errorCode);
        return false;
    }

    const QByteArray expectedLoki =
            QByteArray::fromRawData(LOKI_MAGIC, 4);
    const QByteArray expectedAndroid =
            QByteArray::fromRawData(BOOT_MAGIC, BOOT_MAGIC_SIZE);

    // Find the Loki magic string
    QByteArray lokiMagic = QByteArray::fromRawData(data + 0x400, 4);
    bool isLoki = lokiMagic == expectedLoki;

    // Find the Android magic string
    bool isAndroid = false;
    int headerIndex = -1;

    int searchRange;
    if (isLoki) {
        searchRange = 32;
    } else {
        searchRange = 512;
    }

    for (int i = 0; i <= searchRange; i++) {
        const QByteArray androidMagic = QByteArray::fromRawData(
                data + i, BOOT_MAGIC_SIZE);
        if (androidMagic == expectedAndroid) {
            isAndroid = true;
            headerIndex = i;
            break;
        }
    }

    if (!isAndroid) {
        d->errorCode = PatcherError::BootImageNoAndroidHeaderError;
        d->errorString = PatcherError::errorString(d->errorCode);
        return false;
    }

    bool ret;

    if (isLoki) {
        ret = loadLokiHeader(data, size, headerIndex, isLoki);
    } else {
        ret = loadAndroidHeader(data, size, headerIndex, isLoki);
    }

    return ret;
}

bool BootImage::load(const QByteArray &data)
{
    return load(data.constData(), data.size());
}

bool BootImage::load(const QString &filename)
{
    Q_D(BootImage);

    QFile file(filename);
    uchar *data = nullptr;

    if (!file.open(QFile::ReadOnly)) {
        d->errorCode = PatcherError::FileOpenError;
        d->errorString = PatcherError::errorString(d->errorCode)
                .arg(file.fileName());
        return false;
    }

    // Map the boot image
    data = file.map(0, file.size());

    if (!data) {
        file.close();

        d->errorCode = PatcherError::FileMapError;
        d->errorString = PatcherError::errorString(d->errorCode)
                .arg(file.fileName());
        return false;
    }

    bool ret = load(reinterpret_cast<const char *>(data), file.size());

    file.unmap(data);
    file.close();

    return ret;
}

bool BootImage::loadAndroidHeader(const char * const data,
                                  int size,
                                  const int headerIndex,
                                  const bool isLoki)
{
    Q_D(BootImage);

    // Make sure the file is large enough to contain the header
    if (size < (qint64) (headerIndex + sizeof(BootImageHeader))) {
        d->errorCode = PatcherError::BootImageSmallerThanHeaderError;
        d->errorString = PatcherError::errorString(d->errorCode);
        return false;
    }

    // Read the Android boot image header
    const BootImageHeader *android =
            reinterpret_cast<const BootImageHeader *>(data + headerIndex);

    qDebug() << "Found Android boot image header at" << headerIndex;

    // Save the header struct
    d->header = *android;

    dumpHeader();

    // Don't try to read the various images inside the boot image if it's
    // Loki'd since some offsets and sizes need to be calculated
    if (!isLoki) {
        uint pos = sizeof(BootImageHeader);
        pos += skipPadding(sizeof(BootImageHeader), android->page_size);

        d->kernelImage = QByteArray(data + pos, android->kernel_size);

        pos += android->kernel_size;
        pos += skipPadding(android->kernel_size, android->page_size);

        d->ramdiskImage = QByteArray(data + pos, android->ramdisk_size);

        if (*(data + pos) == 0x02 && *(data + pos + 1) == 0x21) {
            d->ramdiskCompression = BootImagePrivate::LZ4;
        } else {
            d->ramdiskCompression = BootImagePrivate::GZIP;
        }

        pos += android->ramdisk_size;
        pos += skipPadding(android->ramdisk_size, android->page_size);

        // The second bootloader may not exist
        if (android->second_size > 0) {
            d->secondBootloaderImage = QByteArray(data + pos,
                                                  android->second_size);
        } else {
            d->secondBootloaderImage.clear();
        }

        pos += android->second_size;
        pos += skipPadding(android->second_size, android->page_size);

        // The device tree image may not exist as well
        if (android->dt_size > 0) {
            d->deviceTreeImage = QByteArray(data + pos, android->dt_size);
        } else {
            d->deviceTreeImage.clear();
        }
    }

    return true;
}

bool BootImage::loadLokiHeader(const char * const data,
                               int size,
                               const int headerIndex,
                               const bool isLoki)
{
    Q_D(BootImage);

    // Make sure the file is large enough to contain the Loki header
    if (size < (qint64) (0x400 + sizeof(LokiHeader))) {
        d->errorCode = PatcherError::BootImageSmallerThanHeaderError;
        d->errorString = PatcherError::errorString(d->errorCode);
        return false;
    }

    if (!loadAndroidHeader(data, size, headerIndex, isLoki)) {
        // Error code already set
        return false;
    }

    // The kernel tags address is invalid in the loki images
    qDebug("Setting kernel tags address to default: 0x%08x",
           d->header.tags_addr);
    resetKernelTagsAddress();

    const LokiHeader *loki = reinterpret_cast<const LokiHeader *>(data + 0x400);

    qDebug("Found Loki boot image header at 0x%x", 0x400);
    qDebug("- magic:             %.*s", 4, loki->magic);
    qDebug("- recovery:          %u", loki->recovery);
    qDebug("- build:             %.*s", 128, loki->build);
    qDebug("- orig_kernel_size:  %u", loki->orig_kernel_size);
    qDebug("- orig_ramdisk_size: %u", loki->orig_ramdisk_size);
    qDebug("- ramdisk_addr:      0x%08x", loki->ramdisk_addr);

    int gzipOffset = lokiFindGzipOffset(data, size);
    if (gzipOffset < 0) {
        d->errorCode = PatcherError::BootImageNoRamdiskGzipHeaderError;
        d->errorString = PatcherError::errorString(d->errorCode);
        return false;
    }

    int ramdiskSize = lokiFindRamdiskSize(data, size, loki, gzipOffset);
    if (ramdiskSize < 0) {
        d->errorCode = PatcherError::BootImageNoRamdiskSizeError;
        d->errorString = PatcherError::errorString(d->errorCode);
        return false;
    }

    int kernelSize = lokiFindKernelSize(data, loki);
    if (kernelSize < 0) {
        d->errorCode = PatcherError::BootImageNoKernelSizeError;
        d->errorString = PatcherError::errorString(d->errorCode);
        return false;
    }

    uint ramdiskAddr = lokiFindRamdiskAddress(data, size, loki);
    if (ramdiskAddr == 0) {
        d->errorCode = PatcherError::BootImageNoRamdiskAddressError;
        d->errorString = PatcherError::errorString(d->errorCode);
        return false;
    }

    d->header.ramdisk_size = ramdiskSize;
    d->header.kernel_size = kernelSize;
    d->header.ramdisk_addr = ramdiskAddr;

    d->kernelImage = QByteArray(data + d->header.page_size, kernelSize);
    d->ramdiskImage = QByteArray(data + gzipOffset, ramdiskSize);
    d->secondBootloaderImage.clear();
    d->deviceTreeImage.clear();

    return true;
}

int BootImage::lokiFindGzipOffset(const char * const data,
                                  int size) const
{
    // Find the location of the ramdisk inside the boot image
    uint gzipOffset = 0x400 + sizeof(LokiHeader);

    // QByteArray::fromRawData does not copy the data
    const QByteArray gzip1 = QByteArrayLiteral("\x1F\x8B\x08\x00");
    const QByteArray gzip2 = QByteArrayLiteral("\x1F\x8B\x08\x08");
    QByteArray dataArray = QByteArray::fromRawData(
            data + gzipOffset,
            size - gzipOffset);

    QList<uint> offsets;
    QList<bool> timestamps; // True if timestamp is not 0x00000000

    // This is the offset on top of gzipOffset
    int curOffset = -1;
    QByteArray curTimestamp;

    while (true) {
        int newOffset = dataArray.indexOf(gzip1, curOffset + 1);
        if (newOffset == -1) {
            newOffset = dataArray.indexOf(gzip2, curOffset + 1);
        }
        if (newOffset == -1) {
            break;
        }

        curOffset = newOffset;

        qDebug("Found a gzip header at 0x%x", gzipOffset + curOffset);

        // Searching for 1F8B0800 wasn't enough for some boot images. Specifically,
        // ktoonsez's 20140319 kernels had another set of those four bytes before
        // the "real" gzip header. We'll work around that by checking that the
        // timestamp isn't zero (which is technically allowed, but the boot image
        // tools don't do that)
        // http://forum.xda-developers.com/showpost.php?p=51219628&postcount=3767
        curTimestamp = dataArray.mid(curOffset, 4);

        offsets.append(gzipOffset + curOffset);
        timestamps.append(curTimestamp != QByteArrayLiteral("\x00\x00\x00\x00"));
    }

    if (offsets.empty()) {
        qWarning() << "Could not find the ramdisk's gzip header";
        return -1;
    }

    qDebug("Found %d gzip headers", offsets.size());

    // Try gzip offset that is immediately followed by non-null timestamp first
    gzipOffset = 0;
    for (int i = 0; i < offsets.size(); i++) {
        if (timestamps[i]) {
            gzipOffset = offsets[i];
        }
    }

    // Otherwise, just choose the first one
    if (gzipOffset == 0) {
        gzipOffset = offsets[0];
    }

    return gzipOffset;
}

int BootImage::lokiFindRamdiskSize(const char * const data,
                                   int size,
                                   const LokiHeader *loki,
                                   const int &ramdiskOffset) const
{
    Q_D(const BootImage);

    int ramdiskSize = -1;

    // If the boot image was patched with an old version of loki, the ramdisk
    // size is not stored properly. We'll need to guess the size of the archive.
    if (loki->orig_ramdisk_size == 0) {
        // The ramdisk is supposed to be from the gzip header to EOF, but loki needs
        // to store a copy of aboot, so it is put in the last 0x200 bytes of the file.
        ramdiskSize = size - ramdiskOffset - 0x200;
        // For LG kernels:
        // ramdiskSize = size - ramdiskOffset - d->header->page_size;

        // The gzip file is zero padded, so we'll search backwards until we find a
        // non-zero byte
        unsigned int begin = size - 0x200;
        int found = -1;

        if (begin < d->header.page_size) {
            return -1;
        }

        for (unsigned int i = begin; i > begin - d->header.page_size; i--) {
            if (*(data + i) != 0) {
                found = i;
                break;
            }
        }

        if (found == -1) {
            qDebug("Ramdisk size: %d (may include some padding)", ramdiskSize);
        } else {
            ramdiskSize = found - ramdiskOffset;
            qDebug("Ramdisk size: %d (with padding removed)", ramdiskSize);
        }
    } else {
        ramdiskSize = loki->orig_ramdisk_size;
        qDebug("Ramdisk size: %d", ramdiskSize);
    }

    return ramdiskSize;
}

int BootImage::lokiFindKernelSize(const char * const data,
                                  const LokiHeader *loki) const
{
    Q_D(const BootImage);

    int kernelSize = -1;

    // If the boot image was patched with an early version of loki, the original
    // kernel size is not stored in the loki header properly (or in the shellcode).
    // The size is stored in the kernel image's header though, so we'll use that.
    // http://www.simtec.co.uk/products/SWLINUX/files/booting_article.html#d0e309
    if (loki->orig_kernel_size == 0) {
        kernelSize = *(reinterpret_cast<const int *>(
                data + d->header.page_size + 0x2c));
    } else {
        kernelSize = loki->orig_kernel_size;
    }

    qDebug("Kernel size: %d", kernelSize);

    return kernelSize;
}

uint BootImage::lokiFindRamdiskAddress(const char * const data,
                                       int size,
                                       const LokiHeader *loki) const
{
    Q_D(const BootImage);

    // If the boot image was patched with a newer version of loki, find the ramdisk
    // offset in the shell code
    uint ramdiskAddr = 0;

    if (loki->ramdisk_addr != 0) {
        const QByteArray shellCode =
                QByteArray::fromRawData(SHELL_CODE, sizeof(SHELL_CODE));

        for (int i = 0; i < size - shellCode.size(); i++) {
            QByteArray curData = QByteArray::fromRawData(
                    data + i, sizeof(SHELL_CODE));
            if (curData == shellCode) {
                ramdiskAddr = *(reinterpret_cast<const uint *>(
                        data + i + sizeof(SHELL_CODE) - 5));
                break;
            }
        }

        if (ramdiskAddr == 0) {
            qWarning() << "Couldn't determine ramdisk offset";
            return -1;
        }

        qDebug("Original ramdisk address: 0x%08x", ramdiskAddr);
    } else {
        // Otherwise, use the default for jflte
        ramdiskAddr = d->header.kernel_addr - 0x00008000 + 0x02000000;
        qDebug("Default ramdisk address: 0x%08x", ramdiskAddr);
    }

    return ramdiskAddr;
}

QByteArray BootImage::create() const
{
    Q_D(const BootImage);

    QByteArray data;

    // Header
    data.append(reinterpret_cast<const char *>(&d->header),
                sizeof(BootImageHeader));

    // Padding
    QByteArray padding(skipPadding(sizeof(BootImageHeader),
                                   d->header.page_size), 0);
    data.append(padding);

    // Kernel image
    data.append(d->kernelImage);

    // More padding
    padding = QByteArray(skipPadding(d->kernelImage.size(),
                                     d->header.page_size), 0);
    data.append(padding);

    // Ramdisk image
    data.append(d->ramdiskImage);

    // Even more padding
    padding = QByteArray(skipPadding(d->ramdiskImage.size(),
                                     d->header.page_size), 0);
    data.append(padding);

    // Second bootloader image
    if (!d->secondBootloaderImage.isEmpty()) {
        data.append(d->secondBootloaderImage);

        // Enough padding already!
        padding = QByteArray(skipPadding(d->secondBootloaderImage.size(),
                                         d->header.page_size), 0);
        data.append(padding);
    }

    // Device tree image
    if (!d->deviceTreeImage.isEmpty()) {
        data.append(d->deviceTreeImage);

        // Last bit of padding (I hope)
        padding = QByteArray(skipPadding(d->deviceTreeImage.size(),
                                         d->header.page_size), 0);
        data.append(padding);
    }

    return data;
}

bool BootImage::createFile(const QString &path)
{
    Q_D(BootImage);

    QFile file(path);
    if (!file.open(QFile::WriteOnly)) {
        d->errorCode = PatcherError::FileOpenError;
        d->errorString = PatcherError::errorString(d->errorCode);
        return false;
    }

    QByteArray data = create();
    qint64 len = file.write(data);
    if (len < data.size()) {
        file.close();

        d->errorCode = PatcherError::FileWriteError;
        d->errorString = PatcherError::errorString(d->errorCode);
        return false;
    }

    file.close();
    return true;
}

bool BootImage::extract(const QString &directory, const QString &prefix)
{
    Q_D(BootImage);

    QDir dir(directory);
    if (!dir.exists()) {
        d->errorCode = PatcherError::DirectoryNotExistError;
        d->errorString = PatcherError::errorString(d->errorCode);
        return false;
    }

    QString pathPrefix(directory % QLatin1Char('/') % prefix);

    // Write kernel command line
    QFile fileCmdline(pathPrefix % QStringLiteral("-cmdline"));
    fileCmdline.open(QFile::WriteOnly);
    QTextStream streamCmdline(&fileCmdline);
    streamCmdline << QString::fromLocal8Bit(QByteArray(
            reinterpret_cast<const char *>(d->header.cmdline), BOOT_ARGS_SIZE));
    streamCmdline << '\n';
    fileCmdline.close();

    // Write base address on which the offsets are applied
    QFile fileBase(pathPrefix % QStringLiteral("-base"));
    fileBase.open(QFile::WriteOnly);
    QTextStream streamBase(&fileBase);
    streamBase << QStringLiteral("%1").arg(
            d->header.kernel_addr - 0x00008000, 8, 16, QLatin1Char('0'));
    streamBase << '\n';
    fileBase.close();

    // Write ramdisk offset
    QFile fileRamdiskOffset(pathPrefix % QStringLiteral("-ramdisk_offset"));
    fileRamdiskOffset.open(QFile::WriteOnly);
    QTextStream streamRamdiskOffset(&fileRamdiskOffset);
    streamRamdiskOffset << QStringLiteral("%1").arg(
            d->header.ramdisk_addr - d->header.kernel_addr
                    + 0x00008000, 8, 16, QLatin1Char('0'));
    streamRamdiskOffset << '\n';
    fileRamdiskOffset.close();

    // Write second bootloader offset
    if (d->header.second_size > 0) {
        QFile fileSecondOffset(pathPrefix % QStringLiteral("-second_offset"));
        fileSecondOffset.open(QFile::WriteOnly);
        QTextStream streamSecondOffset(&fileSecondOffset);
        streamSecondOffset << QStringLiteral("%1").arg(
                d->header.second_addr - d->header.kernel_addr
                        + 0x00008000, 8, 16, QLatin1Char('0'));
        streamSecondOffset << '\n';
        fileSecondOffset.close();
    }

    // Write kernel tags offset
    if (d->header.tags_addr != 0) {
        QFile fileTagsOffset(pathPrefix % QStringLiteral("-tags_offset"));
        fileTagsOffset.open(QFile::WriteOnly);
        QTextStream streamTagsOffset(&fileTagsOffset);
        streamTagsOffset << QStringLiteral("%1").arg(
                d->header.tags_addr - d->header.kernel_addr
                        + 0x00008000, 8, 16, QLatin1Char('0'));
        streamTagsOffset << '\n';
        fileTagsOffset.close();
    }

    // Write page size
    QFile filePageSize(pathPrefix % QStringLiteral("-pagesize"));
    filePageSize.open(QFile::WriteOnly);
    QTextStream streamPageSize(&filePageSize);
    streamPageSize << d->header.page_size;
    streamPageSize << '\n';
    filePageSize.close();

    // Write kernel image
    QFile fileKernel(pathPrefix % QStringLiteral("-zImage"));
    fileKernel.open(QFile::WriteOnly);
    fileKernel.write(d->kernelImage);
    fileKernel.close();

    // Write ramdisk image
    QString ramdiskFilename;
    if (d->ramdiskCompression == BootImagePrivate::LZ4) {
        ramdiskFilename = pathPrefix % QStringLiteral("-ramdisk.lz4");
    } else {
        ramdiskFilename = pathPrefix % QStringLiteral("-ramdisk.gz");
    }

    QFile fileRamdisk(ramdiskFilename);
    fileRamdisk.open(QFile::WriteOnly);
    fileRamdisk.write(d->ramdiskImage);
    fileRamdisk.close();

    // Write second bootloader image
    if (!d->secondBootloaderImage.isNull()) {
        QFile fileSecond(pathPrefix % QStringLiteral("-second"));
        fileSecond.open(QFile::WriteOnly);
        fileSecond.write(d->secondBootloaderImage);
        fileSecond.close();
    }

    // Write device tree image
    if (!d->deviceTreeImage.isNull()) {
        QFile fileDt(pathPrefix % QStringLiteral("-dt"));
        fileDt.open(QFile::WriteOnly);
        fileDt.write(d->deviceTreeImage);
        fileDt.close();
    }

    return true;
}

int BootImage::skipPadding(const int &itemSize, const int &pageSize) const
{
    uint pageMask = pageSize - 1;

    if ((itemSize & pageMask) == 0) {
        return 0;
    }

    return pageSize - (itemSize & pageMask);
}

void BootImage::updateSHA1Hash()
{
    Q_D(BootImage);

    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(d->kernelImage.constData(), d->kernelImage.size());
    hash.addData(reinterpret_cast<char *>(&d->header.kernel_size),
                 sizeof(d->header.kernel_size));
    hash.addData(d->ramdiskImage.constData(), d->ramdiskImage.size());
    hash.addData(reinterpret_cast<char *>(&d->header.ramdisk_size),
                 sizeof(d->header.ramdisk_size));
    if (!d->secondBootloaderImage.isEmpty()) {
        hash.addData(d->secondBootloaderImage.constData(),
                     d->secondBootloaderImage.size());
    }

    // Bug in AOSP? AOSP's mkbootimg adds the second bootloader size to the SHA1
    // hash even if it's 0
    hash.addData(reinterpret_cast<char *>(&d->header.second_size),
                 sizeof(d->header.second_size));

    if (!d->deviceTreeImage.isEmpty()) {
        hash.addData(d->deviceTreeImage.constData(),
                     d->deviceTreeImage.size());
        hash.addData(reinterpret_cast<char *>(&d->header.dt_size),
                     sizeof(d->header.dt_size));
    }

    QByteArray result = hash.result();
    std::memset(d->header.id, 0, sizeof(d->header.id));
    std::memcpy(d->header.id, result.constData(),
                result.size() > (int) sizeof(d->header.id)
                ? sizeof(d->header.id) : result.size());

    qDebug() << "Computed new ID hash:" << result.toHex();
}

void BootImage::dumpHeader() const
{
    Q_D(const BootImage);

    qDebug("- magic:        %.*s", BOOT_MAGIC_SIZE, d->header.magic);
    qDebug("- kernel_size:  %u",     d->header.kernel_size);
    qDebug("- kernel_addr:  0x%08x", d->header.kernel_addr);
    qDebug("- ramdisk_size: %u",     d->header.ramdisk_size);
    qDebug("- ramdisk_addr: 0x%08x", d->header.ramdisk_addr);
    qDebug("- second_size:  %u",     d->header.second_size);
    qDebug("- second_addr:  0x%08x", d->header.second_addr);
    qDebug("- tags_addr:    0x%08x", d->header.tags_addr);
    qDebug("- page_size:    %u",     d->header.page_size);
    qDebug("- dt_size:      %u",     d->header.dt_size);
    qDebug("- unused:       0x%08x", d->header.unused);
    qDebug("- name:         %.*s", BOOT_NAME_SIZE, d->header.name);
    qDebug("- cmdline:      %.*s", BOOT_ARGS_SIZE, d->header.cmdline);
    //qDebug("- id:           %08x%08x%08x%08x%08x%08x%08x%08x",
    //        d->header.id[0], d->header.id[1], d->header.id[2],
    //        d->header.id[3], d->header.id[4], d->header.id[5],
    //        d->header.id[6], d->header.id[7]);
    QByteArray id = QByteArray::fromRawData(
            reinterpret_cast<const char *>(d->header.id), 40);
    qDebug("- id:           %40s", id.toHex().constData());
}

QString BootImage::boardName() const
{
    Q_D(const BootImage);

    // The string may not be null terminated
    return QString::fromLocal8Bit(QByteArray::fromRawData(
            reinterpret_cast<const char *>(d->header.name), BOOT_NAME_SIZE));
}

void BootImage::setBoardName(QString name)
{
    Q_D(BootImage);

    // -1 for null byte
    qstrcpy(reinterpret_cast<char *>(d->header.name),
            name.leftRef(BOOT_NAME_SIZE - 1).toLocal8Bit().constData());
}

void BootImage::resetBoardName()
{
    Q_D(BootImage);

    qstrcpy(reinterpret_cast<char *>(d->header.name), DEFAULT_BOARD);
}

QString BootImage::kernelCmdline() const
{
    Q_D(const BootImage);

    // The string may not be null terminated
    return QString::fromLocal8Bit(QByteArray::fromRawData(
            reinterpret_cast<const char *>(d->header.cmdline), BOOT_ARGS_SIZE));
}

void BootImage::setKernelCmdline(QString cmdline)
{
    Q_D(BootImage);

    // -1 for null byte
    qstrcpy(reinterpret_cast<char *>(d->header.cmdline),
            cmdline.leftRef(BOOT_ARGS_SIZE - 1).toLocal8Bit().constData());
}

void BootImage::resetKernelCmdline()
{
    Q_D(BootImage);

    qstrcpy(reinterpret_cast<char *>(d->header.cmdline), DEFAULT_CMDLINE);
}

uint BootImage::pageSize() const
{
    Q_D(const BootImage);

    return d->header.page_size;
}

void BootImage::setPageSize(uint size)
{
    Q_D(BootImage);

    // Size should be one of if 2048, 4096, 8192, 16384, 32768, 65536, or 131072
    d->header.page_size = size;
}

void BootImage::resetPageSize()
{
    Q_D(BootImage);

    d->header.page_size = DEFAULT_PAGE_SIZE;
}

uint BootImage::kernelAddress() const
{
    Q_D(const BootImage);

    return d->header.kernel_addr;
}

void BootImage::setKernelAddress(uint address)
{
    Q_D(BootImage);

    d->header.kernel_addr = address;
}

void BootImage::resetKernelAddress()
{
    Q_D(BootImage);

    d->header.kernel_addr = DEFAULT_BASE + DEFAULT_KERNEL_OFFSET;
}

uint BootImage::ramdiskAddress() const
{
    Q_D(const BootImage);

    return d->header.ramdisk_addr;
}

void BootImage::setRamdiskAddress(uint address)
{
    Q_D(BootImage);

    d->header.ramdisk_addr = address;
}

void BootImage::resetRamdiskAddress()
{
    Q_D(BootImage);

    d->header.ramdisk_addr = DEFAULT_BASE + DEFAULT_RAMDISK_OFFSET;
}

uint BootImage::secondBootloaderAddress() const
{
    Q_D(const BootImage);

    return d->header.second_addr;
}

void BootImage::setSecondBootloaderAddress(uint address)
{
    Q_D(BootImage);

    d->header.second_addr = address;
}

void BootImage::resetSecondBootloaderAddress()
{
    Q_D(BootImage);

    d->header.second_addr = DEFAULT_BASE + DEFAULT_SECOND_OFFSET;
}

uint BootImage::kernelTagsAddress() const
{
    Q_D(const BootImage);

    return d->header.tags_addr;
}

void BootImage::setKernelTagsAddress(uint address)
{
    Q_D(BootImage);

    d->header.tags_addr = address;
}

void BootImage::resetKernelTagsAddress()
{
    Q_D(BootImage);

    d->header.tags_addr = DEFAULT_BASE + DEFAULT_TAGS_OFFSET;
}

void BootImage::setAddresses(uint base, uint kernelOffset, uint ramdiskOffset,
                             uint secondBootloaderOffset, uint kernelTagsOffset)
{
    Q_D(BootImage);

    d->header.kernel_addr = base + kernelOffset;
    d->header.ramdisk_addr = base + ramdiskOffset;
    d->header.second_addr = base + secondBootloaderOffset;
    d->header.tags_addr = base + kernelTagsOffset;
}

QByteArray BootImage::kernelImage() const
{
    Q_D(const BootImage);

    return d->kernelImage;
}

void BootImage::setKernelImage(const QByteArray &data)
{
    Q_D(BootImage);

    d->kernelImage = data;
    d->header.kernel_size = data.size();
    updateSHA1Hash();
}

QByteArray BootImage::ramdiskImage() const
{
    Q_D(const BootImage);

    return d->ramdiskImage;
}

void BootImage::setRamdiskImage(const QByteArray &data)
{
    Q_D(BootImage);

    d->ramdiskImage = data;
    d->header.ramdisk_size = data.size();
    updateSHA1Hash();
}

QByteArray BootImage::secondBootloaderImage() const
{
    Q_D(const BootImage);

    return d->secondBootloaderImage;
}

void BootImage::setSecondBootloaderImage(const QByteArray &data)
{
    Q_D(BootImage);

    d->secondBootloaderImage = data;
    d->header.second_size = data.size();
    updateSHA1Hash();
}

QByteArray BootImage::deviceTreeImage() const
{
    Q_D(const BootImage);

    return d->deviceTreeImage;
}

void BootImage::setDeviceTreeImage(const QByteArray &data)
{
    Q_D(BootImage);

    d->deviceTreeImage = data;
    d->header.dt_size = data.size();
    updateSHA1Hash();
}
