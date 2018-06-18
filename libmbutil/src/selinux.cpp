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

#include "mbutil/selinux.h"

#include <chrono>
#include <thread>

#include <cerrno>
#include <cstring>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/xattr.h>
#include <unistd.h>

#include <sepol/sepol.h>

#include "mbcommon/error_code.h"
#include "mbcommon/finally.h"
#include "mbcommon/string.h"
#include "mblog/logging.h"
#include "mbutil/fts.h"

#define LOG_TAG "mbutil/selinux"

static constexpr char SELINUX_XATTR[] = "security.selinux";

static constexpr int OPEN_ATTEMPTS = 5;


namespace mb::util
{

class RecursiveSetContext : public FtsWrapper
{
public:
    RecursiveSetContext(std::string path, std::string context,
                        bool follow_symlinks)
        : FtsWrapper(path, FtsFlag::GroupSpecialFiles)
        , _context(std::move(context))
        , _follow_symlinks(follow_symlinks)
        , _result(oc::success())
    {
    }

    Actions on_reached_directory_post() override
    {
        return set_context() ? Action::Ok : Action::Fail;
    }

    Actions on_reached_file() override
    {
        return set_context() ? Action::Ok : Action::Fail;
    }

    Actions on_reached_symlink() override
    {
        return set_context() ? Action::Ok : Action::Fail;
    }

    Actions on_reached_special_file() override
    {
        return set_context() ? Action::Ok : Action::Fail;
    }

    oc::result<void> result()
    {
        return _result;
    }

private:
    std::string _context;
    bool _follow_symlinks;
    oc::result<void> _result;

    oc::result<void> set_context()
    {
        if (_follow_symlinks) {
            return _result = selinux_set_context(_curr->fts_accpath, _context);
        } else {
            return _result = selinux_lset_context(_curr->fts_accpath, _context);
        }
    }
};

bool selinux_read_policy(const std::string &path, policydb_t *pdb)
{
    using namespace std::chrono_literals;

    struct policy_file pf;
    struct stat sb;
    void *map;
    int fd;

    for (int i = 0; i < OPEN_ATTEMPTS; ++i) {
        fd = open(path.c_str(), O_RDONLY | O_CLOEXEC);
        if (fd < 0) {
            LOGE("[%d/%d] %s: Failed to open sepolicy: %s",
                 i + 1, OPEN_ATTEMPTS, path.c_str(), strerror(errno));
            if (errno == EBUSY) {
                std::this_thread::sleep_for(500ms);
                continue;
            } else {
                return false;
            }
        }
        break;
    }

    auto close_fd = finally([&] {
        close(fd);
    });

    if (fstat(fd, &sb) < 0) {
        LOGE("%s: Failed to stat sepolicy: %s", path.c_str(), strerror(errno));
        return false;
    }

    map = mmap(nullptr, static_cast<size_t>(sb.st_size), PROT_READ, MAP_PRIVATE,
               fd, 0);
    if (map == MAP_FAILED) {
        LOGE("%s: Failed to mmap sepolicy: %s", path.c_str(), strerror(errno));
        return false;
    }

    auto unmap_map = finally([&] {
        munmap(map, static_cast<size_t>(sb.st_size));
    });

    policy_file_init(&pf);
    pf.type = PF_USE_MEMORY;
    pf.data = static_cast<char *>(map);
    pf.len = static_cast<size_t>(sb.st_size);

    auto destroy_pf = finally([&] {
        sepol_handle_destroy(pf.handle);
    });

    return policydb_read(pdb, &pf, 0) == 0;
}

// /sys/fs/selinux/load requires the entire policy to be written in a single
// write(2) call.
// See: http://marc.info/?l=selinux&m=141882521027239&w=2
bool selinux_write_policy(const std::string &path, policydb_t *pdb)
{
    using namespace std::chrono_literals;

    void *data;
    size_t len;
    sepol_handle_t *handle;
    int fd;

    // Don't print warnings to stderr
    handle = sepol_handle_create();
    sepol_msg_set_callback(handle, nullptr, nullptr);

    auto destroy_handle = finally([&] {
        sepol_handle_destroy(handle);
    });

    if (policydb_to_image(handle, pdb, &data, &len) < 0) {
        LOGE("Failed to write policydb to memory");
        return false;
    }

    auto free_data = finally([&] {
        free(data);
    });

    for (int i = 0; i < OPEN_ATTEMPTS; ++i) {
        fd = open(path.c_str(), O_CREAT | O_TRUNC | O_RDWR | O_CLOEXEC, 0644);
        if (fd < 0) {
            LOGE("[%d/%d] %s: Failed to open sepolicy: %s",
                 i + 1, OPEN_ATTEMPTS, path.c_str(), strerror(errno));
            if (errno == EBUSY) {
                std::this_thread::sleep_for(500ms);
                continue;
            } else {
                return false;
            }
        }
        break;
    }

    auto close_fd = finally([&] {
        close(fd);
    });

    if (write(fd, data, len) < 0) {
        LOGE("%s: Failed to write sepolicy: %s", path.c_str(), strerror(errno));
        return false;
    }

    return true;
}

oc::result<std::string> selinux_get_context(const std::string &path)
{
    std::string value;

    ssize_t size = getxattr(path.c_str(), SELINUX_XATTR, nullptr, 0);
    if (size < 0) {
        return ec_from_errno();
    }

    value.resize(static_cast<size_t>(size));

    size = getxattr(path.c_str(), SELINUX_XATTR, value.data(),
                    static_cast<size_t>(size));
    if (size < 0) {
        return ec_from_errno();
    }

    if (!value.empty() && value.back() == '\0') {
        value.pop_back();
    }

    return std::move(value);
}

oc::result<std::string> selinux_lget_context(const std::string &path)
{
    std::string value;

    ssize_t size = lgetxattr(path.c_str(), SELINUX_XATTR, nullptr, 0);
    if (size < 0) {
        return ec_from_errno();
    }

    value.resize(static_cast<size_t>(size));

    size = lgetxattr(path.c_str(), SELINUX_XATTR, value.data(),
                     static_cast<size_t>(size));
    if (size < 0) {
        return ec_from_errno();
    }

    if (!value.empty() && value.back() == '\0') {
        value.pop_back();
    }

    return std::move(value);
}

oc::result<std::string> selinux_fget_context(int fd)
{
    std::string value;

    ssize_t size = fgetxattr(fd, SELINUX_XATTR, nullptr, 0);
    if (size < 0) {
        return ec_from_errno();
    }

    value.resize(static_cast<size_t>(size));

    size = fgetxattr(fd, SELINUX_XATTR, value.data(),
                     static_cast<size_t>(size));
    if (size < 0) {
        return ec_from_errno();
    }

    if (!value.empty() && value.back() == '\0') {
        value.pop_back();
    }

    return std::move(value);
}

oc::result<void> selinux_set_context(const std::string &path,
                                     const std::string &context)
{
    if (setxattr(path.c_str(), SELINUX_XATTR,
                 context.c_str(), context.size() + 1, 0) < 0) {
        return ec_from_errno();
    }

    return oc::success();
}

oc::result<void> selinux_lset_context(const std::string &path,
                                      const std::string &context)
{
    if (lsetxattr(path.c_str(), SELINUX_XATTR,
                  context.c_str(), context.size() + 1, 0) < 0) {
        return ec_from_errno();
    }

    return oc::success();
}

oc::result<void> selinux_fset_context(int fd, const std::string &context)
{
    if (fsetxattr(fd, SELINUX_XATTR,
                  context.c_str(), context.size() + 1, 0) < 0) {
        return ec_from_errno();
    }

    return oc::success();
}

oc::result<void> selinux_set_context_recursive(const std::string &path,
                                   const std::string &context)
{
    RecursiveSetContext rsc(path, context, true);
    if (!rsc.run()) {
        auto ret = rsc.result();
        if (ret) {
            return ec_from_errno();
        } else {
            return ret.as_failure();
        }
    }

    return rsc.result();
}

oc::result<void> selinux_lset_context_recursive(const std::string &path,
                                                const std::string &context)
{
    RecursiveSetContext rsc(path, context, false);
    if (!rsc.run()) {
        auto ret = rsc.result();
        if (ret) {
            return ec_from_errno();
        } else {
            return ret.as_failure();
        }
    }

    return rsc.result();
}

oc::result<bool> selinux_get_enforcing()
{
    int fd = open(SELINUX_ENFORCE_FILE, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        return ec_from_errno();
    }

    auto close_fd = finally([&] {
        close(fd);
    });

    char buf[20] = {};
    if (read(fd, buf, sizeof(buf) - 1) < 0) {
        return ec_from_errno();
    }

    int enforce = 0;
    if (sscanf(buf, "%d", &enforce) != 1) {
        return std::errc::invalid_argument;
    }

    return oc::success(enforce);
}

oc::result<void> selinux_set_enforcing(bool value)
{
    int fd = open(SELINUX_ENFORCE_FILE, O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        return ec_from_errno();
    }

    auto close_fd = finally([&] {
        close(fd);
    });

    if (write(fd, value ? "1" : "0", 1) < 0) {
        return  ec_from_errno();
    }

    return oc::success();
}

#ifndef __ANDROID__
static pid_t gettid(void)
{
    return syscall(__NR_gettid);
}
#endif

// Based on procattr.c from libselinux (public domain)
static oc::result<int> open_attr(pid_t pid, SELinuxAttr attr, int flags)
{
    const char *attr_name;

    switch (attr) {
    case SELinuxAttr::Current:
        attr_name = "current";
        break;
    case SELinuxAttr::Exec:
        attr_name = "exec";
        break;
    case SELinuxAttr::FsCreate:
        attr_name = "fscreate";
        break;
    case SELinuxAttr::KeyCreate:
        attr_name = "keycreate";
        break;
    case SELinuxAttr::Prev:
        attr_name = "prev";
        break;
    case SELinuxAttr::SockCreate:
        attr_name = "sockcreate";
        break;
    default:
        return std::errc::invalid_argument;
    }

    std::string path;

    if (pid > 0) {
        path = format("/proc/%d/attr/%s", pid, attr_name);
    } else if (pid == 0) {
        path = format("/proc/thread-self/attr/%s", attr_name);

        int fd = open(path.c_str(), flags | O_CLOEXEC);
        if (fd < 0 && errno != ENOENT) {
            return ec_from_errno();
        } else if (fd >= 0) {
            return fd;
        }

        path = format("/proc/self/task/%d/attr/%s", gettid(), attr_name);
    } else {
        return std::errc::invalid_argument;
    }

    int fd = open(path.c_str(), flags | O_CLOEXEC);
    if (fd < 0) {
        return ec_from_errno();
    }

    return fd;
}

oc::result<std::string> selinux_get_process_attr(pid_t pid, SELinuxAttr attr)
{
    OUTCOME_TRY(fd, open_attr(pid, attr, O_RDONLY));

    auto close_fd = finally([&]{
        close(fd);
    });

    std::vector<char> buf(static_cast<size_t>(sysconf(_SC_PAGE_SIZE)));
    ssize_t n;

    do {
        n = read(fd, buf.data(), buf.size() - 1);
    } while (n < 0 && errno == EINTR);

    if (n < 0) {
        return ec_from_errno();
    }

    // buf is guaranteed to be NULL-terminated
    return buf.data();
}

oc::result<void> selinux_set_process_attr(pid_t pid, SELinuxAttr attr,
                                          const std::string &context)
{
    OUTCOME_TRY(fd, open_attr(pid, attr, O_WRONLY | O_CLOEXEC));

    auto close_fd = finally([&] {
        close(fd);
    });

    ssize_t n;
    if (context.empty()) {
        // Clear context in the attr file if context is empty
        do {
            n = write(fd, nullptr, 0);
        } while (n < 0 && errno == EINTR);
    } else {
        // Otherwise, write the new context. The written string must be
        // NULL-terminated
        do {
            n = write(fd, context.c_str(), context.size() + 1);
        } while (n < 0 && errno == EINTR);
    }

    if (n < 0) {
        return ec_from_errno();
    }

    return oc::success();
}

}
