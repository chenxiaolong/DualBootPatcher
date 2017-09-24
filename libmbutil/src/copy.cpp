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

#include "mbutil/copy.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fts.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <unistd.h>

#include "mbcommon/finally.h"
#include "mbcommon/string.h"
#include "mblog/logging.h"
#include "mbutil/fts.h"
#include "mbutil/path.h"
#include "mbutil/string.h"

#define LOG_TAG "mbutil/copy"

// WARNING: Everything operates on paths, so it's subject to race conditions
// Directory copy operations will not cross mountpoint boundaries

namespace mb
{
namespace util
{

bool copy_data_fd(int fd_source, int fd_target)
{
    char buf[10240];
    ssize_t nread;

    while ((nread = read(fd_source, buf, sizeof buf)) > 0) {
        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            if ((nwritten = write(fd_target, out_ptr,
                                  static_cast<size_t>(nread))) < 0) {
                return false;
            }

            nread -= nwritten;
            out_ptr += nwritten;
        } while (nread > 0);
    }

    return nread == 0;
}

static bool copy_data(const std::string &source, const std::string &target)
{
    int fd_source = -1;
    int fd_target = -1;

    fd_source = open(source.c_str(), O_RDONLY);
    if (fd_source < 0) {
        return false;
    }

    auto close_source_fd = finally([&] {
        close(fd_source);
    });

    fd_target = open(target.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (fd_target < 0) {
        return false;
    }

    auto close_target_fd = finally([&] {
        close(fd_target);
    });

    return copy_data_fd(fd_source, fd_target);
}

bool copy_xattrs(const std::string &source, const std::string &target)
{
    ssize_t size;
    std::vector<char> names;
    char *end_names;
    char *name;
    std::vector<char> value;

    // xattr names are in a NULL-separated list
    size = llistxattr(source.c_str(), nullptr, 0);
    if (size < 0) {
        if (errno == ENOTSUP) {
            LOGV("%s: xattrs not supported on source filesystem",
                 source.c_str());
            return true;
        } else {
            LOGE("%s: Failed to list xattrs: %s",
                 source.c_str(), strerror(errno));
            return false;
        }
    }

    names.resize(static_cast<size_t>(size + 1));

    size = llistxattr(source.c_str(), names.data(), static_cast<size_t>(size));
    if (size < 0) {
        LOGE("%s: Failed to list xattrs on second try: %s",
             source.c_str(), strerror(errno));
        return false;
    } else {
        names[static_cast<size_t>(size)] = '\0';
        end_names = names.data() + size;
    }

    for (name = names.data(); name != end_names; name = strchr(name, '\0') + 1) {
        if (!*name) {
            continue;
        }

        size = lgetxattr(source.c_str(), name, nullptr, 0);
        if (size < 0) {
            LOGW("%s: Failed to get attribute '%s': %s",
                 source.c_str(), name, strerror(errno));
            continue;
        }

        value.resize(static_cast<size_t>(size));

        size = lgetxattr(source.c_str(), name, value.data(),
                         static_cast<size_t>(size));
        if (size < 0) {
            LOGW("%s: Failed to get attribute '%s' on second try: %s",
                 source.c_str(), name, strerror(errno));
            continue;
        }

        if (lsetxattr(target.c_str(), name, value.data(),
                      static_cast<size_t>(size), 0) < 0) {
            if (errno == ENOTSUP) {
                LOGV("%s: xattrs not supported on target filesystem",
                     target.c_str());
                break;
            } else {
                LOGE("%s: Failed to set xattrs: %s",
                     target.c_str(), strerror(errno));
                return false;
            }
        }
    }

    return true;
}

bool copy_stat(const std::string &source, const std::string &target)
{
    struct stat sb;

    if (lstat(source.c_str(), &sb) < 0) {
        LOGE("%s: Failed to stat: %s", source.c_str(), strerror(errno));
        return false;
    }

    if (lchown(target.c_str(), sb.st_uid, sb.st_gid) < 0) {
        LOGE("%s: Failed to chown: %s", target.c_str(), strerror(errno));
        return false;
    }

    if (!S_ISLNK(sb.st_mode)) {
        if (chmod(target.c_str(), sb.st_mode & (S_ISUID | S_ISGID | S_ISVTX
                                              | S_IRWXU | S_IRWXG | S_IRWXO)) < 0) {
            LOGE("%s: Failed to chmod: %s", target.c_str(), strerror(errno));
            return false;
        }
    }

    return true;
}

bool copy_contents(const std::string &source, const std::string &target)
{
    int fd_source = -1;
    int fd_target = -1;

    if ((fd_source = open(source.c_str(), O_RDONLY)) < 0) {
        return false;
    }

    auto close_source_fd = finally([&] {
        close(fd_source);
    });

    if ((fd_target = open(target.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0) {
        return false;
    }

    auto close_target_fd = finally([&] {
        close(fd_target);
    });

    return copy_data_fd(fd_source, fd_target);
}

bool copy_file(const std::string &source, const std::string &target,
               CopyFlags flags)
{
    mode_t old_umask = umask(0);

    auto restore_umask = finally([&] {
        umask(old_umask);
    });

    if (unlink(target.c_str()) < 0 && errno != ENOENT) {
        LOGE("%s: Failed to remove old file: %s",
             target.c_str(), strerror(errno));
        return false;
    }

    struct stat sb;
    if (((flags & CopyFlag::FollowSymlinks)
            ? stat : lstat)(source.c_str(), &sb) < 0) {
        LOGE("%s: Failed to stat: %s",
             source.c_str(), strerror(errno));
        return false;
    }

    switch (sb.st_mode & S_IFMT) {
    case S_IFBLK:
        if (mknod(target.c_str(), S_IFBLK | S_IRWXU,
                  static_cast<dev_t>(sb.st_rdev)) < 0) {
            LOGW("%s: Failed to create block device: %s",
                 target.c_str(), strerror(errno));
            return false;
        }
        break;

    case S_IFCHR:
        if (mknod(target.c_str(), S_IFCHR | S_IRWXU,
                  static_cast<dev_t>(sb.st_rdev)) < 0) {
            LOGW("%s: Failed to create character device: %s",
                 target.c_str(), strerror(errno));
            return false;
        }
        break;

    case S_IFIFO:
        if (mkfifo(target.c_str(), S_IRWXU) < 0) {
            LOGW("%s: Failed to create FIFO pipe: %s",
                 target.c_str(), strerror(errno));
            return false;
        }
        break;

    case S_IFLNK:
        if (!(flags & CopyFlag::FollowSymlinks)) {
            std::string symlink_path;
            if (!read_link(source, symlink_path)) {
                LOGW("%s: Failed to read symlink path: %s",
                     source.c_str(), strerror(errno));
                return false;
            }

            if (symlink(symlink_path.c_str(), target.c_str()) < 0) {
                LOGW("%s: Failed to create symlink: %s",
                     target.c_str(), strerror(errno));
                return false;
            }

            break;
        }

        // Treat as file
        [[gnu::fallthrough]];
        [[clang::fallthrough]];

    case S_IFREG:
        if (!copy_data(source, target)) {
            LOGE("%s: Failed to copy data: %s",
                 target.c_str(), strerror(errno));
            return false;
        }
        break;

    case S_IFSOCK:
        LOGE("%s: Cannot copy socket", target.c_str());
        errno = EINVAL;
        return false;

    case S_IFDIR:
        LOGE("%s: Cannot copy directory", target.c_str());
        errno = EINVAL;
        return false;
    }

    if ((flags & CopyFlag::CopyAttributes)
            && !copy_stat(source, target)) {
        LOGE("%s: Failed to copy attributes: %s",
             target.c_str(), strerror(errno));
        return false;
    }
    if ((flags & CopyFlag::CopyXattrs)
            && !copy_xattrs(source, target)) {
        LOGE("%s: Failed to copy xattrs: %s",
             target.c_str(), strerror(errno));
        return false;
    }

    return true;
}


class RecursiveCopier : public FtsWrapper
{
public:
    RecursiveCopier(std::string path, std::string target, CopyFlags copyflags)
        : FtsWrapper(path, 0)
        , _copyflags(copyflags)
        , _target(std::move(target))
    {
    }

    bool on_pre_execute() override
    {
        // This is almost *never* useful, so we won't allow it
        if (_copyflags & CopyFlag::FollowSymlinks) {
            _error_msg = "CopyFlag::FollowSymlinks not allowed for recursive copies";
            LOGE("%s", _error_msg.c_str());
            return false;
        }

        // Create the target directory if it doesn't exist
        if (mkdir(_target.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) < 0
                && errno != EEXIST) {
            format(_error_msg, "%s: Failed to create directory: %s",
                   _target.c_str(), strerror(errno));
            LOGE("%s", _error_msg.c_str());
            return false;
        }

        // Ensure target is a directory

        if (stat(_target.c_str(), &sb_target) < 0) {
            format(_error_msg, "%s: Failed to stat: %s",
                   _target.c_str(), strerror(errno));
            LOGE("%s", _error_msg.c_str());
            return false;
        }

        if (!S_ISDIR(sb_target.st_mode)) {
            format(_error_msg, "%s: Target exists but is not a directory",
                   _target.c_str());
            LOGE("%s", _error_msg.c_str());
            return false;
        }

        return true;
    }

    Actions on_changed_path() override
    {
        // Make sure we aren't copying the target on top of itself
        if (sb_target.st_dev == _curr->fts_statp->st_dev
                && sb_target.st_ino == _curr->fts_statp->st_ino) {
            format(_error_msg, "%s: Cannot copy on top of itself",
                   _curr->fts_path);
            LOGE("%s", _error_msg.c_str());
            return Action::Fail | Action::Stop;
        }

        // According to fts_read()'s manpage, fts_path includes the path given
        // to fts_open() as a prefix, so 'curr->fts_path + strlen(source)' will
        // give us a relative path we can append to the target.
        _curtgtpath.clear();

        char *relpath = _curr->fts_path + _path.size();

        _curtgtpath += _target;
        if (!(_copyflags & CopyFlag::ExcludeTopLevel)) {
            if (_curtgtpath.back() != '/') {
                _curtgtpath += "/";
            }
            _curtgtpath += _root->fts_name;
        }
        if (_curtgtpath.back() != '/'
                && *relpath != '/' && *relpath != '\0') {
            _curtgtpath += "/";
        }
        _curtgtpath += relpath;

        return Action::Ok;
    }

    Actions on_reached_directory_pre() override
    {
        // Skip tree?
        bool skip = false;
        bool success = true;

        struct stat sb;

        // Create target directory if it doesn't exist
        if (mkdir(_curtgtpath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) < 0
                && errno != EEXIST) {
            format(_error_msg, "%s: Failed to create directory: %s",
                   _curtgtpath.c_str(), strerror(errno));
            LOGW("%s", _error_msg.c_str());
            success = false;
            skip = true;
        }

        // Ensure target path is a directory
        if (!skip && stat(_curtgtpath.c_str(), &sb) == 0
                && !S_ISDIR(sb.st_mode)) {
            format(_error_msg, "%s: Exists but is not a directory",
                   _curtgtpath.c_str());
            LOGW("%s", _error_msg.c_str());
            success = false;
            skip = true;
        }

        // If we're skipping, then we have to set the attributes now, since
        // on_reached_directory_post() won't be called
        if (skip) {
            if (!cp_attrs()) {
                success = false;
            }

            if (!cp_xattrs()) {
                success = false;
            }
        }

        return (skip ? Actions(Action::Skip) : Actions(0))
                | (success ? Action::Ok : Action::Fail);
    }

    Actions on_reached_directory_post() override
    {
        if (!cp_attrs()) {
            return Action::Fail;
        }

        if (!cp_xattrs()) {
            return Action::Fail;
        }

        return Action::Ok;
    }

    Actions on_reached_file() override
    {
        if (!remove_existing_file()) {
            return Action::Fail;
        }

        // Copy file contents
        if (!copy_data(_curr->fts_accpath, _curtgtpath)) {
            format(_error_msg, "%s: Failed to copy data: %s",
                   _curtgtpath.c_str(), strerror(errno));
            LOGW("%s", _error_msg.c_str());
            return Action::Fail;
        }

        if (!cp_attrs()) {
            return Action::Fail;
        }

        if (!cp_xattrs()) {
            return Action::Fail;
        }

        return Action::Ok;
    }

    Actions on_reached_symlink() override
    {
        if (!remove_existing_file()) {
            return Action::Fail;
        }

        // Find current symlink target
        std::string symlink_path;
        if (!read_link(_curr->fts_accpath, symlink_path)) {
            format(_error_msg, "%s: Failed to read symlink path: %s",
                   _curr->fts_accpath, strerror(errno));
            LOGW("%s", _error_msg.c_str());
            return Action::Fail;
        }

        // Create new symlink
        if (symlink(symlink_path.c_str(), _curtgtpath.c_str()) < 0) {
            format(_error_msg, "%s: Failed to create symlink: %s",
                   _curtgtpath.c_str(), strerror(errno));
            LOGW("%s", _error_msg.c_str());
            return Action::Fail;
        }

        if (!cp_attrs()) {
            return Action::Fail;
        }

        if (!cp_xattrs()) {
            return Action::Fail;
        }

        return Action::Ok;
    }

    Actions on_reached_block_device() override
    {
        if (!remove_existing_file()) {
            return Action::Fail;
        }

        if (mknod(_curtgtpath.c_str(), S_IFBLK | S_IRWXU,
                  static_cast<dev_t>(_curr->fts_statp->st_rdev)) < 0) {
            format(_error_msg, "%s: Failed to create block device: %s",
                   _curtgtpath.c_str(), strerror(errno));
            LOGW("%s", _error_msg.c_str());
            return Action::Fail;
        }

        if (!cp_attrs()) {
            return Action::Fail;
        }

        if (!cp_xattrs()) {
            return Action::Fail;
        }

        return Action::Ok;
    }

    Actions on_reached_character_device() override
    {
        if (!remove_existing_file()) {
            return Action::Fail;
        }

        if (mknod(_curtgtpath.c_str(), S_IFCHR | S_IRWXU,
                  static_cast<dev_t>(_curr->fts_statp->st_rdev)) < 0) {
            format(_error_msg, "%s: Failed to create character device: %s",
                   _curtgtpath.c_str(), strerror(errno));
            LOGW("%s", _error_msg.c_str());
            return Action::Fail;
        }

        if (!cp_attrs()) {
            return Action::Fail;
        }

        if (!cp_xattrs()) {
            return Action::Fail;
        }

        return Action::Ok;
    }

    Actions on_reached_fifo() override
    {
        if (!remove_existing_file()) {
            return Action::Fail;
        }

        if (mkfifo(_curtgtpath.c_str(), S_IRWXU) < 0) {
            format(_error_msg, "%s: Failed to create FIFO pipe: %s",
                   _curtgtpath.c_str(), strerror(errno));
            LOGW("%s", _error_msg.c_str());
            return Action::Fail;
        }

        if (!cp_attrs()) {
            return Action::Fail;
        }

        if (!cp_xattrs()) {
            return Action::Fail;
        }

        return Action::Ok;
    }

    Actions on_reached_socket() override
    {
        LOGD("%s: Skipping socket", _curr->fts_accpath);
        return Action::Skip;
    }

private:
    CopyFlags _copyflags;
    std::string _target;
    struct stat sb_target;
    std::string _curtgtpath;

    bool remove_existing_file()
    {
        // Remove existing file
        if (unlink(_curtgtpath.c_str()) < 0 && errno != ENOENT) {
            format(_error_msg, "%s: Failed to remove old path: %s",
                   _curtgtpath.c_str(), strerror(errno));
            LOGW("%s", _error_msg.c_str());
            return false;
        }
        return true;
    }

    bool cp_attrs()
    {
        if ((_copyflags & CopyFlag::CopyAttributes)
                && !copy_stat(_curr->fts_accpath, _curtgtpath)) {
            format(_error_msg, "%s: Failed to copy attributes: %s",
                   _curtgtpath.c_str(), strerror(errno));
            LOGW("%s", _error_msg.c_str());
            return false;
        }
        return true;
    }

    bool cp_xattrs()
    {
        if ((_copyflags & CopyFlag::CopyXattrs)
                && !copy_xattrs(_curr->fts_accpath, _curtgtpath)) {
            format(_error_msg, "%s: Failed to copy xattrs: %s",
                   _curtgtpath.c_str(), strerror(errno));
            LOGW("%s", _error_msg.c_str());
            return false;
        }
        return true;
    }
};


// Copy as much as possible
bool copy_dir(const std::string &source, const std::string &target,
              CopyFlags flags)
{
    mode_t old_umask = umask(0);

    RecursiveCopier copier(source, target, flags);
    bool ret = copier.run();

    umask(old_umask);

    return ret;
}

}
}
