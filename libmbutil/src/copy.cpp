/*
 * Copyright (C) 2014-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/error_code.h"
#include "mbcommon/finally.h"
#include "mbcommon/string.h"
#include "mblog/logging.h"
#include "mbutil/fts.h"
#include "mbutil/path.h"
#include "mbutil/string.h"

#define LOG_TAG "mbutil/copy"

// WARNING: Everything operates on paths, so it's subject to race conditions
// Directory copy operations will not cross mountpoint boundaries

namespace mb::util
{

oc::result<void> copy_data_fd(int fd_source, int fd_target)
{
    char buf[10240];
    ssize_t nread;

    while ((nread = read(fd_source, buf, sizeof(buf))) > 0) {
        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            if ((nwritten = write(fd_target, out_ptr,
                                  static_cast<size_t>(nread))) < 0) {
                return ec_from_errno();
            }

            nread -= nwritten;
            out_ptr += nwritten;
        } while (nread > 0);
    }

    if (nread < 0) {
        return ec_from_errno();
    }

    return oc::success();
}

static FileOpResult<void> copy_data(const std::string &source,
                                    const std::string &target)
{
    int fd_source = open(source.c_str(), O_RDONLY);
    if (fd_source < 0) {
        return FileOpErrorInfo{source, ec_from_errno()};
    }

    auto close_source_fd = finally([&] {
        close(fd_source);
    });

    int fd_target = open(target.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (fd_target < 0) {
        return FileOpErrorInfo{target, ec_from_errno()};
    }

    auto close_target_fd = finally([&] {
        close(fd_target);
    });

    if (auto r = copy_data_fd(fd_source, fd_target); !r) {
        // TODO: OR SOURCE?
        return FileOpErrorInfo{target, r.error()};
    }

    close_target_fd.dismiss();

    if (close(fd_target) < 0) {
        return FileOpErrorInfo{target, ec_from_errno()};
    }

    return oc::success();
}

FileOpResult<void> copy_xattrs(const std::string &source,
                               const std::string &target)
{
    // xattr names are in a NULL-separated list
    ssize_t size = llistxattr(source.c_str(), nullptr, 0);
    if (size < 0) {
        if (errno == ENOTSUP) {
            LOGV("%s: xattrs not supported on source filesystem",
                 source.c_str());
            return oc::success();
        } else {
            return FileOpErrorInfo{source, ec_from_errno()};
        }
    }

    std::string names;
    names.resize(static_cast<size_t>(size));

    size = llistxattr(source.c_str(), names.data(), names.size());
    if (size < 0) {
        return FileOpErrorInfo{source, ec_from_errno()};
    }

    std::string value;

    for (char *name = names.data(); *name; name = strchr(name, '\0') + 1) {
        size = lgetxattr(source.c_str(), name, nullptr, 0);
        if (size < 0) {
            return FileOpErrorInfo{source, ec_from_errno()};
        }

        value.resize(static_cast<size_t>(size));

        size = lgetxattr(source.c_str(), name, value.data(), value.size());
        if (size < 0) {
            return FileOpErrorInfo{source, ec_from_errno()};
        }

        if (lsetxattr(target.c_str(), name, value.data(), value.size(), 0) < 0) {
            if (errno == ENOTSUP) {
                LOGV("%s: xattrs not supported on target filesystem",
                     target.c_str());
                break;
            } else {
                return FileOpErrorInfo{target, ec_from_errno()};
            }
        }
    }

    return oc::success();
}

FileOpResult<void> copy_stat(const std::string &source,
                             const std::string &target)
{
    struct stat sb;

    if (lstat(source.c_str(), &sb) < 0) {
        return FileOpErrorInfo{source, ec_from_errno()};
    }

    if (lchown(target.c_str(), sb.st_uid, sb.st_gid) < 0) {
        return FileOpErrorInfo{target, ec_from_errno()};
    }

    if (!S_ISLNK(sb.st_mode)) {
        if (chmod(target.c_str(),
                  sb.st_mode & static_cast<mode_t>(~S_IFMT)) < 0) {
            return FileOpErrorInfo{target, ec_from_errno()};
        }
    }

    return oc::success();
}

FileOpResult<void> copy_contents(const std::string &source,
                                 const std::string &target)
{
    int fd_source = open(source.c_str(), O_RDONLY);
    if (fd_source < 0) {
        return FileOpErrorInfo{source, ec_from_errno()};
    }

    auto close_source_fd = finally([&] {
        close(fd_source);
    });

    int fd_target = open(target.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd_target < 0) {
        return FileOpErrorInfo{target, ec_from_errno()};
    }

    auto close_target_fd = finally([&] {
        close(fd_target);
    });

    if (auto r = copy_data_fd(fd_source, fd_target); !r) {
        // TODO: OR SOURCE?
        return FileOpErrorInfo{target, ec_from_errno()};
    }

    return oc::success();
}

FileOpResult<void> copy_file(const std::string &source,
                             const std::string &target, CopyFlags flags)
{
    mode_t old_umask = umask(0);

    auto restore_umask = finally([&] {
        umask(old_umask);
    });

    if (unlink(target.c_str()) < 0 && errno != ENOENT) {
        return FileOpErrorInfo{target, ec_from_errno()};
    }

    struct stat sb;
    if (((flags & CopyFlag::FollowSymlinks)
            ? stat : lstat)(source.c_str(), &sb) < 0) {
        return FileOpErrorInfo{source, ec_from_errno()};
    }

    switch (sb.st_mode & S_IFMT) {
    case S_IFBLK:
        if (mknod(target.c_str(), S_IFBLK | S_IRWXU,
                  static_cast<dev_t>(sb.st_rdev)) < 0) {
            return FileOpErrorInfo{target, ec_from_errno()};
        }
        break;

    case S_IFCHR:
        if (mknod(target.c_str(), S_IFCHR | S_IRWXU,
                  static_cast<dev_t>(sb.st_rdev)) < 0) {
            return FileOpErrorInfo{target, ec_from_errno()};
        }
        break;

    case S_IFIFO:
        if (mkfifo(target.c_str(), S_IRWXU) < 0) {
            return FileOpErrorInfo{target, ec_from_errno()};
        }
        break;

    case S_IFLNK:
        if (!(flags & CopyFlag::FollowSymlinks)) {
            auto symlink_path = read_link(source);
            if (!symlink_path) {
                return FileOpErrorInfo{source, symlink_path.error()};
            }

            if (symlink(symlink_path.value().c_str(), target.c_str()) < 0) {
                return FileOpErrorInfo{target, ec_from_errno()};
            }

            break;
        }

        // Treat as file
        [[fallthrough]];

    case S_IFREG:
        if (auto r = copy_data(source, target); !r) {
            return r.as_failure();
        }
        break;

    case S_IFSOCK:
    case S_IFDIR:
        return FileOpErrorInfo{
                target, std::make_error_code(std::errc::invalid_argument)};
    }

    if (flags & CopyFlag::CopyAttributes) {
        OUTCOME_TRYV(copy_stat(source, target));
    }
    if (flags & CopyFlag::CopyXattrs) {
        OUTCOME_TRYV(copy_xattrs(source, target));
    }

    return oc::success();
}


class RecursiveCopier : public FtsWrapper
{
public:
    FileOpErrorInfo error;

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
            error = {{}, std::make_error_code(std::errc::invalid_argument)};
            return false;
        }

        // Create the target directory if it doesn't exist
        if (mkdir(_target.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) < 0
                && errno != EEXIST) {
            error = {_target, ec_from_errno()};
            return false;
        }

        // Ensure target is a directory

        if (stat(_target.c_str(), &sb_target) < 0) {
            error = {_target, ec_from_errno()};
            return false;
        }

        if (!S_ISDIR(sb_target.st_mode)) {
            error = {_target, std::make_error_code(std::errc::not_a_directory)};
            return false;
        }

        return true;
    }

    Actions on_changed_path() override
    {
        // Make sure we aren't copying the target on top of itself
        if (sb_target.st_dev == _curr->fts_statp->st_dev
                && sb_target.st_ino == _curr->fts_statp->st_ino) {
            error = {_curr->fts_path,
                     std::make_error_code(std::errc::invalid_argument)};
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
            error = {_curtgtpath, ec_from_errno()};
            success = false;
            skip = true;
        }

        // Ensure target path is a directory
        if (!skip && stat(_curtgtpath.c_str(), &sb) == 0
                && !S_ISDIR(sb.st_mode)) {
            error = {_curtgtpath,
                     std::make_error_code(std::errc::not_a_directory)};
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
        if (auto r = copy_data(_curr->fts_accpath, _curtgtpath); !r) {
            error = r.error();
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
        auto symlink_path = read_link(_curr->fts_accpath);
        if (!symlink_path) {
            error = {_curr->fts_accpath, symlink_path.error()};
            return Action::Fail;
        }

        // Create new symlink
        if (symlink(symlink_path.value().c_str(), _curtgtpath.c_str()) < 0) {
            error = {_curtgtpath, ec_from_errno()};
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
            error = {_curtgtpath, ec_from_errno()};
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
            error = {_curtgtpath, ec_from_errno()};
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
            error = {_curtgtpath, ec_from_errno()};
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
            error = {_curtgtpath, ec_from_errno()};
            return false;
        }
        return true;
    }

    bool cp_attrs()
    {
        if (_copyflags & CopyFlag::CopyAttributes) {
            if (auto r = copy_stat(_curr->fts_accpath, _curtgtpath); !r) {
                error = r.error();
                return false;
            }
        }
        return true;
    }

    bool cp_xattrs()
    {
        if (_copyflags & CopyFlag::CopyXattrs) {
            if (auto r = copy_xattrs(_curr->fts_accpath, _curtgtpath); !r) {
                error = r.error();
                return false;
            }
        }
        return true;
    }
};


// Copy as much as possible
FileOpResult<void> copy_dir(const std::string &source,
                            const std::string &target, CopyFlags flags)
{
    mode_t old_umask = umask(0);

    auto restore_umask = finally([&] {
        umask(old_umask);
    });

    RecursiveCopier copier(source, target, flags);

    if (!copier.run()) {
        return std::move(copier.error);
    }

    return oc::success();
}

}
