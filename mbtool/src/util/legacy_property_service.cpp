/*
 * Copyright (C) 2007 The Android Open Source Project
 * Copyright (C) 2018 Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * This is a slight adaptation of TWRP's implementation. Original source code is
 * available at the following link: https://gerrit.omnirom.org/#/c/6019/
 */

#include "util/legacy_property_service.h"

#include <cctype>
#include <climits>
#include <cstring>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "mbcommon/finally.h"
#include "mbcommon/string.h"

#include "mbutil/external/bionic_futex.h"
#include "mbutil/external/system_properties.h"
#include "mbutil/properties.h"


#define TOC_NAME_LEN(toc)       ((toc) >> 24)
#define TOC_TO_INFO(area, toc)  (reinterpret_cast<LegacyPropInfo *>(reinterpret_cast<char *>(area) + ((toc) & 0xFFFFFF)))


static constexpr unsigned int PROP_AREA_MAGIC = 0x504f5250;
static constexpr unsigned int PROP_AREA_VERSION = 0x45434f76;

// (8 header words + 247 toc words) = 1020 bytes
static constexpr size_t PA_COUNT_MAX = 247;
// 1024 bytes header and toc + 247 prop_infos @ 128 bytes = 32640 bytes
static constexpr size_t PA_INFO_START = 1024;
// Property area file size
static constexpr size_t PA_SIZE = 32768;


struct LegacyPropArea
{
    unsigned volatile count;
    unsigned volatile serial;
    unsigned magic;
    unsigned version;
    unsigned reserved[4];
    unsigned toc[1];
};

struct LegacyPropInfo
{
    char name[PROP_NAME_MAX];
    unsigned volatile serial;
    char value[PROP_VALUE_MAX];
};


LegacyPropertyService::LegacyPropertyService()
    : m_initialized(false)
    , m_data(nullptr)
    , m_size(PA_SIZE)
    , m_fd(-1)
    , m_prop_area(nullptr)
    , m_prop_infos(nullptr)
{
}

LegacyPropertyService::~LegacyPropertyService()
{
    if (m_data) {
        munmap(m_data, m_size);
    }
    if (m_fd >= 0) {
        close(m_fd);
    }
}

bool LegacyPropertyService::set(std::string_view name, std::string_view value)
{
    if (name.size() >= PROP_NAME_MAX
            || value.size() >= PROP_VALUE_MAX
            || name.empty()) {
        return false;
    }

    if (auto *pi = find_property(name)) {
        // ro.* properties may NEVER be modified once set
        if (mb::starts_with(name, "ro.")) {
            return false;
        }

        update_property(pi, value);
        m_prop_area->serial++;
        __futex_wake(&m_prop_area->serial, INT32_MAX);
    } else {
        if (m_prop_area->count == PA_COUNT_MAX) {
            return false;
        }

        pi = m_prop_infos + m_prop_area->count;
        pi->serial = (value.size() << 24);
        memcpy(pi->name, name.data(), name.size());
        memcpy(pi->value, value.data(), value.size());
        pi->value[PROP_NAME_MAX - 1] = '\0';
        pi->value[PROP_VALUE_MAX - 1] = '\0';

        m_prop_area->toc[m_prop_area->count] =
                (name.size() << 24) | (reinterpret_cast<uintptr_t>(pi)
                        - reinterpret_cast<uintptr_t>(m_prop_area));

        m_prop_area->count++;
        m_prop_area->serial++;
        __futex_wake(&m_prop_area->serial, INT32_MAX);
    }

    return true;
}

std::pair<int, size_t> LegacyPropertyService::workspace()
{
    return {m_fd, m_size};
}

bool LegacyPropertyService::initialize_workspace()
{
    int fd = open("/dev/__legacy_properties__",
                  O_RDWR | O_CREAT | O_CLOEXEC, 0600);
    if (fd < 0) {
        return false;
    }

    auto close_fd = mb::finally([&] {
        close(fd);
    });

    if (ftruncate(fd, m_size) < 0) {
        return false;
    }

    void *data = mmap(nullptr, m_size, PROT_READ | PROT_WRITE,
                      MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        return false;
    }

    auto unmap_data = mb::finally([&] {
        munmap(data, m_size);
    });

    int ro_fd = open("/dev/__legacy_properties__", O_RDONLY | O_CLOEXEC);
    if (ro_fd < 0) {
        return false;
    }

    unlink("/dev/__legacy_properties__");

    unmap_data.dismiss();

    m_data = data;
    m_fd = ro_fd;

    return true;
}

bool LegacyPropertyService::initialize()
{
    if (m_initialized) {
        return false;
    }

    if (!initialize_workspace()) {
        return false;
    }

    m_prop_infos = reinterpret_cast<LegacyPropInfo *>(
            (reinterpret_cast<char *>(m_data) + PA_INFO_START));

    auto *pa = reinterpret_cast<LegacyPropArea *>(m_data);
    memset(pa, 0, PA_SIZE);
    pa->magic = PROP_AREA_MAGIC;
    pa->version = PROP_AREA_VERSION;

    m_prop_area = pa;
    m_initialized = true;

    return true;
}

LegacyPropInfo * LegacyPropertyService::find_property(std::string_view name)
{
    unsigned count = m_prop_area->count;
    unsigned *toc = m_prop_area->toc;

    while (count--) {
        unsigned entry = *toc++;
        if (TOC_NAME_LEN(entry) != name.size()) {
            continue;
        }

        LegacyPropInfo *pi = TOC_TO_INFO(m_prop_area, entry);
        if (memcmp(name.data(), pi->name, name.size()) != 0) {
            continue;
        }

        return pi;
    }

    return nullptr;
}

void LegacyPropertyService::update_property(LegacyPropInfo *pi,
                                            std::string_view value)
{
    pi->serial = pi->serial | 1;
    memcpy(pi->value, value.data(), value.size());
    pi->value[PROP_VALUE_MAX - 1] = '\0';
    pi->serial = (value.size() << 24) | ((pi->serial + 1) & 0xffffff);
    __futex_wake(&pi->serial, INT32_MAX);
}
