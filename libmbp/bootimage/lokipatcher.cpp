/*
 * Copyright (c) 2013 Dan Rosenberg. All rights reserved.
 * Copyright (c) 2015 Andrew Gunnerson <andrewgunnerson@gmail.com>
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

/*
 * This is a C++ adaptation of Loki and is licensed under the same terms as the
 * original source code. It can be used standalone (outside of MultiBootPatcher)
 * under the terms above.
 */

#include "bootimage/lokipatcher.h"

#include <cstring>

#include "bootimage/header.h"
#include "private/logging.h"

struct LokiTarget {
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

#define PATTERN1 "\xf0\xb5\x8f\xb0\x06\x46\xf0\xf7"
#define PATTERN2 "\xf0\xb5\x8f\xb0\x07\x46\xf0\xf7"
#define PATTERN3 "\x2d\xe9\xf0\x41\x86\xb0\xf1\xf7"
#define PATTERN4 "\x2d\xe9\xf0\x4f\xad\xf5\xc6\x6d"
#define PATTERN5 "\x2d\xe9\xf0\x4f\xad\xf5\x21\x7d"
#define PATTERN6 "\x2d\xe9\xf0\x4f\xf3\xb0\x05\x46"


static bool patchShellcode(uint32_t header, uint32_t ramdisk,
                           unsigned char patch[LOKI_SHELLCODE_SIZE])
{
    bool foundHeader = false;
    bool foundRamdisk = false;
    uint32_t *ptr;

    for (size_t i = 0; i < LOKI_SHELLCODE_SIZE - sizeof(uint32_t); ++i) {
        ptr = reinterpret_cast<uint32_t *>(&patch[i]);
        if (*ptr == 0xffffffff) {
            *ptr = header;
            foundHeader = true;
        }

        if (*ptr == 0xeeeeeeee) {
            *ptr = ramdisk;
            foundRamdisk = true;
        }
    }

    return foundHeader && foundRamdisk;
}

bool LokiPatcher::patchImage(std::vector<unsigned char> *data,
                             std::vector<unsigned char> aboot)
{
    // Prevent reading out of bounds
    data->resize((data->size() + 0x2000 + 0xfff) & ~0xfff);
    aboot.resize((aboot.size() + 0xfff) & ~0xfff);

    uint32_t target = 0;
    uint32_t abootBase = *reinterpret_cast<uint32_t *>(aboot.data() + 12) - 0x28;

    // Find the signature checking function via pattern matching
    for (const unsigned char *ptr = aboot.data();
            ptr < aboot.data() + aboot.size() - 0x1000; ++ptr) {
        if (!memcmp(ptr, PATTERN1, 8) ||
                !memcmp(ptr, PATTERN2, 8) ||
                !memcmp(ptr, PATTERN3, 8) ||
                !memcmp(ptr, PATTERN4, 8) ||
                !memcmp(ptr, PATTERN5, 8)) {
            target = static_cast<uint32_t>(ptr - aboot.data() + abootBase);
            break;
        }
    }

    // Do a second pass for the second LG pattern. This is necessary because
    // apparently some LG models have both LG patterns, which throws off the
    // fingerprinting.

    if (!target) {
        for (const unsigned char *ptr = aboot.data();
                ptr < aboot.data() + aboot.size() - 0x1000; ++ptr) {
            if (memcmp(ptr, PATTERN6, 8) == 0) {
                target = static_cast<uint32_t>(ptr - aboot.data() + abootBase);
                break;
            }
        }
    }

    if (!target) {
        LOGE("[Loki] Failed to find function to patch");
        return false;
    }

    LokiTarget *tgt = nullptr;

    for (size_t i = 0; i < (sizeof(targets) / sizeof(targets[0])); ++i) {
        if (targets[i].check_sigs == target) {
            tgt = &targets[i];
            break;
        }
    }

    if (!tgt) {
        LOGE("[Loki] Unsupported aboot image");
        return false;
    }

    FLOGD("[Loki] Detected target %s %s build %s",
          tgt->vendor, tgt->device, tgt->build);

    BootImageHeader *hdr = reinterpret_cast<BootImageHeader *>(data->data());
    LokiHeader *lokiHdr = reinterpret_cast<LokiHeader *>(data->data() + 0x400);

    // Set the Loki header
    memcpy(lokiHdr->magic, LOKI_MAGIC, LOKI_MAGIC_SIZE);
    lokiHdr->recovery = 0;
    strncpy(lokiHdr->build, tgt->build, sizeof(lokiHdr->build) - 1);

    uint32_t pageSize = hdr->page_size;
    uint32_t pageMask = hdr->page_size - 1;

    uint32_t origKernelSize = hdr->kernel_size;
    uint32_t origRamdiskSize = hdr->ramdisk_size;

    FLOGD("[Loki] Original kernel address: 0x%08x", hdr->kernel_addr);
    FLOGD("[Loki] Original ramdisk address: 0x%08x", hdr->ramdisk_addr);

    // Store the original values in unused fields of the header
    lokiHdr->orig_kernel_size = origKernelSize;
    lokiHdr->orig_ramdisk_size = origRamdiskSize;
    lokiHdr->ramdisk_addr = hdr->kernel_addr +
            ((hdr->kernel_size + pageMask) & ~pageMask);

    unsigned char patch[] = LOKI_SHELLCODE;
    if (!patchShellcode(tgt->hdr, hdr->ramdisk_addr, patch)) {
        LOGE("[Loki] Failed to patch shellcode");
        return false;
    }

    // Ramdisk must be aligned to a page boundary
    hdr->kernel_size = ((hdr->kernel_size + pageMask) & ~pageMask) +
            hdr->ramdisk_size;

    // Guarantee 16-byte alignment
    int offset = tgt->check_sigs & 0xf;

    hdr->ramdisk_addr = tgt->check_sigs - offset;

    int fakeSize;

    if (tgt->lg) {
        fakeSize = pageSize;
        hdr->ramdisk_size = pageSize;
    } else {
        fakeSize = 0x200;
        hdr->ramdisk_size = 0;
    }

    std::vector<unsigned char> newImage;

    if (pageSize > data->size()) {
        FLOGE("Header exceeds boot image size by %" PRIzu " bytes",
              pageSize - data->size());
        return false;
    }

    // Write the image header
    newImage.insert(newImage.end(),
                    data->data(),
                    data->data() + pageSize);

    uint32_t pageKernelSize = (origKernelSize + pageMask) & ~pageMask;

    if (pageSize + pageKernelSize > data->size()) {
        FLOGE("Kernel exceeds boot image size by %" PRIzu " bytes",
              pageSize + pageKernelSize - data->size());
        return false;
    }

    // Write the kernel
    newImage.insert(newImage.end(),
                    data->data() + pageSize,
                    data->data() + pageSize + pageKernelSize);

    uint32_t pageRamdiskSize = (origRamdiskSize + pageMask) & ~pageMask;

    if (pageSize + pageKernelSize + pageRamdiskSize > data->size()) {
        FLOGE("Ramdisk exceeds boot image size by %" PRIzu " bytes",
              pageSize + pageKernelSize + pageRamdiskSize - data->size());
        return false;
    }

    // Write the ramdisk
    newImage.insert(newImage.end(),
                    data->data() + pageSize + pageKernelSize,
                    data->data() + pageSize + pageKernelSize + pageRamdiskSize);

    // Write fake size bytes of original code to the output
    newImage.insert(newImage.end(),
                    aboot.data() + tgt->check_sigs - abootBase - offset,
                    aboot.data() + tgt->check_sigs - abootBase - offset + fakeSize);

    // Save this position for later
    uint32_t pos = newImage.size();

    if (hdr->dt_size) {
        LOGD("[Loki] Writing device tree");

        if (pageSize + pageKernelSize + pageRamdiskSize + hdr->dt_size > data->size()) {
            FLOGE("Device tree image exceeds boot image size by %" PRIzu " bytes",
                  pageSize + pageKernelSize + pageRamdiskSize + hdr->dt_size - data->size());
            return false;
        }

        newImage.insert(newImage.end(),
                        data->data() + pageSize + pageKernelSize + pageRamdiskSize,
                        data->data() + pageSize + pageKernelSize + pageRamdiskSize + hdr->dt_size);
    }

    // Write the patch
    memcpy(newImage.data() + pos - (fakeSize - offset), patch, sizeof(patch));

    LOGD("[Loki] Patching completed");

    data->swap(newImage);

    return true;
}
