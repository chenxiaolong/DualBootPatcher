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

#include "packages.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "util/logging.h"


#define TO_CHAR (char *)
#define TO_XMLCHAR (xmlChar *)

namespace mb
{

static const xmlChar *TAG_CERT                  = TO_XMLCHAR "cert";
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
static const xmlChar *ATTR_INDEX                = TO_XMLCHAR "index";
static const xmlChar *ATTR_INSTALL_STATUS       = TO_XMLCHAR "installStatus";
static const xmlChar *ATTR_INSTALLER            = TO_XMLCHAR "installer";
static const xmlChar *ATTR_IT                   = TO_XMLCHAR "it";
static const xmlChar *ATTR_KEY                  = TO_XMLCHAR "key";
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

static bool parse_tag_cert(xmlNode *node, Packages *pkgs);
static bool parse_tag_sigs(xmlNode *node, Packages *pkgs);
static bool parse_tag_package(xmlNode *node, Packages *pkgs);
static bool parse_tag_packages(xmlNode *node, Packages *pkgs);


Package::Package() :
        name(),
        real_name(),
        code_path(),
        resource_path(),
        native_library_path(),
        primary_cpu_abi(),
        secondary_cpu_abi(),
        cpu_abi_override(),
        pkg_flags(static_cast<Flags>(0)),
        timestamp(0),
        first_install_time(0),
        last_update_time(0),
        version(0),
        is_shared_user(0),
        user_id(0),
        shared_user_id(0),
        uid_error(),
        install_status(),
        installer()
{
}

static char * time_to_string(unsigned long long time)
{
    static char buf[50];

    const time_t t = time / 1000;
    strftime(buf, 50, "%a %b %d %H:%M:%S %Y", localtime(&t));

    return buf;
}

#define DUMP_FLAG(f) LOGD("-                      {} ({:#x})", #f, f);

void Package::dump()
{
    LOGD("Package:");
    if (!name.empty())
        LOGD("- Name:                {}", name);
    if (!real_name.empty())
        LOGD("- Real name:           {}", real_name);
    if (!code_path.empty())
        LOGD("- Code path:           {}", code_path);
    if (!resource_path.empty())
        LOGD("- Resource path:       {}", resource_path);
    if (!native_library_path.empty())
        LOGD("- Native library path: {}", native_library_path);
    if (!primary_cpu_abi.empty())
        LOGD("- Primary CPU ABI:     {}", primary_cpu_abi);
    if (!secondary_cpu_abi.empty())
        LOGD("- Secondary CPU ABI:   {}", secondary_cpu_abi);
    if (!cpu_abi_override.empty())
        LOGD("- CPU ABI override:    {}", cpu_abi_override);

    LOGD("- Flags:               {:#x}", pkg_flags);
    if (pkg_flags & Package::FLAG_SYSTEM)
        DUMP_FLAG(Package::FLAG_SYSTEM);
    if (pkg_flags & Package::FLAG_DEBUGGABLE)
        DUMP_FLAG(Package::FLAG_DEBUGGABLE);
    if (pkg_flags & Package::FLAG_HAS_CODE)
        DUMP_FLAG(Package::FLAG_HAS_CODE);
    if (pkg_flags & Package::FLAG_PERSISTENT)
        DUMP_FLAG(Package::FLAG_PERSISTENT);
    if (pkg_flags & Package::FLAG_FACTORY_TEST)
        DUMP_FLAG(Package::FLAG_FACTORY_TEST);
    if (pkg_flags & Package::FLAG_ALLOW_TASK_REPARENTING)
        DUMP_FLAG(Package::FLAG_ALLOW_TASK_REPARENTING);
    if (pkg_flags & Package::FLAG_ALLOW_CLEAR_USER_DATA)
        DUMP_FLAG(Package::FLAG_ALLOW_CLEAR_USER_DATA);
    if (pkg_flags & Package::FLAG_UPDATED_SYSTEM_APP)
        DUMP_FLAG(Package::FLAG_UPDATED_SYSTEM_APP);
    if (pkg_flags & Package::FLAG_TEST_ONLY)
        DUMP_FLAG(Package::FLAG_TEST_ONLY);
    if (pkg_flags & Package::FLAG_SUPPORTS_SMALL_SCREENS)
        DUMP_FLAG(Package::FLAG_SUPPORTS_SMALL_SCREENS);
    if (pkg_flags & Package::FLAG_SUPPORTS_NORMAL_SCREENS)
        DUMP_FLAG(Package::FLAG_SUPPORTS_NORMAL_SCREENS);
    if (pkg_flags & Package::FLAG_SUPPORTS_LARGE_SCREENS)
        DUMP_FLAG(Package::FLAG_SUPPORTS_LARGE_SCREENS);
    if (pkg_flags & Package::FLAG_RESIZEABLE_FOR_SCREENS)
        DUMP_FLAG(Package::FLAG_RESIZEABLE_FOR_SCREENS);
    if (pkg_flags & Package::FLAG_SUPPORTS_SCREEN_DENSITIES)
        DUMP_FLAG(Package::FLAG_SUPPORTS_SCREEN_DENSITIES);
    if (pkg_flags & Package::FLAG_VM_SAFE_MODE)
        DUMP_FLAG(Package::FLAG_VM_SAFE_MODE);
    if (pkg_flags & Package::FLAG_ALLOW_BACKUP)
        DUMP_FLAG(Package::FLAG_ALLOW_BACKUP);
    if (pkg_flags & Package::FLAG_KILL_AFTER_RESTORE)
        DUMP_FLAG(Package::FLAG_KILL_AFTER_RESTORE);
    if (pkg_flags & Package::FLAG_RESTORE_ANY_VERSION)
        DUMP_FLAG(Package::FLAG_RESTORE_ANY_VERSION);
    if (pkg_flags & Package::FLAG_EXTERNAL_STORAGE)
        DUMP_FLAG(Package::FLAG_EXTERNAL_STORAGE);
    if (pkg_flags & Package::FLAG_SUPPORTS_XLARGE_SCREENS)
        DUMP_FLAG(Package::FLAG_SUPPORTS_XLARGE_SCREENS);
    if (pkg_flags & Package::FLAG_LARGE_HEAP)
        DUMP_FLAG(Package::FLAG_LARGE_HEAP);
    if (pkg_flags & Package::FLAG_STOPPED)
        DUMP_FLAG(Package::FLAG_STOPPED);
    if (pkg_flags & Package::FLAG_SUPPORTS_RTL)
        DUMP_FLAG(Package::FLAG_SUPPORTS_RTL);
    if (pkg_flags & Package::FLAG_INSTALLED)
        DUMP_FLAG(Package::FLAG_INSTALLED);
    if (pkg_flags & Package::FLAG_IS_DATA_ONLY)
        DUMP_FLAG(Package::FLAG_IS_DATA_ONLY);
    if (pkg_flags & Package::FLAG_IS_GAME)
        DUMP_FLAG(Package::FLAG_IS_GAME);
    if (pkg_flags & Package::FLAG_FULL_BACKUP_ONLY)
        DUMP_FLAG(Package::FLAG_FULL_BACKUP_ONLY);
    if (pkg_flags & Package::FLAG_HIDDEN)
        DUMP_FLAG(Package::FLAG_HIDDEN);
    if (pkg_flags & Package::FLAG_CANT_SAVE_STATE)
        DUMP_FLAG(Package::FLAG_CANT_SAVE_STATE);
    if (pkg_flags & Package::FLAG_FORWARD_LOCK)
        DUMP_FLAG(Package::FLAG_FORWARD_LOCK);
    if (pkg_flags & Package::FLAG_PRIVILEGED)
        DUMP_FLAG(Package::FLAG_PRIVILEGED);
    if (pkg_flags & Package::FLAG_MULTIARCH)
        DUMP_FLAG(Package::FLAG_MULTIARCH);

    if (timestamp > 0)
        LOGD("- Timestamp:           {}", time_to_string(timestamp));
    if (first_install_time > 0)
        LOGD("- First install time:  {}", time_to_string(first_install_time));
    if (last_update_time > 0)
        LOGD("- Last update time:    {}", time_to_string(last_update_time));

    LOGD("- Version:             {}", version);

    if (is_shared_user) {
        LOGD("- Shared user ID:      {}", shared_user_id);
    } else {
        LOGD("- User ID:             {}", user_id);
    }

    if (!uid_error.empty())
        LOGD("- UID error:           {}", uid_error);
    if (!install_status.empty())
        LOGD("- Install status:      {}", install_status);
    if (!installer.empty())
        LOGD("- Installer:           {}", installer);
}

bool mb_packages_load_xml(Packages *pkgs, const std::string &path)
{
    LIBXML_TEST_VERSION

    xmlDoc *doc = xmlReadFile(path.c_str(), nullptr, 0);
    if (!doc) {
        LOGE("Failed to parse XML file: {}", path);
        return false;
    }

    xmlNode *root = xmlDocGetRootElement(doc);

    for (xmlNode *cur_node = root; cur_node; cur_node = cur_node->next) {
        if (cur_node->type != XML_ELEMENT_NODE) {
            continue;
        }

        if (xmlStrcmp(cur_node->name, TAG_PACKAGES) == 0) {
            parse_tag_packages(cur_node, pkgs);
        } else {
            LOGW("Unrecognized root tag: {}", cur_node->name);
        }
    }

    xmlFreeDoc(doc);
    xmlCleanupParser();

    return true;
}

static bool parse_tag_cert(xmlNode *node, Packages *pkgs)
{
    assert(xmlStrcmp(node->name, TAG_CERT) == 0);

    std::string index;
    std::string key;

    for (xmlAttr *attr = node->properties; attr; attr = attr->next) {
        const xmlChar *name = attr->name;
        xmlChar *value = xmlGetProp(node, attr->name);

        if (xmlStrcmp(name, ATTR_INDEX) == 0) {
            index = TO_CHAR value;
        } else if (xmlStrcmp(name, ATTR_KEY) == 0) {
            key = TO_CHAR value;
        } else {
            LOGW("Unrecognized attribute '{}' in <{}>", name, TAG_CERT);
        }

        xmlFree(value);
    }

    if (index.empty()) {
        LOGW("Missing or empty index in <{}>", TAG_CERT);
    } else if (!key.empty()) {
        auto it = pkgs->sigs.find(index);
        if (it != pkgs->sigs.end()) {
            // Make sure key matches if it's already in the map
            if (it->second != key) {
                LOGE("Error: Index \"{}\" assigned to multiple keys", index);
                return false;
            }
        } else {
            // Otherwise, add it to the map
            pkgs->sigs.insert(std::make_pair(std::move(index), std::move(key)));
        }
    }

    return true;
}

static bool parse_tag_sigs(xmlNode *node, Packages *pkgs)
{
    assert(xmlStrcmp(node->name, TAG_SIGS) == 0);

    for (xmlNode *cur_node = node->children; cur_node; cur_node = cur_node->next) {
        if (cur_node->type != XML_ELEMENT_NODE) {
            continue;
        }

        if (xmlStrcmp(cur_node->name, TAG_SIGS) == 0) {
            LOGW("Nested <{}> is not allowed", TAG_SIGS);
        } else if (xmlStrcmp(cur_node->name, TAG_CERT) == 0) {
            if (!parse_tag_cert(cur_node, pkgs)) {
                return false;
            }
        } else {
            LOGW("Unrecognized <{}> within <{}>", cur_node->name, TAG_SIGS);
        }
    }

    return true;
}

static bool parse_tag_package(xmlNode *node, Packages *pkgs)
{
    assert(xmlStrcmp(node->name, TAG_PACKAGE) == 0);

    std::shared_ptr<Package> pkg(new Package());

    for (xmlAttr *attr = node->properties; attr; attr = attr->next) {
        const xmlChar *name = attr->name;
        xmlChar *value = xmlGetProp(node, attr->name);

        if (xmlStrcmp(name, ATTR_CODE_PATH) == 0) {
            pkg->code_path = TO_CHAR value;
        } else if (xmlStrcmp(name, ATTR_CPU_ABI_OVERRIDE) == 0) {
            pkg->cpu_abi_override = TO_CHAR value;
        } else if (xmlStrcmp(name, ATTR_FLAGS) == 0) {
            pkg->pkg_flags = static_cast<Package::Flags>(
                    strtoll(TO_CHAR value, nullptr, 10));
        } else if (xmlStrcmp(name, ATTR_FT) == 0) {
            pkg->timestamp = strtoull(TO_CHAR value, nullptr, 16);
        } else if (xmlStrcmp(name, ATTR_INSTALL_STATUS) == 0) {
            pkg->install_status = TO_CHAR value;
        } else if (xmlStrcmp(name, ATTR_INSTALLER) == 0) {
            pkg->installer = TO_CHAR value;
        } else if (xmlStrcmp(name, ATTR_IT) == 0) {
            pkg->first_install_time = strtoull(TO_CHAR value, nullptr, 16);
        } else if (xmlStrcmp(name, ATTR_NAME) == 0) {
            pkg->name = TO_CHAR value;
        } else if (xmlStrcmp(name, ATTR_NATIVE_LIBRARY_PATH) == 0) {
            pkg->native_library_path = TO_CHAR value;
        } else if (xmlStrcmp(name, ATTR_PRIMARY_CPU_ABI) == 0) {
            pkg->primary_cpu_abi = TO_CHAR value;
        } else if (xmlStrcmp(name, ATTR_REAL_NAME) == 0) {
            pkg->real_name = TO_CHAR value;
        } else if (xmlStrcmp(name, ATTR_RESOURCE_PATH) == 0) {
            pkg->resource_path = TO_CHAR value;
        } else if (xmlStrcmp(name, ATTR_SECONDARY_CPU_ABI) == 0) {
            pkg->secondary_cpu_abi = TO_CHAR value;
        } else if (xmlStrcmp(name, ATTR_SHARED_USER_ID) == 0) {
            pkg->shared_user_id = strtol(TO_CHAR value, nullptr, 10);
            pkg->is_shared_user = 1;
        } else if (xmlStrcmp(name, ATTR_UID_ERROR) == 0) {
            pkg->uid_error = TO_CHAR value;
        } else if (xmlStrcmp(name, ATTR_USER_ID) == 0) {
            pkg->user_id = strtol(TO_CHAR value, nullptr, 10);
            pkg->is_shared_user = 0;
        } else if (xmlStrcmp(name, ATTR_UT) == 0) {
            pkg->last_update_time = strtoull(TO_CHAR value, nullptr, 16);
        } else if (xmlStrcmp(name, ATTR_VERSION) == 0) {
            pkg->version = strtol(TO_CHAR value, nullptr, 10);
        } else {
            LOGW("Unrecognized attribute '{}' in <{}>", name, TAG_PACKAGE);
        }

        xmlFree(value);
    }

    for (xmlNode *cur_node = node->children; cur_node; cur_node = cur_node->next) {
        if (cur_node->type != XML_ELEMENT_NODE) {
            continue;
        }

        if (xmlStrcmp(cur_node->name, TAG_PACKAGE) == 0) {
            LOGW("Nested <{}> is not allowed", TAG_PACKAGE);
        } else if (xmlStrcmp(cur_node->name, TAG_DEFINED_KEYSET) == 0
                || xmlStrcmp(cur_node->name, TAG_PERMS) == 0
                || xmlStrcmp(cur_node->name, TAG_PROPER_SIGNING_KEYSET) == 0
                || xmlStrcmp(cur_node->name, TAG_SIGNING_KEYSET) == 0
                || xmlStrcmp(cur_node->name, TAG_UPGRADE_KEYSET) == 0) {
            // Ignore
        } else if (xmlStrcmp(cur_node->name, TAG_SIGS) == 0) {
            if (!parse_tag_sigs(cur_node, pkgs)) {
                return false;
            }
        } else {
            LOGW("Unrecognized <{}> within <{}>", cur_node->name, TAG_PACKAGE);
        }
    }

    pkgs->pkgs.push_back(std::move(pkg));

    return true;
}

static bool parse_tag_packages(xmlNode *node, Packages *pkgs)
{
    assert(xmlStrcmp(node->name, TAG_PACKAGES) == 0);

    for (xmlNode *cur_node = node->children; cur_node; cur_node = cur_node->next) {
        if (cur_node->type != XML_ELEMENT_NODE) {
            continue;
        }

        if (xmlStrcmp(cur_node->name, TAG_PACKAGES) == 0) {
            LOGW("Nested <{}> is not allowed", TAG_PACKAGES);
        } else if (xmlStrcmp(cur_node->name, TAG_PACKAGE) == 0) {
            if (!parse_tag_package(cur_node, pkgs)) {
                return false;
            }
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
            LOGW("Unrecognized <{}> within <{}>", cur_node->name, TAG_PACKAGES);
        }
    }

    return true;
}

}