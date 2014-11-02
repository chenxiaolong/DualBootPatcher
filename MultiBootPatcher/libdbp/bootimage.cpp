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


static const char *LOKI_MAGIC = "LOKI";

struct LokiHeader {
    unsigned char magic[4]; /* 0x494b4f4c */
    unsigned int recovery;  /* 0 = boot.img, 1 = recovery.img */
    char build[128];        /* Build number */

    unsigned int orig_kernel_size;
    unsigned int orig_ramdisk_size;
    unsigned int ramdisk_addr;
};


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


class BootImage::Impl
{
public:
    enum COMPRESSION_TYPES {
        GZIP,
        LZ4
    };

    // Android boot image header
    BootImageHeader header;

    // Various images stored in the boot image
    std::vector<unsigned char> kernelImage;
    std::vector<unsigned char> ramdiskImage;
    std::vector<unsigned char> secondBootloaderImage;
    std::vector<unsigned char> deviceTreeImage;

    // Whether the ramdisk is LZ4 or gzip compressed
    int ramdiskCompression = GZIP;

    PatcherError error;
};


BootImage::BootImage() : m_impl(new Impl())
{
}

BootImage::~BootImage()
{
}

PatcherError BootImage::error() const
{
    return m_impl->error;
}

bool BootImage::load(const std::vector<unsigned char> &data)
{
    // Check that the size of the boot image is okay
    if (data.size() < 512 + sizeof(BootImageHeader)) {
        m_impl->error = PatcherError::createBootImageError(
                PatcherError::BootImageSmallerThanHeaderError);
        return false;
    }

    // Find the Loki magic string
    bool isLoki = std::memcmp(&data[0x400], LOKI_MAGIC, 4) == 0;

    // Find the Android magic string
    bool isAndroid = false;
    int headerIndex = -1;

    int searchRange;
    if (isLoki) {
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
                PatcherError::BootImageNoAndroidHeaderError);
        return false;
    }

    bool ret;

    if (isLoki) {
        ret = loadLokiHeader(data, headerIndex, isLoki);
    } else {
        ret = loadAndroidHeader(data, headerIndex, isLoki);
    }

    return ret;
}

bool BootImage::load(const std::string &filename)
{
    std::vector<unsigned char> data;
    auto ret = FileUtils::readToMemory(filename, &data);
    if (ret.errorCode() != PatcherError::NoError) {
        m_impl->error = ret;
        return false;
    }

    return load(data);
}

bool BootImage::loadAndroidHeader(const std::vector<unsigned char> &data,
                                  const int headerIndex,
                                  const bool isLoki)
{
    // Make sure the file is large enough to contain the header
    if (data.size() < headerIndex + sizeof(BootImageHeader)) {
        m_impl->error = PatcherError::createBootImageError(
                PatcherError::BootImageSmallerThanHeaderError);
        return false;
    }

    // Read the Android boot image header
    auto android = reinterpret_cast<const BootImageHeader *>(&data[headerIndex]);

    Log::log(Log::Debug, "Found Android boot image header at: %d", headerIndex);

    // Save the header struct
    m_impl->header = *android;

    dumpHeader();

    // Don't try to read the various images inside the boot image if it's
    // Loki'd since some offsets and sizes need to be calculated
    if (!isLoki) {
        unsigned int pos = sizeof(BootImageHeader);
        pos += skipPadding(sizeof(BootImageHeader), android->page_size);

        m_impl->kernelImage.assign(
                data.begin() + pos,
                data.begin() + pos + android->kernel_size);

        pos += android->kernel_size;
        pos += skipPadding(android->kernel_size, android->page_size);

        m_impl->ramdiskImage.assign(
                data.begin() + pos,
                data.begin() + pos + android->ramdisk_size);

        if (data[pos] == 0x02 && data[pos + 1] == 0x21) {
            m_impl->ramdiskCompression = Impl::LZ4;
        } else {
            m_impl->ramdiskCompression = Impl::GZIP;
        }

        pos += android->ramdisk_size;
        pos += skipPadding(android->ramdisk_size, android->page_size);

        // The second bootloader may not exist
        if (android->second_size > 0) {
            m_impl->secondBootloaderImage.assign(
                    data.begin() + pos,
                    data.begin() + pos + android->second_size);
        } else {
            m_impl->secondBootloaderImage.clear();
        }

        pos += android->second_size;
        pos += skipPadding(android->second_size, android->page_size);

        // The device tree image may not exist as well
        if (android->dt_size > 0) {
            m_impl->deviceTreeImage.assign(
                    data.begin() + pos,
                    data.begin() + pos + android->dt_size);
        } else {
            m_impl->deviceTreeImage.clear();
        }
    }

    return true;
}

bool BootImage::loadLokiHeader(const std::vector<unsigned char> &data,
                               const int headerIndex,
                               const bool isLoki)
{
    // Make sure the file is large enough to contain the Loki header
    if (data.size() < 0x400 + sizeof(LokiHeader)) {
        m_impl->error = PatcherError::createBootImageError(
                PatcherError::BootImageSmallerThanHeaderError);
        return false;
    }

    if (!loadAndroidHeader(data, headerIndex, isLoki)) {
        // Error code already set
        return false;
    }

    // The kernel tags address is invalid in the loki images
    Log::log(Log::Debug, "Setting kernel tags address to default: 0x%08x",
               m_impl->header.tags_addr);
    resetKernelTagsAddress();

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

    int gzipOffset = lokiFindGzipOffset(data);
    if (gzipOffset < 0) {
        m_impl->error = PatcherError::createBootImageError(
                PatcherError::BootImageNoRamdiskGzipHeaderError);
        return false;
    }

    int ramdiskSize = lokiFindRamdiskSize(data, loki, gzipOffset);
    if (ramdiskSize < 0) {
        m_impl->error = PatcherError::createBootImageError(
                PatcherError::BootImageNoRamdiskSizeError);
        return false;
    }

    int kernelSize = lokiFindKernelSize(data, loki);
    if (kernelSize < 0) {
        m_impl->error = PatcherError::createBootImageError(
                PatcherError::BootImageNoKernelSizeError);
        return false;
    }

    unsigned int ramdiskAddr = lokiFindRamdiskAddress(data, loki);
    if (ramdiskAddr == 0) {
        m_impl->error = PatcherError::createBootImageError(
                PatcherError::BootImageNoRamdiskAddressError);
        return false;
    }

    m_impl->header.ramdisk_size = ramdiskSize;
    m_impl->header.kernel_size = kernelSize;
    m_impl->header.ramdisk_addr = ramdiskAddr;

    m_impl->kernelImage.assign(
            data.begin() + m_impl->header.page_size,
            data.begin() + m_impl->header.page_size + kernelSize);
    m_impl->ramdiskImage.assign(
            data.begin() + gzipOffset,
            data.begin() + gzipOffset + ramdiskSize);
    m_impl->secondBootloaderImage.clear();
    m_impl->deviceTreeImage.clear();

    return true;
}

int BootImage::lokiFindGzipOffset(const std::vector<unsigned char> &data) const
{
    // Find the location of the ramdisk inside the boot image
    unsigned int gzipOffset = 0x400 + sizeof(LokiHeader);

    const char *gzip1 = "\x1F\x8B\x08\x00";
    const char *gzip2 = "\x1F\x8B\x08\x08";

    std::vector<unsigned int> offsets;
    std::vector<bool> timestamps; // True if timestamp is not 0x00000000

    // This is the offset on top of gzipOffset
    int curOffset = gzipOffset - 1;

    while (true) {
        // Try to find first gzip header
        auto it = std::search(data.begin() + curOffset + 1, data.end(),
                              gzip1, gzip1 + 4);

        // Second gzip header
        if (it == data.end()) {
            it = std::search(data.begin() + curOffset + 1, data.end(),
                             gzip2, gzip2 + 4);
        }

        if (it == data.end()) {
            break;
        }

        curOffset = it - data.begin();

        Log::log(Log::Debug, "Found a gzip header at 0x%x", curOffset);

        // Searching for 1F8B0800 wasn't enough for some boot images. Specifically,
        // ktoonsez's 20140319 kernels had another set of those four bytes before
        // the "real" gzip header. We'll work around that by checking that the
        // timestamp isn't zero (which is technically allowed, but the boot image
        // tools don't do that)
        // http://forum.xda-developers.com/showpost.php?p=51219628&postcount=3767
        auto curTimestamp = &data[curOffset];

        offsets.push_back(curOffset);
        timestamps.push_back(
                std::memcmp(curTimestamp, "\x00\x00\x00\x00", 4) == 0);
    }

    if (offsets.empty()) {
        Log::log(Log::Warning, "Could not find the ramdisk's gzip header");
        return -1;
    }

    Log::log(Log::Debug, "Found %lu gzip headers", offsets.size());

    // Try gzip offset that is immediately followed by non-null timestamp first
    gzipOffset = 0;
    for (unsigned int i = 0; i < offsets.size(); ++i) {
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

int BootImage::lokiFindRamdiskSize(const std::vector<unsigned char> &data,
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

        if (begin < m_impl->header.page_size) {
            return -1;
        }

        for (unsigned int i = begin; i > begin - m_impl->header.page_size; i--) {
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

int BootImage::lokiFindKernelSize(const std::vector<unsigned char> &data,
                                  const LokiHeader *loki) const
{
    int kernelSize = -1;

    // If the boot image was patched with an early version of loki, the original
    // kernel size is not stored in the loki header properly (or in the shellcode).
    // The size is stored in the kernel image's header though, so we'll use that.
    // http://www.simtec.co.uk/products/SWLINUX/files/booting_article.html#d0e309
    if (loki->orig_kernel_size == 0) {
        kernelSize = *(reinterpret_cast<const int *>(
                &data[m_impl->header.page_size + 0x2c]));
    } else {
        kernelSize = loki->orig_kernel_size;
    }

    Log::log(Log::Debug, "Kernel size: %d", kernelSize);

    return kernelSize;
}

unsigned int BootImage::lokiFindRamdiskAddress(const std::vector<unsigned char> &data,
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
        ramdiskAddr = m_impl->header.kernel_addr - 0x00008000 + 0x02000000;
        Log::log(Log::Debug, "Default ramdisk address: 0x%08x", ramdiskAddr);
    }

    return ramdiskAddr;
}

std::vector<unsigned char> BootImage::create() const
{
    std::vector<unsigned char> data;

    // Header
    unsigned char *headerBegin =
            reinterpret_cast<unsigned char *>(&m_impl->header);
    data.insert(data.end(), headerBegin, headerBegin + sizeof(BootImageHeader));

    // Padding
    unsigned int paddingSize = skipPadding(
            sizeof(BootImageHeader), m_impl->header.page_size);
    data.insert(data.end(), paddingSize, 0);

    // Kernel image
    data.insert(data.end(), m_impl->kernelImage.begin(),
                m_impl->kernelImage.end());

    // More padding
    paddingSize = skipPadding(
            m_impl->kernelImage.size(), m_impl->header.page_size);
    data.insert(data.end(), paddingSize, 0);

    // Ramdisk image
    data.insert(data.end(), m_impl->ramdiskImage.begin(),
                m_impl->ramdiskImage.end());

    // Even more padding
    paddingSize = skipPadding(
            m_impl->ramdiskImage.size(), m_impl->header.page_size);
    data.insert(data.end(), paddingSize, 0);

    // Second bootloader image
    if (!m_impl->secondBootloaderImage.empty()) {
        data.insert(data.end(), m_impl->secondBootloaderImage.begin(),
                    m_impl->secondBootloaderImage.end());

        // Enough padding already!
        paddingSize = skipPadding(
                m_impl->secondBootloaderImage.size(), m_impl->header.page_size);
        data.insert(data.end(), paddingSize, 0);
    }

    // Device tree image
    if (!m_impl->deviceTreeImage.empty()) {
        data.insert(data.end(), m_impl->deviceTreeImage.begin(),
                    m_impl->deviceTreeImage.end());

        // Last bit of padding (I hope)
        paddingSize = skipPadding(
                m_impl->deviceTreeImage.size(), m_impl->header.page_size);
        data.insert(data.end(), paddingSize, 0);
    }

    return data;
}

bool BootImage::createFile(const std::string &path)
{
    std::ofstream file(path, std::ios::binary);
    if (file.fail()) {
        m_impl->error = PatcherError::createIOError(
                PatcherError::FileOpenError, path);
        return false;
    }

    std::vector<unsigned char> data = create();
    file.write(reinterpret_cast<const char *>(data.data()), data.size());
    if (file.bad()) {
        file.close();

        m_impl->error = PatcherError::createIOError(
                PatcherError::FileWriteError, path);
        return false;
    }

    file.close();
    return true;
}

bool BootImage::extract(const std::string &directory, const std::string &prefix)
{
    if (!boost::filesystem::exists(directory)) {
        m_impl->error = PatcherError::createIOError(
                PatcherError::DirectoryNotExistError, directory);
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
    fileKernel.write(reinterpret_cast<char *>(&m_impl->kernelImage),
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
    fileRamdisk.write(reinterpret_cast<char *>(&m_impl->ramdiskImage),
                      m_impl->ramdiskImage.size());
    fileRamdisk.close();

    // Write second bootloader image
    if (!m_impl->secondBootloaderImage.empty()) {
        std::ofstream fileSecond(pathPrefix + "-second", std::ios::binary);
        fileSecond.write(reinterpret_cast<char *>(&m_impl->secondBootloaderImage),
                         m_impl->secondBootloaderImage.size());
        fileSecond.close();
    }

    // Write device tree image
    if (!m_impl->deviceTreeImage.empty()) {
        std::ofstream fileDt(pathPrefix + "-dt", std::ios::binary);
        fileDt.write(reinterpret_cast<char *>(&m_impl->deviceTreeImage),
                     m_impl->deviceTreeImage.size());
        fileDt.close();
    }

    return true;
}

int BootImage::skipPadding(const int &itemSize, const int &pageSize) const
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

void BootImage::updateSHA1Hash()
{
    boost::uuids::detail::sha1 hash;
    hash.process_bytes(m_impl->kernelImage.data(), m_impl->kernelImage.size());
    hash.process_bytes(reinterpret_cast<char *>(&m_impl->header.kernel_size),
                       sizeof(m_impl->header.kernel_size));
    hash.process_bytes(m_impl->ramdiskImage.data(), m_impl->ramdiskImage.size());
    hash.process_bytes(reinterpret_cast<char *>(&m_impl->header.ramdisk_size),
                       sizeof(m_impl->header.ramdisk_size));
    if (!m_impl->secondBootloaderImage.empty()) {
        hash.process_bytes(m_impl->secondBootloaderImage.data(),
                           m_impl->secondBootloaderImage.size());
    }

    // Bug in AOSP? AOSP's mkbootimg adds the second bootloader size to the SHA1
    // hash even if it's 0
    hash.process_bytes(reinterpret_cast<char *>(&m_impl->header.second_size),
                       sizeof(m_impl->header.second_size));

    if (!m_impl->deviceTreeImage.empty()) {
        hash.process_bytes(m_impl->deviceTreeImage.data(),
                           m_impl->deviceTreeImage.size());
        hash.process_bytes(reinterpret_cast<char *>(&m_impl->header.dt_size),
                           sizeof(m_impl->header.dt_size));
    }

    unsigned int digest[5];
    hash.get_digest(digest);

    std::memset(m_impl->header.id, 0, sizeof(m_impl->header.id));
    std::memcpy(m_impl->header.id, reinterpret_cast<char *>(&digest),
                sizeof(digest) > sizeof(m_impl->header.id)
                ? sizeof(m_impl->header.id) : sizeof(digest));

    // Debug...
    std::string hexDigest = toHex(
            reinterpret_cast<const unsigned char *>(digest), sizeof(digest));

    Log::log(Log::Debug, "Computed new ID hash: %s", hexDigest);
}

void BootImage::dumpHeader() const
{
    Log::log(Log::Debug, "- magic:        %s",
             std::string(m_impl->header.magic, m_impl->header.magic + BOOT_MAGIC_SIZE));
    Log::log(Log::Debug, "- kernel_size:  %u",     m_impl->header.kernel_size);
    Log::log(Log::Debug, "- kernel_addr:  0x%08x", m_impl->header.kernel_addr);
    Log::log(Log::Debug, "- ramdisk_size: %u",     m_impl->header.ramdisk_size);
    Log::log(Log::Debug, "- ramdisk_addr: 0x%08x", m_impl->header.ramdisk_addr);
    Log::log(Log::Debug, "- second_size:  %u",     m_impl->header.second_size);
    Log::log(Log::Debug, "- second_addr:  0x%08x", m_impl->header.second_addr);
    Log::log(Log::Debug, "- tags_addr:    0x%08x", m_impl->header.tags_addr);
    Log::log(Log::Debug, "- page_size:    %u",     m_impl->header.page_size);
    Log::log(Log::Debug, "- dt_size:      %u",     m_impl->header.dt_size);
    Log::log(Log::Debug, "- unused:       0x%08x", m_impl->header.unused);
    Log::log(Log::Debug, "- name:         %s",
             std::string(m_impl->header.name, m_impl->header.name + BOOT_NAME_SIZE));
    Log::log(Log::Debug, "- cmdline:      %s",
             std::string(m_impl->header.cmdline, m_impl->header.cmdline + BOOT_ARGS_SIZE));
    //Log::log(Log::Debug, "- id:           %08x%08x%08x%08x%08x%08x%08x%08x",
    //         m_impl->header.id[0], m_impl->header.id[1], m_impl->header.id[2],
    //         m_impl->header.id[3], m_impl->header.id[4], m_impl->header.id[5],
    //         m_impl->header.id[6], m_impl->header.id[7]);
    Log::log(Log::Debug, "- id:           %s",
             toHex(reinterpret_cast<const unsigned char *>(m_impl->header.id), 40));
}

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

void BootImage::setBoardName(const std::string &name)
{
    // -1 for null byte
    std::strcpy(reinterpret_cast<char *>(m_impl->header.name),
                name.substr(0, BOOT_NAME_SIZE - 1).c_str());
}

void BootImage::resetBoardName()
{
    std::strcpy(reinterpret_cast<char *>(m_impl->header.name), DEFAULT_BOARD);
}

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

void BootImage::setKernelCmdline(const std::string &cmdline)
{
    // -1 for null byte
    std::strcpy(reinterpret_cast<char *>(m_impl->header.cmdline),
                cmdline.substr(0, BOOT_ARGS_SIZE - 1).c_str());
}

void BootImage::resetKernelCmdline()
{
    std::strcpy(reinterpret_cast<char *>(m_impl->header.cmdline),
                DEFAULT_CMDLINE);
}

unsigned int BootImage::pageSize() const
{
    return m_impl->header.page_size;
}

void BootImage::setPageSize(unsigned int size)
{
    // Size should be one of if 2048, 4096, 8192, 16384, 32768, 65536, or 131072
    m_impl->header.page_size = size;
}

void BootImage::resetPageSize()
{
    m_impl->header.page_size = DEFAULT_PAGE_SIZE;
}

unsigned int BootImage::kernelAddress() const
{
    return m_impl->header.kernel_addr;
}

void BootImage::setKernelAddress(unsigned int address)
{
    m_impl->header.kernel_addr = address;
}

void BootImage::resetKernelAddress()
{
    m_impl->header.kernel_addr = DEFAULT_BASE + DEFAULT_KERNEL_OFFSET;
}

unsigned int BootImage::ramdiskAddress() const
{
    return m_impl->header.ramdisk_addr;
}

void BootImage::setRamdiskAddress(unsigned int address)
{
    m_impl->header.ramdisk_addr = address;
}

void BootImage::resetRamdiskAddress()
{
    m_impl->header.ramdisk_addr = DEFAULT_BASE + DEFAULT_RAMDISK_OFFSET;
}

unsigned int BootImage::secondBootloaderAddress() const
{
    return m_impl->header.second_addr;
}

void BootImage::setSecondBootloaderAddress(unsigned int address)
{
    m_impl->header.second_addr = address;
}

void BootImage::resetSecondBootloaderAddress()
{
    m_impl->header.second_addr = DEFAULT_BASE + DEFAULT_SECOND_OFFSET;
}

unsigned int BootImage::kernelTagsAddress() const
{
    return m_impl->header.tags_addr;
}

void BootImage::setKernelTagsAddress(unsigned int address)
{
    m_impl->header.tags_addr = address;
}

void BootImage::resetKernelTagsAddress()
{
    m_impl->header.tags_addr = DEFAULT_BASE + DEFAULT_TAGS_OFFSET;
}

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

std::vector<unsigned char> BootImage::kernelImage() const
{
    return m_impl->kernelImage;
}

void BootImage::setKernelImage(std::vector<unsigned char> data)
{
    m_impl->header.kernel_size = data.size();
    m_impl->kernelImage = std::move(data);
    updateSHA1Hash();
}

std::vector<unsigned char> BootImage::ramdiskImage() const
{
    return m_impl->ramdiskImage;
}

void BootImage::setRamdiskImage(std::vector<unsigned char> data)
{
    m_impl->header.ramdisk_size = data.size();
    m_impl->ramdiskImage = std::move(data);
    updateSHA1Hash();
}

std::vector<unsigned char> BootImage::secondBootloaderImage() const
{
    return m_impl->secondBootloaderImage;
}

void BootImage::setSecondBootloaderImage(std::vector<unsigned char> data)
{
    m_impl->header.second_size = data.size();
    m_impl->secondBootloaderImage = std::move(data);
    updateSHA1Hash();
}

std::vector<unsigned char> BootImage::deviceTreeImage() const
{
    return m_impl->deviceTreeImage;
}

void BootImage::setDeviceTreeImage(std::vector<unsigned char> data)
{
    m_impl->header.dt_size = data.size();
    m_impl->deviceTreeImage = std::move(data);
    updateSHA1Hash();
}
