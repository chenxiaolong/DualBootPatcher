/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "multiboot.h"

#include <cerrno>
#include <cstring>
#include <sys/stat.h>

#include "mbcommon/string.h"
#include "mblog/logging.h"
#include "mbutil/chmod.h"
#include "mbutil/chown.h"
#include "mbutil/copy.h"
#include "mbutil/file.h"
#include "mbutil/fts.h"
#include "mbutil/selinux.h"
#include "mbutil/string.h"

#define LOG_TAG "mbtool/multiboot"


namespace mb
{

class CopySystem : public util::FtsWrapper {
public:
    CopySystem(std::string path, std::string target)
        : FtsWrapper(path, util::FtsFlag::GroupSpecialFiles),
        _target(std::move(target))
    {
    }

    Actions on_changed_path() override
    {
        // We'll set attrs and xattrs after visiting children
        if (_curr->fts_level == 0) {
            return Action::Ok;
        }

        // We only care about the first level
        if (_curr->fts_level != 1) {
            return Action::Next;
        }

        // Don't copy multiboot directory
        if (strcmp(_curr->fts_name, "multiboot") == 0) {
            return Action::Skip;
        }

        _curtgtpath.clear();
        _curtgtpath += _target;
        _curtgtpath += "/";
        _curtgtpath += _curr->fts_name;

        return Action::Ok;
    }

    Actions on_reached_directory_pre() override
    {
        if (_curr->fts_level == 0) {
            return Action::Ok;
        }

        // _target is the correct parameter here (or pathbuf and
        // CopyFlag::ExcludeTopLevel flag)
        if (auto r = util::copy_dir(_curr->fts_accpath, _target,
                                    util::CopyFlag::CopyAttributes
                                  | util::CopyFlag::CopyXattrs); !r) {
            _error_msg = format("Failed to copy directory: %s",
                                r.error().message().c_str());
            LOGW("%s", _error_msg.c_str());
            return Action::Skip | Action::Fail;
        }
        return Action::Skip;
    }

    Actions on_reached_directory_post() override
    {
        if (_curr->fts_level == 0) {
            if (auto r = util::copy_stat(_curr->fts_accpath, _target); !r) {
                LOGE("%s: Failed to copy attributes: %s",
                     _target.c_str(), r.error().message().c_str());
                return Action::Fail;
            }
            if (auto r = util::copy_xattrs(_curr->fts_accpath, _target); !r) {
                LOGE("%s: Failed to copy xattrs: %s",
                     _target.c_str(), r.error().message().c_str());
                return Action::Fail;
            }
        }
        return Action::Ok;
    }

    Actions on_reached_file() override
    {
        if (_curr->fts_level == 0) {
            return Action::Ok;
        }
        return copy_path() ? Action::Ok : Action::Fail;
    }

    Actions on_reached_symlink() override
    {
        if (_curr->fts_level == 0) {
            return Action::Ok;
        }
        return copy_path() ? Action::Ok : Action::Fail;
    }

    Actions on_reached_special_file() override
    {
        if (_curr->fts_level == 0) {
            return Action::Ok;
        }
        return copy_path() ? Action::Ok : Action::Fail;
    }

private:
    std::string _target;
    std::string _curtgtpath;

    bool copy_path()
    {
        if (auto r = util::copy_file(_curr->fts_accpath, _curtgtpath,
                                     util::CopyFlag::CopyAttributes
                                   | util::CopyFlag::CopyXattrs); !r) {
            _error_msg = format("Failed to copy file: %s",
                                r.error().message().c_str());
            LOGW("%s", _error_msg.c_str());
            return false;
        }
        return true;
    }
};

/*!
 * \brief Copy /system directory excluding multiboot files
 *
 * \param source Source directory
 * \param target Target directory
 */
bool copy_system(const std::string &source, const std::string &target)
{
    CopySystem fts(source, target);
    return fts.run();
}

/*!
 * \brief Fix permissions and label on /data/media/0/MultiBoot/
 *
 * This function will do the following on /data/media/0/MultiBoot/:
 * 1. Recursively change ownership to media_rw:media_rw
 * 2. Recursively change mode to 0775
 * 3. Recursively change the SELinux label to the same label as /data/media/0/
 *
 * \return True if all operations succeeded. False, if any failed.
 */
bool fix_multiboot_permissions()
{
    (void) util::create_empty_file(MULTIBOOT_DIR "/.nomedia");

    if (auto r = util::chown(MULTIBOOT_DIR, "media_rw", "media_rw",
                             util::ChownFlag::Recursive); !r) {
        LOGE("Failed to chown %s: %s", MULTIBOOT_DIR,
             r.error().message().c_str());
        return false;
    }

    if (auto r = util::chmod(MULTIBOOT_DIR, 0775,
                             util::ChmodFlag::Recursive); !r) {
        LOGE("Failed to chmod %s: %s", MULTIBOOT_DIR,
             r.error().message().c_str());
        return false;
    }

    if (auto context = util::selinux_lget_context(INTERNAL_STORAGE)) {
        if (auto ret = util::selinux_lset_context_recursive(
                MULTIBOOT_DIR, context.value()); !ret) {
            LOGE("%s: Failed to set context to %s: %s",
                 MULTIBOOT_DIR, context.value().c_str(),
                 ret.error().message().c_str());
            return false;
        }
    }

    return true;
}

bool switch_context(const std::string &context)
{
    auto current = util::selinux_get_process_attr(
            0, util::SELinuxAttr::Current);
    if (!current) {
        LOGE("Failed to get current process context: %s",
             current.error().message().c_str());
        // Don't fail if SELinux is not supported
        return errno == ENOENT;
    }

    LOGI("Current process context: %s", current.value().c_str());

    if (current.value() == context) {
        LOGV("Not switching process context");
        return true;
    }

    LOGV("Setting process context: %s", context.c_str());

    if (auto ret = util::selinux_set_process_attr(
            0, util::SELinuxAttr::Current, context); !ret) {
        LOGE("Failed to set current process context: %s",
             ret.error().message().c_str());
        return false;
    }

    return true;
}

}
