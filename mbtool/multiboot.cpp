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


namespace mb
{

class CopySystem : public util::FTSWrapper {
public:
    CopySystem(std::string path, std::string target)
        : FTSWrapper(path, FTS_GroupSpecialFiles),
        _target(std::move(target))
    {
    }

    virtual int on_changed_path() override
    {
        // We'll set attrs and xattrs after visiting children
        if (_curr->fts_level == 0) {
            return Action::FTS_OK;
        }

        // We only care about the first level
        if (_curr->fts_level != 1) {
            return Action::FTS_Next;
        }

        // Don't copy multiboot directory
        if (strcmp(_curr->fts_name, "multiboot") == 0) {
            return Action::FTS_Skip;
        }

        _curtgtpath.clear();
        _curtgtpath += _target;
        _curtgtpath += "/";
        _curtgtpath += _curr->fts_name;

        return Action::FTS_OK;
    }

    virtual int on_reached_directory_pre() override
    {
        if (_curr->fts_level == 0) {
            return Action::FTS_OK;
        }

        // _target is the correct parameter here (or pathbuf and
        // COPY_EXCLUDE_TOP_LEVEL flag)
        if (!util::copy_dir(_curr->fts_accpath, _target,
                            util::COPY_ATTRIBUTES | util::COPY_XATTRS)) {
            char *msg = mb_format("%s: Failed to copy directory: %s",
                                  _curr->fts_path, strerror(errno));
            if (msg) {
                _error_msg = msg;
                free(msg);
            }
            LOGW("%s", _error_msg.c_str());
            return Action::FTS_Skip | Action::FTS_Fail;
        }
        return Action::FTS_Skip;
    }

    virtual int on_reached_directory_post() override
    {
        if (_curr->fts_level == 0) {
            if (!util::copy_stat(_curr->fts_accpath, _target)) {
                LOGE("%s: Failed to copy attributes: %s",
                     _target.c_str(), strerror(errno));
                return Action::FTS_Fail;
            }
            if (!util::copy_xattrs(_curr->fts_accpath, _target)) {
                LOGE("%s: Failed to copy xattrs: %s",
                     _target.c_str(), strerror(errno));
                return Action::FTS_Fail;
            }
        }
        return Action::FTS_OK;
    }

    virtual int on_reached_file() override
    {
        if (_curr->fts_level == 0) {
            return Action::FTS_OK;
        }
        return copy_path() ? Action::FTS_OK : Action::FTS_Fail;
    }

    virtual int on_reached_symlink() override
    {
        if (_curr->fts_level == 0) {
            return Action::FTS_OK;
        }
        return copy_path() ? Action::FTS_OK : Action::FTS_Fail;
    }

    virtual int on_reached_special_file() override
    {
        if (_curr->fts_level == 0) {
            return Action::FTS_OK;
        }
        return copy_path() ? Action::FTS_OK : Action::FTS_Fail;
    }

private:
    std::string _target;
    std::string _curtgtpath;

    bool copy_path()
    {
        if (!util::copy_file(_curr->fts_accpath, _curtgtpath,
                             util::COPY_ATTRIBUTES | util::COPY_XATTRS)) {
            char *msg = mb_format("%s: Failed to copy file: %s",
                                  _curr->fts_path, strerror(errno));
            if (msg) {
                _error_msg = msg;
                free(msg);
            }
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
bool fix_multiboot_permissions(void)
{
    util::create_empty_file(MULTIBOOT_DIR "/.nomedia");

    if (!util::chown(MULTIBOOT_DIR, "media_rw", "media_rw",
                     util::CHOWN_RECURSIVE)) {
        LOGE("Failed to chown %s", MULTIBOOT_DIR);
        return false;
    }

    if (!util::chmod(MULTIBOOT_DIR, 0775, util::CHMOD_RECURSIVE)) {
        LOGE("Failed to chmod %s", MULTIBOOT_DIR);
        return false;
    }

    std::string context;
    if (util::selinux_lget_context(INTERNAL_STORAGE, &context)
            && !util::selinux_lset_context_recursive(MULTIBOOT_DIR, context)) {
        LOGE("%s: Failed to set context to %s: %s",
             MULTIBOOT_DIR, context.c_str(), strerror(errno));
        return false;
    }

    return true;
}

bool switch_context(const std::string &context)
{
    std::string current;

    if (!util::selinux_get_process_attr(
            0, util::SELinuxAttr::CURRENT, &current)) {
        LOGE("Failed to get current process context: %s", strerror(errno));
        // Don't fail if SELinux is not supported
        return errno == ENOENT;
    }

    LOGI("Current process context: %s", current.c_str());

    if (current == context) {
        LOGV("Not switching process context");
        return true;
    }

    LOGV("Setting process context: %s", context.c_str());

    if (!util::selinux_set_process_attr(
            0, util::SELinuxAttr::CURRENT, context)) {
        LOGE("Failed to set current process context: %s", strerror(errno));
        return false;
    }

    return true;
}

}
