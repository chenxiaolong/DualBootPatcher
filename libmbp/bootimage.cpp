/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
#include <boost/uuid/sha1.hpp>

#include <cppformat/format.h>

#include "private/fileutils.h"
#include "private/logging.h"


namespace mbp
{

static const char *BOOT_MAGIC = "ANDROID!";
static const uint32_t BOOT_MAGIC_SIZE = 8;
static const uint32_t BOOT_NAME_SIZE = 16;
static const uint32_t BOOT_ARGS_SIZE = 512;

static const char *DEFAULT_BOARD = "";
static const char *DEFAULT_CMDLINE = "";
static const uint32_t DEFAULT_PAGE_SIZE = 2048u;
static const uint32_t DEFAULT_BASE = 0x10000000u;
static const uint32_t DEFAULT_KERNEL_OFFSET = 0x00008000u;
static const uint32_t DEFAULT_RAMDISK_OFFSET = 0x01000000u;
static const uint32_t DEFAULT_SECOND_OFFSET = 0x00f00000u;
static const uint32_t DEFAULT_TAGS_OFFSET = 0x00000100u;


/*! \cond INTERNAL */
struct BootImageHeader
{
    unsigned char magic[BOOT_MAGIC_SIZE];

    uint32_t kernel_size;   /* size in bytes */
    uint32_t kernel_addr;   /* physical load addr */

    uint32_t ramdisk_size;  /* size in bytes */
    uint32_t ramdisk_addr;  /* physical load addr */

    uint32_t second_size;   /* size in bytes */
    uint32_t second_addr;   /* physical load addr */

    uint32_t tags_addr;     /* physical addr for kernel tags */
    uint32_t page_size;     /* flash page size we assume */
    uint32_t dt_size;       /* device tree in bytes */
    uint32_t unused;        /* future expansion: should be 0 */
    unsigned char name[BOOT_NAME_SIZE]; /* asciiz product name */

    unsigned char cmdline[BOOT_ARGS_SIZE];

    uint32_t id[8]; /* timestamp / checksum / sha1 / etc */
};
/*! \endcond */


static const char *LOKI_MAGIC = "LOKI";

/*! \cond INTERNAL */
struct LokiHeader {
    unsigned char magic[4]; /* 0x494b4f4c */
    uint32_t recovery;      /* 0 = boot.img, 1 = recovery.img */
    char build[128];        /* Build number */

    uint32_t orig_kernel_size;
    uint32_t orig_ramdisk_size;
    uint32_t ramdisk_addr;
};
/*! \endcond */


// From loki.h in the original source code:
// https://raw.githubusercontent.com/djrbliss/loki/master/loki.h
static const unsigned char SHELL_CODE[] =
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
    Impl(BootImage *parent) : m_parent(parent) {}

    // Android boot image header
    BootImageHeader header;

    // Various images stored in the boot image
    std::vector<unsigned char> kernelImage;
    std::vector<unsigned char> ramdiskImage;
    std::vector<unsigned char> secondBootloaderImage;
    std::vector<unsigned char> deviceTreeImage;

    bool isLoki;

    PatcherError error;

    bool loadAndroidHeader(const std::vector<unsigned char> &data,
                           const uint32_t headerIndex);
    bool loadLokiHeader(const std::vector<unsigned char> &data,
                        const uint32_t headerIndex);
    bool loadLokiNewImage(const std::vector<unsigned char> &data,
                          const LokiHeader *loki);
    bool loadLokiOldImage(const std::vector<unsigned char> &data,
                          const LokiHeader *loki);
    uint32_t lokiOldFindGzipOffset(const std::vector<unsigned char> &data,
                                   const uint32_t startOffset) const;
    uint32_t lokiOldFindRamdiskSize(const std::vector<unsigned char> &data,
                                    const uint32_t ramdiskOffset) const;
    uint32_t lokiFindRamdiskAddress(const std::vector<unsigned char> &data,
                                    const LokiHeader *loki) const;
    uint32_t skipPadding(const uint32_t itemSize,
                         const uint32_t pageSize) const;
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
        LOGE("The boot image is smaller than the boot image header!");
        m_impl->error = PatcherError::createBootImageError(
                ErrorCode::BootImageParseError);
        return false;
    }

    // Find the Loki magic string
    m_impl->isLoki = std::memcmp(&data[0x400], LOKI_MAGIC, 4) == 0;

    // Find the Android magic string
    bool isAndroid = false;
    uint32_t headerIndex;

    uint32_t searchRange;
    if (m_impl->isLoki) {
        searchRange = 32;
    } else {
        searchRange = 512;
    }

    for (uint32_t i = 0; i <= searchRange; ++i) {
        if (std::memcmp(&data[i], BOOT_MAGIC, BOOT_MAGIC_SIZE) == 0) {
            isAndroid = true;
            headerIndex = i;
            break;
        }
    }

    if (!isAndroid) {
        LOGE("The boot image does not contain an boot image header");
        m_impl->error = PatcherError::createBootImageError(
                ErrorCode::BootImageParseError);
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
    if (!ret) {
        m_impl->error = ret;
        return false;
    }

    return load(data);
}

bool BootImage::Impl::loadAndroidHeader(const std::vector<unsigned char> &data,
                                        const uint32_t headerIndex)
{
    // Make sure the file is large enough to contain the header
    if (data.size() < headerIndex + sizeof(BootImageHeader)) {
        LOGE("The boot image is smaller than the boot image header!");
        error = PatcherError::createBootImageError(
                ErrorCode::BootImageParseError);
        return false;
    }

    // Read the Android boot image header
    auto android = reinterpret_cast<const BootImageHeader *>(&data[headerIndex]);

    FLOGD("Found Android boot image header at: {:d}", headerIndex);

    // Save the header struct
    header = *android;

    switch (android->page_size) {
    case 2048:
    case 4096:
    case 8192:
    case 16384:
    case 32768:
    case 65536:
    case 131072:
        break;
    default:
        FLOGE("Invalid page size: {:d}", android->page_size);
        error = PatcherError::createBootImageError(
                ErrorCode::BootImageParseError);
        return false;
    }

    dumpHeader();

    // Don't try to read the various images inside the boot image if it's
    // Loki'd since some offsets and sizes need to be calculated
    if (!isLoki) {
        uint32_t pos = sizeof(BootImageHeader);
        pos += skipPadding(sizeof(BootImageHeader), android->page_size);

        kernelImage.assign(
                data.begin() + pos,
                data.begin() + pos + android->kernel_size);

        pos += android->kernel_size;
        pos += skipPadding(android->kernel_size, android->page_size);

        ramdiskImage.assign(
                data.begin() + pos,
                data.begin() + pos + android->ramdisk_size);

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
                                     const uint32_t headerIndex)
{
    // Make sure the file is large enough to contain the Loki header
    if (data.size() < 0x400 + sizeof(LokiHeader)) {
        LOGE("The boot image is smaller than the loki header!");
        error = PatcherError::createBootImageError(
                ErrorCode::BootImageParseError);
        return false;
    }

    if (!loadAndroidHeader(data, headerIndex)) {
        // Error code already set
        return false;
    }

    const LokiHeader *loki = reinterpret_cast<const LokiHeader *>(&data[0x400]);

    FLOGD("Found Loki boot image header at {:#x}", 0x400);
    FLOGD("- magic:             {:s}", std::string(loki->magic, loki->magic + 4));
    FLOGD("- recovery:          {:d}", loki->recovery);
    FLOGD("- build:             {:s}", std::string(loki->build, loki->build + 128));
    FLOGD("- orig_kernel_size:  {:d}", loki->orig_kernel_size);
    FLOGD("- orig_ramdisk_size: {:d}", loki->orig_ramdisk_size);
    FLOGD("- ramdisk_addr:      {:#08x}", loki->ramdisk_addr);

    if (loki->orig_kernel_size != 0
            && loki->orig_ramdisk_size != 0
            && loki->ramdisk_addr != 0) {
        return loadLokiNewImage(data, loki);
    } else {
        return loadLokiOldImage(data, loki);
    }
}

bool BootImage::Impl::loadLokiNewImage(const std::vector<unsigned char> &data,
                                       const LokiHeader *loki)
{
    LOGD("This is a new loki image");

    uint32_t pageMask = header.page_size - 1;
    uint32_t fakeSize;

    // From loki_unlok.c
    if (header.ramdisk_addr > 0x88f00000 || header.ramdisk_addr < 0xfa00000) {
        fakeSize = header.page_size;
    } else {
        fakeSize = 0x200;
    }

    // Find original ramdisk address
    uint32_t ramdiskAddr = lokiFindRamdiskAddress(data, loki);
    if (ramdiskAddr == 0) {
        LOGE("Could not find ramdisk address in new loki boot image");
        error = PatcherError::createBootImageError(
                ErrorCode::BootImageParseError);
        return false;
    }

    // Restore original values in boot image header
    header.ramdisk_size = loki->orig_ramdisk_size;
    header.kernel_size = loki->orig_kernel_size;
    header.ramdisk_addr = ramdiskAddr;

    uint32_t pageKernelSize = (loki->orig_kernel_size + pageMask) & ~pageMask;
    uint32_t pageRamdiskSize = (loki->orig_ramdisk_size + pageMask) & ~pageMask;

    // Kernel image
    kernelImage.assign(
            data.begin() + header.page_size,
            data.begin() + header.page_size + loki->orig_kernel_size);

    // Ramdisk image
    ramdiskImage.assign(
            data.begin() + header.page_size + pageKernelSize,
            data.begin() + header.page_size + pageKernelSize + loki->orig_ramdisk_size);

    // No second bootloader image
    secondBootloaderImage.clear();

    // Possible device tree image
    if (header.dt_size != 0) {
        auto startPtr = data.begin() + header.page_size
                + pageKernelSize + pageRamdiskSize + fakeSize;
        deviceTreeImage.assign(startPtr, startPtr + header.dt_size);
    } else {
        deviceTreeImage.clear();
    }

    return true;
}

bool BootImage::Impl::loadLokiOldImage(const std::vector<unsigned char> &data,
                                       const LokiHeader *loki)
{
    LOGD("This is an old loki image");

    // The kernel tags address is invalid in the old loki images
    m_parent->resetKernelTagsAddress();
    FLOGD("Setting kernel tags address to default: {:#08x}", header.tags_addr);

    uint32_t kernelSize;
    uint32_t ramdiskSize;
    uint32_t ramdiskAddr;

    // If the boot image was patched with an early version of loki, the original
    // kernel size is not stored in the loki header properly (or in the shellcode).
    // The size is stored in the kernel image's header though, so we'll use that.
    // http://www.simtec.co.uk/products/SWLINUX/files/booting_article.html#d0e309
    kernelSize = *(reinterpret_cast<const int32_t *>(
            &data[header.page_size + 0x2c]));
    FLOGD("Kernel size: {:d}", kernelSize);


    // The ramdisk always comes after the kernel in boot images, so start the
    // search there
    uint32_t gzipOffset = lokiOldFindGzipOffset(
            data, header.page_size + kernelSize);
    if (gzipOffset == 0) {
        LOGE("Could not find gzip offset in old loki boot image");
        error = PatcherError::createBootImageError(
                ErrorCode::BootImageParseError);
        return false;
    }

    ramdiskSize = lokiOldFindRamdiskSize(data, gzipOffset);

    ramdiskAddr = lokiFindRamdiskAddress(data, loki);
    if (ramdiskAddr == 0) {
        LOGE("Could not find ramdisk address in old loki boot image");
        error = PatcherError::createBootImageError(
                ErrorCode::BootImageParseError);
        return false;
    }

    header.ramdisk_size = ramdiskSize;
    header.kernel_size = kernelSize;
    header.ramdisk_addr = ramdiskAddr;

    // Kernel image
    kernelImage.assign(
            data.begin() + header.page_size,
            data.begin() + header.page_size + kernelSize);

    // Ramdisk image
    ramdiskImage.assign(
            data.begin() + gzipOffset,
            data.begin() + gzipOffset + ramdiskSize);

    // No second bootloader image
    secondBootloaderImage.clear();

    // No device tree image
    deviceTreeImage.clear();

    return true;
}

uint32_t BootImage::Impl::lokiOldFindGzipOffset(const std::vector<unsigned char> &data,
                                                const uint32_t startOffset) const
{
    // gzip header:
    // byte 0-1 : magic bytes 0x1f, 0x8b
    // byte 2   : compression (0x08 = deflate)
    // byte 3   : flags
    // byte 4-7 : modification timestamp
    // byte 8   : compression flags
    // byte 9   : operating system

    static const unsigned char gzipDeflate[] = { 0x1f, 0x8b, 0x08 };

    std::vector<uint32_t> offsetsFlag8; // Has original file name
    std::vector<uint32_t> offsetsFlag0; // No flags

    uint32_t curOffset = startOffset - 1;

    while (true) {
        // Try to find gzip header
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
            FLOGD("Found a gzip header (flag 0x08) at {:#x}", curOffset);
            offsetsFlag8.push_back(curOffset);
        } else if (data[curOffset + 3] == '\x00') {
            FLOGD("Found a gzip header (flag 0x00) at {:#x}", curOffset);
            offsetsFlag0.push_back(curOffset);
        } else {
            FLOGW("Unexpected flag {:#02x} found in gzip header at {:#x}",
                  static_cast<int32_t>(data[curOffset + 3]), curOffset);
            continue;
        }
    }

    FLOGD("Found {:d} total gzip headers",
          offsetsFlag8.size() + offsetsFlag0.size());

    uint32_t gzipOffset = 0;

    // Prefer gzip headers with original filename flag since most loki'd boot
    // images will have probably been compressed manually using the gzip tool
    if (!offsetsFlag8.empty()) {
        gzipOffset = offsetsFlag8[0];
    }

    if (gzipOffset == 0) {
        if (offsetsFlag0.empty()) {
            LOGW("Could not find the ramdisk's gzip header");
            return 0;
        } else {
            gzipOffset = offsetsFlag0[0];
        }
    }

    FLOGD("Using offset {:#x}", gzipOffset);

    return gzipOffset;
}

uint32_t BootImage::Impl::lokiOldFindRamdiskSize(const std::vector<unsigned char> &data,
                                                 const uint32_t ramdiskOffset) const
{
    uint32_t ramdiskSize;

    // If the boot image was patched with an old version of loki, the ramdisk
    // size is not stored properly. We'll need to guess the size of the archive.

    // The ramdisk is supposed to be from the gzip header to EOF, but loki needs
    // to store a copy of aboot, so it is put in the last 0x200 bytes of the file.
    ramdiskSize = data.size() - ramdiskOffset - 0x200;
    // For LG kernels:
    // ramdiskSize = data.size() - ramdiskOffset - d->header->page_size;

    // The gzip file is zero padded, so we'll search backwards until we find a
    // non-zero byte
    std::size_t begin = data.size() - 0x200;
    std::size_t location;
    bool found = false;

    if (begin < header.page_size) {
        return -1;
    }

    for (std::size_t i = begin; i > begin - header.page_size; --i) {
        if (data[i] != 0) {
            location = i;
            found = true;
            break;
        }
    }

    if (!found) {
        FLOGD("Ramdisk size: {:d} (may include some padding)", ramdiskSize);
    } else {
        ramdiskSize = location - ramdiskOffset;
        FLOGD("Ramdisk size: {:d} (with padding removed)", ramdiskSize);
    }

    return ramdiskSize;
}

uint32_t BootImage::Impl::lokiFindRamdiskAddress(const std::vector<unsigned char> &data,
                                                 const LokiHeader *loki) const
{
    // If the boot image was patched with a newer version of loki, find the ramdisk
    // offset in the shell code
    uint32_t ramdiskAddr = 0;

    if (loki->ramdisk_addr != 0) {
        auto size = data.size();

        for (uint32_t i = 0; i < size - (sizeof(SHELL_CODE) - 9); ++i) {
            if (std::memcmp(&data[i], SHELL_CODE, sizeof(SHELL_CODE) - 9) == 0) {
                ramdiskAddr = *(reinterpret_cast<const uint32_t *>(
                        &data[i] + sizeof(SHELL_CODE) - 5));
                break;
            }
        }

        if (ramdiskAddr == 0) {
            LOGW("Couldn't determine ramdisk offset");
            return 0;
        }

        FLOGD("Original ramdisk address: {:#08x}", ramdiskAddr);
    } else {
        // Otherwise, use the default for jflte
        ramdiskAddr = header.kernel_addr - 0x00008000 + 0x02000000;
        FLOGD("Default ramdisk address: {:#08x}", ramdiskAddr);
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

    switch (m_impl->header.page_size) {
    case 2048:
    case 4096:
    case 8192:
    case 16384:
    case 32768:
    case 65536:
    case 131072:
        break;
    default:
        FLOGE("Invalid page size: {:d}", m_impl->header.page_size);
        m_impl->error = PatcherError::createBootImageError(
                ErrorCode::BootImageParseError);
        return std::vector<unsigned char>();
    }

    // Header
    unsigned char *headerBegin =
            reinterpret_cast<unsigned char *>(&m_impl->header);
    data.insert(data.end(), headerBegin, headerBegin + sizeof(BootImageHeader));

    // Padding
    uint32_t paddingSize = m_impl->skipPadding(
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
                ErrorCode::FileOpenError, path);
        return false;
    }

    std::vector<unsigned char> data = create();
    if (data.empty()) {
        return false;
    }

    file.write(reinterpret_cast<const char *>(data.data()), data.size());
    if (file.bad()) {
        m_impl->error = PatcherError::createIOError(
                ErrorCode::FileWriteError, path);
        return false;
    }

    return true;
}

bool BootImage::isLoki() const
{
    return m_impl->isLoki;
}

uint32_t BootImage::Impl::skipPadding(const uint32_t itemSize,
                                      const uint32_t pageSize) const
{
    uint32_t pageMask = pageSize - 1;

    if ((itemSize & pageMask) == 0) {
        return 0;
    }

    return pageSize - (itemSize & pageMask);
}

static std::string toHex(const unsigned char *data, uint32_t size) {
    static const char digits[] = "0123456789abcdef";

    std::string hex;
    hex.reserve(2 * sizeof(size));
    for (uint32_t i = 0; i < size; ++i) {
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

    uint32_t digest[5];
    hash.get_digest(digest);

    std::memset(header.id, 0, sizeof(header.id));
    std::memcpy(header.id, reinterpret_cast<char *>(&digest),
                sizeof(digest) > sizeof(header.id)
                ? sizeof(header.id) : sizeof(digest));

    // Debug...
    std::string hexDigest = toHex(
            reinterpret_cast<const unsigned char *>(digest), sizeof(digest));

    FLOGD("Computed new ID hash: {:s}", hexDigest);
}

void BootImage::Impl::dumpHeader() const
{
    FLOGD("- magic:        {:s}",
          std::string(header.magic, header.magic + BOOT_MAGIC_SIZE));
    FLOGD("- kernel_size:  {:d}",    header.kernel_size);
    FLOGD("- kernel_addr:  {:#08x}", header.kernel_addr);
    FLOGD("- ramdisk_size: {:d}",    header.ramdisk_size);
    FLOGD("- ramdisk_addr: {:#08x}", header.ramdisk_addr);
    FLOGD("- second_size:  {:d}",    header.second_size);
    FLOGD("- second_addr:  {:#08x}", header.second_addr);
    FLOGD("- tags_addr:    {:#08x}", header.tags_addr);
    FLOGD("- page_size:    {:d}",    header.page_size);
    FLOGD("- dt_size:      {:d}",    header.dt_size);
    FLOGD("- unused:       {:#08x}", header.unused);
    FLOGD("- name:         {:s}",
          std::string(header.name, header.name + BOOT_NAME_SIZE));
    FLOGD("- cmdline:      {:s}",
          std::string(header.cmdline, header.cmdline + BOOT_ARGS_SIZE));
    FLOGD("- id:           {:s}",
          toHex(reinterpret_cast<const unsigned char *>(header.id), 32));
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
uint32_t BootImage::pageSize() const
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
void BootImage::setPageSize(uint32_t size)
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
uint32_t BootImage::kernelAddress() const
{
    return m_impl->header.kernel_addr;
}

/*!
 * \brief Set the kernel address field in the boot image header
 *
 * \param address Kernel address
 */
void BootImage::setKernelAddress(uint32_t address)
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
uint32_t BootImage::ramdiskAddress() const
{
    return m_impl->header.ramdisk_addr;
}

/*!
 * \brief Set the ramdisk address field in the boot image header
 *
 * \param address Ramdisk address
 */
void BootImage::setRamdiskAddress(uint32_t address)
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
uint32_t BootImage::secondBootloaderAddress() const
{
    return m_impl->header.second_addr;
}

/*!
 * \brief Set the second bootloader address field in the boot image header
 *
 * \param address Second bootloader address
 */
void BootImage::setSecondBootloaderAddress(uint32_t address)
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
uint32_t BootImage::kernelTagsAddress() const
{
    return m_impl->header.tags_addr;
}

/*!
 * \brief Set the kernel tags address field in the boot image header
 *
 * \param address Kernel tags address
 */
void BootImage::setKernelTagsAddress(uint32_t address)
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
void BootImage::setAddresses(uint32_t base, uint32_t kernelOffset,
                             uint32_t ramdiskOffset,
                             uint32_t secondBootloaderOffset,
                             uint32_t kernelTagsOffset)
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

}