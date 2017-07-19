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

// libmbcommon
#include <mbcommon/common.h>
#include <mbcommon/libc/stdio.h>

// libmbbootimg
#include <mbbootimg/entry.h>
#include <mbbootimg/format/android_defs.h>
#include <mbbootimg/header.h>
#include <mbbootimg/reader.h>
#include <mbbootimg/writer.h>

// libmbpio
#include <mbpio/directory.h>
#include <mbpio/error.h>
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


typedef std::unique_ptr<FILE, decltype(fclose) *> ScopedFILE;
typedef std::unique_ptr<MbBiReader, decltype(mb_bi_reader_free) *> ScopedReader;
typedef std::unique_ptr<MbBiWriter, decltype(mb_bi_writer_free) *> ScopedWriter;

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
    "Pass -h/--help as a argument to a command to see it's available options.\n"

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
    "                  (one of: android, bump, loki, mtk, sonyelf)\n" \
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
    "    <output directory>/<prefix>header.txt\n" \
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
    "                  Output type of the boot image (use header.txt if unspecified)\n" \
    "                  (one of: android, bump, loki, mtk, sonyelf)\n" \
    "  --input-<item> <item path>\n" \
    "                  Custom path for a particular item\n" \
    "\n" \
    "The following items are loaded to create a new boot image.\n" \
    "\n" \
    HELP_HEADERS \
    "\n" \
    HELP_IMAGES \
    "\n" \
    HELP_LEGEND \
    "\n" \
    "Input files:\n" \
    "\n" \
    "The header fields are loaded from the following path:\n" \
    "\n" \
    "    <input directory>/<prefix>header.txt\n" \
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

template <typename F>
class Finally {
public:
    Finally(F f) : _f(f)
    {
    }

    ~Finally()
    {
        _f();
    }

private:
    F _f;
};

template <typename F>
Finally<F> finally(F f)
{
    return Finally<F>(f);
}

template<typename UIntType>
static inline bool str_to_unum(const char *str, int base, UIntType *out)
{
    static_assert(!std::is_signed<UIntType>::value,
                  "Integer type is not unsigned");
    static_assert(std::numeric_limits<UIntType>::max() <= ULLONG_MAX,
                  "Integer type to too large to handle");

    char *end;
    errno = 0;
    auto num = strtoull(str, &end, base);
    if (errno == ERANGE
            || num > std::numeric_limits<UIntType>::max()) {
        errno = ERANGE;
        return false;
    } else if (*str == '\0' || *end != '\0') {
        errno = EINVAL;
        return false;
    }
    *out = static_cast<UIntType>(num);
    return true;
}

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
        paths.header = io::pathJoin({dir, prefix + "header.txt"});
    if (paths.kernel.empty())
        paths.kernel = io::pathJoin({dir, prefix + IMAGE_KERNEL});
    if (paths.ramdisk.empty())
        paths.ramdisk = io::pathJoin({dir, prefix + IMAGE_RAMDISK});
    if (paths.second.empty())
        paths.second = io::pathJoin({dir, prefix + IMAGE_SECOND});
    if (paths.dt.empty())
        paths.dt = io::pathJoin({dir, prefix + IMAGE_DT});
    if (paths.aboot.empty())
        paths.aboot = io::pathJoin({dir, prefix + IMAGE_ABOOT});
    if (paths.kernel_mtkhdr.empty())
        paths.kernel_mtkhdr = io::pathJoin({dir, prefix + IMAGE_KERNEL_MTKHDR});
    if (paths.ramdisk_mtkhdr.empty())
        paths.ramdisk_mtkhdr = io::pathJoin({dir, prefix + IMAGE_RAMDISK_MTKHDR});
    if (paths.ipl.empty())
        paths.ipl = io::pathJoin({dir, prefix + IMAGE_IPL});
    if (paths.rpm.empty())
        paths.rpm = io::pathJoin({dir, prefix + IMAGE_RPM});
    if (paths.appsbl.empty())
        paths.appsbl = io::pathJoin({dir, prefix + IMAGE_APPSBL});
}

static void absolute_to_offset(uint32_t *base_ptr,
                               uint32_t *kernel_addr_ptr,
                               uint32_t *ramdisk_addr_ptr,
                               uint32_t *second_addr_ptr,
                               uint32_t *tags_addr_ptr)
{
    bool can_use_offsets = true;
    uint32_t base = 0;
    uint32_t kernel_offset = 0;
    uint32_t ramdisk_offset = 0;
    uint32_t second_offset = 0;
    uint32_t tags_offset = 0;

    if (kernel_addr_ptr) {
        if (*kernel_addr_ptr >= ANDROID_DEFAULT_KERNEL_OFFSET) {
            base = *kernel_addr_ptr - ANDROID_DEFAULT_KERNEL_OFFSET;
            kernel_offset = *kernel_addr_ptr - base;
        } else {
            can_use_offsets = false;
        }
    }
    if (ramdisk_addr_ptr) {
        if (*ramdisk_addr_ptr >= base) {
            ramdisk_offset = *ramdisk_addr_ptr - base;
        } else {
            can_use_offsets = false;
        }
    }
    if (second_addr_ptr) {
        if (*second_addr_ptr >= base) {
            second_offset = *second_addr_ptr - base;
        } else {
            can_use_offsets = false;
        }
    }
    if (tags_addr_ptr) {
        if (*tags_addr_ptr >= base) {
            tags_offset = *tags_addr_ptr - base;
        } else {
            can_use_offsets = false;
        }
    }

    *base_ptr = base;
    if (can_use_offsets) {
        if (kernel_addr_ptr) {
            *kernel_addr_ptr = kernel_offset;
        }
        if (ramdisk_addr_ptr) {
            *ramdisk_addr_ptr = ramdisk_offset;
        }
        if (second_addr_ptr) {
            *second_addr_ptr = second_offset;
        }
        if (tags_addr_ptr) {
            *tags_addr_ptr = tags_offset;
        }
    }
}

static bool offset_to_absolute(uint32_t *base_ptr,
                               uint32_t *kernel_offset_ptr,
                               uint32_t *ramdisk_offset_ptr,
                               uint32_t *second_offset_ptr,
                               uint32_t *tags_offset_ptr)
{
    static const char *overflow_fmt =
            "'%s' (0x%08x) + '%s' (0x%08x) overflows integer\n";

    if (base_ptr) {
        if (kernel_offset_ptr) {
            if (*kernel_offset_ptr > UINT32_MAX - *base_ptr) {
                fprintf(stderr, overflow_fmt, FIELD_BASE, *base_ptr,
                        FIELD_KERNEL_OFFSET, *kernel_offset_ptr);
                return false;
            }
            *kernel_offset_ptr += *base_ptr;
        }
        if (ramdisk_offset_ptr) {
            if (*ramdisk_offset_ptr > UINT32_MAX - *base_ptr) {
                fprintf(stderr, overflow_fmt, FIELD_BASE, *base_ptr,
                        FIELD_RAMDISK_OFFSET, *ramdisk_offset_ptr);
                return false;
            }
            *ramdisk_offset_ptr += *base_ptr;
        }
        if (second_offset_ptr) {
            if (*second_offset_ptr > UINT32_MAX - *base_ptr) {
                fprintf(stderr, overflow_fmt, FIELD_BASE, *base_ptr,
                        FIELD_SECOND_OFFSET, *second_offset_ptr);
                return false;
            }
            *second_offset_ptr += *base_ptr;
        }
        if (tags_offset_ptr) {
            if (*tags_offset_ptr > UINT32_MAX - *base_ptr) {
                fprintf(stderr, overflow_fmt, FIELD_BASE, *base_ptr,
                        FIELD_TAGS_OFFSET, *tags_offset_ptr);
                return false;
            }
            *tags_offset_ptr += *base_ptr;
        }
    }

    return true;
}

static bool read_header(const std::string &path, MbBiHeader *header)
{
    static const char *fmt_unknown_key =
            "Unknown key: '%s'\n";
    static const char *fmt_invalid_value =
            "Invalid value for key '%s': '%s'\n";
    static const char *fmt_unsupported =
            "Ignoring unsupported key for boot image type: '%s'\n";
    static const char *fmt_failed_to_set =
            "Failed to set field: '%s'\n";

    ScopedFILE fp(fopen(path.c_str(), "rb"), fclose);
    if (!fp) {
        fprintf(stderr, "%s: Failed to open for reading: %s\n",
                path.c_str(), strerror(errno));
        return false;
    }

    char *line = nullptr;
    size_t len = 0;
    ssize_t read;

    auto free_line = finally([&]{
        free(line);
    });

    // Fields
    uint32_t base;
    uint32_t kernel_offset;
    uint32_t ramdisk_offset;
    uint32_t second_offset;
    uint32_t tags_offset;
    bool have_base = false;
    bool have_kernel_offset = false;
    bool have_ramdisk_offset = false;
    bool have_second_offset = false;
    bool have_tags_offset = false;

    while ((read = mb_getline(&line, &len, fp.get())) >= 0) {
        char *ptr = line;

        // Skip leading whitespace
        while (*ptr && isspace(*ptr)) {
            ++ptr;
        }

        // Skip empty and commented lines
        if (*ptr == '\0' || *ptr == '#') {
            continue;
        }

        // Strip newline
        if (read > 0 && line[read - 1] == '\n') {
            line[read - 1] = '\0';
            --read;
        }

        char *equals = strchr(ptr, '=');
        if (!equals) {
            fprintf(stderr, "Invalid line: %s\n", line);
            return false;
        }

        *equals = '\0';
        const char *key = ptr;
        const char *value = equals + 1;

        int ret = MB_BI_OK;
        bool valid = true;

        if (strcmp(key, FIELD_CMDLINE) == 0) {
            ret = mb_bi_header_set_kernel_cmdline(header, value);
        } else if (strcmp(key, FIELD_BOARD) == 0) {
            ret = mb_bi_header_set_board_name(header, value);
        } else if (strcmp(key, FIELD_BASE) == 0) {
            valid = str_to_unum(value, 16, &base);
            have_base = true;
        } else if (strcmp(key, FIELD_KERNEL_OFFSET) == 0) {
            valid = str_to_unum(value, 16, &kernel_offset);
            have_kernel_offset = true;
        } else if (strcmp(key, FIELD_RAMDISK_OFFSET) == 0) {
            valid = str_to_unum(value, 16, &ramdisk_offset);
            have_ramdisk_offset = true;
        } else if (strcmp(key, FIELD_SECOND_OFFSET) == 0) {
            valid = str_to_unum(value, 16, &second_offset);
            have_second_offset = true;
        } else if (strcmp(key, FIELD_TAGS_OFFSET) == 0) {
            valid = str_to_unum(value, 16, &tags_offset);
            have_tags_offset = true;
        } else if (strcmp(key, FIELD_IPL_ADDRESS) == 0) {
            uint32_t ipl_address;
            valid = str_to_unum(value, 16, &ipl_address);
            if (valid) {
                ret = mb_bi_header_set_sony_ipl_address(header, ipl_address);
            }
        } else if (strcmp(key, FIELD_RPM_ADDRESS) == 0) {
            uint32_t rpm_address;
            valid = str_to_unum(value, 16, &rpm_address);
            if (valid) {
                ret = mb_bi_header_set_sony_rpm_address(header, rpm_address);
            }
        } else if (strcmp(key, FIELD_APPSBL_ADDRESS) == 0) {
            uint32_t appsbl_address;
            valid = str_to_unum(value, 16, &appsbl_address);
            if (valid) {
                ret = mb_bi_header_set_sony_appsbl_address(header, appsbl_address);
            }
        } else if (strcmp(key, FIELD_ENTRYPOINT) == 0) {
            uint32_t entrypoint;
            valid = str_to_unum(value, 16, &entrypoint);
            if (valid) {
                ret = mb_bi_header_set_entrypoint_address(header, entrypoint);
            }
        } else if (strcmp(key, FIELD_PAGE_SIZE) == 0) {
            uint32_t page_size;
            valid = str_to_unum(value, 10, &page_size);
            if (valid) {
                ret = mb_bi_header_set_page_size(header, page_size);
            }
        } else {
            fprintf(stderr, fmt_unknown_key, key);
            return false;
        }

        if (!valid) {
            fprintf(stderr, fmt_invalid_value, key, value);
            return false;
        } else if (ret == MB_BI_UNSUPPORTED) {
            fprintf(stderr, fmt_unsupported, key);
            continue;
        } else if (ret < 0) {
            fprintf(stderr, fmt_failed_to_set, key);
            return false;
        }
    }

    if (!offset_to_absolute(have_base ? &base : nullptr,
                            have_kernel_offset ? &kernel_offset : nullptr,
                            have_ramdisk_offset ? &ramdisk_offset : nullptr,
                            have_second_offset ? &second_offset : nullptr,
                            have_tags_offset ? &tags_offset : nullptr)) {
        return false;
    }

    int ret;

    if (have_kernel_offset) {
        ret = mb_bi_header_set_kernel_address(header, kernel_offset);
        if (ret == MB_BI_UNSUPPORTED) {
            fprintf(stderr, fmt_unsupported, FIELD_KERNEL_OFFSET);
        } else if (ret < 0) {
            fprintf(stderr, fmt_failed_to_set, FIELD_KERNEL_OFFSET);
            return false;
        }
    }
    if (have_ramdisk_offset) {
        ret = mb_bi_header_set_ramdisk_address(header, ramdisk_offset);
        if (ret == MB_BI_UNSUPPORTED) {
            fprintf(stderr, fmt_unsupported, FIELD_RAMDISK_OFFSET);
        } else if (ret < 0) {
            fprintf(stderr, fmt_failed_to_set, FIELD_RAMDISK_OFFSET);
            return false;
        }
    }
    if (have_second_offset) {
        ret = mb_bi_header_set_secondboot_address(header, second_offset);
        if (ret == MB_BI_UNSUPPORTED) {
            fprintf(stderr, fmt_unsupported, FIELD_SECOND_OFFSET);
        } else if (ret < 0) {
            fprintf(stderr, fmt_failed_to_set, FIELD_SECOND_OFFSET);
            return false;
        }
    }
    if (have_tags_offset) {
        ret = mb_bi_header_set_kernel_tags_address(header, tags_offset);
        if (ret == MB_BI_UNSUPPORTED) {
            fprintf(stderr, fmt_unsupported, FIELD_TAGS_OFFSET);
        } else if (ret < 0) {
            fprintf(stderr, fmt_failed_to_set, FIELD_TAGS_OFFSET);
            return false;
        }
    }

    return true;
}

static bool write_header(const std::string &path, MbBiHeader *header)
{
    // Try to use base relative to the default kernel offset
    uint32_t base;
    uint32_t kernel_offset;
    uint32_t ramdisk_offset;
    uint32_t second_offset;
    uint32_t tags_offset;
    bool have_kernel_offset = mb_bi_header_kernel_address_is_set(header);
    bool have_ramdisk_offset = mb_bi_header_ramdisk_address_is_set(header);
    bool have_second_offset = mb_bi_header_secondboot_address_is_set(header);
    bool have_tags_offset = mb_bi_header_kernel_tags_address_is_set(header);

    if (have_kernel_offset) {
        kernel_offset = mb_bi_header_kernel_address(header);
    }
    if (have_ramdisk_offset) {
        ramdisk_offset = mb_bi_header_ramdisk_address(header);
    }
    if (have_second_offset) {
        second_offset = mb_bi_header_secondboot_address(header);
    }
    if (have_tags_offset) {
        tags_offset = mb_bi_header_kernel_tags_address(header);
    }

    absolute_to_offset(&base,
                       have_kernel_offset ? &kernel_offset : nullptr,
                       have_ramdisk_offset ? &ramdisk_offset : nullptr,
                       have_second_offset ? &second_offset : nullptr,
                       have_tags_offset ? &tags_offset : nullptr);

    ScopedFILE fp(fopen(path.c_str(), "wb"), fclose);
    if (!fp) {
        fprintf(stderr, "%s: Failed to open for writing: %s\n",
                path.c_str(), strerror(errno));
        return false;
    }

    const char *cmdline = mb_bi_header_kernel_cmdline(header);
    const char *board_name = mb_bi_header_board_name(header);

    bool failed =
            (cmdline && *cmdline && fprintf(
                    fp.get(), "%s=%s\n", FIELD_CMDLINE, cmdline) < 0)
            || (board_name && *board_name && fprintf(
                    fp.get(), "%s=%s\n", FIELD_BOARD, board_name) < 0)
            || (fprintf(
                    fp.get(), "%s=%08x\n", FIELD_BASE, base) < 0)
            || (have_kernel_offset && fprintf(
                    fp.get(), "%s=%08x\n", FIELD_KERNEL_OFFSET, kernel_offset) < 0)
            || (have_ramdisk_offset && fprintf(
                    fp.get(), "%s=%08x\n", FIELD_RAMDISK_OFFSET, ramdisk_offset) < 0)
            || (have_second_offset && fprintf(
                    fp.get(), "%s=%08x\n", FIELD_SECOND_OFFSET, second_offset) < 0)
            || (have_tags_offset && fprintf(
                    fp.get(), "%s=%08x\n", FIELD_TAGS_OFFSET, tags_offset) < 0)
            || (mb_bi_header_sony_ipl_address_is_set(header) && fprintf(
                    fp.get(), "%s=%08x\n", FIELD_IPL_ADDRESS,
                            mb_bi_header_sony_ipl_address(header)) < 0)
            || (mb_bi_header_sony_rpm_address_is_set(header) && fprintf(
                    fp.get(), "%s=%08x\n", FIELD_RPM_ADDRESS,
                            mb_bi_header_sony_rpm_address(header)) < 0)
            || (mb_bi_header_sony_appsbl_address_is_set(header) && fprintf(
                    fp.get(), "%s=%08x\n", FIELD_APPSBL_ADDRESS,
                            mb_bi_header_sony_appsbl_address(header)) < 0)
            || (mb_bi_header_entrypoint_address_is_set(header) && fprintf(
                    fp.get(), "%s=%08x\n", FIELD_ENTRYPOINT,
                            mb_bi_header_entrypoint_address(header)) < 0)
            || (mb_bi_header_page_size_is_set(header) && fprintf(
                    fp.get(), "%s=%u\n", FIELD_PAGE_SIZE,
                            mb_bi_header_page_size(header)) < 0);

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

static bool write_data_file_to_entry(const std::string &path, MbBiWriter *biw)
{
    ScopedFILE fp(fopen(path.c_str(), "rb"), fclose);
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

        size_t bytes_written;

        if (mb_bi_writer_write_data(biw, buf, n, &bytes_written) != MB_BI_OK
                || bytes_written != n) {
            fprintf(stderr, "Failed to write entry data: %s\n",
                    mb_bi_writer_error_string(biw));
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

static bool write_data_entry_to_file(const std::string &path, MbBiReader *bir)
{
    ScopedFILE fp(fopen(path.c_str(), "wb"), fclose);
    if (!fp) {
        fprintf(stderr, "%s: Failed to open for writing: %s\n",
                path.c_str(), strerror(errno));
        return false;
    }

    int ret;
    char buf[10240];
    size_t n;

    while ((ret = mb_bi_reader_read_data(bir, buf, sizeof(buf), &n))
            == MB_BI_OK) {
        if (fwrite(buf, 1, n, fp.get()) != n) {
            fprintf(stderr, "%s: Failed to write data: %s\n",
                    path.c_str(), strerror(errno));
            return false;
        }
    }

    if (ret != MB_BI_EOF) {
        fprintf(stderr, "Failed to read entry data: %s\n",
                mb_bi_reader_error_string(bir));
        return false;
    }

    if (fclose(fp.release()) < 0) {
        fprintf(stderr, "%s: Failed to close file: %s\n",
                path.c_str(), strerror(errno));
        return false;
    }

    return true;
}

static bool write_file_to_entry(const Paths &paths, MbBiWriter *biw,
                                MbBiEntry *entry)
{
    std::string path;

    if (!mb_bi_entry_type_is_set(entry)) {
        fprintf(stderr, "No entry type set!\n");
        return false;
    }

    switch (mb_bi_entry_type(entry)) {
    case MB_BI_ENTRY_KERNEL:
        path = paths.kernel;
        break;
    case MB_BI_ENTRY_RAMDISK:
        path = paths.ramdisk;
        break;
    case MB_BI_ENTRY_SECONDBOOT:
        path = paths.second;
        break;
    case MB_BI_ENTRY_DEVICE_TREE:
        path = paths.dt;
        break;
    case MB_BI_ENTRY_ABOOT:
        path = paths.aboot;
        break;
    case MB_BI_ENTRY_MTK_KERNEL_HEADER:
        path = paths.kernel_mtkhdr;
        break;
    case MB_BI_ENTRY_MTK_RAMDISK_HEADER:
        path = paths.ramdisk_mtkhdr;
        break;
    case MB_BI_ENTRY_SONY_IPL:
        path = paths.ipl;
        break;
    case MB_BI_ENTRY_SONY_RPM:
        path = paths.rpm;
        break;
    case MB_BI_ENTRY_SONY_APPSBL:
        path = paths.appsbl;
        break;
    default:
        fprintf(stderr, "Unknown entry type: %d\n", mb_bi_entry_type(entry));
        return false;
    }

    if (mb_bi_writer_write_entry(biw, entry) != MB_BI_OK) {
        fprintf(stderr, "Failed to write entry: %s\n",
                mb_bi_writer_error_string(biw));
        return false;
    }

    return write_data_file_to_entry(path, biw);
}

static bool write_entry_to_file(const Paths &paths, MbBiReader *bir,
                                MbBiEntry *entry)
{
    std::string path;

    if (!mb_bi_entry_type_is_set(entry)) {
        fprintf(stderr, "No entry type set!\n");
        return false;
    }

    switch (mb_bi_entry_type(entry)) {
    case MB_BI_ENTRY_KERNEL:
        path = paths.kernel;
        break;
    case MB_BI_ENTRY_RAMDISK:
        path = paths.ramdisk;
        break;
    case MB_BI_ENTRY_SECONDBOOT:
        path = paths.second;
        break;
    case MB_BI_ENTRY_DEVICE_TREE:
        path = paths.dt;
        break;
    case MB_BI_ENTRY_ABOOT:
        path = paths.aboot;
        break;
    case MB_BI_ENTRY_MTK_KERNEL_HEADER:
        path = paths.kernel_mtkhdr;
        break;
    case MB_BI_ENTRY_MTK_RAMDISK_HEADER:
        path = paths.ramdisk_mtkhdr;
        break;
    case MB_BI_ENTRY_SONY_IPL:
        path = paths.ipl;
        break;
    case MB_BI_ENTRY_SONY_RPM:
        path = paths.rpm;
        break;
    case MB_BI_ENTRY_SONY_APPSBL:
        path = paths.appsbl;
        break;
    default:
        fprintf(stderr, "Unknown entry type: %d\n", mb_bi_entry_type(entry));
        return false;
    }

    return write_data_entry_to_file(path, bir);
}

bool unpack_main(int argc, char *argv[])
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
        {"output",                required_argument, 0, 'o'},
        {"prefix",                required_argument, 0, 'p'},
        {"noprefix",              required_argument, 0, 'n'},
        {"type",                  required_argument, 0, 't'},
        // Arguments without short versions
        {"output-header",         required_argument, 0, OPT_OUTPUT_HEADER},
        {"output-kernel",         required_argument, 0, OPT_OUTPUT_KERNEL},
        {"output-ramdisk",        required_argument, 0, OPT_OUTPUT_RAMDISK},
        {"output-second",         required_argument, 0, OPT_OUTPUT_SECOND},
        {"output-dt",             required_argument, 0, OPT_OUTPUT_DT},
        {"output-kernel_mtkhdr",  required_argument, 0, OPT_OUTPUT_KERNEL_MTKHDR},
        {"output-ramdisk_mtkhdr", required_argument, 0, OPT_OUTPUT_RAMDISK_MTKHDR},
        {"output-ipl",            required_argument, 0, OPT_OUTPUT_IPL},
        {"output-rpm",            required_argument, 0, OPT_OUTPUT_RPM},
        {"output-appsbl",         required_argument, 0, OPT_OUTPUT_APPSBL},
        // Misc
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
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
        prefix = io::baseName(input_file);
        prefix += "-";
    }

    if (output_dir.empty()) {
        output_dir = ".";
    }

    prepend_if_empty(paths, output_dir, prefix);

    if (!io::createDirectories(output_dir)) {
        fprintf(stderr, "%s: Failed to create directory: %s\n",
                output_dir.c_str(), io::lastErrorString().c_str());
        return false;
    }

    // Load the boot image
    ScopedReader bir(mb_bi_reader_new(), mb_bi_reader_free);
    MbBiHeader *header;
    MbBiEntry *entry;
    int ret;

    if (!bir) {
        fprintf(stderr, "Failed to allocate reader: %s\n", strerror(errno));
        return false;
    }

    if (type) {
        ret = mb_bi_reader_enable_format_by_name(bir.get(), type);
        if (ret != MB_BI_OK) {
            fprintf(stderr, "Failed to enable format '%s': %s\n",
                    type, mb_bi_reader_error_string(bir.get()));
            return false;
        }
    } else {
        ret = mb_bi_reader_enable_format_all(bir.get());
        if (ret != MB_BI_OK) {
            fprintf(stderr, "Failed to enable all formats: %s\n",
                    mb_bi_reader_error_string(bir.get()));
            return false;
        }
    }

    ret = mb_bi_reader_open_filename(bir.get(), input_file.c_str());
    if (ret != MB_BI_OK) {
        fprintf(stderr, "%s: Failed to open for reading: %s\n",
                input_file.c_str(), mb_bi_reader_error_string(bir.get()));
        return false;
    }

    ret = mb_bi_reader_read_header(bir.get(), &header);
    if (ret != MB_BI_OK) {
        fprintf(stderr, "%s: Failed to read header: %s\n",
                input_file.c_str(), mb_bi_reader_error_string(bir.get()));
        return false;
    }

    if (!write_header(paths.header, header)) {
        return false;
    }

    while ((ret = mb_bi_reader_read_entry(bir.get(), &entry)) == MB_BI_OK) {
        if (!write_entry_to_file(paths, bir.get(), entry)) {
            return false;
        }
    }

    if (ret != MB_BI_EOF) {
        fprintf(stderr, "Failed to read entry: %s\n",
                mb_bi_reader_error_string(bir.get()));
        return false;
    }

    return true;
}

bool pack_main(int argc, char *argv[])
{
    int opt;
    bool no_prefix = false;
    std::string output_file;
    std::string input_dir;
    std::string prefix;
    const char *type = MB_BI_FORMAT_NAME_ANDROID;
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
        {"input",                required_argument, 0, 'i'},
        {"prefix",               required_argument, 0, 'p'},
        {"noprefix",             required_argument, 0, 'n'},
        {"type",                 required_argument, 0, 't'},
        // Arguments without short versions
        {"input-header",         required_argument, 0, OPT_INPUT_HEADER},
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
        // Misc
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
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
        prefix = io::baseName(output_file);
        prefix += "-";
    }

    if (input_dir.empty()) {
        input_dir = ".";
    }

    prepend_if_empty(paths, input_dir, prefix);

    // Load the boot image
    ScopedWriter biw(mb_bi_writer_new(), mb_bi_writer_free);
    MbBiHeader *header;
    MbBiEntry *entry;
    int ret;

    if (!biw) {
        fprintf(stderr, "Failed to allocate writer: %s\n", strerror(errno));
        return false;
    }

    ret = mb_bi_writer_set_format_by_name(biw.get(), type);
    if (ret != MB_BI_OK) {
        fprintf(stderr, "Invalid boot image type: %s\n", type);
        return false;
    }

    ret = mb_bi_writer_open_filename(biw.get(), output_file.c_str());
    if (ret != MB_BI_OK) {
        fprintf(stderr, "%s: Failed to open for writing: %s\n",
                output_file.c_str(), mb_bi_writer_error_string(biw.get()));
        return false;
    }

    ret = mb_bi_writer_get_header(biw.get(), &header);
    if (ret != MB_BI_OK) {
        fprintf(stderr, "Failed to get header instance: %s\n",
                mb_bi_writer_error_string(biw.get()));
        return false;
    }

    if (!read_header(paths.header, header)) {
        return false;
    }

    ret = mb_bi_writer_write_header(biw.get(), header);
    if (ret != MB_BI_OK) {
        fprintf(stderr, "%s: Failed to read header: %s\n",
                output_file.c_str(), mb_bi_writer_error_string(biw.get()));
        return false;
    }

    while ((ret = mb_bi_writer_get_entry(biw.get(), &entry)) == MB_BI_OK) {
        if (!write_file_to_entry(paths, biw.get(), entry)) {
            return false;
        }
    }

    if (ret != MB_BI_EOF) {
        fprintf(stderr, "Failed to get next entry: %s\n",
                mb_bi_writer_error_string(biw.get()));
        return false;
    }

    ret = mb_bi_writer_close(biw.get());
    if (ret != MB_BI_OK) {
        fprintf(stderr, "Failed to close boot image: %s\n",
                mb_bi_writer_error_string(biw.get()));
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
