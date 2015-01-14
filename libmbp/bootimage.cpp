/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "bootimage.h"

#include <cstring>
#include <fstream>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/format.hpp>
#include <boost/uuid/sha1.hpp>

#include "private/fileutils.h"
#include "private/logging.h"


static const char *BOOT_MAGIC = "ANDROID!";
static const int BOOT_MAGIC_SIZE = 8;
static const int BOOT_NAME_SIZE = 16;
static const int BOOT_ARGS_SIZE = 512;

static const char *DEFAULT_BOARD = "";
static const char *DEFAULT_CMDLINE = "";
static const unsigned int DEFAULT_PAGE_SIZE = 2048u;
static const unsigned int DEFAULT_BASE = 0x10000000u;
static const unsigned int DEFAULT_KERNEL_OFFSET = 0x00008000u;
static const unsigned int DEFAULT_RAMDISK_OFFSET = 0x01000000u;
static const unsigned int DEFAULT_SECOND_OFFSET = 0x00f00000u;
static const unsigned int DEFAULT_TAGS_OFFSET = 0x00000100u;


/*! \cond INTERNAL */
struct BootImageHeader
{
    unsigned char magic[BOOT_MAGIC_SIZE];

    unsigned kernel_size;   /* size in bytes */
    unsigned kernel_addr;   /* physical load addr */

    unsigned ramdisk_size;  /* size in bytes */
    unsigned ramdisk_addr;  /* physical load addr */

    unsigned second_size;   /* size in bytes */
    unsigned second_addr;   /* physical load addr */

    unsigned tags_addr;     /* physical addr for kernel tags */
    unsigned page_size;     /* flash page size we assume */
    unsigned dt_size;       /* device tree in bytes */
    unsigned unused;        /* future expansion: should be 0 */
    unsigned char name[BOOT_NAME_SIZE]; /* asciiz product name */

    unsigned char cmdline[BOOT_ARGS_SIZE];

    unsigned id[8]; /* timestamp / checksum / sha1 / etc */
};
/*! \endcond */


static const char *LOKI_MAGIC = "LOKI";

/*! \cond INTERNAL */
struct LokiHeader {
    unsigned char magic[4]; /* 0x494b4f4c */
    unsigned int recovery;  /* 0 = boot.img, 1 = recovery.img */
    char build[128];        /* Build number */

    unsigned int orig_kernel_size;
    unsigned int orig_ramdisk_size;
    unsigned int ramdisk_addr;
};
/*! \endcond */


// From loki.h in the original source code:
// https://raw.githubusercontent.com/djrbliss/loki/master/loki.h
const char *SHELL_CODE =
        "\xfe\xb5"
        "\x0d\x4d"
        "\xd5\xf8"
        "\x88\x04"
        "\xab\x68"
        "\x98\x42"
        "\x12\xd0"
        "\xd5\xf8"
        "\x90\x64"
        "\x0a\x4c"
        "\xd5\xf8"
        "\x8c\x74"
        "\x07\xf5\x80\x57"
        "\x0f\xce"
        "\x0f\xc4"
        "\x10\x3f"
        "\xfb\xdc"
        "\xd5\xf8"
        "\x88\x04"
        "\x04\x49"
        "\xd5\xf8"
        "\x8c\x24"
        "\xa8\x60"
        "\x69\x61"
        "\x2a\x61"
        "\x00\x20"
        "\xfe\xbd"
        "\xff\xff\xff\xff"
        "\xee\xee\xee\xee";


/*! \cond INTERNAL */
class BootImage::Impl
{
public:
    enum COMPRESSION_TYPES {
        GZIP,
        LZ4
    };

    Impl(BootImage *parent) : m_parent(parent) {}

    // Android boot image header
    BootImageHeader header;

    // Various images stored in the boot image
    std::vector<unsigned char> kernelImage;
    std::vector<unsigned char> ramdiskImage;
    std::vector<unsigned char> secondBootloaderImage;
    std::vector<unsigned char> deviceTreeImage;

    // Whether the ramdisk is LZ4 or gzip compressed
    int ramdiskCompression = GZIP;

    bool isLoki;

    PatcherError error;

    bool loadAndroidHeader(const std::vector<unsigned char> &data,
                           const int headerIndex);
    bool loadLokiHeader(const std::vector< unsigned char> &data,
                        const int headerIndex);
    int lokiFindGzipOffset(const std::vector<unsigned char> &data,
                           const unsigned int startOffset) const;
    int lokiFindRamdiskSize(const std::vector<unsigned char> &data,
                            const LokiHeader *loki,
                            const int &ramdiskOffset) const;
    int lokiFindKernelSize(const std::vector<unsigned char> &data,
                           const LokiHeader *loki) const;
    unsigned int lokiFindRamdiskAddress(const std::vector<unsigned char> &data,
                                        const LokiHeader *loki) const;
    int skipPadding(const int &itemSize,
                    const int &pageSize) const;
    void updateSHA1Hash();

    void dumpHeader() const;

private:
    BootImage *m_parent;
};
/*! \endcond */


/*!
 * \class BootImage
 * \brief Handles the creation and manipulation of Android boot images
 *
 * BootImage provides a complete implementation of the Android boot image spec.
 * BootImage supports plain Android boot images as well as boot images patched
 * with both older and newer versions of Dan Rosenberg's loki tool. However,
 * only plain boot images can be built from this class.
 *
 * The following parameters in the Android header can be changed:
 *
 * - Board name (truncated if length > 16)
 * - Kernel cmdline (truncated if length > 512)
 * - Page size
 * - Kernel address [1]
 * - Ramdisk address [1]
 * - Second bootloader address [1]
 * - Kernel tags address [1]
 * - Kernel size [2]
 * - Ramdisk size [2]
 * - Second bootloader size [2]
 * - Device tree size [2]
 * - SHA1 identifier [3]
 *
 * [1] - Can be set using a base and an offset
 *
 * ]2] - Cannot be manually changed. This is automatically updated when the
 *       corresponding image is set
 *
 * [3] - This is automatically computed when the images within the boot image
 *       are changed
 *
 *
 * If the boot image is patched with loki, the following parameters may be used:
 *
 * - Original kernel size
 * - Original ramdisk size
 * - Ramdisk address
 *
 * However, because some of these parameters were set to zero in early versions
 * of loki, they are sometimes ignored and BootImage will search the file for
 * the location of the kernel image and ramdisk image.
 */

BootImage::BootImage() : m_impl(new Impl(this))
{
}

BootImage::~BootImage()
{
}

/*!
 * \brief Get error information
 *
 * \note The returned PatcherError contains valid information only if an
 *       operation has failed.
 *
 * \return PatcherError containing information about the error
 */
PatcherError BootImage::error() const
{
    return m_impl->error;
}

/*!
 * \brief Load a boot image from binary data
 *
 * This function loads a boot image from a vector containing the binary data.
 * The boot image headers and other images (eg. kernel and ramdisk) will be
 * copied and stored.
 *
 * \warning If the boot image cannot be loaded, do not use the same BootImage
 *          object to load another boot image as it may contain partially
 *          loaded data.
 *
 * \return Whether the boot image was successfully read and parsed.
 */
bool BootImage::load(const std::vector<unsigned char> &data)
{
    // Check that the size of the boot image is okay
    if (data.size() < 512 + sizeof(BootImageHeader)) {
        m_impl->error = PatcherError::createBootImageError(
                MBP::ErrorCode::BootImageSmallerThanHeaderError);
        return false;
    }

    // Find the Loki magic string
    m_impl->isLoki = std::memcmp(&data[0x400], LOKI_MAGIC, 4) == 0;

    // Find the Android magic string
    bool isAndroid = false;
    int headerIndex = -1;

    int searchRange;
    if (m_impl->isLoki) {
        searchRange = 32;
    } else {
        searchRange = 512;
    }

    for (int i = 0; i <= searchRange; ++i) {
        if (std::memcmp(&data[i], BOOT_MAGIC, BOOT_MAGIC_SIZE) == 0) {
            isAndroid = true;
            headerIndex = i;
            break;
        }
    }

    if (!isAndroid) {
        m_impl->error = PatcherError::createBootImageError(
                MBP::ErrorCode::BootImageNoAndroidHeaderError);
        return false;
    }

    bool ret;

    if (m_impl->isLoki) {
        ret = m_impl->loadLokiHeader(data, headerIndex);
    } else {
        ret = m_impl->loadAndroidHeader(data, headerIndex);
    }

    return ret;
}

/*!
 * \brief Load a boot image file
 *
 * This function reads a boot image file and then calls
 * BootImage::load(const std::vector<unsigned char> &)
 *
 * \warning If the boot image cannot be loaded, do not use the same BootImage
 *          object to load another boot image as it may contain partially
 *          loaded data.
 *
 * \sa BootImage::load(const std::vector<unsigned char> &)
 *
 * \return Whether the boot image was successfully read and parsed.
 */
bool BootImage::load(const std::string &filename)
{
    std::vector<unsigned char> data;
    auto ret = FileUtils::readToMemory(filename, &data);
    if (ret.errorCode() != MBP::ErrorCode::NoError) {
        m_impl->error = ret;
        return false;
    }

    return load(data);
}

bool BootImage::Impl::loadAndroidHeader(const std::vector<unsigned char> &data,
                                        const int headerIndex)
{
    // Make sure the file is large enough to contain the header
    if (data.size() < headerIndex + sizeof(BootImageHeader)) {
        error = PatcherError::createBootImageError(
                MBP::ErrorCode::BootImageSmallerThanHeaderError);
        return false;
    }

    // Read the Android boot image header
    auto android = reinterpret_cast<const BootImageHeader *>(&data[headerIndex]);

    Log::log(Log::Debug, "Found Android boot image header at: %d", headerIndex);

    // Save the header struct
    header = *android;

    dumpHeader();

    // Don't try to read the various images inside the boot image if it's
    // Loki'd since some offsets and sizes need to be calculated
    if (!isLoki) {
        unsigned int pos = sizeof(BootImageHeader);
        pos += skipPadding(sizeof(BootImageHeader), android->page_size);

        kernelImage.assign(
                data.begin() + pos,
                data.begin() + pos + android->kernel_size);

        pos += android->kernel_size;
        pos += skipPadding(android->kernel_size, android->page_size);

        ramdiskImage.assign(
                data.begin() + pos,
                data.begin() + pos + android->ramdisk_size);

        if (data[pos] == 0x02 && data[pos + 1] == 0x21) {
            ramdiskCompression = Impl::LZ4;
        } else {
            ramdiskCompression = Impl::GZIP;
        }

        pos += android->ramdisk_size;
        pos += skipPadding(android->ramdisk_size, android->page_size);

        // The second bootloader may not exist
        if (android->second_size > 0) {
            secondBootloaderImage.assign(
                    data.begin() + pos,
                    data.begin() + pos + android->second_size);
        } else {
            secondBootloaderImage.clear();
        }

        pos += android->second_size;
        pos += skipPadding(android->second_size, android->page_size);

        // The device tree image may not exist as well
        if (android->dt_size > 0) {
            deviceTreeImage.assign(
                    data.begin() + pos,
                    data.begin() + pos + android->dt_size);
        } else {
            deviceTreeImage.clear();
        }
    }

    return true;
}

bool BootImage::Impl::loadLokiHeader(const std::vector<unsigned char> &data,
                                     const int headerIndex)
{
    // Make sure the file is large enough to contain the Loki header
    if (data.size() < 0x400 + sizeof(LokiHeader)) {
        error = PatcherError::createBootImageError(
                MBP::ErrorCode::BootImageSmallerThanHeaderError);
        return false;
    }

    if (!loadAndroidHeader(data, headerIndex)) {
        // Error code already set
        return false;
    }

    // The kernel tags address is invalid in the loki images
    Log::log(Log::Debug, "Setting kernel tags address to default: 0x%08x",
             header.tags_addr);
    m_parent->resetKernelTagsAddress();

    const LokiHeader *loki = reinterpret_cast<const LokiHeader *>(&data[0x400]);

    Log::log(Log::Debug, "Found Loki boot image header at 0x%x", 0x400);
    Log::log(Log::Debug, "- magic:             %s",
             std::string(loki->magic, loki->magic + 4));
    Log::log(Log::Debug, "- recovery:          %u", loki->recovery);
    Log::log(Log::Debug, "- build:             %s",
             std::string(loki->build, loki->build + 128));
    Log::log(Log::Debug, "- orig_kernel_size:  %u", loki->orig_kernel_size);
    Log::log(Log::Debug, "- orig_ramdisk_size: %u", loki->orig_ramdisk_size);
    Log::log(Log::Debug, "- ramdisk_addr:      0x%08x", loki->ramdisk_addr);

    int kernelSize = lokiFindKernelSize(data, loki);
    if (kernelSize < 0) {
        error = PatcherError::createBootImageError(
            MBP::ErrorCode::BootImageNoKernelSizeError);
        return false;
    }

    // The ramdisk always comes after the kernel in boot images, so start the
    // search there
    int gzipOffset = lokiFindGzipOffset(data, header.page_size + kernelSize);
    if (gzipOffset < 0) {
        error = PatcherError::createBootImageError(
                MBP::ErrorCode::BootImageNoRamdiskGzipHeaderError);
        return false;
    }

    int ramdiskSize = lokiFindRamdiskSize(data, loki, gzipOffset);
    if (ramdiskSize < 0) {
        error = PatcherError::createBootImageError(
                MBP::ErrorCode::BootImageNoRamdiskSizeError);
        return false;
    }

    unsigned int ramdiskAddr = lokiFindRamdiskAddress(data, loki);
    if (ramdiskAddr == 0) {
        error = PatcherError::createBootImageError(
                MBP::ErrorCode::BootImageNoRamdiskAddressError);
        return false;
    }

    header.ramdisk_size = ramdiskSize;
    header.kernel_size = kernelSize;
    header.ramdisk_addr = ramdiskAddr;

    kernelImage.assign(
            data.begin() + header.page_size,
            data.begin() + header.page_size + kernelSize);
    ramdiskImage.assign(
            data.begin() + gzipOffset,
            data.begin() + gzipOffset + ramdiskSize);
    secondBootloaderImage.clear();
    deviceTreeImage.clear();

    return true;
}

int BootImage::Impl::lokiFindGzipOffset(const std::vector<unsigned char> &data,
                                        const unsigned int startOffset) const
{
    // gzip header:
    // byte 0-1 : magic bytes 0x1f, 0x8b
    // byte 2   : compression (0x08 = deflate)
    // byte 3   : flags
    // byte 4-7 : modification timestamp
    // byte 8   : compression flags
    // byte 9   : operating system

    static const unsigned char gzipDeflate[] = { 0x1f, 0x8b, 0x08 };

    std::vector<unsigned int> offsetsFlag8; // Has original file name
    std::vector<unsigned int> offsetsFlag0; // No flags

    unsigned int curOffset = startOffset - 1;

    while (true) {
        // Try to find first gzip header
        auto it = std::search(data.begin() + curOffset + 1, data.end(),
                              gzipDeflate, gzipDeflate + 3);

        if (it == data.end()) {
            break;
        }

        curOffset = it - data.begin();

        // We're checking 1 more byte so make sure it's within bounds
        if (curOffset + 1 >= data.size()) {
            break;
        }

        if (data[curOffset + 3] == '\x08') {
            Log::log(Log::Debug, "Found a gzip header (flag 0x08) at 0x%x", curOffset);
            offsetsFlag8.push_back(curOffset);
        } else if (data[curOffset + 3] == '\x00') {
            Log::log(Log::Debug, "Found a gzip header (flag 0x00) at 0x%x", curOffset);
            offsetsFlag0.push_back(curOffset);
        } else {
            Log::log(Log::Warning, "Unexpected flag 0x%02x found in gzip header at 0x%x",
                     static_cast<int>(data[curOffset + 3]), curOffset);
            continue;
        }
    }

    Log::log(Log::Debug, "Found %lu total gzip headers",
             offsetsFlag8.size() + offsetsFlag0.size());

    unsigned int gzipOffset = 0;

    // Prefer gzip headers with original filename flag since most loki'd boot
    // images will have probably been compressed manually using the gzip tool
    if (!offsetsFlag8.empty()) {
        gzipOffset = offsetsFlag8[0];
    }

    if (gzipOffset == 0) {
        if (offsetsFlag0.empty()) {
            Log::log(Log::Warning, "Could not find the ramdisk's gzip header");
            return -1;
        } else {
            gzipOffset = offsetsFlag0[0];
        }
    }

    Log::log(Log::Debug, "Using offset 0x%x", gzipOffset);

    return gzipOffset;
}

int BootImage::Impl::lokiFindRamdiskSize(const std::vector<unsigned char> &data,
                                         const LokiHeader *loki,
                                         const int &ramdiskOffset) const
{
    int ramdiskSize = -1;

    // If the boot image was patched with an old version of loki, the ramdisk
    // size is not stored properly. We'll need to guess the size of the archive.
    if (loki->orig_ramdisk_size == 0) {
        // The ramdisk is supposed to be from the gzip header to EOF, but loki needs
        // to store a copy of aboot, so it is put in the last 0x200 bytes of the file.
        ramdiskSize = data.size() - ramdiskOffset - 0x200;
        // For LG kernels:
        // ramdiskSize = data.size() - ramdiskOffset - d->header->page_size;

        // The gzip file is zero padded, so we'll search backwards until we find a
        // non-zero byte
        unsigned int begin = data.size() - 0x200;
        int found = -1;

        if (begin < header.page_size) {
            return -1;
        }

        for (unsigned int i = begin; i > begin - header.page_size; i--) {
            if (data[i] != 0) {
                found = i;
                break;
            }
        }

        if (found == -1) {
            Log::log(Log::Debug, "Ramdisk size: %d (may include some padding)", ramdiskSize);
        } else {
            ramdiskSize = found - ramdiskOffset;
            Log::log(Log::Debug, "Ramdisk size: %d (with padding removed)", ramdiskSize);
        }
    } else {
        ramdiskSize = loki->orig_ramdisk_size;
        Log::log(Log::Debug, "Ramdisk size: %d", ramdiskSize);
    }

    return ramdiskSize;
}

int BootImage::Impl::lokiFindKernelSize(const std::vector<unsigned char> &data,
                                        const LokiHeader *loki) const
{
    int kernelSize = -1;

    // If the boot image was patched with an early version of loki, the original
    // kernel size is not stored in the loki header properly (or in the shellcode).
    // The size is stored in the kernel image's header though, so we'll use that.
    // http://www.simtec.co.uk/products/SWLINUX/files/booting_article.html#d0e309
    if (loki->orig_kernel_size == 0) {
        kernelSize = *(reinterpret_cast<const int *>(
                &data[header.page_size + 0x2c]));
    } else {
        kernelSize = loki->orig_kernel_size;
    }

    Log::log(Log::Debug, "Kernel size: %d", kernelSize);

    return kernelSize;
}

unsigned int BootImage::Impl::lokiFindRamdiskAddress(const std::vector<unsigned char> &data,
                                                     const LokiHeader *loki) const
{
    // If the boot image was patched with a newer version of loki, find the ramdisk
    // offset in the shell code
    unsigned int ramdiskAddr = 0;

    if (loki->ramdisk_addr != 0) {
        auto size = data.size();

        for (unsigned int i = 0; i < size - (sizeof(SHELL_CODE) - 9); ++i) {
            if (std::memcmp(&data[i], SHELL_CODE, sizeof(SHELL_CODE) - 9)) {
                ramdiskAddr = *(reinterpret_cast<const unsigned char *>(
                        &data[i] + sizeof(SHELL_CODE) - 5));
                break;
            }
        }

        if (ramdiskAddr == 0) {
            Log::log(Log::Warning, "Couldn't determine ramdisk offset");
            return -1;
        }

        Log::log(Log::Debug, "Original ramdisk address: 0x%08x", ramdiskAddr);
    } else {
        // Otherwise, use the default for jflte
        ramdiskAddr = header.kernel_addr - 0x00008000 + 0x02000000;
        Log::log(Log::Debug, "Default ramdisk address: 0x%08x", ramdiskAddr);
    }

    return ramdiskAddr;
}

/*!
 * \brief Constructs the boot image binary data
 *
 * This function builds the bootable boot image binary data that the BootImage
 * represents. This is equivalent to AOSP's \a mkbootimg tool.
 *
 * \return Boot image binary data
 */
std::vector<unsigned char> BootImage::create() const
{
    std::vector<unsigned char> data;

    // Header
    unsigned char *headerBegin =
            reinterpret_cast<unsigned char *>(&m_impl->header);
    data.insert(data.end(), headerBegin, headerBegin + sizeof(BootImageHeader));

    // Padding
    unsigned int paddingSize = m_impl->skipPadding(
            sizeof(BootImageHeader), m_impl->header.page_size);
    data.insert(data.end(), paddingSize, 0);

    // Kernel image
    data.insert(data.end(), m_impl->kernelImage.begin(),
                m_impl->kernelImage.end());

    // More padding
    paddingSize = m_impl->skipPadding(
            m_impl->kernelImage.size(), m_impl->header.page_size);
    data.insert(data.end(), paddingSize, 0);

    // Ramdisk image
    data.insert(data.end(), m_impl->ramdiskImage.begin(),
                m_impl->ramdiskImage.end());

    // Even more padding
    paddingSize = m_impl->skipPadding(
            m_impl->ramdiskImage.size(), m_impl->header.page_size);
    data.insert(data.end(), paddingSize, 0);

    // Second bootloader image
    if (!m_impl->secondBootloaderImage.empty()) {
        data.insert(data.end(), m_impl->secondBootloaderImage.begin(),
                    m_impl->secondBootloaderImage.end());

        // Enough padding already!
        paddingSize = m_impl->skipPadding(
                m_impl->secondBootloaderImage.size(), m_impl->header.page_size);
        data.insert(data.end(), paddingSize, 0);
    }

    // Device tree image
    if (!m_impl->deviceTreeImage.empty()) {
        data.insert(data.end(), m_impl->deviceTreeImage.begin(),
                    m_impl->deviceTreeImage.end());

        // Last bit of padding (I hope)
        paddingSize = m_impl->skipPadding(
                m_impl->deviceTreeImage.size(), m_impl->header.page_size);
        data.insert(data.end(), paddingSize, 0);
    }

    return data;
}

/*!
 * \brief Constructs boot image and writes it to a file
 *
 * This is a convenience function that calls BootImage::create() and writes the
 * data to the specified file.
 *
 * \return Whether the file was successfully written
 *
 * \sa BootImage::create()
 */
bool BootImage::createFile(const std::string &path)
{
    std::ofstream file(path, std::ios::binary);
    if (file.fail()) {
        m_impl->error = PatcherError::createIOError(
                MBP::ErrorCode::FileOpenError, path);
        return false;
    }

    std::vector<unsigned char> data = create();
    file.write(reinterpret_cast<const char *>(data.data()), data.size());
    if (file.bad()) {
        file.close();

        m_impl->error = PatcherError::createIOError(
                MBP::ErrorCode::FileWriteError, path);
        return false;
    }

    file.close();
    return true;
}

/*!
 * \brief Extracts boot image header and data to a directory
 *
 * This function extracts and various pieces of header information and the
 * images to a directory. The extracted data is complete and can be used to
 * recreate the boot image.
 *
 * The files are extracted to `[directory]/[prefix]-[file]`, where \a directory
 * and \a prefix are passed as parameters. ("boot.img" is the common prefix.)
 *
 * The following files will be written by this function:
 *
 * - `[prefix]-cmdline` : Kernel command line
 * - `[prefix]-base` : Base address for offsets
 * - `[prefix]-ramdisk_offset` : Address offset of the ramdisk image
 * - `[prefix]-second_offset` : Address offset of the second bootloader image
 *                             (if it exists)
 * - `[prefix]-tags_offset` : Address offset of the kernel tags image
 *                           (if it exists)
 * - `[prefix]-pagesize` : Page size
 * - `[prefix]-zImage` : Kernel image
 * - `[prefix]-ramdisk.gz` : Ramdisk image (if compressed with gzip)
 * - `[prefix]-ramdisk.lz4` : Ramdisk image (if compressed with LZ4)
 * - `[prefix]-second` : Second bootloader image (if it exists)
 * - `[prefix]-dt` : Device tree image (if it exists)
 *
 * \param directory Output directory
 * \param prefix Filename prefix
 *
 * \return Whether extraction was successful
 */
bool BootImage::extract(const std::string &directory, const std::string &prefix)
{
    if (!boost::filesystem::exists(directory)) {
        m_impl->error = PatcherError::createIOError(
                MBP::ErrorCode::DirectoryNotExistError, directory);
        return false;
    }

    std::string pathPrefix(directory);
    pathPrefix += "/";
    pathPrefix += prefix;

    // Write kernel command line
    std::ofstream fileCmdline(pathPrefix + "-cmdline", std::ios::binary);
    fileCmdline << std::string(reinterpret_cast<char *>(
            m_impl->header.cmdline), BOOT_ARGS_SIZE);
    fileCmdline << '\n';
    fileCmdline.close();

    // Write base address on which the offsets are applied
    std::ofstream fileBase(pathPrefix + "-base", std::ios::binary);
    fileBase << boost::format("%08x") % (m_impl->header.kernel_addr - 0x00008000);
    fileBase << '\n';
    fileBase.close();

    // Write ramdisk offset
    std::ofstream fileRamdiskOffset(pathPrefix + "-ramdisk_offset",
                                    std::ios::binary);
    fileRamdiskOffset << boost::format("%08x")
            % (m_impl->header.ramdisk_addr - m_impl->header.kernel_addr
                    + 0x00008000);
    fileRamdiskOffset << '\n';
    fileRamdiskOffset.close();

    // Write second bootloader offset
    if (m_impl->header.second_size > 0) {
        std::ofstream fileSecondOffset(pathPrefix + "-second_offset",
                                       std::ios::binary);
        fileSecondOffset << boost::format("%08x")
                % (m_impl->header.second_addr - m_impl->header.kernel_addr
                        + 0x00008000);
        fileSecondOffset << '\n';
        fileSecondOffset.close();
    }

    // Write kernel tags offset
    if (m_impl->header.tags_addr != 0) {
        std::ofstream fileTagsOffset(pathPrefix + "-tags_offset",
                                     std::ios::binary);
        fileTagsOffset << boost::format("%08x")
                % (m_impl->header.tags_addr - m_impl->header.kernel_addr
                        + 0x00008000);
        fileTagsOffset << '\n';
        fileTagsOffset.close();
    }

    // Write page size
    std::ofstream filePageSize(pathPrefix + "-pagesize", std::ios::binary);
    filePageSize << m_impl->header.page_size;
    filePageSize << '\n';
    filePageSize.close();

    // Write kernel image
    std::ofstream fileKernel(pathPrefix + "-zImage", std::ios::binary);
    fileKernel.write(reinterpret_cast<char *>(m_impl->kernelImage.data()),
                     m_impl->kernelImage.size());
    fileKernel.close();

    // Write ramdisk image
    auto ramdiskFilename = pathPrefix;
    if (m_impl->ramdiskCompression == Impl::LZ4) {
        ramdiskFilename += "-ramdisk.lz4";
    } else {
        ramdiskFilename += "-ramdisk.gz";
    }

    std::ofstream fileRamdisk(ramdiskFilename, std::ios::binary);
    fileRamdisk.write(reinterpret_cast<char *>(m_impl->ramdiskImage.data()),
                      m_impl->ramdiskImage.size());
    fileRamdisk.close();

    // Write second bootloader image
    if (!m_impl->secondBootloaderImage.empty()) {
        std::ofstream fileSecond(pathPrefix + "-second", std::ios::binary);
        fileSecond.write(reinterpret_cast<char *>(m_impl->secondBootloaderImage.data()),
                         m_impl->secondBootloaderImage.size());
        fileSecond.close();
    }

    // Write device tree image
    if (!m_impl->deviceTreeImage.empty()) {
        std::ofstream fileDt(pathPrefix + "-dt", std::ios::binary);
        fileDt.write(reinterpret_cast<char *>(m_impl->deviceTreeImage.data()),
                     m_impl->deviceTreeImage.size());
        fileDt.close();
    }

    return true;
}

bool BootImage::isLoki() const
{
    return m_impl->isLoki;
}

int BootImage::Impl::skipPadding(const int &itemSize, const int &pageSize) const
{
    unsigned int pageMask = pageSize - 1;

    if ((itemSize & pageMask) == 0) {
        return 0;
    }

    return pageSize - (itemSize & pageMask);
}

static std::string toHex(const unsigned char *data, unsigned int size) {
    static const char digits[] = "0123456789abcdef";

    std::string hex;
    hex.reserve(2 * sizeof(size));
    for (unsigned int i = 0; i < size; ++i) {
        hex += digits[(data[i] >> 4) & 0xf];
        hex += digits[data[i] & 0xF];
    }

    return hex;
}

void BootImage::Impl::updateSHA1Hash()
{
    boost::uuids::detail::sha1 hash;
    hash.process_bytes(kernelImage.data(), kernelImage.size());
    hash.process_bytes(reinterpret_cast<char *>(&header.kernel_size),
                       sizeof(header.kernel_size));
    hash.process_bytes(ramdiskImage.data(), ramdiskImage.size());
    hash.process_bytes(reinterpret_cast<char *>(&header.ramdisk_size),
                       sizeof(header.ramdisk_size));
    if (!secondBootloaderImage.empty()) {
        hash.process_bytes(secondBootloaderImage.data(),
                           secondBootloaderImage.size());
    }

    // Bug in AOSP? AOSP's mkbootimg adds the second bootloader size to the SHA1
    // hash even if it's 0
    hash.process_bytes(reinterpret_cast<char *>(&header.second_size),
                       sizeof(header.second_size));

    if (!deviceTreeImage.empty()) {
        hash.process_bytes(deviceTreeImage.data(), deviceTreeImage.size());
        hash.process_bytes(reinterpret_cast<char *>(&header.dt_size),
                           sizeof(header.dt_size));
    }

    unsigned int digest[5];
    hash.get_digest(digest);

    std::memset(header.id, 0, sizeof(header.id));
    std::memcpy(header.id, reinterpret_cast<char *>(&digest),
                sizeof(digest) > sizeof(header.id)
                ? sizeof(header.id) : sizeof(digest));

    // Debug...
    std::string hexDigest = toHex(
            reinterpret_cast<const unsigned char *>(digest), sizeof(digest));

    Log::log(Log::Debug, "Computed new ID hash: %s", hexDigest);
}

void BootImage::Impl::dumpHeader() const
{
    Log::log(Log::Debug, "- magic:        %s",
             std::string(header.magic, header.magic + BOOT_MAGIC_SIZE));
    Log::log(Log::Debug, "- kernel_size:  %u",     header.kernel_size);
    Log::log(Log::Debug, "- kernel_addr:  0x%08x", header.kernel_addr);
    Log::log(Log::Debug, "- ramdisk_size: %u",     header.ramdisk_size);
    Log::log(Log::Debug, "- ramdisk_addr: 0x%08x", header.ramdisk_addr);
    Log::log(Log::Debug, "- second_size:  %u",     header.second_size);
    Log::log(Log::Debug, "- second_addr:  0x%08x", header.second_addr);
    Log::log(Log::Debug, "- tags_addr:    0x%08x", header.tags_addr);
    Log::log(Log::Debug, "- page_size:    %u",     header.page_size);
    Log::log(Log::Debug, "- dt_size:      %u",     header.dt_size);
    Log::log(Log::Debug, "- unused:       0x%08x", header.unused);
    Log::log(Log::Debug, "- name:         %s",
             std::string(header.name, header.name + BOOT_NAME_SIZE));
    Log::log(Log::Debug, "- cmdline:      %s",
             std::string(header.cmdline, header.cmdline + BOOT_ARGS_SIZE));
    //Log::log(Log::Debug, "- id:           %08x%08x%08x%08x%08x%08x%08x%08x",
    //         header.id[0], header.id[1], header.id[2],
    //         header.id[3], header.id[4], header.id[5],
    //         header.id[6], header.id[7]);
    Log::log(Log::Debug, "- id:           %s",
             toHex(reinterpret_cast<const unsigned char *>(header.id), 40));
}

/*!
 * \brief Board name field in the boot image header
 *
 * \return Board name
 */
std::string BootImage::boardName() const
{
    // The string may not be null terminated
    void *location;
    if ((location = std::memchr(m_impl->header.name, 0, BOOT_NAME_SIZE)) != nullptr) {
        return std::string(reinterpret_cast<char *>(m_impl->header.name),
                           reinterpret_cast<char *>(location));
    } else {
        return std::string(reinterpret_cast<char *>(m_impl->header.name),
                           BOOT_NAME_SIZE);
    }
}

/*!
 * \brief Set the board name field in the boot image header
 *
 * \param name Board name
 */
void BootImage::setBoardName(const std::string &name)
{
    // -1 for null byte
    std::strcpy(reinterpret_cast<char *>(m_impl->header.name),
                name.substr(0, BOOT_NAME_SIZE - 1).c_str());
}

/*!
 * \brief Resets the board name field in the boot image header to the default
 *
 * The board name field is empty by default.
 */
void BootImage::resetBoardName()
{
    std::strcpy(reinterpret_cast<char *>(m_impl->header.name), DEFAULT_BOARD);
}

/*!
 * \brief Kernel cmdline in the boot image header
 *
 * \return Kernel cmdline
 */
std::string BootImage::kernelCmdline() const
{
    // The string may not be null terminated
    void *location;
    if ((location = std::memchr(m_impl->header.cmdline, 0, BOOT_ARGS_SIZE)) != nullptr) {
        return std::string(reinterpret_cast<char *>(m_impl->header.cmdline),
                           reinterpret_cast<char *>(location));
    } else {
        return std::string(reinterpret_cast<char *>(m_impl->header.cmdline),
                           BOOT_ARGS_SIZE);
    }
}

/*!
 * \brief Set the kernel cmdline in the boot image header
 *
 * \param cmdline Kernel cmdline
 */
void BootImage::setKernelCmdline(const std::string &cmdline)
{
    // -1 for null byte
    std::strcpy(reinterpret_cast<char *>(m_impl->header.cmdline),
                cmdline.substr(0, BOOT_ARGS_SIZE - 1).c_str());
}

/*!
 * \brief Resets the kernel cmdline to the default
 *
 * The kernel cmdline is empty by default.
 */
void BootImage::resetKernelCmdline()
{
    std::strcpy(reinterpret_cast<char *>(m_impl->header.cmdline),
                DEFAULT_CMDLINE);
}

/*!
 * \brief Page size field in the boot image header
 *
 * \return Page size
 */
unsigned int BootImage::pageSize() const
{
    return m_impl->header.page_size;
}

/*!
 * \brief Set the page size field in the boot image header
 *
 * \note The page size should be one of if 2048, 4096, 8192, 16384, 32768,
 *       65536, or 131072
 *
 * \param size Page size
 */
void BootImage::setPageSize(unsigned int size)
{
    // Size should be one of if 2048, 4096, 8192, 16384, 32768, 65536, or 131072
    m_impl->header.page_size = size;
}

/*!
 * \brief Resets the page size field in the header to the default
 *
 * The default page size is 2048 bytes.
 */
void BootImage::resetPageSize()
{
    m_impl->header.page_size = DEFAULT_PAGE_SIZE;
}

/*!
 * \brief Kernel address field in the boot image header
 *
 * \return Kernel address
 */
unsigned int BootImage::kernelAddress() const
{
    return m_impl->header.kernel_addr;
}

/*!
 * \brief Set the kernel address field in the boot image header
 *
 * \param address Kernel address
 */
void BootImage::setKernelAddress(unsigned int address)
{
    m_impl->header.kernel_addr = address;
}

/*!
 * \brief Resets the kernel address field in the header to the default
 *
 * The default kernel address is 0x10000000 + 0x00008000.
 */
void BootImage::resetKernelAddress()
{
    m_impl->header.kernel_addr = DEFAULT_BASE + DEFAULT_KERNEL_OFFSET;
}

/*!
 * \brief Ramdisk address field in the boot image header
 *
 * \return Ramdisk address
 */
unsigned int BootImage::ramdiskAddress() const
{
    return m_impl->header.ramdisk_addr;
}

/*!
 * \brief Set the ramdisk address field in the boot image header
 *
 * \param address Ramdisk address
 */
void BootImage::setRamdiskAddress(unsigned int address)
{
    m_impl->header.ramdisk_addr = address;
}

/*!
 * \brief Resets the ramdisk address field in the header to the default
 *
 * The default ramdisk address is 0x10000000 + 0x01000000.
 */
void BootImage::resetRamdiskAddress()
{
    m_impl->header.ramdisk_addr = DEFAULT_BASE + DEFAULT_RAMDISK_OFFSET;
}

/*!
 * \brief Second bootloader address field in the boot image header
 *
 * \return Second bootloader address
 */
unsigned int BootImage::secondBootloaderAddress() const
{
    return m_impl->header.second_addr;
}

/*!
 * \brief Set the second bootloader address field in the boot image header
 *
 * \param address Second bootloader address
 */
void BootImage::setSecondBootloaderAddress(unsigned int address)
{
    m_impl->header.second_addr = address;
}

/*!
 * \brief Resets the second bootloader address field in the header to the default
 *
 * The default second bootloader address is 0x10000000 + 0x00f00000.
 */
void BootImage::resetSecondBootloaderAddress()
{
    m_impl->header.second_addr = DEFAULT_BASE + DEFAULT_SECOND_OFFSET;
}

/*!
 * \brief Kernel tags address field in the boot image header
 *
 * \return Kernel tags address
 */
unsigned int BootImage::kernelTagsAddress() const
{
    return m_impl->header.tags_addr;
}

/*!
 * \brief Set the kernel tags address field in the boot image header
 *
 * \param address Kernel tags address
 */
void BootImage::setKernelTagsAddress(unsigned int address)
{
    m_impl->header.tags_addr = address;
}

/*!
 * \brief Resets the kernel tags address field in the header to the default
 *
 * The default kernel tags address is 0x10000000 + 0x00000100.
 */
void BootImage::resetKernelTagsAddress()
{
    m_impl->header.tags_addr = DEFAULT_BASE + DEFAULT_TAGS_OFFSET;
}

/*!
 * \brief Set all of the addresses using offsets and a base address
 *
 * - `[Kernel address] = [Base] + [Kernel offset]`
 * - `[Ramdisk address] = [Base] + [Ramdisk offset]`
 * - `[Second bootloader address] = [Base] + [Second bootloader offset]`
 * - `[Kernel tags address] = [Base] + [Kernel tags offset]`
 *
 * \param base Base address
 * \param kernelOffset Kernel offset
 * \param ramdiskOffset Ramdisk offset
 * \param secondBootloaderOffset Second bootloader offset
 * \param kernelTagsOffset Kernel tags offset
 */
void BootImage::setAddresses(unsigned int base, unsigned int kernelOffset,
                             unsigned int ramdiskOffset,
                             unsigned int secondBootloaderOffset,
                             unsigned int kernelTagsOffset)
{
    m_impl->header.kernel_addr = base + kernelOffset;
    m_impl->header.ramdisk_addr = base + ramdiskOffset;
    m_impl->header.second_addr = base + secondBootloaderOffset;
    m_impl->header.tags_addr = base + kernelTagsOffset;
}

/*!
 * \brief Kernel image
 *
 * \return Vector containing the kernel image binary data
 */
std::vector<unsigned char> BootImage::kernelImage() const
{
    return m_impl->kernelImage;
}

/*!
 * \brief Set the kernel image
 *
 * This will automatically update the kernel size in the boot image header and
 * recalculate the SHA1 hash.
 */
void BootImage::setKernelImage(std::vector<unsigned char> data)
{
    m_impl->header.kernel_size = data.size();
    m_impl->kernelImage = std::move(data);
    m_impl->updateSHA1Hash();
}

/*!
 * \brief Ramdisk image
 *
 * \return Vector containing the ramdisk image binary data
 */
std::vector<unsigned char> BootImage::ramdiskImage() const
{
    return m_impl->ramdiskImage;
}

/*!
 * \brief Set the ramdisk image
 *
 * This will automatically update the ramdisk size in the boot image header and
 * recalculate the SHA1 hash.
 */
void BootImage::setRamdiskImage(std::vector<unsigned char> data)
{
    m_impl->header.ramdisk_size = data.size();
    m_impl->ramdiskImage = std::move(data);
    m_impl->updateSHA1Hash();
}

/*!
 * \brief Second bootloader image
 *
 * \return Vector containing the second bootloader image binary data
 */
std::vector<unsigned char> BootImage::secondBootloaderImage() const
{
    return m_impl->secondBootloaderImage;
}

/*!
 * \brief Set the second bootloader image
 *
 * This will automatically update the second bootloader size in the boot image
 * header and recalculate the SHA1 hash.
 */
void BootImage::setSecondBootloaderImage(std::vector<unsigned char> data)
{
    m_impl->header.second_size = data.size();
    m_impl->secondBootloaderImage = std::move(data);
    m_impl->updateSHA1Hash();
}

/*!
 * \brief Device tree image
 *
 * \return Vector containing the device tree image binary data
 */
std::vector<unsigned char> BootImage::deviceTreeImage() const
{
    return m_impl->deviceTreeImage;
}

/*!
 * \brief Set the device tree image
 *
 * This will automatically update the device tree size in the boot image
 * header and recalculate the SHA1 hash.
 */
void BootImage::setDeviceTreeImage(std::vector<unsigned char> data)
{
    m_impl->header.dt_size = data.size();
    m_impl->deviceTreeImage = std::move(data);
    m_impl->updateSHA1Hash();
}
