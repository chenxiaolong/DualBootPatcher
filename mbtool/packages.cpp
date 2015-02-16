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

#include <pugixml.hpp>

#include "util/logging.h"


namespace mb
{

static const char *TAG_CERT                  = "cert";
static const char *TAG_DATABASE_VERSION      = "database-version";
static const char *TAG_DEFINED_KEYSET        = "defined-keyset";
static const char *TAG_KEYSET_SETTINGS       = "keyset-settings";
static const char *TAG_LAST_PLATFORM_VERSION = "last-platform-version";
static const char *TAG_PACKAGE               = "package";
static const char *TAG_PACKAGES              = "packages";
static const char *TAG_PERMISSION_TREES      = "permission-trees";
static const char *TAG_PERMISSIONS           = "permissions";
static const char *TAG_PERMS                 = "perms";
static const char *TAG_PROPER_SIGNING_KEYSET = "proper-signing-keyset";
static const char *TAG_RENAMED_PACKAGE       = "renamed-package";
static const char *TAG_SHARED_USER           = "shared-user";
static const char *TAG_SIGNING_KEYSET        = "signing-keyset";
static const char *TAG_SIGS                  = "sigs";
static const char *TAG_UPDATED_PACKAGE       = "updated-package";
static const char *TAG_UPGRADE_KEYSET        = "upgrade-keyset";

static const char *ATTR_CODE_PATH            = "codePath";
static const char *ATTR_CPU_ABI_OVERRIDE     = "cpuAbiOverride";
static const char *ATTR_FLAGS                = "flags";
static const char *ATTR_FT                   = "ft";
static const char *ATTR_INDEX                = "index";
static const char *ATTR_INSTALL_STATUS       = "installStatus";
static const char *ATTR_INSTALLER            = "installer";
static const char *ATTR_IT                   = "it";
static const char *ATTR_KEY                  = "key";
static const char *ATTR_NAME                 = "name";
static const char *ATTR_NATIVE_LIBRARY_PATH  = "nativeLibraryPath";
static const char *ATTR_PRIMARY_CPU_ABI      = "primaryCpuAbi";
static const char *ATTR_REAL_NAME            = "realName";
static const char *ATTR_RESOURCE_PATH        = "resourcePath";
static const char *ATTR_SECONDARY_CPU_ABI    = "secondaryCpuAbi";
static const char *ATTR_SHARED_USER_ID       = "sharedUserId";
static const char *ATTR_UID_ERROR            = "uidError";
static const char *ATTR_USER_ID              = "userId";
static const char *ATTR_UT                   = "ut";
static const char *ATTR_VERSION              = "version";

static bool parse_tag_cert(pugi::xml_node node, Packages *pkgs,
                           std::shared_ptr<Package> pkg);
static bool parse_tag_sigs(pugi::xml_node node, Packages *pkgs,
                           std::shared_ptr<Package> pkg);
static bool parse_tag_package(pugi::xml_node node, Packages *pkgs);
static bool parse_tag_packages(pugi::xml_node node, Packages *pkgs);


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

static char * time_to_string(uint64_t time)
{
    static char buf[50];

    const time_t t = time / 1000;
    strftime(buf, sizeof(buf), "%a %b %d %H:%M:%S %Y", localtime(&t));

    return buf;
}

#define DUMP_FLAG(flag) LOGD(fmt_flag, "", #flag, \
                             static_cast<uint64_t>(Package::flag))

void Package::dump()
{
    static const char *fmt_string = "- {:<22} {}";
    static const char *fmt_int    = "- {:<22} {:d}";
    static const char *fmt_hex    = "- {:<22} {:#x}";
    static const char *fmt_flag   = "- {:<22} {} ({:#x})";

    LOGD("Package:");
    if (!name.empty())
        LOGD(fmt_string, "Name:", name);
    if (!real_name.empty())
        LOGD(fmt_string, "Real name:", real_name);
    if (!code_path.empty())
        LOGD(fmt_string, "Code path:", code_path);
    if (!resource_path.empty())
        LOGD(fmt_string, "Resource path:", resource_path);
    if (!native_library_path.empty())
        LOGD(fmt_string, "Native library path:", native_library_path);
    if (!primary_cpu_abi.empty())
        LOGD(fmt_string, "Primary CPU ABI:", primary_cpu_abi);
    if (!secondary_cpu_abi.empty())
        LOGD(fmt_string, "Secondary CPU ABI:", secondary_cpu_abi);
    if (!cpu_abi_override.empty())
        LOGD(fmt_string, "CPU ABI override:", cpu_abi_override);

    LOGD(fmt_hex, "Flags:", static_cast<uint64_t>(pkg_flags));
    if (pkg_flags & Package::FLAG_SYSTEM)
        DUMP_FLAG(FLAG_SYSTEM);
    if (pkg_flags & Package::FLAG_DEBUGGABLE)
        DUMP_FLAG(FLAG_DEBUGGABLE);
    if (pkg_flags & Package::FLAG_HAS_CODE)
        DUMP_FLAG(FLAG_HAS_CODE);
    if (pkg_flags & Package::FLAG_PERSISTENT)
        DUMP_FLAG(FLAG_PERSISTENT);
    if (pkg_flags & Package::FLAG_FACTORY_TEST)
        DUMP_FLAG(FLAG_FACTORY_TEST);
    if (pkg_flags & Package::FLAG_ALLOW_TASK_REPARENTING)
        DUMP_FLAG(FLAG_ALLOW_TASK_REPARENTING);
    if (pkg_flags & Package::FLAG_ALLOW_CLEAR_USER_DATA)
        DUMP_FLAG(FLAG_ALLOW_CLEAR_USER_DATA);
    if (pkg_flags & Package::FLAG_UPDATED_SYSTEM_APP)
        DUMP_FLAG(FLAG_UPDATED_SYSTEM_APP);
    if (pkg_flags & Package::FLAG_TEST_ONLY)
        DUMP_FLAG(FLAG_TEST_ONLY);
    if (pkg_flags & Package::FLAG_SUPPORTS_SMALL_SCREENS)
        DUMP_FLAG(FLAG_SUPPORTS_SMALL_SCREENS);
    if (pkg_flags & Package::FLAG_SUPPORTS_NORMAL_SCREENS)
        DUMP_FLAG(FLAG_SUPPORTS_NORMAL_SCREENS);
    if (pkg_flags & Package::FLAG_SUPPORTS_LARGE_SCREENS)
        DUMP_FLAG(FLAG_SUPPORTS_LARGE_SCREENS);
    if (pkg_flags & Package::FLAG_RESIZEABLE_FOR_SCREENS)
        DUMP_FLAG(FLAG_RESIZEABLE_FOR_SCREENS);
    if (pkg_flags & Package::FLAG_SUPPORTS_SCREEN_DENSITIES)
        DUMP_FLAG(FLAG_SUPPORTS_SCREEN_DENSITIES);
    if (pkg_flags & Package::FLAG_VM_SAFE_MODE)
        DUMP_FLAG(FLAG_VM_SAFE_MODE);
    if (pkg_flags & Package::FLAG_ALLOW_BACKUP)
        DUMP_FLAG(FLAG_ALLOW_BACKUP);
    if (pkg_flags & Package::FLAG_KILL_AFTER_RESTORE)
        DUMP_FLAG(FLAG_KILL_AFTER_RESTORE);
    if (pkg_flags & Package::FLAG_RESTORE_ANY_VERSION)
        DUMP_FLAG(FLAG_RESTORE_ANY_VERSION);
    if (pkg_flags & Package::FLAG_EXTERNAL_STORAGE)
        DUMP_FLAG(FLAG_EXTERNAL_STORAGE);
    if (pkg_flags & Package::FLAG_SUPPORTS_XLARGE_SCREENS)
        DUMP_FLAG(FLAG_SUPPORTS_XLARGE_SCREENS);
    if (pkg_flags & Package::FLAG_LARGE_HEAP)
        DUMP_FLAG(FLAG_LARGE_HEAP);
    if (pkg_flags & Package::FLAG_STOPPED)
        DUMP_FLAG(FLAG_STOPPED);
    if (pkg_flags & Package::FLAG_SUPPORTS_RTL)
        DUMP_FLAG(FLAG_SUPPORTS_RTL);
    if (pkg_flags & Package::FLAG_INSTALLED)
        DUMP_FLAG(FLAG_INSTALLED);
    if (pkg_flags & Package::FLAG_IS_DATA_ONLY)
        DUMP_FLAG(FLAG_IS_DATA_ONLY);
    if (pkg_flags & Package::FLAG_IS_GAME)
        DUMP_FLAG(FLAG_IS_GAME);
    if (pkg_flags & Package::FLAG_FULL_BACKUP_ONLY)
        DUMP_FLAG(FLAG_FULL_BACKUP_ONLY);
    if (pkg_flags & Package::FLAG_HIDDEN)
        DUMP_FLAG(FLAG_HIDDEN);
    if (pkg_flags & Package::FLAG_CANT_SAVE_STATE)
        DUMP_FLAG(FLAG_CANT_SAVE_STATE);
    if (pkg_flags & Package::FLAG_FORWARD_LOCK)
        DUMP_FLAG(FLAG_FORWARD_LOCK);
    if (pkg_flags & Package::FLAG_PRIVILEGED)
        DUMP_FLAG(FLAG_PRIVILEGED);
    if (pkg_flags & Package::FLAG_MULTIARCH)
        DUMP_FLAG(FLAG_MULTIARCH);

    if (timestamp > 0)
        LOGD(fmt_string, "Timestamp:", time_to_string(timestamp));
    if (first_install_time > 0)
        LOGD(fmt_string, "First install time:", time_to_string(first_install_time));
    if (last_update_time > 0)
        LOGD(fmt_string, "Last update time:", time_to_string(last_update_time));

    LOGD(fmt_int, "Version:", version);

    if (is_shared_user) {
        LOGD(fmt_int, "Shared user ID:", shared_user_id);
    } else {
        LOGD(fmt_int, "User ID:", user_id);
    }

    if (!uid_error.empty())
        LOGD(fmt_string, "UID error:", uid_error);
    if (!install_status.empty())
        LOGD(fmt_string, "Install status:", install_status);
    if (!installer.empty())
        LOGD(fmt_string, "Installer:", installer);
}

bool Packages::load_xml(const std::string &path)
{
    pkgs.clear();
    sigs.clear();

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(path.c_str());
    if (!result) {
        LOGE("Failed to parse XML file: {}: {}", path, result.description());
        return false;
    }

    pugi::xml_node root = doc.root();

    for (pugi::xml_node cur_node : root.children()) {
        if (cur_node.type() != pugi::xml_node_type::node_element) {
            continue;
        }

        if (strcmp(cur_node.name(), TAG_PACKAGES) == 0) {
            if (!parse_tag_packages(cur_node, this)) {
                return false;
            }
        } else {
            LOGW("Unrecognized root tag: {}", cur_node.name());
        }
    }

    return true;
}

static bool parse_tag_cert(pugi::xml_node node, Packages *pkgs,
                           std::shared_ptr<Package> pkg)
{
    assert(strcmp(node->name, TAG_CERT) == 0);

    std::string index;
    std::string key;

    for (pugi::xml_attribute attr : node.attributes()) {
        const pugi::char_t *name = attr.name();
        const pugi::char_t *value = attr.value();

        if (strcmp(name, ATTR_INDEX) == 0) {
            index = value;
        } else if (strcmp(name, ATTR_KEY) == 0) {
            key = value;
        } else {
            LOGW("Unrecognized attribute '{}' in <{}>", name, TAG_CERT);
        }
    }

    if (index.empty()) {
        LOGW("Missing or empty index in <{}>", TAG_CERT);
    } else {
        pkg->sig_indexes.push_back(index);
    }
    if (!index.empty() && !key.empty()) {
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

static bool parse_tag_sigs(pugi::xml_node node, Packages *pkgs,
                           std::shared_ptr<Package> pkg)
{
    assert(strcmp(node.name(), TAG_SIGS) == 0);

    for (pugi::xml_node cur_node : node.children()) {
        if (cur_node.type() != pugi::xml_node_type::node_element) {
            continue;
        }

        if (strcmp(cur_node.name(), TAG_SIGS) == 0) {
            LOGW("Nested <{}> is not allowed", TAG_SIGS);
        } else if (strcmp(cur_node.name(), TAG_CERT) == 0) {
            if (!parse_tag_cert(cur_node, pkgs, pkg)) {
                return false;
            }
        } else {
            LOGW("Unrecognized <{}> within <{}>", cur_node.name(), TAG_SIGS);
        }
    }

    return true;
}

static bool parse_tag_package(pugi::xml_node node, Packages *pkgs)
{
    assert(strcmp(node.name(), TAG_PACKAGE) == 0);

    std::shared_ptr<Package> pkg(new Package());

    for (pugi::xml_attribute attr : node.attributes()) {
        const pugi::char_t *name = attr.name();
        const pugi::char_t *value = attr.value();

        if (strcmp(name, ATTR_CODE_PATH) == 0) {
            pkg->code_path = value;
        } else if (strcmp(name, ATTR_CPU_ABI_OVERRIDE) == 0) {
            pkg->cpu_abi_override = value;
        } else if (strcmp(name, ATTR_FLAGS) == 0) {
            pkg->pkg_flags = static_cast<Package::Flags>(
                    strtoll(value, nullptr, 10));
        } else if (strcmp(name, ATTR_FT) == 0) {
            pkg->timestamp = strtoull(value, nullptr, 16);
        } else if (strcmp(name, ATTR_INSTALL_STATUS) == 0) {
            pkg->install_status = value;
        } else if (strcmp(name, ATTR_INSTALLER) == 0) {
            pkg->installer = value;
        } else if (strcmp(name, ATTR_IT) == 0) {
            pkg->first_install_time = strtoull(value, nullptr, 16);
        } else if (strcmp(name, ATTR_NAME) == 0) {
            pkg->name = value;
        } else if (strcmp(name, ATTR_NATIVE_LIBRARY_PATH) == 0) {
            pkg->native_library_path = value;
        } else if (strcmp(name, ATTR_PRIMARY_CPU_ABI) == 0) {
            pkg->primary_cpu_abi = value;
        } else if (strcmp(name, ATTR_REAL_NAME) == 0) {
            pkg->real_name = value;
        } else if (strcmp(name, ATTR_RESOURCE_PATH) == 0) {
            pkg->resource_path = value;
        } else if (strcmp(name, ATTR_SECONDARY_CPU_ABI) == 0) {
            pkg->secondary_cpu_abi = value;
        } else if (strcmp(name, ATTR_SHARED_USER_ID) == 0) {
            pkg->shared_user_id = strtol(value, nullptr, 10);
            pkg->is_shared_user = 1;
        } else if (strcmp(name, ATTR_UID_ERROR) == 0) {
            pkg->uid_error = value;
        } else if (strcmp(name, ATTR_USER_ID) == 0) {
            pkg->user_id = strtol(value, nullptr, 10);
            pkg->is_shared_user = 0;
        } else if (strcmp(name, ATTR_UT) == 0) {
            pkg->last_update_time = strtoull(value, nullptr, 16);
        } else if (strcmp(name, ATTR_VERSION) == 0) {
            pkg->version = strtol(value, nullptr, 10);
        } else {
            LOGW("Unrecognized attribute '{}' in <{}>", name, TAG_PACKAGE);
        }
    }

    for (pugi::xml_node cur_node : node.children()) {
        if (cur_node.type() != pugi::xml_node_type::node_element) {
            continue;
        }

        if (strcmp(cur_node.name(), TAG_PACKAGE) == 0) {
            LOGW("Nested <{}> is not allowed", TAG_PACKAGE);
        } else if (strcmp(cur_node.name(), TAG_DEFINED_KEYSET) == 0
                || strcmp(cur_node.name(), TAG_PERMS) == 0
                || strcmp(cur_node.name(), TAG_PROPER_SIGNING_KEYSET) == 0
                || strcmp(cur_node.name(), TAG_SIGNING_KEYSET) == 0
                || strcmp(cur_node.name(), TAG_UPGRADE_KEYSET) == 0) {
            // Ignore
        } else if (strcmp(cur_node.name(), TAG_SIGS) == 0) {
            if (!parse_tag_sigs(cur_node, pkgs, pkg)) {
                return false;
            }
        } else {
            LOGW("Unrecognized <{}> within <{}>", cur_node.name(), TAG_PACKAGE);
        }
    }

    pkgs->pkgs.push_back(std::move(pkg));

    return true;
}

static bool parse_tag_packages(pugi::xml_node node, Packages *pkgs)
{
    assert(strcmp(node.name(), TAG_PACKAGES) == 0);

    for (pugi::xml_node cur_node : node.children()) {
        if (cur_node.type() != pugi::xml_node_type::node_element) {
            continue;
        }

        if (strcmp(cur_node.name(), TAG_PACKAGES) == 0) {
            LOGW("Nested <{}> is not allowed", TAG_PACKAGES);
        } else if (strcmp(cur_node.name(), TAG_PACKAGE) == 0) {
            if (!parse_tag_package(cur_node, pkgs)) {
                return false;
            }
        } else if (strcmp(cur_node.name(), TAG_DATABASE_VERSION) == 0
                || strcmp(cur_node.name(), TAG_KEYSET_SETTINGS) == 0
                || strcmp(cur_node.name(), TAG_LAST_PLATFORM_VERSION) == 0
                || strcmp(cur_node.name(), TAG_PERMISSION_TREES) == 0
                || strcmp(cur_node.name(), TAG_PERMISSIONS) == 0
                || strcmp(cur_node.name(), TAG_RENAMED_PACKAGE) == 0
                || strcmp(cur_node.name(), TAG_SHARED_USER) == 0
                || strcmp(cur_node.name(), TAG_UPDATED_PACKAGE) == 0) {
            // Ignore
        } else {
            LOGW("Unrecognized <{}> within <{}>", cur_node.name(), TAG_PACKAGES);
        }
    }

    return true;
}

std::shared_ptr<Package> Packages::find_by_uid(uid_t uid)
{
    auto it = std::find_if(pkgs.begin(), pkgs.end(),
                           [&](const std::shared_ptr<Package> &pkg) {
        return !pkg->is_shared_user && pkg->user_id == static_cast<int>(uid);
    });
    return it == pkgs.end() ? std::shared_ptr<Package>() : *it;
}

}