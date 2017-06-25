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

#include "mbutil/selinux.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/xattr.h>
#include <unistd.h>

#include <sepol/sepol.h>

#include "mblog/logging.h"
#include "mbutil/finally.h"
#include "mbutil/fts.h"

#define SELINUX_XATTR           "security.selinux"

#define OPEN_ATTEMPTS           5


namespace mb
{
namespace util
{

class RecursiveSetContext : public FTSWrapper {
public:
    RecursiveSetContext(std::string path, std::string context,
                        bool follow_symlinks)
        : FTSWrapper(path, FTS_GroupSpecialFiles),
        _context(std::move(context)),
        _follow_symlinks(follow_symlinks)
    {
    }

    virtual int on_reached_directory_post() override
    {
        return set_context() ? Action::FTS_OK : Action::FTS_Fail;
    }

    virtual int on_reached_file() override
    {
        return set_context() ? Action::FTS_OK : Action::FTS_Fail;
    }

    virtual int on_reached_symlink() override
    {
        return set_context() ? Action::FTS_OK : Action::FTS_Fail;
    }

    virtual int on_reached_special_file() override
    {
        return set_context() ? Action::FTS_OK : Action::FTS_Fail;
    }

private:
    std::string _context;
    bool _follow_symlinks;

    bool set_context()
    {
        if (_follow_symlinks) {
            return selinux_set_context(_curr->fts_accpath, _context);
        } else {
            return selinux_lset_context(_curr->fts_accpath, _context);
        }
    }
};

bool selinux_read_policy(const std::string &path, policydb_t *pdb)
{
    struct policy_file pf;
    struct stat sb;
    void *map;
    int fd;

    for (int i = 0; i < OPEN_ATTEMPTS; ++i) {
        fd = open(path.c_str(), O_RDONLY);
        if (fd < 0) {
            LOGE("[%d/%d] %s: Failed to open sepolicy: %s",
                 i + 1, OPEN_ATTEMPTS, path.c_str(), strerror(errno));
            if (errno == EBUSY) {
                usleep(500 * 1000);
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

    map = mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) {
        LOGE("%s: Failed to mmap sepolicy: %s", path.c_str(), strerror(errno));
        return false;
    }

    auto unmap_map = finally([&] {
        munmap(map, sb.st_size);
    });

    policy_file_init(&pf);
    pf.type = PF_USE_MEMORY;
    pf.data = (char *) map;
    pf.len = sb.st_size;

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
        fd = open(path.c_str(), O_CREAT | O_TRUNC | O_RDWR, 0644);
        if (fd < 0) {
            LOGE("[%d/%d] %s: Failed to open sepolicy: %s",
                 i + 1, OPEN_ATTEMPTS, path.c_str(), strerror(errno));
            if (errno == EBUSY) {
                usleep(500 * 1000);
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

bool selinux_get_context(const std::string &path, std::string *context)
{
    ssize_t size;
    std::vector<char> value;

    size = getxattr(path.c_str(), SELINUX_XATTR, nullptr, 0);
    if (size < 0) {
        return false;
    }

    value.resize(size);

    size = getxattr(path.c_str(), SELINUX_XATTR, value.data(), size);
    if (size < 0) {
        return false;
    }

    value.push_back('\0');
    *context = value.data();

    return true;
}

bool selinux_lget_context(const std::string &path, std::string *context)
{
    ssize_t size;
    std::vector<char> value;

    size = lgetxattr(path.c_str(), SELINUX_XATTR, nullptr, 0);
    if (size < 0) {
        return false;
    }

    value.resize(size);

    size = lgetxattr(path.c_str(), SELINUX_XATTR, value.data(), size);
    if (size < 0) {
        return false;
    }

    value.push_back('\0');
    *context = value.data();

    return true;
}

bool selinux_fget_context(int fd, std::string *context)
{
    ssize_t size;
    std::vector<char> value;

    size = fgetxattr(fd, SELINUX_XATTR, nullptr, 0);
    if (size < 0) {
        return false;
    }

    value.resize(size);

    size = fgetxattr(fd, SELINUX_XATTR, value.data(), size);
    if (size < 0) {
        return false;
    }

    value.push_back('\0');
    *context = value.data();

    return true;
}

bool selinux_set_context(const std::string &path, const std::string &context)
{
    return setxattr(path.c_str(), SELINUX_XATTR,
                    context.c_str(), context.size() + 1, 0) == 0;
}

bool selinux_lset_context(const std::string &path, const std::string &context)
{
    return lsetxattr(path.c_str(), SELINUX_XATTR,
                     context.c_str(), context.size() + 1, 0) == 0;
}

bool selinux_fset_context(int fd, const std::string &context)
{
    return fsetxattr(fd, SELINUX_XATTR,
                     context.c_str(), context.size() + 1, 0) == 0;
}

bool selinux_set_context_recursive(const std::string &path,
                                   const std::string &context)
{
    return RecursiveSetContext(path, context, true).run();
}

bool selinux_lset_context_recursive(const std::string &path,
                                    const std::string &context)
{
    return RecursiveSetContext(path, context, false).run();
}

bool selinux_get_enforcing(int *value)
{
    int fd = open(SELINUX_ENFORCE_FILE, O_RDONLY);
    if (fd < 0) {
        return false;
    }

    char buf[20];
    memset(buf, 0, sizeof(buf));
    int ret = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (ret < 0) {
        return false;
    }

    int enforce = 0;
    if (sscanf(buf, "%d", &enforce) != 1) {
        return false;
    }

    *value = enforce;

    return true;
}

bool selinux_set_enforcing(int value)
{
    int fd = open(SELINUX_ENFORCE_FILE, O_RDWR);
    if (fd < 0) {
        return false;
    }

    char buf[20];
    snprintf(buf, sizeof(buf), "%d", value);
    int ret = write(fd, buf, strlen(buf));
    close(fd);
    if (ret < 0) {
        return false;
    }

    return true;
}

#ifndef __ANDROID__
static pid_t gettid(void)
{
    return syscall(__NR_gettid);
}
#endif

// Based on procattr.c from libselinux (public domain)
static int open_attr(pid_t pid, SELinuxAttr attr, int flags)
{
    const char *attr_name;

    switch (attr) {
    case SELinuxAttr::CURRENT:
        attr_name = "current";
        break;
    case SELinuxAttr::EXEC:
        attr_name = "exec";
        break;
    case SELinuxAttr::FSCREATE:
        attr_name = "fscreate";
        break;
    case SELinuxAttr::KEYCREATE:
        attr_name = "keycreate";
        break;
    case SELinuxAttr::PREV:
        attr_name = "prev";
        break;
    case SELinuxAttr::SOCKCREATE:
        attr_name = "SOCKCREATE";
        break;
    default:
        errno = EINVAL;
        return false;
    }

    int fd;
    int ret;
    char *path;
    pid_t tid;

    if (pid > 0) {
        ret = asprintf(&path, "/proc/%d/attr/%s", pid, attr_name);
    } else if (pid == 0) {
        ret = asprintf(&path, "/proc/thread-self/attr/%s", attr_name);
        if (ret < 0) {
            return -1;
        }

        fd = open(path, flags | O_CLOEXEC);
        if (fd >= 0 || errno != ENOENT) {
            goto out;
        }

        free(path);
        tid = gettid();
        ret = asprintf(&path, "/proc/self/task/%d/attr/%s", tid, attr_name);
    } else {
        errno = EINVAL;
        return -1;
    }

    if (ret < 0) {
        return -1;
    }

    fd = open(path, flags | O_CLOEXEC);

out:
    free(path);
    return fd;
}

bool selinux_get_process_attr(pid_t pid, SELinuxAttr attr,
                              std::string *context_out)
{
    int fd = open_attr(pid, attr, O_RDONLY);
    if (fd < 0) {
        return false;
    }

    auto close_fd = util::finally([&]{
        int saved_errno = errno;
        close(fd);
        errno = saved_errno;
    });

    std::vector<char> buf(sysconf(_SC_PAGE_SIZE));
    ssize_t n;

    do {
        n = read(fd, buf.data(), buf.size() - 1);
    } while (n < 0 && errno == EINTR);

    if (n < 0) {
        return false;
    }

    // buf is guaranteed to be NULL-terminated
    *context_out = buf.data();
    return true;
}

bool selinux_set_process_attr(pid_t pid, SELinuxAttr attr,
                              const std::string &context)
{
    int fd = open_attr(pid, attr, O_WRONLY | O_CLOEXEC);
    if (fd < 0) {
        return false;
    }

    auto close_fd = util::finally([&]{
        int saved_errno = errno;
        close(fd);
        errno = saved_errno;
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

    return n >= 0;
}

}
}
