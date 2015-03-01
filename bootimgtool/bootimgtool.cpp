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

#include <string>
#include <unordered_map>

#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstring>

#include <getopt.h>

#include <boost/filesystem.hpp>

#include <libmbp/bootimage.h>
#include <libmbp/logging.h>
#include <libmbp/patchererror.h>


typedef std::unique_ptr<std::FILE, int (*)(std::FILE *)> file_ptr;

namespace bf = boost::filesystem;


static const char MainUsage[] =
    "Usage: bootimgtool <command> [<args>]\n"
    "\n"
    "Available commands:\n"
    "  unpack         Unpack a boot image\n"
    "  pack           Assemble boot image from unpacked files\n"
    "\n"
    "Pass -h/--help as a argument to a command to see it's available options.\n";

static const char UnpackUsage[] =
    "Usage: bootimgtool unpack [input file] [options]\n"
    "\n"
    "Options:\n"
    "  -o, --output [output directory]\n"
    "                  Output directory (current directory if unspecified)\n"
    "  -p, --prefix [prefix]\n"
    "                  Prefix to prepend to output filenames\n"
    "  -n, --noprefix  Do not prepend a prefix to the item filenames\n"
    "  --output-[item] [item path]\n"
    "                  Custom path for a particular item\n"
    "\n"
    "The following items are extracted from the boot image. These files contain\n"
    "all of the information necessary to recreate an identical boot image.\n"
    "\n"
    "  cmdline         Kernel command line\n"
    "  base            Base address for offsets\n"
    "  kernel_offset   Address offset of the kernel image\n"
    "  ramdisk_offset  Address offset of the ramdisk image\n"
    "  second_offset   Address offset of the second bootloader image\n"
    "  tags_offset     Address offset of the kernel tags image\n"
    "  page_size       Page size\n"
    "  kernel`         Kernel image\n"
    "  ramdisk         Ramdisk image\n"
    "  second          Second bootloader image\n"
    "  dt              Device tree image\n"
    "\n"
    "Output files:\n"
    "\n"
    "By default, the items are unpacked to the following path:\n"
    "\n"
    "    [output directory/[prefix]-[item]\n"
    "\n"
    "If a prefix wasn't specified, then the input filename is used as a prefix.\n"
    "(eg. \"bootimgtool unpack boot.img -o /tmp\" will unpack /tmp/boot.img-kernel,\n"
    "..., etc.). If the -n/--noprefix option was specified, then the items are\n"
    "unpacked to the following path:\n"
    "\n"
    "    [output directory]/[item]\n"
    "\n"
    "If the --output-[item]=[item path] option is specified, then that particular\n"
    "item is unpacked to the specified [item path].\n"
    "\n"
    "Examples:\n"
    "\n"
    "1. Plain ol' unpacking (just make this thing work!). This extracts boot.img to\n"
    "   boot.img-cmdline, boot.img-base, ...\n"
    "\n"
    "        bootimgtool unpack boot.img\n"
    "\n"
    "2. Unpack to a different directory, but put the kernel in /tmp/\n"
    "\n"
    "        bootimgtool unpack boot.img -o extracted --output-kernel /tmp/kernel.img\n";

static const char PackUsage[] =
    "Usage: bootimgtool pack [output file] [options]\n"
    "\n"
    "Options:\n"
    "  -i, --input [input directory]\n"
    "                  Input directory (current directory if unspecified)\n"
    "  -p, --prefix [prefix]\n"
    "                  Prefix to prepend to item filenames\n"
    "  -n, --noprefix  Do not prepend a prefix to the item filenames\n"
    "  --input-[item] [item path]\n"
    "                  Custom path for a particular item\n"
    "  --value-[item] [item value]\n"
    "                  Specify a value for an item directly\n"
    "\n"
    "The following items are loaded to create a new boot image.\n"
    "\n"
    "  cmdline *        Kernel command line\n"
    "  base *           Base address for offsets\n"
    "  kernel_offset *  Address offset of the kernel image\n"
    "  ramdisk_offset * Address offset of the ramdisk image\n"
    "  second_offset *  Address offset of the second bootloader image\n"
    "  tags_offset *    Address offset of the kernel tags image\n"
    "  page_size *      Page size\n"
    "  kernel`          Kernel image\n"
    "  ramdisk          Ramdisk image\n"
    "  second           Second bootloader image\n"
    "  dt               Device tree image\n"
    "\n"
    "Items marked with an asterisk can be specified by value using the --value-*\n"
    "options. (eg. --value-page_size=2048).\n"
    "\n"
    "Input files:\n"
    "\n"
    "By default, the items are loaded from the following path:\n"
    "\n"
    "    [input directory]/[prefix]-[item]\n"
    "\n"
    "If a prefix wasn't specified, then the output filename is used as a prefix.\n"
    "(eg. \"bootimgtool pack boot.img -i /tmp\" will load /tmp/boot.img-cmdline,\n"
    "..., etc.). If the -n/--noprefix option was specified, then the items are\n"
    "loaded from the following path:\n"
    "\n"
    "    [input directory]/[item]\n"
    "\n"
    "If the --input-[item]=[item path] option is specified, then that particular\n"
    "item is loaded from the specified [item path].\n"
    "\n"
    "Examples:\n"
    "\n"
    "1. To rebuild a boot image that was extracted using the bootimgtool \"unpack\"\n"
    "   command, just specify the same directory that was used to extract the boot\n"
    "   image.\n"
    "\n"
    "        bootimgtool unpack -o extracted boot.img\n"
    "        bootimgtool pack boot.img -i extracted\n"
    "\n"
    "2. Create boot.img from unpacked files in /tmp/android, but use the kernel\n"
    "   located at the path /tmp/newkernel.\n"
    "\n"
    "        bootimgtool pack boot.img -i /tmp/android --input-kernel /tmp/newkernel\n";


static std::string error_to_string(const mbp::PatcherError &error) {
    switch (error.errorCode()) {
    case mbp::ErrorCode::FileOpenError:
        return "Failed to open file: " + error.filename();
    case mbp::ErrorCode::FileReadError:
        return "Failed to read from file: " + error.filename();
    case mbp::ErrorCode::FileWriteError:
        return "Failed to write to file: " + error.filename();
    case mbp::ErrorCode::DirectoryNotExistError:
        return "Directory does not exist: " + error.filename();
    case mbp::ErrorCode::BootImageParseError:
        return "Failed to parse boot image";
    case mbp::ErrorCode::CpioFileAlreadyExistsError:
        return "File already exists in cpio archive: " + error.filename();
    case mbp::ErrorCode::CpioFileNotExistError:
        return "File does not exist in cpio archive: " + error.filename();
    case mbp::ErrorCode::ArchiveReadOpenError:
        return "Failed to open archive for reading";
    case mbp::ErrorCode::ArchiveReadDataError:
        return "Failed to read archive data for file: " + error.filename();
    case mbp::ErrorCode::ArchiveReadHeaderError:
        return "Failed to read archive entry header";
    case mbp::ErrorCode::ArchiveWriteOpenError:
        return "Failed to open archive for writing";
    case mbp::ErrorCode::ArchiveWriteDataError:
        return "Failed to write archive data for file: " + error.filename();
    case mbp::ErrorCode::ArchiveWriteHeaderError:
        return "Failed to write archive header for file: " + error.filename();
    case mbp::ErrorCode::ArchiveCloseError:
        return "Failed to close archive";
    case mbp::ErrorCode::ArchiveFreeError:
        return "Failed to free archive header memory";
    case mbp::ErrorCode::NoError:
    case mbp::ErrorCode::UnknownError:
    case mbp::ErrorCode::PatcherCreateError:
    case mbp::ErrorCode::AutoPatcherCreateError:
    case mbp::ErrorCode::RamdiskPatcherCreateError:
    case mbp::ErrorCode::XmlParseFileError:
    case mbp::ErrorCode::OnlyZipSupported:
    case mbp::ErrorCode::OnlyBootImageSupported:
    case mbp::ErrorCode::PatchingCancelled:
    case mbp::ErrorCode::SystemCacheFormatLinesNotFound:
    default:
        assert(false);
    }

    return std::string();
}

static void mbp_log_cb(mbp::LogLevel prio, const std::string &msg)
{
    switch (prio) {
    case mbp::LogLevel::Debug:
    case mbp::LogLevel::Error:
    case mbp::LogLevel::Info:
    case mbp::LogLevel::Verbose:
    case mbp::LogLevel::Warning:
        printf("%s\n", msg.c_str());
        break;
    }
}

__attribute__((format(printf, 2, 3)))
static bool write_file_fmt(const std::string &path, const char *fmt, ...)
{
    file_ptr fp(fopen(path.c_str(), "wb"), fclose);
    if (!fp) {
        return false;
    }

    va_list ap;
    va_start(ap, fmt);

    int ret = vfprintf(fp.get(), fmt, ap);

    va_end(ap);

    return ret >= 0;

}

static bool write_file_data(const std::string &path,
                            const void *data, std::size_t size)
{
    file_ptr fp(fopen(path.c_str(), "wb"), fclose);
    if (!fp) {
        return false;
    }

    if (fwrite(data, 1, size, fp.get()) != size && ferror(fp.get())) {
        return false;
    }

    return true;
}

static bool write_file_data(const std::string &path,
                            const std::vector<unsigned char> &data)
{
    return write_file_data(path, data.data(), data.size());
}

static bool read_file_data(const std::string &path,
                           std::vector<unsigned char> *out)
{
    file_ptr fp(fopen(path.c_str(), "rb"), fclose);
    if (!fp) {
        return false;
    }

    fseek(fp.get(), 0, SEEK_END);
    std::size_t size = ftell(fp.get());
    rewind(fp.get());

    std::vector<unsigned char> data(size);

    auto nread = fread(data.data(), 1, size, fp.get());
    if (nread != size) {
        return false;
    }

    data.swap(*out);
    return true;
}

static bool str_to_uint32(uint32_t *out, const char *str, int base = 0)
{
    char *end;
    errno = 0;
    uint32_t num = strtoul(str, &end, base);
    if (errno == ERANGE) {
        return false;
    }
    if (*str == '\0' || *end != '\0') {
        return false;
    }
    *out = num;
    return true;
}

bool unpack_main(int argc, char *argv[])
{
    int opt;
    bool no_prefix = false;
    std::string input_file;
    std::string output_dir;
    std::string prefix;
    std::string path_cmdline;
    std::string path_base;
    std::string path_kernel_offset;
    std::string path_ramdisk_offset;
    std::string path_second_offset;
    std::string path_tags_offset;
    std::string path_page_size;
    std::string path_kernel;
    std::string path_ramdisk;
    std::string path_second;
    std::string path_dt;

    // Arguments with no short options
    enum unpack_options : int
    {
        OPT_OUTPUT_CMDLINE        = 10000 + 1,
        OPT_OUTPUT_BASE           = 10000 + 2,
        OPT_OUTPUT_KERNEL_OFFSET  = 10000 + 3,
        OPT_OUTPUT_RAMDISK_OFFSET = 10000 + 4,
        OPT_OUTPUT_SECOND_OFFSET  = 10000 + 5,
        OPT_OUTPUT_TAGS_OFFSET    = 10000 + 6,
        OPT_OUTPUT_PAGE_SIZE      = 10000 + 7,
        OPT_OUTPUT_KERNEL         = 10000 + 8,
        OPT_OUTPUT_RAMDISK        = 10000 + 9,
        OPT_OUTPUT_SECOND         = 10000 + 10,
        OPT_OUTPUT_DT             = 10000 + 11
    };

    static struct option long_options[] = {
        // Arguments with short versions
        {"output",                required_argument, 0, 'o'},
        {"prefix",                required_argument, 0, 'p'},
        {"noprefix",              required_argument, 0, 'n'},
        // Arguments without short versions
        {"output-cmdline",        required_argument, 0, OPT_OUTPUT_CMDLINE},
        {"output-base",           required_argument, 0, OPT_OUTPUT_BASE},
        {"output-kernel_offset",  required_argument, 0, OPT_OUTPUT_KERNEL_OFFSET},
        {"output-ramdisk_offset", required_argument, 0, OPT_OUTPUT_RAMDISK_OFFSET},
        {"output-second_offset",  required_argument, 0, OPT_OUTPUT_SECOND_OFFSET},
        {"output-tags_offset",    required_argument, 0, OPT_OUTPUT_TAGS_OFFSET},
        {"output-page_size",      required_argument, 0, OPT_OUTPUT_PAGE_SIZE},
        {"output-kernel",         required_argument, 0, OPT_OUTPUT_KERNEL},
        {"output-ramdisk",        required_argument, 0, OPT_OUTPUT_RAMDISK},
        {"output-second",         required_argument, 0, OPT_OUTPUT_SECOND},
        {"output-dt",             required_argument, 0, OPT_OUTPUT_DT},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, "o:p:n", long_options, &long_index)) != -1) {
        switch (opt) {
        case 'o':                       output_dir = optarg;          break;
        case 'p':                       prefix = optarg;              break;
        case 'n':                       no_prefix = optarg;           break;
        case OPT_OUTPUT_CMDLINE:        path_cmdline = optarg;        break;
        case OPT_OUTPUT_BASE:           path_base = optarg;           break;
        case OPT_OUTPUT_KERNEL_OFFSET:  path_kernel_offset = optarg;  break;
        case OPT_OUTPUT_RAMDISK_OFFSET: path_ramdisk_offset = optarg; break;
        case OPT_OUTPUT_SECOND_OFFSET:  path_second_offset = optarg;  break;
        case OPT_OUTPUT_TAGS_OFFSET:    path_tags_offset = optarg;    break;
        case OPT_OUTPUT_PAGE_SIZE:      path_page_size = optarg;      break;
        case OPT_OUTPUT_KERNEL:         path_kernel = optarg;         break;
        case OPT_OUTPUT_RAMDISK:        path_ramdisk = optarg;        break;
        case OPT_OUTPUT_SECOND:         path_second = optarg;         break;
        case OPT_OUTPUT_DT:             path_dt = optarg;             break;

        case 'h':
            fprintf(stdout, UnpackUsage);
            return true;

        default:
            fprintf(stderr, UnpackUsage);
            return false;
        }
    }

    // There should be one other arguments
    if (argc - optind != 1) {
        fprintf(stderr, UnpackUsage);
        return false;
    }

    input_file = argv[optind];

    if (no_prefix) {
        prefix.clear();
    } else {
        if (prefix.empty()) {
            prefix = bf::path(input_file).filename().string();
        }
        prefix += "-";
    }

    bf::path bf_output_dir(output_dir);

    if (path_cmdline.empty())
        path_cmdline = (bf_output_dir / (prefix + "cmdline")).string();
    if (path_base.empty())
        path_base = (bf_output_dir / (prefix + "base")).string();
    if (path_kernel_offset.empty())
        path_kernel_offset = (bf_output_dir / (prefix + "kernel_offset")).string();
    if (path_ramdisk_offset.empty())
        path_ramdisk_offset = (bf_output_dir / (prefix + "ramdisk_offset")).string();
    if (path_second_offset.empty())
        path_second_offset = (bf_output_dir / (prefix + "second_offset")).string();
    if (path_tags_offset.empty())
        path_tags_offset = (bf_output_dir / (prefix + "tags_offset")).string();
    if (path_page_size.empty())
        path_page_size = (bf_output_dir / (prefix + "page_size")).string();
    if (path_kernel.empty())
        path_kernel = (bf_output_dir / (prefix + "kernel")).string();
    if (path_ramdisk.empty())
        path_ramdisk = (bf_output_dir / (prefix + "ramdisk")).string();
    if (path_second.empty())
        path_second = (bf_output_dir / (prefix + "second")).string();
    if (path_dt.empty())
        path_dt = (bf_output_dir / (prefix + "dt")).string();

    printf("Output files:\n");
    printf("- cmdline:        %s\n", path_cmdline.c_str());
    printf("- base:           %s\n", path_base.c_str());
    printf("- kernel_offset:  %s\n", path_kernel_offset.c_str());
    printf("- ramdisk_offset: %s\n", path_ramdisk_offset.c_str());
    printf("- second_offset:  %s\n", path_second_offset.c_str());
    printf("- tags_offset:    %s\n", path_tags_offset.c_str());
    printf("- page_size:      %s\n", path_page_size.c_str());
    printf("- kernel:         %s\n", path_kernel.c_str());
    printf("- ramdisk:        %s\n", path_ramdisk.c_str());
    printf("- second:         %s\n", path_second.c_str());
    printf("- dt:             %s\n", path_dt.c_str());
    printf("\n");

    if (!bf::exists(bf_output_dir)) {
        if (!bf::create_directories(bf_output_dir)) {
            fprintf(stderr, "Failed to create %s\n", output_dir.c_str());
            return false;
        }
    }

    // Load the boot image
    mbp::BootImage bi;
    if (!bi.load(input_file)) {
        fprintf(stderr, "%s\n", error_to_string(bi.error()).c_str());
        return false;
    }

    /* Extract all the stuff! */

    // Use base relative to the default kernel offset
    uint32_t base = bi.kernelAddress() - mbp::BootImage::DefaultKernelOffset;
    uint32_t kernel_offset = mbp::BootImage::DefaultKernelOffset;
    uint32_t ramdisk_offset = bi.ramdiskAddress() - base;
    uint32_t second_offset = bi.secondBootloaderAddress() - base;
    uint32_t tags_offset = bi.kernelTagsAddress() - base;

    // Write kernel command line
    if (!write_file_fmt(path_cmdline, "%s\n", bi.kernelCmdline().c_str())) {
        fprintf(stderr, "%s: %s\n", path_cmdline.c_str(), strerror(errno));
        return false;
    }

    // Write base address on which the offsets are applied
    if (!write_file_fmt(path_base, "%08x\n", base)) {
        fprintf(stderr, "%s: %s\n", path_base.c_str(), strerror(errno));
        return false;
    }

    // Write kernel offset
    if (!write_file_fmt(path_kernel_offset, "%08x\n", kernel_offset)) {
        fprintf(stderr, "%s: %s\n", path_kernel_offset.c_str(), strerror(errno));
        return false;
    }

    // Write ramdisk offset
    if (!write_file_fmt(path_ramdisk_offset, "%08x\n", ramdisk_offset)) {
        fprintf(stderr, "%s: %s\n", path_ramdisk_offset.c_str(), strerror(errno));
        return false;
    }

    // Write second bootloader offset
    if (!write_file_fmt(path_second_offset, "%08x\n", second_offset)) {
        fprintf(stderr, "%s: %s\n", path_second_offset.c_str(), strerror(errno));
        return false;
    }

    // Write kernel tags offset
    if (!write_file_fmt(path_tags_offset, "%08x\n", tags_offset)) {
        fprintf(stderr, "%s: %s\n", path_tags_offset.c_str(), strerror(errno));
        return false;
    }

    // Write page size
    if (!write_file_fmt(path_page_size, "%u\n", bi.pageSize())) {
        fprintf(stderr, "%s: %s\n", path_page_size.c_str(), strerror(errno));
        return false;
    }

    std::vector<unsigned char> kernel_image = bi.kernelImage();
    std::vector<unsigned char> ramdisk_image = bi.ramdiskImage();
    std::vector<unsigned char> second_image = bi.secondBootloaderImage();
    std::vector<unsigned char> dt_image = bi.deviceTreeImage();

    // Write kernel image
    if (!write_file_data(path_kernel, kernel_image)) {
        fprintf(stderr, "%s: %s\n", path_kernel.c_str(), strerror(errno));
        return false;
    }

    // Write ramdisk image
    if (!write_file_data(path_ramdisk, ramdisk_image)) {
        fprintf(stderr, "%s: %s\n", path_ramdisk.c_str(), strerror(errno));
        return false;
    }

    // Write second bootloader image
    if (!write_file_data(path_second, second_image)) {
        fprintf(stderr, "%s: %s\n", path_second.c_str(), strerror(errno));
        return false;
    }

    // Write device tree image
    if (!write_file_data(path_dt, dt_image)) {
        fprintf(stderr, "%s: %s\n", path_dt.c_str(), strerror(errno));
        return false;
    }

    printf("\nDone\n");

    return true;
}

bool pack_main(int argc, char *argv[])
{
    int opt;
    bool no_prefix = false;
    std::string output_file;
    std::string input_dir;
    std::string prefix;
    std::string path_cmdline;
    std::string path_base;
    std::string path_kernel_offset;
    std::string path_ramdisk_offset;
    std::string path_second_offset;
    std::string path_tags_offset;
    std::string path_page_size;
    std::string path_kernel;
    std::string path_ramdisk;
    std::string path_second;
    std::string path_dt;
    // Values
    std::unordered_map<int, bool> values;
    std::string cmdline;
    uint32_t base;
    uint32_t kernel_offset;
    uint32_t ramdisk_offset;
    uint32_t second_offset;
    uint32_t tags_offset;
    uint32_t page_size;
    std::vector<unsigned char> kernel_image;
    std::vector<unsigned char> ramdisk_image;
    std::vector<unsigned char> second_image;
    std::vector<unsigned char> dt_image;

    // Arguments with no short options
    enum pack_options : int
    {
        // Paths
        OPT_INPUT_CMDLINE        = 10000 + 1,
        OPT_INPUT_BASE           = 10000 + 2,
        OPT_INPUT_KERNEL_OFFSET  = 10000 + 3,
        OPT_INPUT_RAMDISK_OFFSET = 10000 + 4,
        OPT_INPUT_SECOND_OFFSET  = 10000 + 5,
        OPT_INPUT_TAGS_OFFSET    = 10000 + 6,
        OPT_INPUT_PAGE_SIZE      = 10000 + 7,
        OPT_INPUT_KERNEL         = 10000 + 8,
        OPT_INPUT_RAMDISK        = 10000 + 9,
        OPT_INPUT_SECOND         = 10000 + 10,
        OPT_INPUT_DT             = 10000 + 11,
        // Values
        OPT_VALUE_CMDLINE        = 20000 + 1,
        OPT_VALUE_BASE           = 20000 + 2,
        OPT_VALUE_KERNEL_OFFSET  = 20000 + 3,
        OPT_VALUE_RAMDISK_OFFSET = 20000 + 4,
        OPT_VALUE_SECOND_OFFSET  = 20000 + 5,
        OPT_VALUE_TAGS_OFFSET    = 20000 + 6,
        OPT_VALUE_PAGE_SIZE      = 20000 + 7
    };

    static struct option long_options[] = {
        // Arguments with short versions
        {"input",                required_argument, 0, 'i'},
        {"prefix",               required_argument, 0, 'p'},
        {"noprefix",             required_argument, 0, 'n'},
        // Arguments without short versions
        {"input-cmdline",        required_argument, 0, OPT_INPUT_CMDLINE},
        {"input-base",           required_argument, 0, OPT_INPUT_BASE},
        {"input-kernel_offset",  required_argument, 0, OPT_INPUT_KERNEL_OFFSET},
        {"input-ramdisk_offset", required_argument, 0, OPT_INPUT_RAMDISK_OFFSET},
        {"input-second_offset",  required_argument, 0, OPT_INPUT_SECOND_OFFSET},
        {"input-tags_offset",    required_argument, 0, OPT_INPUT_TAGS_OFFSET},
        {"input-page_size",      required_argument, 0, OPT_INPUT_PAGE_SIZE},
        {"input-kernel",         required_argument, 0, OPT_INPUT_KERNEL},
        {"input-ramdisk",        required_argument, 0, OPT_INPUT_RAMDISK},
        {"input-second",         required_argument, 0, OPT_INPUT_SECOND},
        {"input-dt",             required_argument, 0, OPT_INPUT_DT},
        // Value arguments
        {"value-cmdline",        required_argument, 0, OPT_VALUE_CMDLINE},
        {"value-base",           required_argument, 0, OPT_VALUE_BASE},
        {"value-kernel_offset",  required_argument, 0, OPT_VALUE_KERNEL_OFFSET},
        {"value-ramdisk_offset", required_argument, 0, OPT_VALUE_RAMDISK_OFFSET},
        {"value-second_offset",  required_argument, 0, OPT_VALUE_SECOND_OFFSET},
        {"value-tags_offset",    required_argument, 0, OPT_VALUE_TAGS_OFFSET},
        {"value-page_size",      required_argument, 0, OPT_VALUE_PAGE_SIZE},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, "i:p:n", long_options, &long_index)) != -1) {
        switch (opt) {
        case 'i':                      input_dir = optarg;           break;
        case 'p':                      prefix = optarg;              break;
        case 'n':                      no_prefix = true;             break;
        case OPT_INPUT_CMDLINE:        path_cmdline = optarg;        break;
        case OPT_INPUT_BASE:           path_base = optarg;           break;
        case OPT_INPUT_KERNEL_OFFSET:  path_kernel_offset = optarg;  break;
        case OPT_INPUT_RAMDISK_OFFSET: path_ramdisk_offset = optarg; break;
        case OPT_INPUT_SECOND_OFFSET:  path_second_offset = optarg;  break;
        case OPT_INPUT_TAGS_OFFSET:    path_tags_offset = optarg;    break;
        case OPT_INPUT_PAGE_SIZE:      path_page_size = optarg;      break;
        case OPT_INPUT_KERNEL:         path_kernel = optarg;         break;
        case OPT_INPUT_RAMDISK:        path_ramdisk = optarg;        break;
        case OPT_INPUT_SECOND:         path_second = optarg;         break;
        case OPT_INPUT_DT:             path_dt = optarg;             break;

        case OPT_VALUE_CMDLINE:
            path_cmdline.clear();
            values[opt] = true;
            cmdline = optarg;
            break;

        case OPT_VALUE_BASE:
            path_base.clear();
            values[opt] = true;
            if (!str_to_uint32(&base, optarg, 16)) {
                fprintf(stderr, "Invalid base: %s\n", optarg);
                return false;
            }
            break;

        case OPT_VALUE_KERNEL_OFFSET:
            path_kernel_offset.clear();
            values[opt] = true;
            if (!str_to_uint32(&kernel_offset, optarg, 16)) {
                fprintf(stderr, "Invalid kernel_offset: %s\n", optarg);
                return false;
            }
            break;

        case OPT_VALUE_RAMDISK_OFFSET:
            path_ramdisk_offset.clear();
            values[opt] = true;
            if (!str_to_uint32(&ramdisk_offset, optarg, 16)) {
                fprintf(stderr, "Invalid ramdisk_offset: %s\n", optarg);
                return false;
            }
            break;

        case OPT_VALUE_SECOND_OFFSET:
            path_second_offset.clear();
            values[opt] = true;
            if (!str_to_uint32(&second_offset, optarg, 16)) {
                fprintf(stderr, "Invalid second_offset: %s\n", optarg);
                return false;
            }
            break;

        case OPT_VALUE_TAGS_OFFSET:
            path_tags_offset.clear();
            values[opt] = true;
            if (!str_to_uint32(&tags_offset, optarg, 16)) {
                fprintf(stderr, "Invalid tags_offset: %s\n", optarg);
                return false;
            }
            break;

        case OPT_VALUE_PAGE_SIZE:
            path_page_size.clear();
            values[opt] = true;
            if (!str_to_uint32(&page_size, optarg, 16)) {
                fprintf(stderr, "Invalid page_size: %s\n", optarg);
                return false;
            }
            break;

        case 'h':
            fprintf(stdout, PackUsage);
            return true;

        default:
            fprintf(stderr, PackUsage);
            return false;
        }
    }

    // There should be one other arguments
    if (argc - optind != 1) {
        fprintf(stderr, PackUsage);
        return false;
    }

    output_file = argv[optind];

    if (no_prefix) {
        prefix.clear();
    } else {
        if (prefix.empty()) {
            prefix = bf::path(output_file).filename().string();
        }
        prefix += "-";
    }

    bf::path bf_input_dir(input_dir);

    if (path_cmdline.empty() && !values[OPT_VALUE_CMDLINE])
        path_cmdline = (bf_input_dir / (prefix + "cmdline")).string();
    if (path_base.empty() && !values[OPT_VALUE_BASE])
        path_base = (bf_input_dir / (prefix + "base")).string();
    if (path_kernel_offset.empty() && !values[OPT_VALUE_KERNEL_OFFSET])
        path_kernel_offset = (bf_input_dir / (prefix + "kernel_offset")).string();
    if (path_ramdisk_offset.empty() && !values[OPT_VALUE_RAMDISK_OFFSET])
        path_ramdisk_offset = (bf_input_dir / (prefix + "ramdisk_offset")).string();
    if (path_second_offset.empty() && !values[OPT_VALUE_SECOND_OFFSET])
        path_second_offset = (bf_input_dir / (prefix + "second_offset")).string();
    if (path_tags_offset.empty() && !values[OPT_VALUE_TAGS_OFFSET])
        path_tags_offset = (bf_input_dir / (prefix + "tags_offset")).string();
    if (path_page_size.empty() && !values[OPT_VALUE_PAGE_SIZE])
        path_page_size = (bf_input_dir / (prefix + "page_size")).string();
    if (path_kernel.empty())
        path_kernel = (bf_input_dir / (prefix + "kernel")).string();
    if (path_ramdisk.empty())
        path_ramdisk = (bf_input_dir / (prefix + "ramdisk")).string();
    if (path_second.empty())
        path_second = (bf_input_dir / (prefix + "second")).string();
    if (path_dt.empty())
        path_dt = (bf_input_dir / (prefix + "dt")).string();

    printf("Input files:\n");
    if (!path_cmdline.empty())
        printf("- cmdline:        (path)  %s\n", path_cmdline.c_str());
    else
        printf("- cmdline:        (value) %s\n", cmdline.c_str());
    if (!path_base.empty())
        printf("- base:           (path)  %s\n", path_base.c_str());
    else
        printf("- base:           (value) %08x\n", base);
    if (!path_kernel_offset.empty())
        printf("- kernel_offset:  (path)  %s\n", path_kernel_offset.c_str());
    else
        printf("- kernel_offset:  (value) %08x\n", kernel_offset);
    if (!path_ramdisk_offset.empty())
        printf("- ramdisk_offset: (path)  %s\n", path_ramdisk_offset.c_str());
    else
        printf("- ramdisk_offset: (value) %08x\n", ramdisk_offset);
    if (!path_second_offset.empty())
        printf("- second_offset:  (path)  %s\n", path_second_offset.c_str());
    else
        printf("- second_offset:  (value) %08x\n", second_offset);
    if (!path_tags_offset.empty())
        printf("- tags_offset:    (path)  %s\n", path_tags_offset.c_str());
    else
        printf("- tags_offset:    (value) %08x\n", tags_offset);
    if (!path_page_size.empty())
        printf("- page_size:      (path)  %s\n", path_page_size.c_str());
    else
        printf("- page_size:      (value) %u\n", page_size);
    printf("- kernel:         (path)  %s\n", path_kernel.c_str());
    printf("- ramdisk:        (path)  %s\n", path_ramdisk.c_str());
    printf("- second:         (path)  %s\n", path_second.c_str());
    printf("- dt:             (path)  %s\n", path_dt.c_str());
    printf("\n");

    if (!bf::exists(bf_input_dir)) {
        fprintf(stderr, "%s does not exist\n", input_dir.c_str());
        return false;
    }

    // Create new boot image
    mbp::BootImage bi;

    /* Load all the stuff! */

    // Read kernel command line
    if (!values[OPT_VALUE_CMDLINE]) {
        file_ptr fp(fopen(path_cmdline.c_str(), "rb"), fclose);
        if (fp) {
            char buf[mbp::BootImage::BootArgsSize];
            if (!fgets(buf, mbp::BootImage::BootArgsSize, fp.get())) {
                if (ferror(fp.get())) {
                    fprintf(stderr, "%s: %s\n",
                            path_cmdline.c_str(), strerror(errno));
                }
            }
            cmdline = buf;
            auto pos = cmdline.find('\n');
            if (pos != std::string::npos) {
                cmdline.erase(pos);
            }
        } else {
            cmdline = mbp::BootImage::DefaultCmdline;
        }
    }

    // Read base address on which the offsets are applied
    if (!values[OPT_VALUE_BASE]) {
        file_ptr fp(fopen(path_base.c_str(), "rb"), fclose);
        if (fp) {
            int count = fscanf(fp.get(), "%08x", &base);
            if (count == EOF && ferror(fp.get())) {
                fprintf(stderr, "%s: %s\n",
                        path_base.c_str(), strerror(errno));
                return false;
            } else if (count != 1) {
                fprintf(stderr, "%s: Error: expected '%%08x' format\n",
                        path_base.c_str());
                return false;
            }
        } else {
            base = mbp::BootImage::DefaultBase;
        }
    }

    // Read kernel offset
    if (!values[OPT_VALUE_KERNEL_OFFSET]) {
        file_ptr fp(fopen(path_kernel_offset.c_str(), "rb"), fclose);
        if (fp) {
            int count = fscanf(fp.get(), "%08x", &kernel_offset);
            if (count == EOF && ferror(fp.get())) {
                fprintf(stderr, "%s: %s\n",
                        path_kernel_offset.c_str(), strerror(errno));
                return false;
            } else if (count != 1) {
                fprintf(stderr, "%s: Error: expected '%%08x' format\n",
                        path_kernel_offset.c_str());
                return false;
            }
        } else {
            kernel_offset = mbp::BootImage::DefaultKernelOffset;
        }
    }

    // Read ramdisk offset
    if (!values[OPT_VALUE_RAMDISK_OFFSET]) {
        file_ptr fp(fopen(path_ramdisk_offset.c_str(), "rb"), fclose);
        if (fp) {
            int count = fscanf(fp.get(), "%08x", &ramdisk_offset);
            if (count == EOF && ferror(fp.get())) {
                fprintf(stderr, "%s: %s\n",
                        path_ramdisk_offset.c_str(), strerror(errno));
                return false;
            } else if (count != 1) {
                fprintf(stderr, "%s: Error: expected '%%08x' format\n",
                        path_ramdisk_offset.c_str());
                return false;
            }
        } else {
            ramdisk_offset = mbp::BootImage::DefaultRamdiskOffset;
        }
    }

    // Read second bootloader offset
    if (!values[OPT_VALUE_SECOND_OFFSET]) {
        file_ptr fp(fopen(path_second_offset.c_str(), "rb"), fclose);
        if (fp) {
            int count = fscanf(fp.get(), "%08x", &second_offset);
            if (count == EOF && ferror(fp.get())) {
                fprintf(stderr, "%s: %s\n",
                        path_second_offset.c_str(), strerror(errno));
                return false;
            } else if (count != 1) {
                fprintf(stderr, "%s: Error: expected '%%08x' format\n",
                        path_second_offset.c_str());
                return false;
            }
        } else {
            second_offset = mbp::BootImage::DefaultSecondOffset;
        }
    }

    // Read kernel tags offset
    if (!values[OPT_VALUE_TAGS_OFFSET]) {
        file_ptr fp(fopen(path_tags_offset.c_str(), "rb"), fclose);
        if (fp) {
            int count = fscanf(fp.get(), "%08x", &tags_offset);
            if (count == EOF && ferror(fp.get())) {
                fprintf(stderr, "%s: %s\n",
                        path_tags_offset.c_str(), strerror(errno));
                return false;
            } else if (count != 1) {
                fprintf(stderr, "%s: Error: expected '%%08x' format\n",
                        path_tags_offset.c_str());
                return false;
            }
        } else {
            tags_offset = mbp::BootImage::DefaultTagsOffset;
        }
    }

    // Read page size
    if (!values[OPT_VALUE_PAGE_SIZE]) {
        file_ptr fp(fopen(path_page_size.c_str(), "rb"), fclose);
        if (fp) {
            int count = fscanf(fp.get(), "%u", &page_size);
            if (count == EOF && ferror(fp.get())) {
                fprintf(stderr, "%s: %s\n",
                        path_page_size.c_str(), strerror(errno));
                return false;
            } else if (count != 1) {
                fprintf(stderr, "%s: Error: expected '%%u' format\n",
                        path_page_size.c_str());
                return false;
            }
        } else {
            page_size = mbp::BootImage::DefaultPageSize;
        }
    }

    // Read kernel image
    if (!read_file_data(path_kernel, &kernel_image)) {
        fprintf(stderr, "%s: %s\n", path_kernel.c_str(), strerror(errno));
        return false;
    }

    // Read ramdisk image
    if (!read_file_data(path_ramdisk, &ramdisk_image)) {
        fprintf(stderr, "%s: %s\n", path_ramdisk.c_str(), strerror(errno));
        return false;
    }

    // Read second bootloader image
    if (!read_file_data(path_second, &second_image) && errno != ENOENT) {
        fprintf(stderr, "%s: %s\n", path_second.c_str(), strerror(errno));
        return false;
    }

    // Read device tree image
    if (!read_file_data(path_dt, &dt_image) && errno != ENOENT) {
        fprintf(stderr, "%s: %s\n", path_dt.c_str(), strerror(errno));
        return false;
    }

    bi.setKernelCmdline(std::move(cmdline));
    bi.setAddresses(base, kernel_offset, ramdisk_offset, second_offset, tags_offset);
    bi.setPageSize(page_size);
    bi.setKernelImage(std::move(kernel_image));
    bi.setRamdiskImage(std::move(ramdisk_image));
    bi.setSecondBootloaderImage(std::move(second_image));
    bi.setDeviceTreeImage(std::move(dt_image));

    // Create boot image
    if (!bi.createFile(output_file)) {
        fprintf(stderr, "Failed to create boot image\n");
        return false;
    }

    printf("\nDone\n");

    return true;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stdout, MainUsage);
        return EXIT_FAILURE;
    }

    mbp::setLogCallback(mbp_log_cb);

    std::string command(argv[1]);
    bool ret = false;

    if (command == "unpack") {
        ret = unpack_main(--argc, ++argv);
    } else if (command == "pack") {
        ret = pack_main(--argc, ++argv);
    } else {
        fprintf(stderr, MainUsage);
        return EXIT_FAILURE;
    }

    return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
