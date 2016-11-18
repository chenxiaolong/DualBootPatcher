/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
#include <cstdlib>
#include <cstring>

#include <getopt.h>

#include <mblog/logging.h>

#include <mbpio/directory.h>
#include <mbpio/error.h>
#include <mbpio/path.h>

#include <mbp/bootimage.h>
#include <mbp/errors.h>


typedef std::unique_ptr<std::FILE, int (*)(std::FILE *)> file_ptr;


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
    "  cmdline         Kernel command line                           [ABLMS]\n"
    "  board           Board name field in the header                [ABLM ]\n"
    "  base            Base address for offsets                      [ABLM ]\n"
    "  kernel_offset   Address offset of the kernel image            [ABLMS]\n"
    "  ramdisk_offset  Address offset of the ramdisk image           [ABLMS]\n"
    "  second_offset   Address offset of the second bootloader image [ABLM ]\n"
    "  tags_offset     Address offset of the kernel tags image       [ABLM ]\n"
    "  ipl_address     Address of the ipl image                      [    S]\n"
    "  rpm_address     Address of the rpm image                      [    S]\n"
    "  appsbl_address  Address of the appsbl image                   [    S]\n"
    "  entrypoint      Address of the entry point                    [    S]\n"
    "  page_size       Page size                                     [ABLM ]\n"
    "  kernel`         Kernel image                                  [ABLMS]\n"
    "  ramdisk         Ramdisk image                                 [ABLMS]\n"
    "  second          Second bootloader image                       [ABLM ]\n"
    "  dt              Device tree image                             [ABLM ]\n"
    "  kernel_mtkhdr   Kernel MTK header                             [   M ]\n"
    "  ramdisk_mtkhdr  Ramdisk MTK header                            [   M ]\n"
    "  ipl             Ipl image                                     [    S]\n"
    "  rpm             Rpm image                                     [    S]\n"
    "  appsbl          Appsbl image                                  [    S]\n"
    "  sin             Sin image                                     [    S]\n"
    "  sinhdr          Sin header                                    [    S]\n"
    "\n"
    "Legend:\n"
    "  [A B L M S]\n"
    "   | | | | `- Used by Sony ELF boot images\n"
    "   | | | `- Used by MTK Android boot images\n"
    "   | | `- Used by Loki'd Android boot images\n"
    "   | `- Used by bump'd Android boot images\n"
    "   `- Used by plain Android boot images\n"
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
    "  -t, --type [type]\n"
    "                  Output type of the boot image\n"
    "                  [android, bump, loki, mtk, sonyelf]\n"
    "  --input-[item] [item path]\n"
    "                  Custom path for a particular item\n"
    "  --value-[item] [item value]\n"
    "                  Specify a value for an item directly\n"
    "\n"
    "The following items are loaded to create a new boot image.\n"
    "\n"
    "  cmdline *        Kernel command line                           [ABLMS]\n"
    "  board *          Board name field in the header                [ABLM ]\n"
    "  base *           Base address for offsets                      [ABLM ]\n"
    "  kernel_offset *  Address offset of the kernel image            [ABLMS]\n"
    "  ramdisk_offset * Address offset of the ramdisk image           [ABLMS]\n"
    "  second_offset *  Address offset of the second bootloader image [ABLM ]\n"
    "  tags_offset *    Address offset of the kernel tags image       [ABLM ]\n"
    "  ipl_address *    Address of the ipl image                      [    S]\n"
    "  rpm_address *    Address of the rpm image                      [    S]\n"
    "  appsbl_address * Address of the appsbl image                   [    S]\n"
    "  entrypoint *     Address of the entrypoint                     [    S]\n"
    "  page_size *      Page size                                     [ABLM ]\n"
    "  kernel`          Kernel image                                  [ABLMS]\n"
    "  ramdisk          Ramdisk image                                 [ABLMS]\n"
    "  second           Second bootloader image                       [ABLM ]\n"
    "  dt               Device tree image                             [ABLM ]\n"
    "  kernel_mtkhdr    Kernel MTK header                             [   M ]\n"
    "  ramdisk_mtkhdr   Ramdisk MTK header                            [   M ]\n"
    "  aboot            Aboot image                                   [  L  ]\n"
    "  ipl              Ipl image                                     [    S]\n"
    "  rpm              Rpm image                                     [    S]\n"
    "  appsbl           Appsbl image                                  [    S]\n"
    "  sin              Sin image                                     [    S]\n"
    "  sinhdr           Sin header                                    [    S]\n"
    "\n"
    "Legend:\n"
    "  [A B L M S]\n"
    "   | | | | `- Used by Sony ELF boot images\n"
    "   | | | `- Used by MTK Android boot images\n"
    "   | | `- Used by Loki'd Android boot images\n"
    "   | `- Used by bump'd Android boot images\n"
    "   `- Used by plain Android boot images\n"
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


static std::string error_to_string(const mbp::ErrorCode &error) {
    switch (error) {
    case mbp::ErrorCode::MemoryAllocationError:
        return "Failed to allocate memory";
    case mbp::ErrorCode::FileOpenError:
        return "Failed to open file";
    case mbp::ErrorCode::FileCloseError:
        return "Failed to close file";
    case mbp::ErrorCode::FileReadError:
        return "Failed to read from file";
    case mbp::ErrorCode::FileWriteError:
        return "Failed to write to file";
    case mbp::ErrorCode::FileSeekError:
        return "Failed to seek file";
    case mbp::ErrorCode::FileTellError:
        return "Failed to get file position";
    case mbp::ErrorCode::BootImageParseError:
        return "Failed to parse boot image";
    case mbp::ErrorCode::BootImageApplyBumpError:
        return "Failed to apply Bump to boot image";
    case mbp::ErrorCode::BootImageApplyLokiError:
        return "Failed to apply Loki to boot image";
    case mbp::ErrorCode::CpioFileAlreadyExistsError:
        return "File already exists in cpio archive";
    case mbp::ErrorCode::CpioFileNotExistError:
        return "File does not exist in cpio archive";
    case mbp::ErrorCode::ArchiveReadOpenError:
        return "Failed to open archive for reading";
    case mbp::ErrorCode::ArchiveReadDataError:
        return "Failed to read archive data for file";
    case mbp::ErrorCode::ArchiveReadHeaderError:
        return "Failed to read archive entry header";
    case mbp::ErrorCode::ArchiveWriteOpenError:
        return "Failed to open archive for writing";
    case mbp::ErrorCode::ArchiveWriteDataError:
        return "Failed to write archive data for file";
    case mbp::ErrorCode::ArchiveWriteHeaderError:
        return "Failed to write archive header for file";
    case mbp::ErrorCode::ArchiveCloseError:
        return "Failed to close archive";
    case mbp::ErrorCode::ArchiveFreeError:
        return "Failed to free archive header memory";
    case mbp::ErrorCode::NoError:
    case mbp::ErrorCode::PatcherCreateError:
    case mbp::ErrorCode::AutoPatcherCreateError:
    case mbp::ErrorCode::RamdiskPatcherCreateError:
    case mbp::ErrorCode::PatchingCancelled:
    case mbp::ErrorCode::BootImageTooLargeError:
    default:
        assert(false);
    }

    return std::string();
}

class BasicLogger : public mb::log::BaseLogger
{
public:
    virtual void log(mb::log::LogLevel prio, const char *fmt, va_list ap) override
    {
        (void) prio;
        vprintf(fmt, ap);
        printf("\n");
    }
};

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
    std::string path_board;
    std::string path_base;
    std::string path_kernel_offset;
    std::string path_ramdisk_offset;
    std::string path_second_offset;
    std::string path_tags_offset;
    std::string path_ipl_address;
    std::string path_rpm_address;
    std::string path_appsbl_address;
    std::string path_entrypoint;
    std::string path_page_size;
    std::string path_kernel;
    std::string path_ramdisk;
    std::string path_second;
    std::string path_dt;
    std::string path_kernel_mtkhdr;
    std::string path_ramdisk_mtkhdr;
    std::string path_ipl;
    std::string path_rpm;
    std::string path_appsbl;
    std::string path_sin;
    std::string path_sinhdr;

    // Arguments with no short options
    enum unpack_options : int
    {
        OPT_OUTPUT_CMDLINE        = 10000 + 1,
        OPT_OUTPUT_BOARD          = 10000 + 2,
        OPT_OUTPUT_BASE           = 10000 + 3,
        OPT_OUTPUT_KERNEL_OFFSET  = 10000 + 4,
        OPT_OUTPUT_RAMDISK_OFFSET = 10000 + 5,
        OPT_OUTPUT_SECOND_OFFSET  = 10000 + 6,
        OPT_OUTPUT_TAGS_OFFSET    = 10000 + 7,
        OPT_OUTPUT_IPL_ADDRESS    = 10000 + 8,
        OPT_OUTPUT_RPM_ADDRESS    = 10000 + 9,
        OPT_OUTPUT_APPSBL_ADDRESS = 10000 + 10,
        OPT_OUTPUT_ENTRYPOINT     = 10000 + 11,
        OPT_OUTPUT_PAGE_SIZE      = 10000 + 12,
        OPT_OUTPUT_KERNEL         = 10000 + 13,
        OPT_OUTPUT_RAMDISK        = 10000 + 14,
        OPT_OUTPUT_SECOND         = 10000 + 15,
        OPT_OUTPUT_DT             = 10000 + 16,
        OPT_OUTPUT_KERNEL_MTKHDR  = 10000 + 17,
        OPT_OUTPUT_RAMDISK_MTKHDR = 10000 + 18,
        OPT_OUTPUT_IPL            = 10000 + 19,
        OPT_OUTPUT_RPM            = 10000 + 20,
        OPT_OUTPUT_APPSBL         = 10000 + 21,
        OPT_OUTPUT_SIN            = 10000 + 22,
        OPT_OUTPUT_SINHDR         = 10000 + 23
    };

    static struct option long_options[] = {
        // Arguments with short versions
        {"output",                required_argument, 0, 'o'},
        {"prefix",                required_argument, 0, 'p'},
        {"noprefix",              required_argument, 0, 'n'},
        // Arguments without short versions
        {"output-cmdline",        required_argument, 0, OPT_OUTPUT_CMDLINE},
        {"output-board",          required_argument, 0, OPT_OUTPUT_BOARD},
        {"output-base",           required_argument, 0, OPT_OUTPUT_BASE},
        {"output-kernel_offset",  required_argument, 0, OPT_OUTPUT_KERNEL_OFFSET},
        {"output-ramdisk_offset", required_argument, 0, OPT_OUTPUT_RAMDISK_OFFSET},
        {"output-second_offset",  required_argument, 0, OPT_OUTPUT_SECOND_OFFSET},
        {"output-tags_offset",    required_argument, 0, OPT_OUTPUT_TAGS_OFFSET},
        {"output-ipl_address",    required_argument, 0, OPT_OUTPUT_IPL_ADDRESS},
        {"output-rpm_address",    required_argument, 0, OPT_OUTPUT_RPM_ADDRESS},
        {"output-appsbl_address", required_argument, 0, OPT_OUTPUT_APPSBL_ADDRESS},
        {"output-entrypoint",     required_argument, 0, OPT_OUTPUT_ENTRYPOINT},
        {"output-page_size",      required_argument, 0, OPT_OUTPUT_PAGE_SIZE},
        {"output-kernel",         required_argument, 0, OPT_OUTPUT_KERNEL},
        {"output-ramdisk",        required_argument, 0, OPT_OUTPUT_RAMDISK},
        {"output-second",         required_argument, 0, OPT_OUTPUT_SECOND},
        {"output-dt",             required_argument, 0, OPT_OUTPUT_DT},
        {"output-kernel_mtkhdr",  required_argument, 0, OPT_OUTPUT_KERNEL_MTKHDR},
        {"output-ramdisk_mtkhdr", required_argument, 0, OPT_OUTPUT_RAMDISK_MTKHDR},
        {"output-ipl",            required_argument, 0, OPT_OUTPUT_IPL},
        {"output-rpm",            required_argument, 0, OPT_OUTPUT_RPM},
        {"output-appsbl",         required_argument, 0, OPT_OUTPUT_APPSBL},
        {"output-sin",            required_argument, 0, OPT_OUTPUT_SIN},
        {"output-sinhdr",         required_argument, 0, OPT_OUTPUT_SINHDR},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, "o:p:n", long_options, &long_index)) != -1) {
        switch (opt) {
        case 'o':                       output_dir = optarg;          break;
        case 'p':                       prefix = optarg;              break;
        case 'n':                       no_prefix = true;             break;
        case OPT_OUTPUT_CMDLINE:        path_cmdline = optarg;        break;
        case OPT_OUTPUT_BOARD:          path_board = optarg;          break;
        case OPT_OUTPUT_BASE:           path_base = optarg;           break;
        case OPT_OUTPUT_KERNEL_OFFSET:  path_kernel_offset = optarg;  break;
        case OPT_OUTPUT_RAMDISK_OFFSET: path_ramdisk_offset = optarg; break;
        case OPT_OUTPUT_SECOND_OFFSET:  path_second_offset = optarg;  break;
        case OPT_OUTPUT_TAGS_OFFSET:    path_tags_offset = optarg;    break;
        case OPT_OUTPUT_IPL_ADDRESS:    path_ipl_address = optarg;    break;
        case OPT_OUTPUT_RPM_ADDRESS:    path_rpm_address = optarg;    break;
        case OPT_OUTPUT_APPSBL_ADDRESS: path_appsbl_address = optarg; break;
        case OPT_OUTPUT_ENTRYPOINT:     path_entrypoint = optarg;     break;
        case OPT_OUTPUT_PAGE_SIZE:      path_page_size = optarg;      break;
        case OPT_OUTPUT_KERNEL:         path_kernel = optarg;         break;
        case OPT_OUTPUT_RAMDISK:        path_ramdisk = optarg;        break;
        case OPT_OUTPUT_SECOND:         path_second = optarg;         break;
        case OPT_OUTPUT_DT:             path_dt = optarg;             break;
        case OPT_OUTPUT_KERNEL_MTKHDR:  path_kernel_mtkhdr = optarg;  break;
        case OPT_OUTPUT_RAMDISK_MTKHDR: path_ramdisk_mtkhdr = optarg; break;
        case OPT_OUTPUT_IPL:            path_ipl = optarg;            break;
        case OPT_OUTPUT_RPM:            path_rpm = optarg;            break;
        case OPT_OUTPUT_APPSBL:         path_appsbl = optarg;         break;
        case OPT_OUTPUT_SIN:            path_sin = optarg;            break;
        case OPT_OUTPUT_SINHDR:         path_sinhdr = optarg;         break;

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
            prefix = io::baseName(input_file);
        }
        prefix += "-";
    }

    if (output_dir.empty()) {
        output_dir = ".";
    }

    if (path_cmdline.empty())
        path_cmdline = io::pathJoin({output_dir, prefix + "cmdline"});
    if (path_board.empty())
        path_board = io::pathJoin({output_dir, prefix + "board"});
    if (path_base.empty())
        path_base = io::pathJoin({output_dir, prefix + "base"});
    if (path_kernel_offset.empty())
        path_kernel_offset = io::pathJoin({output_dir, prefix + "kernel_offset"});
    if (path_ramdisk_offset.empty())
        path_ramdisk_offset = io::pathJoin({output_dir, prefix + "ramdisk_offset"});
    if (path_second_offset.empty())
        path_second_offset = io::pathJoin({output_dir, prefix + "second_offset"});
    if (path_tags_offset.empty())
        path_tags_offset = io::pathJoin({output_dir, prefix + "tags_offset"});
    if (path_ipl_address.empty())
        path_ipl_address = io::pathJoin({output_dir, prefix + "ipl_address"});
    if (path_rpm_address.empty())
        path_rpm_address = io::pathJoin({output_dir, prefix + "rpm_address"});
    if (path_appsbl_address.empty())
        path_appsbl_address = io::pathJoin({output_dir, prefix + "appsbl_address"});
    if (path_entrypoint.empty())
        path_entrypoint = io::pathJoin({output_dir, prefix + "entrypoint"});
    if (path_page_size.empty())
        path_page_size = io::pathJoin({output_dir, prefix + "page_size"});
    if (path_kernel.empty())
        path_kernel = io::pathJoin({output_dir, prefix + "kernel"});
    if (path_ramdisk.empty())
        path_ramdisk = io::pathJoin({output_dir, prefix + "ramdisk"});
    if (path_second.empty())
        path_second = io::pathJoin({output_dir, prefix + "second"});
    if (path_dt.empty())
        path_dt = io::pathJoin({output_dir, prefix + "dt"});
    if (path_kernel_mtkhdr.empty())
        path_kernel_mtkhdr = io::pathJoin({output_dir, prefix + "kernel_mtkhdr"});
    if (path_ramdisk_mtkhdr.empty())
        path_ramdisk_mtkhdr = io::pathJoin({output_dir, prefix + "ramdisk_mtkhdr"});
    if (path_ipl.empty())
        path_ipl = io::pathJoin({output_dir, prefix + "ipl"});
    if (path_rpm.empty())
        path_rpm = io::pathJoin({output_dir, prefix + "rpm"});
    if (path_appsbl.empty())
        path_appsbl = io::pathJoin({output_dir, prefix + "appsbl"});
    if (path_sin.empty())
        path_sin = io::pathJoin({output_dir, prefix + "sin"});
    if (path_sinhdr.empty())
        path_sinhdr = io::pathJoin({output_dir, prefix + "sinhdr"});

    if (!io::createDirectories(output_dir)) {
        fprintf(stderr, "%s: Failed to create directory: %s\n",
                output_dir.c_str(), io::lastErrorString().c_str());
        return false;
    }

    // Load the boot image
    mbp::BootImage bi;
    if (!bi.loadFile(input_file)) {
        fprintf(stderr, "%s\n", error_to_string(bi.error()).c_str());
        return false;
    }

    uint64_t supportMask = mbp::BootImage::typeSupportMask(bi.wasType());
    printf("\nOutput files:\n");
#define PRINT_IF(supported, fmt, ...) \
    if (supportMask & (supported)) { \
        printf(fmt, __VA_ARGS__); \
    }
    PRINT_IF(SUPPORTS_CMDLINE,         "- cmdline:        %s\n", path_cmdline.c_str());
    PRINT_IF(SUPPORTS_BOARD_NAME,      "- board:          %s\n", path_board.c_str());
    PRINT_IF(SUPPORTS_OFFSET_BASE,     "- base:           %s\n", path_base.c_str());
    PRINT_IF(SUPPORTS_KERNEL_ADDRESS,  "- kernel_offset:  %s\n", path_kernel_offset.c_str());
    PRINT_IF(SUPPORTS_RAMDISK_ADDRESS, "- ramdisk_offset: %s\n", path_ramdisk_offset.c_str());
    PRINT_IF(SUPPORTS_SECOND_ADDRESS,  "- second_offset:  %s\n", path_second_offset.c_str());
    PRINT_IF(SUPPORTS_TAGS_ADDRESS,    "- tags_offset:    %s\n", path_tags_offset.c_str());
    PRINT_IF(SUPPORTS_IPL_ADDRESS,     "- ipl_address:    %s\n", path_ipl_address.c_str());
    PRINT_IF(SUPPORTS_RPM_ADDRESS,     "- rpm_address:    %s\n", path_rpm_address.c_str());
    PRINT_IF(SUPPORTS_APPSBL_ADDRESS,  "- appsbl_address: %s\n", path_appsbl_address.c_str());
    PRINT_IF(SUPPORTS_ENTRYPOINT,      "- entrypoint:     %s\n", path_entrypoint.c_str());
    PRINT_IF(SUPPORTS_PAGE_SIZE,       "- page_size:      %s\n", path_page_size.c_str());
    PRINT_IF(SUPPORTS_KERNEL_IMAGE,    "- kernel:         %s\n", path_kernel.c_str());
    PRINT_IF(SUPPORTS_RAMDISK_IMAGE,   "- ramdisk:        %s\n", path_ramdisk.c_str());
    PRINT_IF(SUPPORTS_SECOND_IMAGE,    "- second:         %s\n", path_second.c_str());
    PRINT_IF(SUPPORTS_DT_IMAGE,        "- dt:             %s\n", path_dt.c_str());
    PRINT_IF(SUPPORTS_KERNEL_MTKHDR,   "- kernel_mtkhdr:  %s\n", path_kernel_mtkhdr.c_str());
    PRINT_IF(SUPPORTS_RAMDISK_MTKHDR,  "- ramdisk_mtkhdr: %s\n", path_ramdisk_mtkhdr.c_str());
    PRINT_IF(SUPPORTS_IPL_IMAGE,       "- ipl:            %s\n", path_ipl.c_str());
    PRINT_IF(SUPPORTS_RPM_IMAGE,       "- rpm:            %s\n", path_rpm.c_str());
    PRINT_IF(SUPPORTS_APPSBL_IMAGE,    "- appsbl:         %s\n", path_appsbl.c_str());
    PRINT_IF(SUPPORTS_SONY_SIN_IMAGE,  "- sin:            %s\n", path_sin.c_str());
    PRINT_IF(SUPPORTS_SONY_SIN_HEADER, "- sinhdr:         %s\n", path_sinhdr.c_str());
#undef PRINT_IF

    /* Extract all the stuff! */

    // Use base relative to the default kernel offset
    uint32_t base = bi.kernelAddress() - mbp::BootImage::AndroidDefaultKernelOffset;
    uint32_t kernel_offset = mbp::BootImage::AndroidDefaultKernelOffset;
    uint32_t ramdisk_offset = bi.ramdiskAddress() - base;
    uint32_t second_offset = bi.secondBootloaderAddress() - base;
    uint32_t tags_offset = bi.kernelTagsAddress() - base;

#define WRITE_FILE_FMT(file, fmt, ...) \
    if (!write_file_fmt(file, fmt, __VA_ARGS__)) { \
        fprintf(stderr, "%s: %s\n", (file).c_str(), strerror(errno)); \
        return false; \
    }

    // Write kernel command line
    if (supportMask & SUPPORTS_CMDLINE) {
        WRITE_FILE_FMT(path_cmdline, "%s\n", bi.kernelCmdlineC());
    }

    // Write board name field
    if (supportMask & SUPPORTS_BOARD_NAME) {
        WRITE_FILE_FMT(path_board, "%s\n", bi.boardNameC());
    }

    // Write base address on which the offsets are applied
    if (supportMask & SUPPORTS_OFFSET_BASE) {
        WRITE_FILE_FMT(path_base, "%08x\n", base);
    }

    // Write kernel offset
    if (supportMask & SUPPORTS_KERNEL_ADDRESS) {
        WRITE_FILE_FMT(path_kernel_offset, "%08x\n", kernel_offset);
    }

    // Write ramdisk offset
    if (supportMask & SUPPORTS_RAMDISK_ADDRESS) {
        WRITE_FILE_FMT(path_ramdisk_offset, "%08x\n", ramdisk_offset);
    }

    // Write second bootloader offset
    if (supportMask & SUPPORTS_SECOND_ADDRESS) {
        WRITE_FILE_FMT(path_second_offset, "%08x\n", second_offset);
    }

    // Write kernel tags offset
    if (supportMask & SUPPORTS_TAGS_ADDRESS) {
        WRITE_FILE_FMT(path_tags_offset, "%08x\n", tags_offset);
    }

    // Write ipl address
    if (supportMask & SUPPORTS_IPL_ADDRESS) {
        WRITE_FILE_FMT(path_ipl_address, "%08x\n", bi.iplAddress());
    }

    // Write rpm address
    if (supportMask & SUPPORTS_RPM_ADDRESS) {
        WRITE_FILE_FMT(path_rpm_address, "%08x\n", bi.rpmAddress());
    }

    // Write appsbl address
    if (supportMask & SUPPORTS_APPSBL_ADDRESS) {
        WRITE_FILE_FMT(path_appsbl_address, "%08x\n", bi.appsblAddress());
    }

    // Write entrypoint address
    if (supportMask & SUPPORTS_ENTRYPOINT) {
        WRITE_FILE_FMT(path_entrypoint, "%08x\n", bi.entrypointAddress());
    }

    // Write page size
    if (supportMask & SUPPORTS_PAGE_SIZE) {
        WRITE_FILE_FMT(path_page_size, "%u\n", bi.pageSize());
    }

#undef WRITE_FILE_FMT

#define WRITE_FILE_DATA(file, data) \
    if (!write_file_data(file, data)) { \
        fprintf(stderr, "%s: %s\n", (file).c_str(), strerror(errno)); \
        return false; \
    }

    // Write kernel image
    if (supportMask & SUPPORTS_KERNEL_IMAGE) {
        WRITE_FILE_DATA(path_kernel, bi.kernelImage());
    }

    // Write ramdisk image
    if (supportMask & SUPPORTS_RAMDISK_IMAGE) {
        WRITE_FILE_DATA(path_ramdisk, bi.ramdiskImage());
    }

    // Write second bootloader image
    if (supportMask & SUPPORTS_SECOND_IMAGE) {
        WRITE_FILE_DATA(path_second, bi.secondBootloaderImage());
    }

    // Write device tree image
    if (supportMask & SUPPORTS_DT_IMAGE) {
        WRITE_FILE_DATA(path_dt, bi.deviceTreeImage());
    }

    // Write MTK kernel header
    if (supportMask & SUPPORTS_KERNEL_MTKHDR) {
        WRITE_FILE_DATA(path_kernel_mtkhdr, bi.kernelMtkHeader());
    }

    // Write MTK ramdisk header
    if (supportMask & SUPPORTS_RAMDISK_MTKHDR) {
        WRITE_FILE_DATA(path_ramdisk_mtkhdr, bi.ramdiskMtkHeader());
    }

    // Write ipl image
    if (supportMask & SUPPORTS_IPL_IMAGE) {
        WRITE_FILE_DATA(path_ipl, bi.iplImage());
    }

    // Write rpm image
    if (supportMask & SUPPORTS_RPM_IMAGE) {
        WRITE_FILE_DATA(path_rpm, bi.rpmImage());
    }

    // Write appsbl image
    if (supportMask & SUPPORTS_APPSBL_IMAGE) {
        WRITE_FILE_DATA(path_appsbl, bi.appsblImage());
    }

    // Write sin image
    if (supportMask & SUPPORTS_SONY_SIN_IMAGE) {
        WRITE_FILE_DATA(path_sin, bi.sinImage());
    }

    // Write sinhdr image
    if (supportMask & SUPPORTS_SONY_SIN_HEADER) {
        WRITE_FILE_DATA(path_sinhdr, bi.sinHeader());
    }

#undef WRITE_FILE_DATA

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
    std::string path_board;
    std::string path_base;
    std::string path_kernel_offset;
    std::string path_ramdisk_offset;
    std::string path_second_offset;
    std::string path_tags_offset;
    std::string path_ipl_address;
    std::string path_rpm_address;
    std::string path_appsbl_address;
    std::string path_entrypoint;
    std::string path_page_size;
    std::string path_kernel;
    std::string path_ramdisk;
    std::string path_second;
    std::string path_dt;
    std::string path_aboot;
    std::string path_kernel_mtkhdr;
    std::string path_ramdisk_mtkhdr;
    std::string path_ipl;
    std::string path_rpm;
    std::string path_appsbl;
    std::string path_sin;
    std::string path_sinhdr;
    // Values
    std::unordered_map<int, bool> values;
    std::string cmdline;
    std::string board;
    uint32_t base;
    uint32_t kernel_offset;
    uint32_t ramdisk_offset;
    uint32_t second_offset;
    uint32_t tags_offset;
    uint32_t ipl_address;
    uint32_t rpm_address;
    uint32_t appsbl_address;
    uint32_t entrypoint;
    uint32_t page_size;
    std::vector<unsigned char> kernel_image;
    std::vector<unsigned char> ramdisk_image;
    std::vector<unsigned char> second_image;
    std::vector<unsigned char> dt_image;
    std::vector<unsigned char> aboot_image;
    std::vector<unsigned char> kernel_mtkhdr;
    std::vector<unsigned char> ramdisk_mtkhdr;
    std::vector<unsigned char> ipl_image;
    std::vector<unsigned char> rpm_image;
    std::vector<unsigned char> appsbl_image;
    std::vector<unsigned char> sin_image;
    std::vector<unsigned char> sin_header;
    mbp::BootImage::Type type = mbp::BootImage::Type::Android;

    // Arguments with no short options
    enum pack_options : int
    {
        // Paths
        OPT_INPUT_CMDLINE        = 10000 + 1,
        OPT_INPUT_BOARD          = 10000 + 2,
        OPT_INPUT_BASE           = 10000 + 3,
        OPT_INPUT_KERNEL_OFFSET  = 10000 + 4,
        OPT_INPUT_RAMDISK_OFFSET = 10000 + 5,
        OPT_INPUT_SECOND_OFFSET  = 10000 + 6,
        OPT_INPUT_TAGS_OFFSET    = 10000 + 7,
        OPT_INPUT_IPL_ADDRESS    = 10000 + 8,
        OPT_INPUT_RPM_ADDRESS    = 10000 + 9,
        OPT_INPUT_APPSBL_ADDRESS = 10000 + 10,
        OPT_INPUT_ENTRYPOINT     = 10000 + 11,
        OPT_INPUT_PAGE_SIZE      = 10000 + 12,
        OPT_INPUT_KERNEL         = 10000 + 13,
        OPT_INPUT_RAMDISK        = 10000 + 14,
        OPT_INPUT_SECOND         = 10000 + 15,
        OPT_INPUT_DT             = 10000 + 16,
        OPT_INPUT_ABOOT          = 10000 + 17,
        OPT_INPUT_KERNEL_MTKHDR  = 10000 + 18,
        OPT_INPUT_RAMDISK_MTKHDR = 10000 + 19,
        OPT_INPUT_IPL            = 10000 + 20,
        OPT_INPUT_RPM            = 10000 + 21,
        OPT_INPUT_APPSBL         = 10000 + 22,
        OPT_INPUT_SIN            = 10000 + 23,
        OPT_INPUT_SINHDR         = 10000 + 24,
        // Values
        OPT_VALUE_CMDLINE        = 20000 + 1,
        OPT_VALUE_BOARD          = 20000 + 2,
        OPT_VALUE_BASE           = 20000 + 3,
        OPT_VALUE_KERNEL_OFFSET  = 20000 + 4,
        OPT_VALUE_RAMDISK_OFFSET = 20000 + 5,
        OPT_VALUE_SECOND_OFFSET  = 20000 + 6,
        OPT_VALUE_TAGS_OFFSET    = 20000 + 7,
        OPT_VALUE_IPL_ADDRESS    = 20000 + 8,
        OPT_VALUE_RPM_ADDRESS    = 20000 + 9,
        OPT_VALUE_APPSBL_ADDRESS = 20000 + 10,
        OPT_VALUE_ENTRYPOINT     = 20000 + 11,
        OPT_VALUE_PAGE_SIZE      = 20000 + 12
    };

    static struct option long_options[] = {
        // Arguments with short versions
        {"input",                required_argument, 0, 'i'},
        {"prefix",               required_argument, 0, 'p'},
        {"noprefix",             required_argument, 0, 'n'},
        {"type",                 required_argument, 0, 't'},
        // Arguments without short versions
        {"input-cmdline",        required_argument, 0, OPT_INPUT_CMDLINE},
        {"input-board",          required_argument, 0, OPT_INPUT_BOARD},
        {"input-base",           required_argument, 0, OPT_INPUT_BASE},
        {"input-kernel_offset",  required_argument, 0, OPT_INPUT_KERNEL_OFFSET},
        {"input-ramdisk_offset", required_argument, 0, OPT_INPUT_RAMDISK_OFFSET},
        {"input-second_offset",  required_argument, 0, OPT_INPUT_SECOND_OFFSET},
        {"input-tags_offset",    required_argument, 0, OPT_INPUT_TAGS_OFFSET},
        {"input-ipl_address",    required_argument, 0, OPT_INPUT_IPL_ADDRESS},
        {"input-rpm_address",    required_argument, 0, OPT_INPUT_RPM_ADDRESS},
        {"input-appsbl_address", required_argument, 0, OPT_INPUT_APPSBL_ADDRESS},
        {"input-entrypoint",     required_argument, 0, OPT_INPUT_ENTRYPOINT},
        {"input-page_size",      required_argument, 0, OPT_INPUT_PAGE_SIZE},
        {"input-kernel",         required_argument, 0, OPT_INPUT_KERNEL},
        {"input-ramdisk",        required_argument, 0, OPT_INPUT_RAMDISK},
        {"input-second",         required_argument, 0, OPT_INPUT_SECOND},
        {"input-dt",             required_argument, 0, OPT_INPUT_DT},
        {"input-aboot",          required_argument, 0, OPT_INPUT_ABOOT},
        {"input-kernel_mtkhdr",  required_argument, 0, OPT_INPUT_KERNEL_MTKHDR},
        {"input-ramdisk_mtkhdr", required_argument, 0, OPT_INPUT_RAMDISK_MTKHDR},
        {"input-ipl",            required_argument, 0, OPT_INPUT_IPL},
        {"input-rpm",            required_argument, 0, OPT_INPUT_RPM},
        {"input-appsbl",         required_argument, 0, OPT_INPUT_APPSBL},
        {"input-sin",            required_argument, 0, OPT_INPUT_SIN},
        {"input-sinhdr",         required_argument, 0, OPT_INPUT_SINHDR},
        // Value arguments
        {"value-cmdline",        required_argument, 0, OPT_VALUE_CMDLINE},
        {"value-board",          required_argument, 0, OPT_VALUE_BOARD},
        {"value-base",           required_argument, 0, OPT_VALUE_BASE},
        {"value-kernel_offset",  required_argument, 0, OPT_VALUE_KERNEL_OFFSET},
        {"value-ramdisk_offset", required_argument, 0, OPT_VALUE_RAMDISK_OFFSET},
        {"value-second_offset",  required_argument, 0, OPT_VALUE_SECOND_OFFSET},
        {"value-tags_offset",    required_argument, 0, OPT_VALUE_TAGS_OFFSET},
        {"value-ipl_address",    required_argument, 0, OPT_VALUE_IPL_ADDRESS},
        {"value-rpm_address",    required_argument, 0, OPT_VALUE_RPM_ADDRESS},
        {"value-appsbl_address", required_argument, 0, OPT_VALUE_APPSBL_ADDRESS},
        {"value-entrypoint",     required_argument, 0, OPT_VALUE_ENTRYPOINT},
        {"value-page_size",      required_argument, 0, OPT_VALUE_PAGE_SIZE},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, "i:p:nt:", long_options, &long_index)) != -1) {
        switch (opt) {
        case 'i':                      input_dir = optarg;           break;
        case 'p':                      prefix = optarg;              break;
        case 'n':                      no_prefix = true;             break;
        case OPT_INPUT_CMDLINE:        path_cmdline = optarg;        break;
        case OPT_INPUT_BOARD:          path_board = optarg;          break;
        case OPT_INPUT_BASE:           path_base = optarg;           break;
        case OPT_INPUT_KERNEL_OFFSET:  path_kernel_offset = optarg;  break;
        case OPT_INPUT_RAMDISK_OFFSET: path_ramdisk_offset = optarg; break;
        case OPT_INPUT_SECOND_OFFSET:  path_second_offset = optarg;  break;
        case OPT_INPUT_TAGS_OFFSET:    path_tags_offset = optarg;    break;
        case OPT_INPUT_IPL_ADDRESS:    path_ipl_address = optarg;    break;
        case OPT_INPUT_RPM_ADDRESS:    path_rpm_address = optarg;    break;
        case OPT_INPUT_APPSBL_ADDRESS: path_appsbl_address = optarg; break;
        case OPT_INPUT_ENTRYPOINT:     path_entrypoint = optarg;     break;
        case OPT_INPUT_PAGE_SIZE:      path_page_size = optarg;      break;
        case OPT_INPUT_KERNEL:         path_kernel = optarg;         break;
        case OPT_INPUT_RAMDISK:        path_ramdisk = optarg;        break;
        case OPT_INPUT_SECOND:         path_second = optarg;         break;
        case OPT_INPUT_DT:             path_dt = optarg;             break;
        case OPT_INPUT_ABOOT:          path_aboot = optarg;          break;
        case OPT_INPUT_KERNEL_MTKHDR:  path_kernel_mtkhdr = optarg;  break;
        case OPT_INPUT_RAMDISK_MTKHDR: path_ramdisk_mtkhdr = optarg; break;
        case OPT_INPUT_IPL:            path_ipl = optarg;            break;
        case OPT_INPUT_RPM:            path_rpm = optarg;            break;
        case OPT_INPUT_APPSBL:         path_appsbl = optarg;         break;
        case OPT_INPUT_SIN:            path_sin = optarg;            break;
        case OPT_INPUT_SINHDR:         path_sinhdr = optarg;         break;

        case OPT_VALUE_CMDLINE:
            path_cmdline.clear();
            values[opt] = true;
            cmdline = optarg;
            break;

        case OPT_VALUE_BOARD:
            path_board.clear();
            values[opt] = true;
            board = optarg;
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

        case OPT_VALUE_IPL_ADDRESS:
            path_ipl_address.clear();
            values[opt] = true;
            if (!str_to_uint32(&ipl_address, optarg, 16)) {
                fprintf(stderr, "Invalid ipl_address: %s\n", optarg);
                return false;
            }
            break;

        case OPT_VALUE_RPM_ADDRESS:
            path_rpm_address.clear();
            values[opt] = true;
            if (!str_to_uint32(&rpm_address, optarg, 16)) {
                fprintf(stderr, "Invalid rpm_address: %s\n", optarg);
                return false;
            }
            break;

        case OPT_VALUE_APPSBL_ADDRESS:
            path_appsbl_address.clear();
            values[opt] = true;
            if (!str_to_uint32(&appsbl_address, optarg, 16)) {
                fprintf(stderr, "Invalid appsbl_address: %s\n", optarg);
                return false;
            }
            break;

        case OPT_VALUE_ENTRYPOINT:
            path_entrypoint.clear();
            values[opt] = true;
            if (!str_to_uint32(&entrypoint, optarg, 16)) {
                fprintf(stderr, "Invalid entrypoint: %s\n", optarg);
                return false;
            }
            break;

        case OPT_VALUE_PAGE_SIZE:
            path_page_size.clear();
            values[opt] = true;
            if (!str_to_uint32(&page_size, optarg, 10)) {
                fprintf(stderr, "Invalid page_size: %s\n", optarg);
                return false;
            }
            break;

        case 't':
            if (strcmp(optarg, "android") == 0) {
                type = mbp::BootImage::Type::Android;
            } else if (strcmp(optarg, "bump") == 0) {
                type = mbp::BootImage::Type::Bump;
            } else if (strcmp(optarg, "loki") == 0) {
                type = mbp::BootImage::Type::Loki;
            } else if (strcmp(optarg, "mtk") == 0) {
                type = mbp::BootImage::Type::Mtk;
            } else if (strcmp(optarg, "sonyelf") == 0) {
                type = mbp::BootImage::Type::SonyElf;
            } else {
                fprintf(stderr, "Invalid type: %s\n", optarg);
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

    // There should be one other argument
    if (argc - optind != 1) {
        fprintf(stderr, PackUsage);
        return false;
    }

    output_file = argv[optind];

    if (no_prefix) {
        prefix.clear();
    } else {
        if (prefix.empty()) {
            prefix = io::baseName(output_file);
        }
        prefix += "-";
    }

    if (input_dir.empty()) {
        input_dir = ".";
    }

    // Create new boot image
    mbp::BootImage bi;

    uint64_t support_mask = mbp::BootImage::typeSupportMask(type);

    static const char *not_supported =
            "Warning: Target type does not support %s\n";
    static const char *fmt_path   = "- %-14s (path)  %s\n";
    static const char *fmt_string = "- %-14s (value) %s\n";
    static const char *fmt_hex    = "- %-14s (value) 0x%08x\n";
    static const char *fmt_uint   = "- %-14s (value) %u\n";

    ////////////////////////////////////////////////////////////////////////////
    // Kernel cmdline
    ////////////////////////////////////////////////////////////////////////////

    if (support_mask & SUPPORTS_CMDLINE) {
        if (values[OPT_VALUE_CMDLINE]) {
            printf(fmt_string, "cmdline", cmdline.c_str());
        } else {
            if (path_cmdline.empty())
                path_cmdline = io::pathJoin({input_dir, prefix + "cmdline"});

            printf(fmt_path, "cmdline", path_cmdline.c_str());

            file_ptr fp(fopen(path_cmdline.c_str(), "rb"), fclose);
            if (fp) {
                std::vector<char> buf(mbp::BootImage::AndroidBootArgsSize + 1);
                if (!fgets(buf.data(), mbp::BootImage::AndroidBootArgsSize + 1, fp.get())) {
                    if (ferror(fp.get())) {
                        fprintf(stderr, "%s: %s\n",
                                path_cmdline.c_str(), strerror(errno));
                        return false;
                    }
                }
                cmdline = buf.data();
                auto pos = cmdline.find('\n');
                if (pos != std::string::npos) {
                    cmdline.erase(pos);
                }
            } else if (errno != ENOENT) {
                fprintf(stderr, "%s: %s\n",
                        path_cmdline.c_str(), strerror(errno));
                return false;
            } else {
                cmdline = mbp::BootImage::DefaultCmdline;
            }
        }

        bi.setKernelCmdline(std::move(cmdline));
    } else {
        if (!path_cmdline.empty())
            printf(not_supported, "--input-cmdline");
        if (values[OPT_VALUE_CMDLINE])
            printf(not_supported, "--value-cmdline");
    }

    ////////////////////////////////////////////////////////////////////////////
    // Board name
    ////////////////////////////////////////////////////////////////////////////

    if (support_mask & SUPPORTS_BOARD_NAME) {
        if (values[OPT_VALUE_BOARD]) {
            printf(fmt_string, "board", board.c_str());
        } else {
            if (path_board.empty())
                path_board = io::pathJoin({input_dir, prefix + "board"});

            printf(fmt_path, "board", path_board.c_str());

            file_ptr fp(fopen(path_board.c_str(), "rb"), fclose);
            if (fp) {
                std::vector<char> buf(mbp::BootImage::AndroidBootNameSize + 1);
                if (!fgets(buf.data(), mbp::BootImage::AndroidBootNameSize + 1, fp.get())) {
                    if (ferror(fp.get())) {
                        fprintf(stderr, "%s: %s\n",
                                path_board.c_str(), strerror(errno));
                        return false;
                    }
                }
                board = buf.data();
                auto pos = board.find('\n');
                if (pos != std::string::npos) {
                    board.erase(pos);
                }
            } else if (errno != ENOENT) {
                fprintf(stderr, "%s: %s\n",
                        path_board.c_str(), strerror(errno));
                return false;
            } else {
                board = mbp::BootImage::AndroidDefaultBoard;
            }
        }

        bi.setBoardName(std::move(board));
    } else {
        if (!path_board.empty())
            printf(not_supported, "--input-board");
        if (values[OPT_VALUE_BOARD])
            printf(not_supported, "--value-board");
    }

    ////////////////////////////////////////////////////////////////////////////
    // Base for address offsets
    ////////////////////////////////////////////////////////////////////////////

    if (support_mask & SUPPORTS_OFFSET_BASE) {
        if (values[OPT_VALUE_BASE]) {
            printf(fmt_hex, "base", base);
        } else {
            if (path_base.empty())
                path_base = io::pathJoin({input_dir, prefix + "base"});

            printf(fmt_path, "base", path_base.c_str());

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
            } else if (errno != ENOENT) {
                fprintf(stderr, "%s: %s\n",
                        path_base.c_str(), strerror(errno));
                return false;
            } else {
                if (type == mbp::BootImage::Type::SonyElf) {
                    // We use absolute addresses for Sony ELF boot images
                    base = 0;
                } else {
                    base = mbp::BootImage::AndroidDefaultBase;
                }
            }
        }

        // The base will be used by the offsets below
    } else {
        if (!path_base.empty())
            printf(not_supported, "--input-base");
        if (values[OPT_VALUE_BASE])
            printf(not_supported, "--value-base");
    }

    ////////////////////////////////////////////////////////////////////////////
    // Kernel offset
    ////////////////////////////////////////////////////////////////////////////

    if (support_mask & SUPPORTS_KERNEL_ADDRESS) {
        if (values[OPT_VALUE_KERNEL_OFFSET]) {
            printf(fmt_hex, "kernel_offset", kernel_offset);
        } else {
            if (path_kernel_offset.empty())
                path_kernel_offset = io::pathJoin({input_dir, prefix + "kernel_offset"});

            printf(fmt_path, "kernel_offset", path_kernel_offset.c_str());

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
            } else if (errno != ENOENT) {
                fprintf(stderr, "%s: %s\n",
                        path_kernel_offset.c_str(), strerror(errno));
                return false;
            } else {
                if (type == mbp::BootImage::Type::SonyElf) {
                    kernel_offset = mbp::BootImage::SonyElfDefaultKernelAddress;
                } else {
                    kernel_offset = mbp::BootImage::AndroidDefaultKernelOffset;
                }
            }
        }

        bi.setKernelAddress(base + kernel_offset);
    } else {
        if (!path_kernel_offset.empty())
            printf(not_supported, "--input-kernel_offset");
        if (values[OPT_VALUE_KERNEL_OFFSET])
            printf(not_supported, "--value-kernel_offset");
    }

    ////////////////////////////////////////////////////////////////////////////
    // Ramdisk offset
    ////////////////////////////////////////////////////////////////////////////

    if (support_mask & SUPPORTS_RAMDISK_ADDRESS) {
        if (values[OPT_VALUE_RAMDISK_OFFSET]) {
            printf(fmt_hex, "ramdisk_offset", ramdisk_offset);
        } else {
            if (path_ramdisk_offset.empty())
                path_ramdisk_offset = io::pathJoin({input_dir, prefix + "ramdisk_offset"});

            printf(fmt_path, "ramdisk_offset", path_ramdisk_offset.c_str());

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
            } else if (errno != ENOENT) {
                fprintf(stderr, "%s: %s\n",
                        path_ramdisk_offset.c_str(), strerror(errno));
                return false;
            } else {
                if (type == mbp::BootImage::Type::SonyElf) {
                    ramdisk_offset = mbp::BootImage::SonyElfDefaultRamdiskAddress;
                } else {
                    ramdisk_offset = mbp::BootImage::AndroidDefaultRamdiskOffset;
                }
            }
        }

        bi.setRamdiskAddress(base + ramdisk_offset);
    } else {
        if (!path_ramdisk_offset.empty())
            printf(not_supported, "--input-ramdisk_offset");
        if (values[OPT_VALUE_RAMDISK_OFFSET])
            printf(not_supported, "--value-ramdisk_offset");
    }

    ////////////////////////////////////////////////////////////////////////////
    // Second bootloader offset
    ////////////////////////////////////////////////////////////////////////////

    if (support_mask & SUPPORTS_SECOND_ADDRESS) {
        if (values[OPT_VALUE_SECOND_OFFSET]) {
            printf(fmt_hex, "second_offset", second_offset);
        } else {
            if (path_second_offset.empty())
                path_second_offset = io::pathJoin({input_dir, prefix + "second_offset"});

            printf(fmt_path, "second_offset", path_second_offset.c_str());

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
            } else if (errno != ENOENT) {
                fprintf(stderr, "%s: %s\n",
                        path_second_offset.c_str(), strerror(errno));
                return false;
            } else {
                second_offset = mbp::BootImage::AndroidDefaultSecondOffset;
            }
        }

        bi.setSecondBootloaderAddress(base + second_offset);
    } else {
        if (!path_second_offset.empty())
            printf(not_supported, "--input-second_offset");
        if (values[OPT_VALUE_SECOND_OFFSET])
            printf(not_supported, "--value-second_offset");
    }

    ////////////////////////////////////////////////////////////////////////////
    // Kernel tags offset
    ////////////////////////////////////////////////////////////////////////////

    if (support_mask & SUPPORTS_TAGS_ADDRESS) {
        if (values[OPT_VALUE_TAGS_OFFSET]) {
            printf(fmt_hex, "tags_offset", tags_offset);
        } else {
            if (path_tags_offset.empty())
                path_tags_offset = io::pathJoin({input_dir, prefix + "tags_offset"});

            printf(fmt_path, "tags_offset", path_tags_offset.c_str());

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
            } else if (errno != ENOENT) {
                fprintf(stderr, "%s: %s\n",
                        path_tags_offset.c_str(), strerror(errno));
                return false;
            } else {
                tags_offset = mbp::BootImage::AndroidDefaultTagsOffset;
            }
        }

        bi.setKernelTagsAddress(base + tags_offset);
    } else {
        if (!path_tags_offset.empty())
            printf(not_supported, "--input-tags_offset");
        if (values[OPT_VALUE_TAGS_OFFSET])
            printf(not_supported, "--value-tags_offset");
    }

    ////////////////////////////////////////////////////////////////////////////
    // Ipl address
    ////////////////////////////////////////////////////////////////////////////

    if (support_mask & SUPPORTS_IPL_ADDRESS) {
        if (values[OPT_VALUE_IPL_ADDRESS]) {
            printf(fmt_hex, "ipl_address", ipl_address);
        } else {
            if (path_ipl_address.empty())
                path_ipl_address = io::pathJoin({input_dir, prefix + "ipl_address"});

            printf(fmt_path, "ipl_address", path_ipl_address.c_str());

            file_ptr fp(fopen(path_ipl_address.c_str(), "rb"), fclose);
            if (fp) {
                int count = fscanf(fp.get(), "%08x", &ipl_address);
                if (count == EOF && ferror(fp.get())) {
                    fprintf(stderr, "%s: %s\n",
                            path_ipl_address.c_str(), strerror(errno));
                    return false;
                } else if (count != 1) {
                    fprintf(stderr, "%s: Error: expected '%%08x' format\n",
                            path_ipl_address.c_str());
                    return false;
                }
            } else if (errno != ENOENT) {
                fprintf(stderr, "%s: %s\n",
                        path_ipl_address.c_str(), strerror(errno));
                return false;
            } else {
                ipl_address = mbp::BootImage::SonyElfDefaultIplAddress;
            }
        }

        bi.setIplAddress(ipl_address);
    } else {
        if (!path_ipl_address.empty())
            printf(not_supported, "--input-ipl_address");
        if (values[OPT_VALUE_IPL_ADDRESS])
            printf(not_supported, "--value-ipl_address");
    }

    ////////////////////////////////////////////////////////////////////////////
    // Rpm address
    ////////////////////////////////////////////////////////////////////////////

    if (support_mask & SUPPORTS_RPM_ADDRESS) {
        if (values[OPT_VALUE_RPM_ADDRESS]) {
            printf(fmt_hex, "rpm_address", rpm_address);
        } else {
            if (path_rpm_address.empty())
                path_rpm_address = io::pathJoin({input_dir, prefix + "rpm_address"});

            printf(fmt_path, "rpm_address", path_rpm_address.c_str());

            file_ptr fp(fopen(path_rpm_address.c_str(), "rb"), fclose);
            if (fp) {
                int count = fscanf(fp.get(), "%08x", &rpm_address);
                if (count == EOF && ferror(fp.get())) {
                    fprintf(stderr, "%s: %s\n",
                            path_rpm_address.c_str(), strerror(errno));
                    return false;
                } else if (count != 1) {
                    fprintf(stderr, "%s: Error: expected '%%08x' format\n",
                            path_rpm_address.c_str());
                    return false;
                }
            } else if (errno != ENOENT) {
                fprintf(stderr, "%s: %s\n",
                        path_rpm_address.c_str(), strerror(errno));
                return false;
            } else {
                rpm_address = mbp::BootImage::SonyElfDefaultRpmAddress;
            }
        }

        bi.setRpmAddress(rpm_address);
    } else {
        if (!path_rpm_address.empty())
            printf(not_supported, "--input-rpm_address");
        if (values[OPT_VALUE_RPM_ADDRESS])
            printf(not_supported, "--value-rpm_address");
    }

    ////////////////////////////////////////////////////////////////////////////
    // Appsbl address
    ////////////////////////////////////////////////////////////////////////////

    if (support_mask & SUPPORTS_APPSBL_ADDRESS) {
        if (values[OPT_VALUE_APPSBL_ADDRESS]) {
            printf(fmt_hex, "appsbl_address", appsbl_address);
        } else {
            if (path_appsbl_address.empty())
                path_appsbl_address = io::pathJoin({input_dir, prefix + "appsbl_address"});

            printf(fmt_path, "appsbl_address", path_appsbl_address.c_str());

            file_ptr fp(fopen(path_appsbl_address.c_str(), "rb"), fclose);
            if (fp) {
                int count = fscanf(fp.get(), "%08x", &appsbl_address);
                if (count == EOF && ferror(fp.get())) {
                    fprintf(stderr, "%s: %s\n",
                            path_appsbl_address.c_str(), strerror(errno));
                    return false;
                } else if (count != 1) {
                    fprintf(stderr, "%s: Error: expected '%%08x' format\n",
                            path_appsbl_address.c_str());
                    return false;
                }
            } else if (errno != ENOENT) {
                fprintf(stderr, "%s: %s\n",
                        path_appsbl_address.c_str(), strerror(errno));
                return false;
            } else {
                appsbl_address = mbp::BootImage::SonyElfDefaultAppsblAddress;
            }
        }

        bi.setAppsblAddress(appsbl_address);
    } else {
        if (!path_appsbl_address.empty())
            printf(not_supported, "--input-appsbl_address");
        if (values[OPT_VALUE_APPSBL_ADDRESS])
            printf(not_supported, "--value-appsbl_address");
    }

    ////////////////////////////////////////////////////////////////////////////
    // Entrypoint address
    ////////////////////////////////////////////////////////////////////////////

    if (support_mask & SUPPORTS_ENTRYPOINT) {
        if (values[OPT_VALUE_ENTRYPOINT]) {
            printf(fmt_hex, "entrypoint", entrypoint);
        } else {
            if (path_entrypoint.empty())
                path_entrypoint = io::pathJoin({input_dir, prefix + "entrypoint"});

            printf(fmt_path, "entrypoint", path_entrypoint.c_str());

            file_ptr fp(fopen(path_entrypoint.c_str(), "rb"), fclose);
            if (fp) {
                int count = fscanf(fp.get(), "%08x", &entrypoint);
                if (count == EOF && ferror(fp.get())) {
                    fprintf(stderr, "%s: %s\n",
                            path_entrypoint.c_str(), strerror(errno));
                    return false;
                } else if (count != 1) {
                    fprintf(stderr, "%s: Error: expected '%%08x' format\n",
                            path_entrypoint.c_str());
                    return false;
                }
            } else if (errno != ENOENT) {
                fprintf(stderr, "%s: %s\n",
                        path_entrypoint.c_str(), strerror(errno));
                return false;
            } else {
                entrypoint = mbp::BootImage::SonyElfDefaultEntrypointAddress;
            }
        }

        bi.setEntrypointAddress(entrypoint);
    } else {
        if (!path_entrypoint.empty())
            printf(not_supported, "--input-entrypoint");
        if (values[OPT_VALUE_ENTRYPOINT])
            printf(not_supported, "--value-entrypoint");
    }

    ////////////////////////////////////////////////////////////////////////////
    // Page size
    ////////////////////////////////////////////////////////////////////////////

    if (support_mask & SUPPORTS_PAGE_SIZE) {
        if (values[OPT_VALUE_PAGE_SIZE]) {
            printf(fmt_uint, "page_size", page_size);
        } else {
            if (path_page_size.empty())
                path_page_size = io::pathJoin({input_dir, prefix + "page_size"});

            printf(fmt_path, "page_size", path_page_size.c_str());

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
            } else if (errno != ENOENT) {
                fprintf(stderr, "%s: %s\n",
                        path_page_size.c_str(), strerror(errno));
                return false;
            } else {
                page_size = mbp::BootImage::AndroidDefaultPageSize;
            }
        }

        bi.setPageSize(page_size);
    } else {
        if (!path_page_size.empty())
            printf(not_supported, "--input-page_size");
        if (values[OPT_VALUE_PAGE_SIZE])
            printf(not_supported, "--value-page_size");
    }

    ////////////////////////////////////////////////////////////////////////////
    // Kernel image
    ////////////////////////////////////////////////////////////////////////////

    if (support_mask & SUPPORTS_KERNEL_IMAGE) {
        if (path_kernel.empty())
            path_kernel = io::pathJoin({input_dir, prefix + "kernel"});

        printf(fmt_path, "kernel", path_kernel.c_str());

        if (!read_file_data(path_kernel, &kernel_image)) {
            fprintf(stderr, "%s: %s\n", path_kernel.c_str(), strerror(errno));
            return false;
        }

        bi.setKernelImage(std::move(kernel_image));
    } else {
        if (!path_kernel.empty())
            printf(not_supported, "--input-kernel");
    }

    ////////////////////////////////////////////////////////////////////////////
    // Ramdisk image
    ////////////////////////////////////////////////////////////////////////////

    if (support_mask & SUPPORTS_RAMDISK_IMAGE) {
        if (path_ramdisk.empty())
            path_ramdisk = io::pathJoin({input_dir, prefix + "ramdisk"});

        printf(fmt_path, "ramdisk", path_ramdisk.c_str());

        if (!read_file_data(path_ramdisk, &ramdisk_image)) {
            fprintf(stderr, "%s: %s\n", path_ramdisk.c_str(), strerror(errno));
            return false;
        }

        bi.setRamdiskImage(std::move(ramdisk_image));
    } else {
        if (!path_ramdisk.empty())
            printf(not_supported, "--input-ramdisk");
    }

    ////////////////////////////////////////////////////////////////////////////
    // Second bootloader image
    ////////////////////////////////////////////////////////////////////////////

    if (support_mask & SUPPORTS_SECOND_IMAGE) {
        if (path_second.empty())
            path_second = io::pathJoin({input_dir, prefix + "second"});

        printf(fmt_path, "second", path_second.c_str());

        if (!read_file_data(path_second, &second_image) && errno != ENOENT) {
            fprintf(stderr, "%s: %s\n", path_second.c_str(), strerror(errno));
            return false;
        }

        bi.setSecondBootloaderImage(std::move(second_image));
    } else {
        if (!path_second.empty())
            printf(not_supported, "--input-second");
    }

    ////////////////////////////////////////////////////////////////////////////
    // Device tree image
    ////////////////////////////////////////////////////////////////////////////

    if (support_mask & SUPPORTS_DT_IMAGE) {
        if (path_dt.empty())
            path_dt = io::pathJoin({input_dir, prefix + "dt"});

        printf(fmt_path, "dt", path_dt.c_str());

        if (!read_file_data(path_dt, &dt_image) && errno != ENOENT) {
            fprintf(stderr, "%s: %s\n", path_dt.c_str(), strerror(errno));
            return false;
        }

        bi.setDeviceTreeImage(std::move(dt_image));
    } else {
        if (!path_dt.empty())
            printf(not_supported, "--input-dt");
    }

    ////////////////////////////////////////////////////////////////////////////
    // Aboot image
    ////////////////////////////////////////////////////////////////////////////

    if (support_mask & SUPPORTS_ABOOT_IMAGE) {
        if (path_aboot.empty()) {
            // The aboot image is required
            fprintf(stderr, "An aboot image must be specified to create a loki image\n");
            return false;
        }

        printf(fmt_path, "aboot", path_aboot.c_str());

        if (!read_file_data(path_aboot, &aboot_image) && errno != ENOENT) {
            fprintf(stderr, "%s: %s\n", path_aboot.c_str(), strerror(errno));
            return false;
        }

        bi.setAbootImage(std::move(aboot_image));
    } else {
        if (!path_aboot.empty())
            printf(not_supported, "--input-aboot");
    }

    ////////////////////////////////////////////////////////////////////////////
    // MTK kernel header
    ////////////////////////////////////////////////////////////////////////////

    if (support_mask & SUPPORTS_KERNEL_MTKHDR) {
        if (path_kernel_mtkhdr.empty())
            path_kernel_mtkhdr = io::pathJoin({input_dir, prefix + "kernel_mtkhdr"});

        printf(fmt_path, "kernel_mtkhdr", path_kernel_mtkhdr.c_str());

        if (!read_file_data(path_kernel_mtkhdr, &kernel_mtkhdr)) {
            fprintf(stderr, "%s: %s\n", path_kernel_mtkhdr.c_str(), strerror(errno));
            return false;
        }

        bi.setKernelMtkHeader(std::move(kernel_mtkhdr));
    } else {
        if (!path_kernel_mtkhdr.empty())
            printf(not_supported, "--input-kernel_mtkhdr");
    }

    ////////////////////////////////////////////////////////////////////////////
    // MTK ramdisk header
    ////////////////////////////////////////////////////////////////////////////

    if (support_mask & SUPPORTS_RAMDISK_MTKHDR) {
        if (path_ramdisk_mtkhdr.empty())
            path_ramdisk_mtkhdr = io::pathJoin({input_dir, prefix + "ramdisk_mtkhdr"});

        printf(fmt_path, "ramdisk_mtkhdr", path_ramdisk_mtkhdr.c_str());

        if (!read_file_data(path_ramdisk_mtkhdr, &ramdisk_mtkhdr)) {
            fprintf(stderr, "%s: %s\n", path_ramdisk_mtkhdr.c_str(), strerror(errno));
            return false;
        }

        bi.setRamdiskMtkHeader(std::move(ramdisk_mtkhdr));
    } else {
        if (!path_ramdisk_mtkhdr.empty())
            printf(not_supported, "--input-ramdisk_mtkhdr");
    }

    ////////////////////////////////////////////////////////////////////////////
    // Ipl image
    ////////////////////////////////////////////////////////////////////////////

    if (support_mask & SUPPORTS_IPL_IMAGE) {
        if (path_ipl.empty())
            path_ipl = io::pathJoin({input_dir, prefix + "ipl"});

        printf(fmt_path, "ipl", path_ipl.c_str());

        if (!read_file_data(path_ipl, &ipl_image) && errno != ENOENT) {
            fprintf(stderr, "%s: %s\n", path_ipl.c_str(), strerror(errno));
            return false;
        }

        bi.setIplImage(std::move(ipl_image));
    } else {
        if (!path_ipl.empty())
            printf(not_supported, "--input-ipl");
    }

    ////////////////////////////////////////////////////////////////////////////
    // Rpm image
    ////////////////////////////////////////////////////////////////////////////

    if (support_mask & SUPPORTS_RPM_IMAGE) {
        if (path_rpm.empty())
            path_rpm = io::pathJoin({input_dir, prefix + "rpm"});

        printf(fmt_path, "rpm", path_rpm.c_str());

        if (!read_file_data(path_rpm, &rpm_image) && errno != ENOENT) {
            fprintf(stderr, "%s: %s\n", path_rpm.c_str(), strerror(errno));
            return false;
        }

        bi.setRpmImage(std::move(rpm_image));
    } else {
        if (!path_rpm.empty())
            printf(not_supported, "--input-rpm");
    }

    ////////////////////////////////////////////////////////////////////////////
    // Appsbl image
    ////////////////////////////////////////////////////////////////////////////

    if (support_mask & SUPPORTS_APPSBL_IMAGE) {
        if (path_appsbl.empty())
            path_appsbl = io::pathJoin({input_dir, prefix + "appsbl"});

        printf(fmt_path, "appsbl", path_appsbl.c_str());

        if (!read_file_data(path_appsbl, &appsbl_image) && errno != ENOENT) {
            fprintf(stderr, "%s: %s\n", path_appsbl.c_str(), strerror(errno));
            return false;
        }

        bi.setAppsblImage(std::move(appsbl_image));
    } else {
        if (!path_appsbl.empty())
            printf(not_supported, "--input-appsbl");
    }

    ////////////////////////////////////////////////////////////////////////////
    // Sony SIN! image
    ////////////////////////////////////////////////////////////////////////////

    if (support_mask & SUPPORTS_SONY_SIN_IMAGE) {
        if (path_sin.empty())
            path_sin = io::pathJoin({input_dir, prefix + "sin"});

        printf(fmt_path, "sin", path_sin.c_str());

        if (!read_file_data(path_sin, &sin_image) && errno != ENOENT) {
            fprintf(stderr, "%s: %s\n", path_sin.c_str(), strerror(errno));
            return false;
        }

        bi.setSinImage(std::move(sin_image));
    } else {
        if (!path_sin.empty())
            printf(not_supported, "--input-sin");
    }

    ////////////////////////////////////////////////////////////////////////////
    // Sony SIN! header
    ////////////////////////////////////////////////////////////////////////////

    if (support_mask & SUPPORTS_SONY_SIN_HEADER) {
        if (path_sinhdr.empty())
            path_sinhdr = io::pathJoin({input_dir, prefix + "sinhdr"});

        printf(fmt_path, "sinhdr", path_sinhdr.c_str());

        if (!read_file_data(path_sinhdr, &sin_header) && errno != ENOENT) {
            fprintf(stderr, "%s: %s\n", path_sinhdr.c_str(), strerror(errno));
            return false;
        }

        bi.setSinHeader(std::move(sin_header));
    } else {
        if (!path_sinhdr.empty())
            printf(not_supported, "--input-sinhdr");
    }

    // Create boot image

    bi.setTargetType(type);

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

    mb::log::log_set_logger(std::make_shared<BasicLogger>());

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
