/*
 * Copyright (C) 2014-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of DualBootPatcher
 *
 * DualBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DualBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DualBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <limits>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>

#include <cassert>
#include <climits>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <getopt.h>

// rapidjson
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>

// libmbcommon
#include <mbcommon/common.h>
#include <mbcommon/integer.h>
#include <mbcommon/string.h>

// libmbbootimg
#include <mbbootimg/entry.h>
#include <mbbootimg/format/android_defs.h>
#include <mbbootimg/header.h>
#include <mbbootimg/reader.h>
#include <mbbootimg/writer.h>

// libmbpio
#include <mbpio/directory.h>
#include <mbpio/path.h>

#define FIELD_CMDLINE                   "cmdline"
#define FIELD_BOARD                     "board"
#define FIELD_BASE                      "base"
#define FIELD_KERNEL_OFFSET             "kernel_offset"
#define FIELD_RAMDISK_OFFSET            "ramdisk_offset"
#define FIELD_SECOND_OFFSET             "second_offset"
#define FIELD_TAGS_OFFSET               "tags_offset"
#define FIELD_IPL_ADDRESS               "ipl_address"
#define FIELD_RPM_ADDRESS               "rpm_address"
#define FIELD_APPSBL_ADDRESS            "appsbl_address"
#define FIELD_ENTRYPOINT                "entrypoint"
#define FIELD_PAGE_SIZE                 "page_size"

#define IMAGE_KERNEL                    "kernel"
#define IMAGE_RAMDISK                   "ramdisk"
#define IMAGE_SECOND                    "second"
#define IMAGE_DT                        "dt"
#define IMAGE_ABOOT                     "aboot"
#define IMAGE_KERNEL_MTKHDR             "kernel_mtkhdr"
#define IMAGE_RAMDISK_MTKHDR            "ramdisk_mtkhdr"
#define IMAGE_IPL                       "ipl"
#define IMAGE_RPM                       "rpm"
#define IMAGE_APPSBL                    "appsbl"

#ifdef _WIN32
#  define CLOEXEC_FLAG "N"
#else
#  define CLOEXEC_FLAG "e"
#endif


namespace rj = rapidjson;

using namespace mb::bootimg;

typedef std::unique_ptr<FILE, decltype(fclose) *> ScopedFILE;

#define HELP_HEADERS \
    "Header fields:\n" \
    "  cmdline         Kernel command line                           [ABLMS]\n" \
    "  board           Board name field in the header                [ABLM ]\n" \
    "  base            Base address for offsets                      [ABLM ]\n" \
    "  kernel_offset   Address offset of the kernel image            [ABLMS]\n" \
    "  ramdisk_offset  Address offset of the ramdisk image           [ABLMS]\n" \
    "  second_offset   Address offset of the second bootloader image [ABLM ]\n" \
    "  tags_offset     Address offset of the kernel tags image       [ABLM ]\n" \
    "  ipl_address     Address of the ipl image                      [    S]\n" \
    "  rpm_address     Address of the rpm image                      [    S]\n" \
    "  appsbl_address  Address of the appsbl image                   [    S]\n" \
    "  entrypoint      Address of the entry point                    [    S]\n" \
    "  page_size       Page size                                     [ABLM ]\n"

#define HELP_IMAGES \
    "Images:\n" \
    "  kernel          Kernel image                                  [ABLMS]\n" \
    "  ramdisk         Ramdisk image                                 [ABLMS]\n" \
    "  second          Second bootloader image                       [ABLM ]\n" \
    "  dt              Device tree image                             [ABLM ]\n" \
    "  kernel_mtkhdr   Kernel MTK header                             [   M ]\n" \
    "  ramdisk_mtkhdr  Ramdisk MTK header                            [   M ]\n" \
    "  ipl             Ipl image                                     [    S]\n" \
    "  rpm             Rpm image                                     [    S]\n" \
    "  appsbl          Appsbl image                                  [    S]\n"

#define HELP_IMAGES_ABOOT \
    "  aboot           Aboot image                                   [  L  ]\n"

#define HELP_LEGEND \
    "Legend:\n" \
    "  [A B L M S]\n" \
    "   | | | | `- Supported by Sony ELF boot images\n" \
    "   | | | `- Supported by MTK Android boot images\n" \
    "   | | `- Supported by Loki'd Android boot images\n" \
    "   | `- Supported by bump'd Android boot images\n" \
    "   `- Supported by plain Android boot images\n"

#define HELP_MAIN_USAGE \
    "Usage: bootimgtool <command> [<args>...]\n" \
    "\n" \
    "Available commands:\n" \
    "  unpack         Unpack a boot image\n" \
    "  pack           Assemble boot image from unpacked files\n" \
    "\n" \
    "Pass -h/--help as a argument to a command to see its available options.\n"

#define HELP_UNPACK_USAGE \
    "Usage: bootimgtool unpack <input file> [<option>...]\n" \
    "\n" \
    "Options:\n" \
    "  -o, --output <output directory>\n" \
    "                  Output directory (current directory if unspecified)\n" \
    "  -p, --prefix <prefix>\n" \
    "                  Prefix to prepend to output filenames\n" \
    "                  (defaults to \"<input file>-\")\n" \
    "  -n, --noprefix  Do not prepend a prefix to the item filenames\n" \
    "  -t, --type <type>\n" \
    "                  Input type of the boot image (autodetect if unspecified)\n" \
    "                  (one of: android, bump, loki, mtk, sony_elf)\n" \
    "  --output-<item> <item path>\n" \
    "                  Custom path for a particular item\n" \
    "\n" \
    "The following items are extracted from the boot image.\n" \
    "\n" \
    HELP_HEADERS \
    "\n" \
    HELP_IMAGES \
    "\n" \
    HELP_LEGEND \
    "\n" \
    "Output files:\n" \
    "\n" \
    "The header fields are stored in the following path:\n" \
    "\n" \
    "    <output directory>/<prefix>header.json\n" \
    "\n" \
    "The file format is a list of newline-separated \"<key>=<value>\" pairs where\n" \
    "lines containing only whitespace and lines that begin with '#' following any\n" \
    "leading whitespace are ignored.\n" \
    "\n" \
    "The images are unpacked to the following path:\n" \
    "\n" \
    "    <output directory>/<prefix><image>\n" \
    "\n" \
    "If the --output-<item>=<item path> option is specified, then that particular\n" \
    "item is unpacked to the specified <item path>.\n" \
    "\n" \
    "Examples:\n" \
    "\n" \
    "1. Unpack a boot image to the current directory\n" \
    "\n" \
    "        bootimgtool unpack boot.img\n" \
    "\n" \
    "2. Unpack a boot image to a different directory, but put the kernel in /tmp/\n" \
    "\n" \
    "        bootimgtool unpack boot.img -o extracted --output-kernel /tmp/kernel.img\n" \
    "\n"

#define HELP_PACK_USAGE \
    "Usage: bootimgtool pack <output file> [<option>...]\n" \
    "\n" \
    "Options:\n" \
    "  -i, --input <input directory>\n" \
    "                  Input directory (current directory if unspecified)\n" \
    "  -p, --prefix <prefix>\n" \
    "                  Prefix to prepend to item filenames\n" \
    "                  (defaults to \"<output file>-\")\n" \
    "  -n, --noprefix  Do not prepend a prefix to the item filenames\n" \
    "  -t, --type <type>\n" \
    "                  Output type of the boot image (use header.json if unspecified)\n" \
    "                  (one of: android, bump, loki, mtk, sony_elf)\n" \
    "  --input-<item> <item path>\n" \
    "                  Custom path for a particular item\n" \
    "\n" \
    "The following items are loaded to create a new boot image.\n" \
    "\n" \
    HELP_HEADERS \
    "\n" \
    HELP_IMAGES \
    HELP_IMAGES_ABOOT \
    "\n" \
    HELP_LEGEND \
    "\n" \
    "Input files:\n" \
    "\n" \
    "The header fields are loaded from the following path:\n" \
    "\n" \
    "    <input directory>/<prefix>header.json\n" \
    "\n" \
    "The file format is a list of newline-separated \"<key>=<value>\" pairs where\n" \
    "lines containing only whitespace and lines that begin with '#' following any\n" \
    "leading whitespace are ignored.\n" \
    "\n" \
    "The images are loaded from the following path:\n" \
    "\n" \
    "    <input directory>/<prefix><item>\n" \
    "\n" \
    "If the --input-<item>=<item path> option is specified, then that particular\n" \
    "item is loaded from the specified <item path>.\n" \
    "\n" \
    "Examples:\n" \
    "\n" \
    "1. Build a boot image from unpacked components in the current directory\n" \
    "\n" \
    "        bootimgtool pack boot.img\n" \
    "\n" \
    "2. Build a boot image from unpacked components in /tmp/android, but use the\n" \
    "   kernel located at /tmp/newkernel.\n" \
    "\n" \
    "        bootimgtool pack boot.img -i /tmp/android --input-kernel /tmp/newkernel\n" \
    "\n"

struct Paths
{
    std::string header;
    std::string kernel;
    std::string ramdisk;
    std::string second;
    std::string dt;
    std::string aboot;
    std::string kernel_mtkhdr;
    std::string ramdisk_mtkhdr;
    std::string ipl;
    std::string rpm;
    std::string appsbl;
};

static void prepend_if_empty(Paths &paths, const std::string &dir,
                             const std::string &prefix)
{
    if (paths.header.empty())
        paths.header = mb::io::path_join({dir, prefix + "header.json"});
    if (paths.kernel.empty())
        paths.kernel = mb::io::path_join({dir, prefix + IMAGE_KERNEL});
    if (paths.ramdisk.empty())
        paths.ramdisk = mb::io::path_join({dir, prefix + IMAGE_RAMDISK});
    if (paths.second.empty())
        paths.second = mb::io::path_join({dir, prefix + IMAGE_SECOND});
    if (paths.dt.empty())
        paths.dt = mb::io::path_join({dir, prefix + IMAGE_DT});
    if (paths.aboot.empty())
        paths.aboot = mb::io::path_join({dir, prefix + IMAGE_ABOOT});
    if (paths.kernel_mtkhdr.empty())
        paths.kernel_mtkhdr = mb::io::path_join({dir, prefix + IMAGE_KERNEL_MTKHDR});
    if (paths.ramdisk_mtkhdr.empty())
        paths.ramdisk_mtkhdr = mb::io::path_join({dir, prefix + IMAGE_RAMDISK_MTKHDR});
    if (paths.ipl.empty())
        paths.ipl = mb::io::path_join({dir, prefix + IMAGE_IPL});
    if (paths.rpm.empty())
        paths.rpm = mb::io::path_join({dir, prefix + IMAGE_RPM});
    if (paths.appsbl.empty())
        paths.appsbl = mb::io::path_join({dir, prefix + IMAGE_APPSBL});
}

static void absolute_to_offset(std::optional<uint32_t> &base,
                               std::optional<uint32_t> &kernel_addr,
                               std::optional<uint32_t> &ramdisk_addr,
                               std::optional<uint32_t> &second_addr,
                               std::optional<uint32_t> &tags_addr)
{
    bool can_use_offsets = true;
    uint32_t base_tmp = 0;
    uint32_t kernel_offset = 0;
    uint32_t ramdisk_offset = 0;
    uint32_t second_offset = 0;
    uint32_t tags_offset = 0;

    if (kernel_addr) {
        if (*kernel_addr >= android::DEFAULT_KERNEL_OFFSET) {
            base_tmp = *kernel_addr - android::DEFAULT_KERNEL_OFFSET;
            kernel_offset = *kernel_addr - base_tmp;
        } else {
            can_use_offsets = false;
        }
    }
    if (ramdisk_addr) {
        if (*ramdisk_addr >= base_tmp) {
            ramdisk_offset = *ramdisk_addr - base_tmp;
        } else {
            can_use_offsets = false;
        }
    }
    if (second_addr) {
        if (*second_addr >= base_tmp) {
            second_offset = *second_addr - base_tmp;
        } else {
            can_use_offsets = false;
        }
    }
    if (tags_addr) {
        if (*tags_addr >= base_tmp) {
            tags_offset = *tags_addr - base_tmp;
        } else {
            can_use_offsets = false;
        }
    }

    base = base_tmp;
    if (can_use_offsets) {
        if (kernel_addr) {
            kernel_addr = kernel_offset;
        }
        if (ramdisk_addr) {
            ramdisk_addr = ramdisk_offset;
        }
        if (second_addr) {
            second_addr = second_offset;
        }
        if (tags_addr) {
            tags_addr = tags_offset;
        }
    }
}

static bool offset_to_absolute(std::optional<uint32_t> &base,
                               std::optional<uint32_t> &kernel_offset,
                               std::optional<uint32_t> &ramdisk_offset,
                               std::optional<uint32_t> &second_offset,
                               std::optional<uint32_t> &tags_offset)
{
    static const char *overflow_fmt =
            "'%s' (0x%08x) + '%s' (0x%08x) overflows integer\n";

    if (base) {
        if (kernel_offset) {
            if (*kernel_offset > UINT32_MAX - *base) {
                fprintf(stderr, overflow_fmt, FIELD_BASE, *base,
                        FIELD_KERNEL_OFFSET, *kernel_offset);
                return false;
            }
            *kernel_offset += *base;
        }
        if (ramdisk_offset) {
            if (*ramdisk_offset > UINT32_MAX - *base) {
                fprintf(stderr, overflow_fmt, FIELD_BASE, *base,
                        FIELD_RAMDISK_OFFSET, *ramdisk_offset);
                return false;
            }
            *ramdisk_offset += *base;
        }
        if (second_offset) {
            if (*second_offset > UINT32_MAX - *base) {
                fprintf(stderr, overflow_fmt, FIELD_BASE, *base,
                        FIELD_SECOND_OFFSET, *second_offset);
                return false;
            }
            *second_offset += *base;
        }
        if (tags_offset) {
            if (*tags_offset > UINT32_MAX - *base) {
                fprintf(stderr, overflow_fmt, FIELD_BASE, *base,
                        FIELD_TAGS_OFFSET, *tags_offset);
                return false;
            }
            *tags_offset += *base;
        }
    }

    return true;
}

static inline std::string get_string(const rj::Value &node)
{
    return {node.GetString(), node.GetStringLength()};
}

static bool read_header(const std::string &path, Header &header)
{
    static const char *fmt_unknown_key =
            "Unknown key '%s' or invalid value type\n";
    static const char *fmt_unsupported =
            "Ignoring unsupported key for boot image type: '%s'\n";

    ScopedFILE fp(fopen(path.c_str(), "rb" CLOEXEC_FLAG), fclose);
    if (!fp) {
        fprintf(stderr, "%s: Failed to open for reading: %s\n",
                path.c_str(), strerror(errno));
        return false;
    }

    char read_buf[65536];
    rj::Document document;
    rj::Reader reader;
    rj::FileReadStream is(fp.get(), read_buf, sizeof(read_buf));

    if (document.ParseStream(is).HasParseError()) {
        fprintf(stderr, "%s: JSON parse error at offset %" MB_PRIzu ": %s\n",
                path.c_str(), document.GetErrorOffset(),
                rj::GetParseError_En(document.GetParseError()));
        return false;
    }

    if (!document.IsObject()) {
        fprintf(stderr, "%s: Root is not an oboject\n", path.c_str());
        return false;
    }

    // Fields
    std::optional<uint32_t> base;
    std::optional<uint32_t> kernel_offset;
    std::optional<uint32_t> ramdisk_offset;
    std::optional<uint32_t> second_offset;
    std::optional<uint32_t> tags_offset;

    for (auto const &item : document.GetObject()) {
        auto const &key = get_string(item.name);

        bool ret = true;

        if (key == FIELD_CMDLINE && item.value.IsString()) {
            ret = header.set_kernel_cmdline(get_string(item.value));
        } else if (key == FIELD_BOARD && item.value.IsString()) {
            ret = header.set_board_name(get_string(item.value));
        } else if (key == FIELD_BASE && item.value.IsUint()) {
            base = item.value.GetUint();
        } else if (key == FIELD_KERNEL_OFFSET && item.value.IsUint()) {
            kernel_offset = item.value.GetUint();
        } else if (key == FIELD_RAMDISK_OFFSET && item.value.IsUint()) {
            ramdisk_offset = item.value.GetUint();
        } else if (key == FIELD_SECOND_OFFSET && item.value.IsUint()) {
            second_offset = item.value.GetUint();
        } else if (key == FIELD_TAGS_OFFSET && item.value.IsUint()) {
            tags_offset = item.value.GetUint();
        } else if (key == FIELD_IPL_ADDRESS && item.value.IsUint()) {
            ret = header.set_sony_ipl_address(item.value.GetUint());
        } else if (key == FIELD_RPM_ADDRESS && item.value.IsUint()) {
            ret = header.set_sony_rpm_address(item.value.GetUint());
        } else if (key == FIELD_APPSBL_ADDRESS && item.value.IsUint()) {
            ret = header.set_sony_appsbl_address(item.value.GetUint());
        } else if (key == FIELD_ENTRYPOINT && item.value.IsUint()) {
            ret = header.set_entrypoint_address(item.value.GetUint());
        } else if (key == FIELD_PAGE_SIZE && item.value.IsUint()) {
            ret = header.set_page_size(item.value.GetUint());
        } else {
            fprintf(stderr, fmt_unknown_key, key.c_str());
            return false;
        }

        if (!ret) {
            fprintf(stderr, fmt_unsupported, key.c_str());
            continue;
        }
    }

    if (!offset_to_absolute(base, kernel_offset, ramdisk_offset, second_offset,
                            tags_offset)) {
        return false;
    }

    if (kernel_offset && !header.set_kernel_address(kernel_offset)) {
        fprintf(stderr, fmt_unsupported, FIELD_KERNEL_OFFSET);
    }
    if (ramdisk_offset && !header.set_ramdisk_address(ramdisk_offset)) {
        fprintf(stderr, fmt_unsupported, FIELD_RAMDISK_OFFSET);
    }
    if (second_offset && !header.set_secondboot_address(second_offset)) {
        fprintf(stderr, fmt_unsupported, FIELD_SECOND_OFFSET);
    }
    if (tags_offset && !header.set_kernel_tags_address(tags_offset)) {
        fprintf(stderr, fmt_unsupported, FIELD_TAGS_OFFSET);
    }

    return true;
}

static bool write_header(const std::string &path, const Header &header)
{
    // Try to use base relative to the default kernel offset
    std::optional<uint32_t> base;
    auto kernel_offset = header.kernel_address();
    auto ramdisk_offset = header.ramdisk_address();
    auto second_offset = header.secondboot_address();
    auto tags_offset = header.kernel_tags_address();

    absolute_to_offset(base, kernel_offset, ramdisk_offset, second_offset,
                       tags_offset);

    ScopedFILE fp(fopen(path.c_str(), "wb" CLOEXEC_FLAG), fclose);
    if (!fp) {
        fprintf(stderr, "%s: Failed to open for writing: %s\n",
                path.c_str(), strerror(errno));
        return false;
    }

    // NOTE: RapidJSON has no way of reporting write errors at the moment
    char write_buf[65536];
    rj::FileWriteStream os(fp.get(), write_buf, sizeof(write_buf));
    rj::PrettyWriter<rj::FileWriteStream> writer(os);

    auto cmdline = header.kernel_cmdline();
    auto board_name = header.board_name();
    auto sony_ipl_address = header.sony_ipl_address();
    auto sony_rpm_address = header.sony_rpm_address();
    auto sony_appsbl_address = header.sony_appsbl_address();
    auto entrypoint_address = header.entrypoint_address();
    auto page_size = header.page_size();

    bool failed =
            !writer.StartObject()
            || (cmdline && !cmdline->empty()
                    && !(writer.Key(FIELD_CMDLINE) && writer.String(*cmdline)))
            || (board_name && !board_name->empty()
                    && !(writer.Key(FIELD_BOARD) && writer.String(*board_name)))
            || (base
                    && !(writer.Key(FIELD_BASE) && writer.Uint(*base)))
            || (kernel_offset
                    && !(writer.Key(FIELD_KERNEL_OFFSET) && writer.Uint(*kernel_offset)))
            || (ramdisk_offset
                    && !(writer.Key(FIELD_RAMDISK_OFFSET) && writer.Uint(*ramdisk_offset)))
            || (second_offset
                    && !(writer.Key(FIELD_SECOND_OFFSET) && writer.Uint(*second_offset)))
            || (tags_offset
                    && !(writer.Key(FIELD_TAGS_OFFSET) && writer.Uint(*tags_offset)))
            || (sony_ipl_address
                    && !(writer.Key(FIELD_IPL_ADDRESS) && writer.Uint(*sony_ipl_address)))
            || (sony_rpm_address
                    && !(writer.Key(FIELD_RPM_ADDRESS) && writer.Uint(*sony_rpm_address)))
            || (sony_appsbl_address
                    && !(writer.Key(FIELD_APPSBL_ADDRESS) && writer.Uint(*sony_appsbl_address)))
            || (entrypoint_address
                    && !(writer.Key(FIELD_ENTRYPOINT) && writer.Uint(*entrypoint_address)))
            || (page_size
                    && !(writer.Key(FIELD_PAGE_SIZE) && writer.Uint(*page_size)))
            || !writer.EndObject();

    writer.Flush();

    if (failed) {
        fprintf(stderr, "%s: Failed to write file: %s\n",
                path.c_str(), strerror(errno));
        return false;
    }

    if (fclose(fp.release()) < 0) {
        fprintf(stderr, "%s: Failed to close file: %s\n",
                path.c_str(), strerror(errno));
        return false;
    }

    return true;
}

static bool write_data_file_to_entry(const std::string &path, Writer &writer)
{
    ScopedFILE fp(fopen(path.c_str(), "rb" CLOEXEC_FLAG), fclose);
    if (!fp) {
        // Entries are optional
        if (errno == ENOENT) {
            return true;
        } else {
            fprintf(stderr, "%s: Failed to open for reading: %s\n",
                    path.c_str(), strerror(errno));
            return false;
        }
    }

    char buf[10240];
    size_t n;

    while (true) {
        n = fread(buf, 1, sizeof(buf), fp.get());

        auto bytes_written = writer.write_data(buf, n);
        if (!bytes_written) {
            fprintf(stderr, "Failed to write entry data: %s\n",
                    bytes_written.error().message().c_str());
            return false;
        }

        if (n < sizeof(buf)) {
            if (ferror(fp.get())) {
                fprintf(stderr, "%s: Failed to read file: %s\n",
                        path.c_str(), strerror(errno));
            } else {
                break;
            }
        }
    }

    return true;
}

static bool write_data_entry_to_file(const std::string &path, Reader &reader)
{
    ScopedFILE fp(fopen(path.c_str(), "wb" CLOEXEC_FLAG), fclose);
    if (!fp) {
        fprintf(stderr, "%s: Failed to open for writing: %s\n",
                path.c_str(), strerror(errno));
        return false;
    }

    char buf[10240];

    while (true) {
        auto n = reader.read_data(buf, sizeof(buf));
        if (!n) {
            fprintf(stderr, "Failed to read entry data: %s\n",
                    n.error().message().c_str());
            return false;
        } else if (n.value() == 0) {
            break;
        }

        if (fwrite(buf, 1, n.value(), fp.get()) != n.value()) {
            fprintf(stderr, "%s: Failed to write data: %s\n",
                    path.c_str(), strerror(errno));
            return false;
        }
    }

    if (fclose(fp.release()) < 0) {
        fprintf(stderr, "%s: Failed to close file: %s\n",
                path.c_str(), strerror(errno));
        return false;
    }

    return true;
}

static bool write_file_to_entry(const Paths &paths, Writer &writer,
                                const Entry &entry)
{
    std::string path;

    auto type = entry.type();

    if (!type) {
        fprintf(stderr, "No entry type set!\n");
        return false;
    }

    switch (*type) {
    case ENTRY_TYPE_KERNEL:
        path = paths.kernel;
        break;
    case ENTRY_TYPE_RAMDISK:
        path = paths.ramdisk;
        break;
    case ENTRY_TYPE_SECONDBOOT:
        path = paths.second;
        break;
    case ENTRY_TYPE_DEVICE_TREE:
        path = paths.dt;
        break;
    case ENTRY_TYPE_ABOOT:
        path = paths.aboot;
        break;
    case ENTRY_TYPE_MTK_KERNEL_HEADER:
        path = paths.kernel_mtkhdr;
        break;
    case ENTRY_TYPE_MTK_RAMDISK_HEADER:
        path = paths.ramdisk_mtkhdr;
        break;
    case ENTRY_TYPE_SONY_IPL:
        path = paths.ipl;
        break;
    case ENTRY_TYPE_SONY_RPM:
        path = paths.rpm;
        break;
    case ENTRY_TYPE_SONY_APPSBL:
        path = paths.appsbl;
        break;
    default:
        fprintf(stderr, "Unknown entry type: %d\n", *type);
        return false;
    }

    auto ret = writer.write_entry(entry);
    if (!ret) {
        fprintf(stderr, "Failed to write entry: %s\n",
                ret.error().message().c_str());
        return false;
    }

    return write_data_file_to_entry(path, writer);
}

static bool write_entry_to_file(const Paths &paths, Reader &reader,
                                const Entry &entry)
{
    std::string path;

    auto type = entry.type();

    if (!type) {
        fprintf(stderr, "No entry type set!\n");
        return false;
    }

    switch (*type) {
    case ENTRY_TYPE_KERNEL:
        path = paths.kernel;
        break;
    case ENTRY_TYPE_RAMDISK:
        path = paths.ramdisk;
        break;
    case ENTRY_TYPE_SECONDBOOT:
        path = paths.second;
        break;
    case ENTRY_TYPE_DEVICE_TREE:
        path = paths.dt;
        break;
    case ENTRY_TYPE_ABOOT:
        path = paths.aboot;
        break;
    case ENTRY_TYPE_MTK_KERNEL_HEADER:
        path = paths.kernel_mtkhdr;
        break;
    case ENTRY_TYPE_MTK_RAMDISK_HEADER:
        path = paths.ramdisk_mtkhdr;
        break;
    case ENTRY_TYPE_SONY_IPL:
        path = paths.ipl;
        break;
    case ENTRY_TYPE_SONY_RPM:
        path = paths.rpm;
        break;
    case ENTRY_TYPE_SONY_APPSBL:
        path = paths.appsbl;
        break;
    default:
        fprintf(stderr, "Unknown entry type: %d\n", *type);
        return false;
    }

    return write_data_entry_to_file(path, reader);
}

static bool unpack_main(int argc, char *argv[])
{
    int opt;
    bool no_prefix = false;
    std::string input_file;
    std::string output_dir;
    std::string prefix;
    const char *type = nullptr;
    Paths paths;

    // Arguments with no short options
    enum unpack_options : int
    {
        OPT_OUTPUT_HEADER         = 10000 + 1,
        OPT_OUTPUT_KERNEL         = 10000 + 2,
        OPT_OUTPUT_RAMDISK        = 10000 + 3,
        OPT_OUTPUT_SECOND         = 10000 + 4,
        OPT_OUTPUT_DT             = 10000 + 5,
        OPT_OUTPUT_KERNEL_MTKHDR  = 10000 + 6,
        OPT_OUTPUT_RAMDISK_MTKHDR = 10000 + 7,
        OPT_OUTPUT_IPL            = 10000 + 8,
        OPT_OUTPUT_RPM            = 10000 + 9,
        OPT_OUTPUT_APPSBL         = 10000 + 10,
    };

    static const char short_options[] = "o:p:n" "h";

    static struct option long_options[] = {
        // Arguments with short versions
        {"output",                required_argument, nullptr, 'o'},
        {"prefix",                required_argument, nullptr, 'p'},
        {"noprefix",              required_argument, nullptr, 'n'},
        {"type",                  required_argument, nullptr, 't'},
        // Arguments without short versions
        {"output-header",         required_argument, nullptr, OPT_OUTPUT_HEADER},
        {"output-kernel",         required_argument, nullptr, OPT_OUTPUT_KERNEL},
        {"output-ramdisk",        required_argument, nullptr, OPT_OUTPUT_RAMDISK},
        {"output-second",         required_argument, nullptr, OPT_OUTPUT_SECOND},
        {"output-dt",             required_argument, nullptr, OPT_OUTPUT_DT},
        {"output-kernel_mtkhdr",  required_argument, nullptr, OPT_OUTPUT_KERNEL_MTKHDR},
        {"output-ramdisk_mtkhdr", required_argument, nullptr, OPT_OUTPUT_RAMDISK_MTKHDR},
        {"output-ipl",            required_argument, nullptr, OPT_OUTPUT_IPL},
        {"output-rpm",            required_argument, nullptr, OPT_OUTPUT_RPM},
        {"output-appsbl",         required_argument, nullptr, OPT_OUTPUT_APPSBL},
        // Misc
        {"help",                  no_argument,       nullptr, 'h'},
        {nullptr,                 0,                 nullptr, 0},
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, short_options,
                              long_options, &long_index)) != -1) {
        switch (opt) {
        case 'o':                       output_dir = optarg;           break;
        case 'p':                       prefix = optarg;               break;
        case 'n':                       no_prefix = true;              break;
        case 't':                       type = optarg;                 break;
        case OPT_OUTPUT_HEADER:         paths.header = optarg;         break;
        case OPT_OUTPUT_KERNEL:         paths.kernel = optarg;         break;
        case OPT_OUTPUT_RAMDISK:        paths.ramdisk = optarg;        break;
        case OPT_OUTPUT_SECOND:         paths.second = optarg;         break;
        case OPT_OUTPUT_DT:             paths.dt = optarg;             break;
        case OPT_OUTPUT_KERNEL_MTKHDR:  paths.kernel_mtkhdr = optarg;  break;
        case OPT_OUTPUT_RAMDISK_MTKHDR: paths.ramdisk_mtkhdr = optarg; break;
        case OPT_OUTPUT_IPL:            paths.ipl = optarg;            break;
        case OPT_OUTPUT_RPM:            paths.rpm = optarg;            break;
        case OPT_OUTPUT_APPSBL:         paths.appsbl = optarg;         break;

        case 'h':
            fputs(HELP_UNPACK_USAGE, stdout);
            return true;

        default:
            fputs(HELP_UNPACK_USAGE, stderr);
            return false;
        }
    }

    // There should be one other arguments
    if (argc - optind != 1) {
        fputs(HELP_UNPACK_USAGE, stderr);
        return false;
    }

    input_file = argv[optind];

    if (no_prefix) {
        prefix.clear();
    } else if (prefix.empty()) {
        prefix = mb::io::base_name(input_file);
        prefix += "-";
    }

    if (output_dir.empty()) {
        output_dir = ".";
    }

    prepend_if_empty(paths, output_dir, prefix);

    if (auto r = mb::io::create_directories(output_dir); !r) {
        fprintf(stderr, "%s: Failed to create directory: %s\n",
                output_dir.c_str(), r.error().message().c_str());
        return false;
    }

    // Load the boot image
    Reader reader;
    Header header;
    Entry entry;

    if (type) {
        auto ret = reader.enable_format_by_name(type);
        if (!ret) {
            fprintf(stderr, "Failed to enable format '%s': %s\n",
                    type, ret.error().message().c_str());
            return false;
        }
    } else {
        auto ret = reader.enable_format_all();
        if (!ret) {
            fprintf(stderr, "Failed to enable all formats: %s\n",
                    ret.error().message().c_str());
            return false;
        }
    }

    auto ret = reader.open_filename(input_file);
    if (!ret) {
        fprintf(stderr, "%s: Failed to open for reading: %s\n",
                input_file.c_str(), ret.error().message().c_str());
        return false;
    }

    ret = reader.read_header(header);
    if (!ret) {
        fprintf(stderr, "%s: Failed to read header: %s\n",
                input_file.c_str(), ret.error().message().c_str());
        return false;
    }

    if (!write_header(paths.header, header)) {
        return false;
    }

    while (true) {
        ret = reader.read_entry(entry);
        if (!ret) {
            if (ret.error() == ReaderError::EndOfEntries) {
                break;
            }
            fprintf(stderr, "Failed to read entry: %s\n",
                    ret.error().message().c_str());
            return false;
        }

        if (!write_entry_to_file(paths, reader, entry)) {
            return false;
        }
    }

    return true;
}

static bool pack_main(int argc, char *argv[])
{
    int opt;
    bool no_prefix = false;
    std::string output_file;
    std::string input_dir;
    std::string prefix;
    const char *type = FORMAT_NAME_ANDROID;
    Paths paths;

    // Arguments with no short options
    enum pack_options : int
    {
        // Paths
        OPT_INPUT_HEADER         = 10000 + 1,
        OPT_INPUT_KERNEL         = 10000 + 2,
        OPT_INPUT_RAMDISK        = 10000 + 3,
        OPT_INPUT_SECOND         = 10000 + 4,
        OPT_INPUT_DT             = 10000 + 5,
        OPT_INPUT_ABOOT          = 10000 + 6,
        OPT_INPUT_KERNEL_MTKHDR  = 10000 + 7,
        OPT_INPUT_RAMDISK_MTKHDR = 10000 + 8,
        OPT_INPUT_IPL            = 10000 + 9,
        OPT_INPUT_RPM            = 10000 + 10,
        OPT_INPUT_APPSBL         = 10000 + 11,
    };

    static const char short_options[] = "i:p:nt:" "h";

    static struct option long_options[] = {
        // Arguments with short versions
        {"input",                required_argument, nullptr, 'i'},
        {"prefix",               required_argument, nullptr, 'p'},
        {"noprefix",             required_argument, nullptr, 'n'},
        {"type",                 required_argument, nullptr, 't'},
        // Arguments without short versions
        {"input-header",         required_argument, nullptr, OPT_INPUT_HEADER},
        {"input-kernel",         required_argument, nullptr, OPT_INPUT_KERNEL},
        {"input-ramdisk",        required_argument, nullptr, OPT_INPUT_RAMDISK},
        {"input-second",         required_argument, nullptr, OPT_INPUT_SECOND},
        {"input-dt",             required_argument, nullptr, OPT_INPUT_DT},
        {"input-aboot",          required_argument, nullptr, OPT_INPUT_ABOOT},
        {"input-kernel_mtkhdr",  required_argument, nullptr, OPT_INPUT_KERNEL_MTKHDR},
        {"input-ramdisk_mtkhdr", required_argument, nullptr, OPT_INPUT_RAMDISK_MTKHDR},
        {"input-ipl",            required_argument, nullptr, OPT_INPUT_IPL},
        {"input-rpm",            required_argument, nullptr, OPT_INPUT_RPM},
        {"input-appsbl",         required_argument, nullptr, OPT_INPUT_APPSBL},
        // Misc
        {"help",                 no_argument,       nullptr, 'h'},
        {nullptr,                0,                 nullptr, 0},
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, short_options,
                              long_options, &long_index)) != -1) {
        switch (opt) {
        case 'i':                      input_dir = optarg;            break;
        case 'p':                      prefix = optarg;               break;
        case 'n':                      no_prefix = true;              break;
        case 't':                      type = optarg;                 break;
        case OPT_INPUT_HEADER:         paths.header = optarg;         break;
        case OPT_INPUT_KERNEL:         paths.kernel = optarg;         break;
        case OPT_INPUT_RAMDISK:        paths.ramdisk = optarg;        break;
        case OPT_INPUT_SECOND:         paths.second = optarg;         break;
        case OPT_INPUT_DT:             paths.dt = optarg;             break;
        case OPT_INPUT_ABOOT:          paths.aboot = optarg;          break;
        case OPT_INPUT_KERNEL_MTKHDR:  paths.kernel_mtkhdr = optarg;  break;
        case OPT_INPUT_RAMDISK_MTKHDR: paths.ramdisk_mtkhdr = optarg; break;
        case OPT_INPUT_IPL:            paths.ipl = optarg;            break;
        case OPT_INPUT_RPM:            paths.rpm = optarg;            break;
        case OPT_INPUT_APPSBL:         paths.appsbl = optarg;         break;

        case 'h':
            fputs(HELP_PACK_USAGE, stdout);
            return true;

        default:
            fputs(HELP_PACK_USAGE, stderr);
            return false;
        }
    }

    // There should be one other argument
    if (argc - optind != 1) {
        fputs(HELP_PACK_USAGE, stderr);
        return false;
    }

    output_file = argv[optind];

    if (no_prefix) {
        prefix.clear();
    } else if (prefix.empty()) {
        prefix = mb::io::base_name(output_file);
        prefix += "-";
    }

    if (input_dir.empty()) {
        input_dir = ".";
    }

    prepend_if_empty(paths, input_dir, prefix);

    // Load the boot image
    Writer writer;
    Header header;
    Entry entry;

    if (!writer.set_format_by_name(type)) {
        fprintf(stderr, "Invalid boot image type: %s\n", type);
        return false;
    }

    auto ret = writer.open_filename(output_file);
    if (!ret) {
        fprintf(stderr, "%s: Failed to open for writing: %s\n",
                output_file.c_str(), ret.error().message().c_str());
        return false;
    }

    ret = writer.get_header(header);
    if (!ret) {
        fprintf(stderr, "Failed to get header instance: %s\n",
                ret.error().message().c_str());
        return false;
    }

    if (!read_header(paths.header, header)) {
        return false;
    }

    ret = writer.write_header(header);
    if (!ret) {
        fprintf(stderr, "%s: Failed to read header: %s\n",
                output_file.c_str(), ret.error().message().c_str());
        return false;
    }

    while (true) {
        ret = writer.get_entry(entry);
        if (!ret) {
            if (ret.error() == WriterError::EndOfEntries) {
                break;
            }
            fprintf(stderr, "Failed to get next entry: %s\n",
                    ret.error().message().c_str());
            return false;
        }

        if (!write_file_to_entry(paths, writer, entry)) {
            return false;
        }
    }

    ret = writer.close();
    if (!ret) {
        fprintf(stderr, "Failed to close boot image: %s\n",
                ret.error().message().c_str());
        return false;
    }

    return true;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fputs(HELP_MAIN_USAGE, stdout);
        return EXIT_FAILURE;
    }

    std::string command(argv[1]);
    bool ret = false;

    if (command == "unpack") {
        ret = unpack_main(--argc, ++argv);
    } else if (command == "pack") {
        ret = pack_main(--argc, ++argv);
    } else {
        fputs(HELP_MAIN_USAGE, stderr);
        return EXIT_FAILURE;
    }

    return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
