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

#include "roms.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "util/logging.h"


// 0: No extra debugging output
// 1: Normal debugging output
// 2: A crap ton of debugging output
#define ROMS_DEBUG 1


#define RAW_SYSTEM "/raw-system"
#define RAW_CACHE "/raw-cache"
#define RAW_DATA "/raw-data"
#define SYSTEM "/system"
#define CACHE "/cache"
#define DATA "/data"

#define BUILD_PROP "build.prop"

#define SEP "/"


#define TO_CHAR (char *)
#define TO_XMLCHAR (xmlChar *)

static const xmlChar *TAG_DATABASE_VERSION      = TO_XMLCHAR "database-version";
static const xmlChar *TAG_DEFINED_KEYSET        = TO_XMLCHAR "defined-keyset";
static const xmlChar *TAG_KEYSET_SETTINGS       = TO_XMLCHAR "keyset-settings";
static const xmlChar *TAG_LAST_PLATFORM_VERSION = TO_XMLCHAR "last-platform-version";
static const xmlChar *TAG_PACKAGE               = TO_XMLCHAR "package";
static const xmlChar *TAG_PACKAGES              = TO_XMLCHAR "packages";
static const xmlChar *TAG_PERMISSION_TREES      = TO_XMLCHAR "permission-trees";
static const xmlChar *TAG_PERMISSIONS           = TO_XMLCHAR "permissions";
static const xmlChar *TAG_PERMS                 = TO_XMLCHAR "perms";
static const xmlChar *TAG_PROPER_SIGNING_KEYSET = TO_XMLCHAR "proper-signing-keyset";
static const xmlChar *TAG_RENAMED_PACKAGE       = TO_XMLCHAR "renamed-package";
static const xmlChar *TAG_SHARED_USER           = TO_XMLCHAR "shared-user";
static const xmlChar *TAG_SIGNING_KEYSET        = TO_XMLCHAR "signing-keyset";
static const xmlChar *TAG_SIGS                  = TO_XMLCHAR "sigs";
static const xmlChar *TAG_UPDATED_PACKAGE       = TO_XMLCHAR "updated-package";
static const xmlChar *TAG_UPGRADE_KEYSET        = TO_XMLCHAR "upgrade-keyset";

static const xmlChar *ATTR_CODE_PATH            = TO_XMLCHAR "codePath";
static const xmlChar *ATTR_CPU_ABI_OVERRIDE     = TO_XMLCHAR "cpuAbiOverride";
static const xmlChar *ATTR_FLAGS                = TO_XMLCHAR "flags";
static const xmlChar *ATTR_FT                   = TO_XMLCHAR "ft";
static const xmlChar *ATTR_INSTALL_STATUS       = TO_XMLCHAR "installStatus";
static const xmlChar *ATTR_INSTALLER            = TO_XMLCHAR "installer";
static const xmlChar *ATTR_IT                   = TO_XMLCHAR "it";
static const xmlChar *ATTR_NAME                 = TO_XMLCHAR "name";
static const xmlChar *ATTR_NATIVE_LIBRARY_PATH  = TO_XMLCHAR "nativeLibraryPath";
static const xmlChar *ATTR_PRIMARY_CPU_ABI      = TO_XMLCHAR "primaryCpuAbi";
static const xmlChar *ATTR_REAL_NAME            = TO_XMLCHAR "realName";
static const xmlChar *ATTR_RESOURCE_PATH        = TO_XMLCHAR "resourcePath";
static const xmlChar *ATTR_SECONDARY_CPU_ABI    = TO_XMLCHAR "secondaryCpuAbi";
static const xmlChar *ATTR_SHARED_USER_ID       = TO_XMLCHAR "sharedUserId";
static const xmlChar *ATTR_UID_ERROR            = TO_XMLCHAR "uidError";
static const xmlChar *ATTR_USER_ID              = TO_XMLCHAR "userId";
static const xmlChar *ATTR_UT                   = TO_XMLCHAR "ut";
static const xmlChar *ATTR_VERSION              = TO_XMLCHAR "version";


static void rom_init(struct rom *rom);
static void rom_cleanup(struct rom *rom);
#if ROMS_DEBUG >= 2
static void rom_dump(struct rom *rom);
#endif
static int add_rom(struct roms *roms, char *id, char *name,
                   char *description, int system_uses_image, char *system_path,
                   char *cache_path, char *data_path);

static void package_init(struct package *pkg);
static void package_cleanup(struct package *pkg);
#if ROMS_DEBUG >= 2
static void package_dump(struct package *pkg);
#endif
static int parse_tag_package(xmlNode *node, struct rom *rom);
static int parse_tag_packages(xmlNode *node, struct rom *rom);


static void rom_init(struct rom *rom)
{
    memset(rom, 0, sizeof(struct rom));
}

static void rom_cleanup(struct rom *rom)
{
    // free(NULL) is valid
    free(rom->id);
    free(rom->name);
    free(rom->description);
    free(rom->system_path);
    free(rom->cache_path);
    free(rom->data_path);

    mb_rom_cleanup_packages(rom);
}

#if ROMS_DEBUG >= 2
static void rom_dump(struct rom *rom)
{
    LOGD("ROM:");
    if (rom->id)
        LOGD("- ID:                        %s", rom->id);
    if (rom->name)
        LOGD("- Name:                      %s", rom->name);
    if (rom->description)
        LOGD("- Description:               %s", rom->description);
    if (rom->system_image_path) {
        if (rom->system_uses_image) {
            LOGD("- /system image path:        %s", rom->system_path);
        } else {
            LOGD("- /system bind mount source: %s", rom->system_path);
        }
    }
    if (rom->cache_path)
        LOGD("- /cache bind mount source:  %s", rom->cache_path);
    if (rom->data_path)
        LOGD("- /data bind mount source:   %s", rom->data_path);
}
#endif

static int add_rom(struct roms *roms, char *id, char *name,
                   char *description, int system_uses_image, char *system_path,
                   char *cache_path, char *data_path)
{
    if (roms->len == 0) {
        roms->list = malloc(1 * sizeof(struct rom));
    } else {
        roms->list = realloc(roms->list, (roms->len + 1) * sizeof(struct rom));
    }

    struct rom *r = &roms->list[roms->len];
    rom_init(r);

    if (id)
        r->id = strdup(id);
    if (name)
        r->name = strdup(name);
    if (description)
        r->description = strdup(description);

    r->system_uses_image = system_uses_image;

    if (system_path)
        r->system_path = strdup(system_path);
    if (cache_path)
        r->cache_path = strdup(cache_path);
    if (data_path)
        r->data_path = strdup(data_path);

    ++roms->len;

#if ROMS_DEBUG >= 2
    rom_dump(r);
#endif

    return 0;
}

int mb_roms_get_builtin(struct roms *roms)
{
    add_rom(roms, "primary", NULL, NULL, 0, SYSTEM, CACHE, DATA);

    // /system/multiboot/dual/system.img
    // /cache/multiboot/dual/cache/
    // /data/multiboot/dual/data/

    // /cache/multiboot/%s/system.img
    // /system/multiboot/%s/cache/
    // /data/multiboot/%s/data/

    // /data/multiboot/%s/system.img
    // /data/multiboot/%s/cache/
    // /data/multiboot/%s/data/

    add_rom(roms, "dual", NULL, NULL, 1,
            SYSTEM SEP "multiboot" SEP "dual" SEP "system.img",
            CACHE SEP "multiboot" SEP "dual" SEP "cache",
            DATA SEP "multiboot" SEP "dual" SEP "data");

    // Multislots
    char id[50] = { 0 };
    char system[50] = { 0 };
    char cache[50] = { 0 };
    char data[50] = { 0 };
    for (int i = 0; i < 3; ++i) {
        const char *fmtSystem = CACHE SEP "multiboot" SEP "%s" SEP "system.img";
        const char *fmtCache = SYSTEM SEP "multiboot" SEP "%s" SEP "cache";
        const char *fmtData = DATA SEP "multiboot" SEP "%s" SEP "data";

        snprintf(id, 50, "multi-slot-%d", i);
        snprintf(system, 50, fmtSystem, id);
        snprintf(cache, 50, fmtCache, id);
        snprintf(data, 50, fmtData, id);

        add_rom(roms, id, NULL, NULL, 1, system, cache, data);
    }

    return 0;
}

struct rom * mb_find_rom_by_id(struct roms *roms, const char *id)
{
    for (unsigned int i = 0; i < roms->len; ++i) {
        struct rom *r = &roms->list[i];
        if (strcmp(r->id, id) == 0) {
            return r;
        }
    }

    return NULL;
}

int mb_roms_init(struct roms *roms)
{
    roms->list = NULL;
    roms->len = 0;

    return 0;
}

int mb_roms_cleanup(struct roms *roms)
{
    for (unsigned int i = 0; i < roms->len; ++i) {
        struct rom *r = &roms->list[i];

        rom_cleanup(r);
    }

    free(roms->list);
    roms->list = NULL;
    roms->len = 0;

    return 0;
}

static void package_init(struct package *pkg)
{
    // Initialize to sane defaults
    pkg->name = NULL;
    pkg->real_name = NULL;
    pkg->code_path = NULL;
    pkg->resource_path = NULL;
    pkg->native_library_path = NULL;
    pkg->primary_cpu_abi = NULL;
    pkg->secondary_cpu_abi = NULL;
    pkg->cpu_abi_override = NULL;
    pkg->pkg_flags = 0;
    pkg->timestamp = 0;
    pkg->first_install_time = 0;
    pkg->last_update_time = 0;
    pkg->version = 0;
    pkg->is_shared_user = 0;
    pkg->user_id = 0;
    pkg->shared_user_id = 0;
    pkg->uid_error = NULL;
    pkg->install_status = NULL;
    pkg->installer = NULL;
}

static void package_cleanup(struct package *pkg)
{
    if (pkg->name) free(pkg->name);
    if (pkg->real_name) free(pkg->name);
    if (pkg->code_path) free(pkg->code_path);
    if (pkg->resource_path) free(pkg->resource_path);
    if (pkg->native_library_path) free(pkg->native_library_path);
    if (pkg->primary_cpu_abi) free(pkg->primary_cpu_abi);
    if (pkg->secondary_cpu_abi) free(pkg->secondary_cpu_abi);
    if (pkg->cpu_abi_override) free(pkg->cpu_abi_override);

    pkg->pkg_flags = 0;
    pkg->timestamp = 0;
    pkg->first_install_time = 0;
    pkg->last_update_time = 0;
    pkg->version = 0;
    pkg->is_shared_user = 0;
    pkg->user_id = 0;
    pkg->shared_user_id = 0;

    if (pkg->uid_error) free(pkg->uid_error);
    if (pkg->install_status) free(pkg->install_status);
    if (pkg->installer) free(pkg->installer);
}

#if ROMS_DEBUG >= 2
static char * time_to_string(unsigned long long time)
{
    static char buf[50];

    const time_t t = time / 1000;
    strftime(buf, 50, "%a %b %d %H:%M:%S %Y", localtime(&t));

    return buf;
}

#define DUMP_FLAG(f) LOGD("-                      %s (0x%d)", #f, f);

static void package_dump(struct package *pkg)
{
    LOGD("Package:");
    if (pkg->name)
        LOGD("- Name:                %s", pkg->name);
    if (pkg->real_name)
        LOGD("- Real name:           %s", pkg->real_name);
    if (pkg->code_path)
        LOGD("- Code path:           %s", pkg->code_path);
    if (pkg->resource_path)
        LOGD("- Resource path:       %s", pkg->resource_path);
    if (pkg->native_library_path)
        LOGD("- Native library path: %s", pkg->native_library_path);
    if (pkg->primary_cpu_abi)
        LOGD("- Primary CPU ABI:     %s", pkg->primary_cpu_abi);
    if (pkg->secondary_cpu_abi)
        LOGD("- Secondary CPU ABI:   %s", pkg->secondary_cpu_abi);
    if (pkg->cpu_abi_override)
        LOGD("- CPU ABI override:    %s", pkg->cpu_abi_override);

    LOGD("- Flags:               0x%x", pkg->pkg_flags);
    if (pkg->pkg_flags & FLAG_SYSTEM)
        DUMP_FLAG(FLAG_SYSTEM);
    if (pkg->pkg_flags & FLAG_DEBUGGABLE)
        DUMP_FLAG(FLAG_DEBUGGABLE);
    if (pkg->pkg_flags & FLAG_HAS_CODE)
        DUMP_FLAG(FLAG_HAS_CODE);
    if (pkg->pkg_flags & FLAG_PERSISTENT)
        DUMP_FLAG(FLAG_PERSISTENT);
    if (pkg->pkg_flags & FLAG_FACTORY_TEST)
        DUMP_FLAG(FLAG_FACTORY_TEST);
    if (pkg->pkg_flags & FLAG_ALLOW_TASK_REPARENTING)
        DUMP_FLAG(FLAG_ALLOW_TASK_REPARENTING);
    if (pkg->pkg_flags & FLAG_ALLOW_CLEAR_USER_DATA)
        DUMP_FLAG(FLAG_ALLOW_CLEAR_USER_DATA);
    if (pkg->pkg_flags & FLAG_UPDATED_SYSTEM_APP)
        DUMP_FLAG(FLAG_UPDATED_SYSTEM_APP);
    if (pkg->pkg_flags & FLAG_TEST_ONLY)
        DUMP_FLAG(FLAG_TEST_ONLY);
    if (pkg->pkg_flags & FLAG_SUPPORTS_SMALL_SCREENS)
        DUMP_FLAG(FLAG_SUPPORTS_SMALL_SCREENS);
    if (pkg->pkg_flags & FLAG_SUPPORTS_NORMAL_SCREENS)
        DUMP_FLAG(FLAG_SUPPORTS_NORMAL_SCREENS);
    if (pkg->pkg_flags & FLAG_SUPPORTS_LARGE_SCREENS)
        DUMP_FLAG(FLAG_SUPPORTS_LARGE_SCREENS);
    if (pkg->pkg_flags & FLAG_RESIZEABLE_FOR_SCREENS)
        DUMP_FLAG(FLAG_RESIZEABLE_FOR_SCREENS);
    if (pkg->pkg_flags & FLAG_SUPPORTS_SCREEN_DENSITIES)
        DUMP_FLAG(FLAG_SUPPORTS_SCREEN_DENSITIES);
    if (pkg->pkg_flags & FLAG_VM_SAFE_MODE)
        DUMP_FLAG(FLAG_VM_SAFE_MODE);
    if (pkg->pkg_flags & FLAG_ALLOW_BACKUP)
        DUMP_FLAG(FLAG_ALLOW_BACKUP);
    if (pkg->pkg_flags & FLAG_KILL_AFTER_RESTORE)
        DUMP_FLAG(FLAG_KILL_AFTER_RESTORE);
    if (pkg->pkg_flags & FLAG_RESTORE_ANY_VERSION)
        DUMP_FLAG(FLAG_RESTORE_ANY_VERSION);
    if (pkg->pkg_flags & FLAG_EXTERNAL_STORAGE)
        DUMP_FLAG(FLAG_EXTERNAL_STORAGE);
    if (pkg->pkg_flags & FLAG_SUPPORTS_XLARGE_SCREENS)
        DUMP_FLAG(FLAG_SUPPORTS_XLARGE_SCREENS);
    if (pkg->pkg_flags & FLAG_LARGE_HEAP)
        DUMP_FLAG(FLAG_LARGE_HEAP);
    if (pkg->pkg_flags & FLAG_STOPPED)
        DUMP_FLAG(FLAG_STOPPED);
    if (pkg->pkg_flags & FLAG_SUPPORTS_RTL)
        DUMP_FLAG(FLAG_SUPPORTS_RTL);
    if (pkg->pkg_flags & FLAG_INSTALLED)
        DUMP_FLAG(FLAG_INSTALLED);
    if (pkg->pkg_flags & FLAG_IS_DATA_ONLY)
        DUMP_FLAG(FLAG_IS_DATA_ONLY);
    if (pkg->pkg_flags & FLAG_IS_GAME)
        DUMP_FLAG(FLAG_IS_GAME);
    if (pkg->pkg_flags & FLAG_FULL_BACKUP_ONLY)
        DUMP_FLAG(FLAG_FULL_BACKUP_ONLY);
    if (pkg->pkg_flags & FLAG_HIDDEN)
        DUMP_FLAG(FLAG_HIDDEN);
    if (pkg->pkg_flags & FLAG_CANT_SAVE_STATE)
        DUMP_FLAG(FLAG_CANT_SAVE_STATE);
    if (pkg->pkg_flags & FLAG_FORWARD_LOCK)
        DUMP_FLAG(FLAG_FORWARD_LOCK);
    if (pkg->pkg_flags & FLAG_PRIVILEGED)
        DUMP_FLAG(FLAG_PRIVILEGED);
    if (pkg->pkg_flags & FLAG_MULTIARCH)
        DUMP_FLAG(FLAG_MULTIARCH);

    if (pkg->timestamp > 0)
        LOGD("- Timestamp:           %s", time_to_string(pkg->timestamp));
    if (pkg->first_install_time > 0)
        LOGD("- First install time:  %s", time_to_string(pkg->first_install_time));
    if (pkg->last_update_time > 0)
        LOGD("- Last update time:    %s", time_to_string(pkg->last_update_time));

    LOGD("- Version:             %d", pkg->version);

    if (pkg->is_shared_user) {
        LOGD("- Shared user ID:      %d", pkg->shared_user_id);
    } else {
        LOGD("- User ID:             %d", pkg->user_id);
    }

    if (pkg->uid_error)
        LOGD("- UID error:           %s", pkg->uid_error);
    if (pkg->install_status)
        LOGD("- Install status:      %s", pkg->install_status);
    if (pkg->installer)
        LOGD("- Installer:           %s", pkg->installer);
}
#endif

static int parse_tag_package(xmlNode *node, struct rom *rom)
{
    assert(xmlStrcmp(node->name, TAG_PACKAGE) == 0);

    if (rom->pkgs_len == 0) {
        rom->pkgs = malloc(1 * sizeof(struct package));
    } else {
        rom->pkgs = realloc(rom->pkgs, (rom->pkgs_len + 1) * sizeof(struct package));
    }

    struct package *pkg = &rom->pkgs[rom->pkgs_len];
    package_init(pkg);

    for (xmlAttr *attr = node->properties; attr; attr = attr->next) {
        const xmlChar *name = attr->name;
        xmlChar *value = xmlGetProp(node, attr->name);

        if (xmlStrcmp(name, ATTR_CODE_PATH) == 0) {
            pkg->code_path = strdup(TO_CHAR value);
        } else if (xmlStrcmp(name, ATTR_CPU_ABI_OVERRIDE) == 0) {
            pkg->cpu_abi_override = strdup(TO_CHAR value);
        } else if (xmlStrcmp(name, ATTR_FLAGS) == 0) {
            pkg->pkg_flags = strtol(TO_CHAR value, NULL, 10);
        } else if (xmlStrcmp(name, ATTR_FT) == 0) {
            pkg->timestamp = strtoull(TO_CHAR value, NULL, 16);
        } else if (xmlStrcmp(name, ATTR_INSTALL_STATUS) == 0) {
            pkg->install_status = strdup(TO_CHAR value);
        } else if (xmlStrcmp(name, ATTR_INSTALLER) == 0) {
            pkg->installer = strdup(TO_CHAR value);
        } else if (xmlStrcmp(name, ATTR_IT) == 0) {
            pkg->first_install_time = strtoull(TO_CHAR value, NULL, 16);
        } else if (xmlStrcmp(name, ATTR_NAME) == 0) {
            pkg->name = strdup(TO_CHAR value);
        } else if (xmlStrcmp(name, ATTR_NATIVE_LIBRARY_PATH) == 0) {
            pkg->native_library_path = strdup(TO_CHAR value);
        } else if (xmlStrcmp(name, ATTR_PRIMARY_CPU_ABI) == 0) {
            pkg->primary_cpu_abi = strdup(TO_CHAR value);
        } else if (xmlStrcmp(name, ATTR_REAL_NAME) == 0) {
            pkg->real_name = strdup(TO_CHAR value);
        } else if (xmlStrcmp(name, ATTR_RESOURCE_PATH) == 0) {
            pkg->resource_path = strdup(TO_CHAR value);
        } else if (xmlStrcmp(name, ATTR_SECONDARY_CPU_ABI) == 0) {
            pkg->secondary_cpu_abi = strdup(TO_CHAR value);
        } else if (xmlStrcmp(name, ATTR_SHARED_USER_ID) == 0) {
            pkg->shared_user_id = strtol(TO_CHAR value, NULL, 10);
            pkg->is_shared_user = 1;
        } else if (xmlStrcmp(name, ATTR_UID_ERROR) == 0) {
            pkg->uid_error = strdup(TO_CHAR value);
        } else if (xmlStrcmp(name, ATTR_USER_ID) == 0) {
            pkg->user_id = strtol(TO_CHAR value, NULL, 10);
            pkg->is_shared_user = 0;
        } else if (xmlStrcmp(name, ATTR_UT) == 0) {
            pkg->last_update_time = strtoull(TO_CHAR value, NULL, 16);
        } else if (xmlStrcmp(name, ATTR_VERSION) == 0) {
            pkg->version = strtol(TO_CHAR value, NULL, 10);
        } else {
            LOGW("Unrecognized attribute '%s' in <%s>", name, TAG_PACKAGE);
        }

        xmlFree(value);
    }

    for (xmlNode *cur_node = node->children; cur_node; cur_node = cur_node->next) {
        if (cur_node->type != XML_ELEMENT_NODE) {
            continue;
        }

        if (xmlStrcmp(cur_node->name, TAG_PACKAGE) == 0) {
            LOGW("Nested <%s> is not allowed", TAG_PACKAGE);
        } else if (xmlStrcmp(cur_node->name, TAG_DEFINED_KEYSET) == 0
                || xmlStrcmp(cur_node->name, TAG_PERMS) == 0
                || xmlStrcmp(cur_node->name, TAG_PROPER_SIGNING_KEYSET) == 0
                || xmlStrcmp(cur_node->name, TAG_SIGNING_KEYSET) == 0
                || xmlStrcmp(cur_node->name, TAG_SIGS) == 0
                || xmlStrcmp(cur_node->name, TAG_UPGRADE_KEYSET) == 0) {
            // Ignore
        } else {
            LOGW("Unrecognized <%s> within <%s>", cur_node->name, TAG_PACKAGE);
        }
    }

#if ROMS_DEBUG >= 2
    package_dump(pkg);
#endif

    ++rom->pkgs_len;

    return 0;
}

static int parse_tag_packages(xmlNode *node, struct rom *rom)
{
    assert(xmlStrcmp(node->name, TAG_PACKAGES) == 0);

    for (xmlNode *cur_node = node->children; cur_node; cur_node = cur_node->next) {
        if (cur_node->type != XML_ELEMENT_NODE) {
            continue;
        }

        if (xmlStrcmp(cur_node->name, TAG_PACKAGES) == 0) {
            LOGW("Nested <%s> is not allowed", TAG_PACKAGES);
        } else if (xmlStrcmp(cur_node->name, TAG_PACKAGE) == 0) {
            parse_tag_package(cur_node, rom);
        } else if (xmlStrcmp(cur_node->name, TAG_DATABASE_VERSION) == 0
                || xmlStrcmp(cur_node->name, TAG_KEYSET_SETTINGS) == 0
                || xmlStrcmp(cur_node->name, TAG_LAST_PLATFORM_VERSION) == 0
                || xmlStrcmp(cur_node->name, TAG_PERMISSION_TREES) == 0
                || xmlStrcmp(cur_node->name, TAG_PERMISSIONS) == 0
                || xmlStrcmp(cur_node->name, TAG_RENAMED_PACKAGE) == 0
                || xmlStrcmp(cur_node->name, TAG_SHARED_USER) == 0
                || xmlStrcmp(cur_node->name, TAG_UPDATED_PACKAGE) == 0) {
            // Ignore
        } else {
            LOGW("Unrecognized <%s> within <%s>", cur_node->name, TAG_PACKAGES);
        }
    }

    return 0;
}

int mb_rom_load_packages(struct rom *rom)
{
    char path[100];
    path[0] = '\0';
    strlcat(path, rom->data_path, 100);
    strlcat(path, "/system/packages.xml", 100);

    LIBXML_TEST_VERSION

    xmlDoc *doc = xmlReadFile(path, NULL, 0);
    if (!doc) {
        LOGE("Failed to parse XML file: %s", path);
        return -1;
    }

    xmlNode *root = xmlDocGetRootElement(doc);

    for (xmlNode *cur_node = root; cur_node; cur_node = cur_node->next) {
        if (cur_node->type != XML_ELEMENT_NODE) {
            continue;
        }

        if (xmlStrcmp(cur_node->name, TAG_PACKAGES) == 0) {
            parse_tag_packages(cur_node, rom);
        } else {
            LOGW("Unrecognized root tag: %s", cur_node->name);
        }
    }

    xmlFreeDoc(doc);
    xmlCleanupParser();

    return 0;
}

int mb_rom_cleanup_packages(struct rom *rom)
{
    for (unsigned int i = 0; i < rom->pkgs_len; ++i) {
        struct package *pkg = &rom->pkgs[i];
        package_cleanup(pkg);
    }

    free(rom->pkgs);
    rom->pkgs = NULL;
    rom->pkgs_len = 0;

    return 0;
}
