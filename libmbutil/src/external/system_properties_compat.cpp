/*
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * This file is only used to provide backwards compatibility to property areas
 * created by old versions of init, which occurs when an ota runs.  The updater
 * binary is compiled statically against the newest bionic, but the recovery
 * ramdisk may be using an old version of init.  This can all be removed once
 * OTAs from pre-K versions are no longer supported.
 */

#include <type_traits>

#include <cstring>

#include "mbutil/external/bionic_futex.h"

#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include "mbutil/external/_system_properties.h"

#define TOC_NAME_LEN(toc)        ((toc) >> 24)
#define TOC_TO_INFO(area, toc)   (reinterpret_cast<PropInfoCompat *>( \
    reinterpret_cast<char *>(area) + ((toc) & 0xFFFFFF)))
#define SERIAL_DIRTY(serial)     ((serial) & 1)
#define SERIAL_VALUE_LEN(serial) ((serial) >> 24)

namespace mb
{

struct PropAreaCompat
{
    unsigned volatile count;
    unsigned volatile serial;
    unsigned magic;
    unsigned version;
    unsigned reserved[4];
    unsigned toc[1];
};

struct prop_area;

struct PropInfoCompat
{
    char name[PROP_NAME_MAX];
    unsigned volatile serial;
    char value[PROP_VALUE_MAX];
};

extern prop_area *__system_property_area__;

__LIBC_HIDDEN__ uint32_t __system_property_serial_compat(const prop_info *_pi)
{
    auto *pi = reinterpret_cast<const PropInfoCompat *>(_pi);
    return pi->serial;
}

__LIBC_HIDDEN__ const prop_info *__system_property_find_compat(const char *name)
{
    auto *pa = reinterpret_cast<PropAreaCompat *>(__system_property_area__);
    unsigned count = pa->count;
    unsigned *toc = pa->toc;
    unsigned len = strlen(name);

    if (len >= PROP_NAME_MAX) {
        return nullptr;
    }
    if (len < 1) {
        return nullptr;
    }

    while (count--) {
        unsigned entry = *toc++;
        if (TOC_NAME_LEN(entry) != len) {
            continue;
        }

        PropInfoCompat *pi = TOC_TO_INFO(pa, entry);
        if (memcmp(name, pi->name, len) != 0) {
            continue;
        }

        return reinterpret_cast<const prop_info *>(pi);
    }

    return nullptr;
}

__LIBC_HIDDEN__ int __system_property_read_compat(const prop_info *_pi,
                                                  char *name, char *value)
{
    auto *pi = reinterpret_cast<const PropInfoCompat *>(_pi);

    for (;;) {
        unsigned serial = pi->serial;
        while (SERIAL_DIRTY(serial)) {
            __futex_wait(const_cast<unsigned volatile *>(&pi->serial), serial,
                         nullptr);
            serial = pi->serial;
        }
        unsigned len = SERIAL_VALUE_LEN(serial);
        memcpy(value, pi->value, len + 1);
        if (serial == pi->serial) {
            if (name) {
                strcpy(name, pi->name);
            }
            return len;
        }
    }
}

__LIBC_HIDDEN__ int __system_property_foreach_compat(
        void (*propfn)(const prop_info *pi, void *cookie),
        void *cookie)
{
    auto *pa = reinterpret_cast<PropAreaCompat *>(__system_property_area__);

    for (unsigned i = 0; i < pa->count; i++) {
        unsigned entry = pa->toc[i];
        auto *pi = TOC_TO_INFO(pa, entry);
        propfn(reinterpret_cast<const prop_info *>(pi), cookie);
    }

    return 0;
}

}
