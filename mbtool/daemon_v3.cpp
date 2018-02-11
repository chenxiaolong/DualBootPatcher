/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "daemon_v3.h"

#include <unordered_map>
#include <unordered_set>

#include <fcntl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "mbcommon/error_code.h"
#include "mbcommon/finally.h"
#include "mbcommon/string.h"
#include "mbcommon/version.h"
#include "mblog/logging.h"
#include "mbutil/command.h"
#include "mbutil/copy.h"
#include "mbutil/delete.h"
#include "mbutil/directory.h"
#include "mbutil/fts.h"
#include "mbutil/path.h"
#include "mbutil/properties.h"
#include "mbutil/selinux.h"
#include "mbutil/socket.h"
#include "mbutil/string.h"

#include "init.h"
#include "packages.h"
#include "reboot.h"
#include "romconfig.h"
#include "roms.h"
#include "signature.h"
#include "switcher.h"
#include "wipe.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wdocumentation"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

// flatbuffers
#include "protocol/request_generated.h"
#include "protocol/response_generated.h"

#pragma GCC diagnostic pop

#define LOG_TAG "mbtool/daemon_v3"

namespace mb
{

namespace v3 = mbtool::daemon::v3;
namespace fb = flatbuffers;

static std::unordered_map<int, int> fd_map;
static int fd_count = 0;

static bool v3_send_response(int fd, const fb::FlatBufferBuilder &builder)
{
    return util::socket_write_bytes(
            fd, builder.GetBufferPointer(), builder.GetSize()).has_value();
}

static bool v3_send_response_invalid(int fd)
{
    fb::FlatBufferBuilder builder;
    auto response = v3::CreateResponse(builder, v3::ResponseType_Invalid,
                                       v3::CreateInvalid(builder).Union());
    builder.Finish(response);
    return v3_send_response(fd, builder);
}

static bool v3_send_response_unsupported(int fd)
{
    fb::FlatBufferBuilder builder;
    auto response = v3::CreateResponse(builder, v3::ResponseType_Unsupported,
                                       v3::CreateUnsupported(builder).Union());
    builder.Finish(response);
    return v3_send_response(fd, builder);
}

static bool v3_file_chmod(int fd, const v3::Request *msg)
{
    auto request = static_cast<const v3::FileChmodRequest *>(msg->request());
    if (fd_map.find(request->id()) == fd_map.end()) {
        return v3_send_response_invalid(fd);
    }

    int ffd = fd_map[request->id()];

    // Don't allow setting setuid or setgid permissions
    mode_t mode = static_cast<mode_t>(request->mode());
    mode_t masked = mode & (S_IRWXU | S_IRWXG | S_IRWXO);
    if (masked != mode) {
        return v3_send_response_invalid(fd);
    }

    fb::FlatBufferBuilder builder;
    fb::Offset<v3::FileChmodError> error;

    bool ret = fchmod(ffd, mode) == 0;
    int saved_errno = errno;

    if (!ret) {
        error = v3::CreateFileChmodErrorDirect(
                builder, saved_errno, strerror(saved_errno));
    }

    auto response = v3::CreateFileChmodResponseDirect(
            builder, ret, ret ? nullptr : strerror(saved_errno), error);

    // Wrap response
    builder.Finish(v3::CreateResponse(
            builder, v3::ResponseType_FileChmodResponse, response.Union()));

    return v3_send_response(fd, builder);
}

static bool v3_file_close(int fd, const v3::Request *msg)
{
    auto request = static_cast<const v3::FileCloseRequest *>(msg->request());
    auto it = fd_map.find(request->id());
    if (it == fd_map.end()) {
        return v3_send_response_invalid(fd);
    }

    // Remove ID from map
    int ffd = it->second;
    fd_map.erase(it);

    fb::FlatBufferBuilder builder;
    fb::Offset<v3::FileCloseError> error;

    bool ret = close(ffd) == 0;
    int saved_errno = errno;

    if (!ret) {
        error = v3::CreateFileCloseErrorDirect(
                builder, saved_errno, strerror(saved_errno));
    }

    auto response = v3::CreateFileCloseResponseDirect(
            builder, ret, ret ? nullptr : strerror(saved_errno), error);

    // Wrap response
    builder.Finish(v3::CreateResponse(
            builder, v3::ResponseType_FileCloseResponse, response.Union()));

    return v3_send_response(fd, builder);
}

static bool v3_file_open(int fd, const v3::Request *msg)
{
    auto request = static_cast<const v3::FileOpenRequest *>(msg->request());
    if (!request->path()) {
        return v3_send_response_invalid(fd);
    }

    int flags = O_CLOEXEC;

    if (request->flags()) {
        for (short openflag : *request->flags()) {
            if (openflag == v3::FileOpenFlag_APPEND) {
                flags |= O_APPEND;
            } else if (openflag == v3::FileOpenFlag_CREAT) {
                flags |= O_CREAT;
            } else if (openflag == v3::FileOpenFlag_EXCL) {
                flags |= O_EXCL;
            } else if (openflag == v3::FileOpenFlag_RDONLY) {
                flags |= O_RDONLY;
            } else if (openflag == v3::FileOpenFlag_RDWR) {
                flags |= O_RDWR;
            } else if (openflag == v3::FileOpenFlag_TRUNC) {
                flags |= O_TRUNC;
            } else if (openflag == v3::FileOpenFlag_WRONLY) {
                flags |= O_WRONLY;
            }
        }
    }

    fb::FlatBufferBuilder builder;
    fb::Offset<v3::FileOpenError> error;
    int id = -1;

    int ffd = open(request->path()->c_str(), flags, request->perms());
    int saved_errno = errno;

    if (ffd >= 0) {
        // Assign a new ID
        id = fd_count++;
        fd_map[id] = ffd;
    } else {
        error = v3::CreateFileOpenErrorDirect(
                builder, saved_errno, strerror(saved_errno));
    }

    auto response = v3::CreateFileOpenResponseDirect(
            builder, ffd >= 0, ffd >= 0 ? nullptr : strerror(saved_errno), id,
            error);

    // Wrap response
    builder.Finish(v3::CreateResponse(
            builder, v3::ResponseType_FileOpenResponse, response.Union()));

    return v3_send_response(fd, builder);
}

static bool v3_file_read(int fd, const v3::Request *msg)
{
    auto request = static_cast<const v3::FileReadRequest *>(msg->request());
    auto it = fd_map.find(request->id());
    if (it == fd_map.end()) {
        return v3_send_response_invalid(fd);
    }

    int ffd = it->second;

    std::vector<unsigned char> buf(static_cast<size_t>(request->count()));

    fb::FlatBufferBuilder builder;
    fb::Offset<v3::FileReadError> error;
    fb::Offset<fb::Vector<unsigned char>> data;

    ssize_t ret = read(ffd, buf.data(), buf.size());
    int saved_errno = errno;

    if (ret >= 0) {
        data = builder.CreateVector(buf.data(), static_cast<size_t>(ret));
    } else {
        error = v3::CreateFileReadErrorDirect(
                builder, saved_errno, strerror(saved_errno));
    }

    auto response = v3::CreateFileReadResponse(
            builder, ret >= 0,
            ret >= 0 ? 0 : builder.CreateString(strerror(saved_errno)),
            static_cast<size_t>(ret), data, error);

    // Wrap response
    builder.Finish(v3::CreateResponse(
            builder, v3::ResponseType_FileReadResponse, response.Union()));

    return v3_send_response(fd, builder);
}

static bool v3_file_seek(int fd, const v3::Request *msg)
{
    auto request = static_cast<const v3::FileSeekRequest *>(msg->request());
    auto it = fd_map.find(request->id());
    if (it == fd_map.end()) {
        return v3_send_response_invalid(fd);
    }

    int ffd = it->second;
    int64_t offset = request->offset();
    int whence;

    if (request->whence() == v3::FileSeekWhence_SEEK_SET) {
        whence = SEEK_SET;
    } else if (request->whence() == v3::FileSeekWhence_SEEK_CUR) {
        whence = SEEK_CUR;
    } else if (request->whence() == v3::FileSeekWhence_SEEK_END) {
        whence = SEEK_END;
    } else {
        return v3_send_response_invalid(fd);
    }

    fb::FlatBufferBuilder builder;
    fb::Offset<v3::FileSeekError> error;

    // Ahh, posix...
    errno = 0;
    off64_t new_offset = lseek64(ffd, offset, whence);
    int saved_errno = errno;
    bool ret = new_offset >= 0 && saved_errno == 0;

    if (!ret) {
        error = v3::CreateFileSeekErrorDirect(
                builder, saved_errno, strerror(saved_errno));
    }

    auto response = v3::CreateFileSeekResponseDirect(
            builder, ret, ret ? nullptr : strerror(saved_errno), new_offset,
            error);

    // Wrap response
    builder.Finish(v3::CreateResponse(
            builder, v3::ResponseType_FileSeekResponse, response.Union()));

    return v3_send_response(fd, builder);
}

static bool v3_file_selinux_get_label(int fd, const v3::Request *msg)
{
    auto request = static_cast<const v3::FileSELinuxGetLabelRequest *>(
            msg->request());
    auto it = fd_map.find(request->id());
    if (it == fd_map.end()) {
        return v3_send_response_invalid(fd);
    }

    int ffd = it->second;

    fb::FlatBufferBuilder builder;
    fb::Offset<v3::FileSELinuxGetLabelError> error;

    auto label = util::selinux_fget_context(ffd);
    if (!label) {
        error = v3::CreateFileSELinuxGetLabelErrorDirect(
                builder, label.error().value(),
                label.error().message().c_str());
    }

    auto response = v3::CreateFileSELinuxGetLabelResponseDirect(
            builder, !!label, label ? nullptr : label.error().message().c_str(),
            label ? label.value().c_str() : nullptr, error);

    // Wrap response
    builder.Finish(v3::CreateResponse(
            builder, v3::ResponseType_PathSELinuxGetLabelResponse,
            response.Union()));

    return v3_send_response(fd, builder);
}

static bool v3_file_selinux_set_label(int fd, const v3::Request *msg)
{
    auto request = static_cast<const v3::FileSELinuxSetLabelRequest *>(
            msg->request());
    auto it = fd_map.find(request->id());
    if (it == fd_map.end() || !request->label()) {
        return v3_send_response_invalid(fd);
    }

    int ffd = it->second;

    fb::FlatBufferBuilder builder;
    fb::Offset<v3::FileSELinuxSetLabelError> error;

    auto ret = util::selinux_fset_context(ffd, request->label()->str());
    if (!ret) {
        error = v3::CreateFileSELinuxSetLabelErrorDirect(
                builder, ret.error().value(), ret.error().message().c_str());
    }

    auto response = v3::CreateFileSELinuxSetLabelResponseDirect(
            builder, !!ret, ret ? nullptr : ret.error().message().c_str(),
            error);

    // Wrap response
    builder.Finish(v3::CreateResponse(
            builder, v3::ResponseType_FileSELinuxSetLabelResponse,
            response.Union()));

    return v3_send_response(fd, builder);
}

static bool v3_file_stat(int fd, const v3::Request *msg)
{
    auto request = static_cast<const v3::FileStatRequest *>(msg->request());
    auto it = fd_map.find(request->id());
    if (it == fd_map.end()) {
        return v3_send_response_invalid(fd);
    }

    int ffd = it->second;

    fb::FlatBufferBuilder builder;
    fb::Offset<v3::FileStatError> error;
    fb::Offset<v3::StructStat> statbuf;
    struct stat sb;

    bool ret = fstat(ffd, &sb) == 0;
    int saved_errno = errno;

    if (ret) {
        v3::StructStatBuilder ssb(builder);
        ssb.add_dev(sb.st_dev);
        ssb.add_ino(sb.st_ino);
        ssb.add_mode(sb.st_mode);
        ssb.add_nlink(sb.st_nlink);
        ssb.add_uid(sb.st_uid);
        ssb.add_gid(sb.st_gid);
        ssb.add_rdev(sb.st_rdev);
        ssb.add_size(static_cast<uint64_t>(sb.st_size));
        ssb.add_blksize(static_cast<uint64_t>(sb.st_blksize));
        ssb.add_blocks(static_cast<uint64_t>(sb.st_blocks));
        ssb.add_atime(static_cast<uint64_t>(sb.st_atime));
        ssb.add_mtime(static_cast<uint64_t>(sb.st_mtime));
        ssb.add_ctime(static_cast<uint64_t>(sb.st_ctime));
        statbuf = ssb.Finish();
    } else {
        error = v3::CreateFileStatErrorDirect(
                builder, saved_errno, strerror(saved_errno));
    }

    auto response = v3::CreateFileStatResponseDirect(
            builder, ret, ret ? nullptr : strerror(saved_errno), statbuf,
            error);

    // Wrap response
    builder.Finish(v3::CreateResponse(
            builder, v3::ResponseType_FileStatResponse, response.Union()));

    return v3_send_response(fd, builder);
}

static bool v3_file_write(int fd, const v3::Request *msg)
{
    auto request = static_cast<const v3::FileWriteRequest *>(msg->request());
    auto it = fd_map.find(request->id());
    if (it == fd_map.end() || !request->data()) {
        return v3_send_response_invalid(fd);
    }

    int ffd = it->second;

    fb::FlatBufferBuilder builder;
    fb::Offset<v3::FileWriteError> error;

    ssize_t ret = write(ffd, request->data()->Data(), request->data()->size());
    int saved_errno = errno;

    if (ret < 0) {
        error = v3::CreateFileWriteErrorDirect(
                builder, saved_errno, strerror(saved_errno));
    }

    auto response = v3::CreateFileWriteResponseDirect(
            builder, ret >= 0, ret >= 0 ? nullptr : strerror(saved_errno),
            static_cast<size_t>(ret), error);

    // Wrap response
    builder.Finish(v3::CreateResponse(
            builder, v3::ResponseType_FileWriteResponse, response.Union()));

    return v3_send_response(fd, builder);
}

static bool v3_path_chmod(int fd, const v3::Request *msg)
{
    auto request = static_cast<const v3::PathChmodRequest *>(msg->request());
    if (!request->path()) {
        return v3_send_response_invalid(fd);
    }

    // Don't allow setting setuid or setgid permissions
    mode_t mode = static_cast<mode_t>(request->mode());
    mode_t masked = mode & (S_IRWXU | S_IRWXG | S_IRWXO);
    if (masked != mode) {
        return v3_send_response_invalid(fd);
    }

    fb::FlatBufferBuilder builder;
    fb::Offset<v3::PathChmodError> error;

    bool ret = chmod(request->path()->c_str(), mode) == 0;
    int saved_errno = errno;

    if (!ret) {
        error = v3::CreatePathChmodErrorDirect(
                builder, saved_errno, strerror(saved_errno));
    }

    auto response = v3::CreatePathChmodResponseDirect(
            builder, ret, ret ? nullptr : strerror(saved_errno), error);

    // Wrap response
    builder.Finish(v3::CreateResponse(
            builder, v3::ResponseType_PathChmodResponse, response.Union()));

    return v3_send_response(fd, builder);
}

static bool v3_path_copy(int fd, const v3::Request *msg)
{
    auto request = static_cast<const v3::PathCopyRequest *>(msg->request());
    if (!request->source() || !request->target()) {
        return v3_send_response_invalid(fd);
    }

    fb::FlatBufferBuilder builder;
    fb::Offset<v3::PathCopyError> error;

    auto ret = util::copy_contents(request->source()->str(),
                                   request->target()->str());
    if (!ret) {
        error = v3::CreatePathCopyErrorDirect(
                builder, ret.error().ec.value(), ret.error().message().c_str());
    }

    auto response = v3::CreatePathCopyResponseDirect(
            builder, !!ret, ret ? nullptr : ret.error().message().c_str(),
            error);

    // Wrap response
    builder.Finish(v3::CreateResponse(
            builder, v3::ResponseType_PathCopyResponse, response.Union()));

    return v3_send_response(fd, builder);
}

static bool v3_path_delete(int fd, const v3::Request *msg)
{
    auto request = static_cast<const v3::PathDeleteRequest *>(msg->request());
    if (!request->path()) {
        return v3_send_response_invalid(fd);
    }

    bool ret = true;
    std::error_code ec;

    switch (request->flag()) {
    case v3::PathDeleteFlag_REMOVE:
        if (!(ret = remove(request->path()->c_str()) == 0)) {
            ec = ec_from_errno();
        }
        break;
    case v3::PathDeleteFlag_UNLINK:
        if (!(ret = unlink(request->path()->c_str()) == 0)) {
            ec = ec_from_errno();
        }
        break;
    case v3::PathDeleteFlag_RMDIR:
        if (!(ret = rmdir(request->path()->c_str()) == 0)) {
            ec = ec_from_errno();
        }
        break;
    case v3::PathDeleteFlag_RECURSIVE:
        if (auto r = util::delete_recursive(request->path()->str()); !r) {
            ret = false;
            ec = r.error().ec;
        }
        break;
    default:
        return v3_send_response_invalid(fd);
    }

    fb::FlatBufferBuilder builder;
    fb::Offset<v3::PathDeleteError> error;

    if (!ret) {
        error = v3::CreatePathDeleteErrorDirect(
                builder, ec.value(), ec.message().c_str());
    }

    auto response = v3::CreatePathDeleteResponseDirect(
            builder, ret, ret ? nullptr : ec.message().c_str(), error);

    // Wrap response
    builder.Finish(v3::CreateResponse(
            builder, v3::ResponseType_PathDeleteResponse, response.Union()));

    return v3_send_response(fd, builder);
}

static bool v3_path_mkdir(int fd, const v3::Request *msg)
{
    auto request = static_cast<const v3::PathMkdirRequest *>(msg->request());
    if (!request->path()) {
        return v3_send_response_invalid(fd);
    }

    // Don't allow setting setuid or setgid permissions
    mode_t mode = static_cast<mode_t>(request->mode());
    mode_t masked = mode & (S_IRWXU | S_IRWXG | S_IRWXO);
    if (masked != mode) {
        return v3_send_response_invalid(fd);
    }

    fb::FlatBufferBuilder builder;
    fb::Offset<v3::PathMkdirError> error;

    oc::result<void> ret = oc::success();
    if (request->recursive()) {
        ret = util::mkdir_recursive(request->path()->str(), mode);
    } else {
        if (mkdir(request->path()->c_str(), mode) < 0) {
            ret = ec_from_errno();
        }
    }

    if (!ret) {
        error = v3::CreatePathMkdirErrorDirect(
                builder, ret.error().value(), ret.error().message().c_str());
    }

    auto response = v3::CreatePathMkdirResponseDirect(
            builder, !!ret, ret ? nullptr : ret.error().message().c_str(),
            error);

    // Wrap response
    builder.Finish(v3::CreateResponse(
            builder, v3::ResponseType_PathMkdirResponse, response.Union()));

    return v3_send_response(fd, builder);
}

static bool v3_path_readlink(int fd, const v3::Request *msg)
{
    auto request = static_cast<const v3::PathReadlinkRequest *>(msg->request());
    if (!request->path()) {
        return v3_send_response_invalid(fd);
    }

    auto target = util::read_link(request->path()->str());

    fb::FlatBufferBuilder builder;
    fb::Offset<v3::PathReadlinkError> error;

    if (!target) {
        error = v3::CreatePathReadlinkErrorDirect(
                builder, target.error().value(),
                target.error().message().c_str());
    }

    auto response = v3::CreatePathReadlinkResponseDirect(
            builder, target ? target.value().c_str() : nullptr, error);

    // Wrap response
    builder.Finish(v3::CreateResponse(
            builder, v3::ResponseType_PathReadlinkResponse, response.Union()));

    return v3_send_response(fd, builder);
}

static bool v3_path_selinux_get_label(int fd, const v3::Request *msg)
{
    auto request = static_cast<const v3::PathSELinuxGetLabelRequest *>(
            msg->request());
    if (!request->path()) {
        return v3_send_response_invalid(fd);
    }

    oc::result<std::string> label = oc::success();
    if (request->follow_symlinks()) {
        label = util::selinux_get_context(request->path()->str());
    } else {
        label = util::selinux_lget_context(request->path()->str());
    }

    fb::FlatBufferBuilder builder;
    fb::Offset<v3::PathSELinuxGetLabelError> error;

    if (!label) {
        error = v3::CreatePathSELinuxGetLabelErrorDirect(
                builder, label.error().value(),
                label.error().message().c_str());
    }

    auto response = v3::CreatePathSELinuxGetLabelResponseDirect(
            builder, !!label, label ? nullptr : label.error().message().c_str(),
            label ? label.value().c_str() : nullptr, error);

    // Wrap response
    builder.Finish(v3::CreateResponse(
            builder, v3::ResponseType_PathSELinuxGetLabelResponse,
            response.Union()));

    return v3_send_response(fd, builder);
}

static bool v3_path_selinux_set_label(int fd, const v3::Request *msg)
{
    auto request = static_cast<const v3::PathSELinuxSetLabelRequest *>(
            msg->request());
    if (!request->path()) {
        return v3_send_response_invalid(fd);
    }

    oc::result<void> ret = oc::success();
    if (request->follow_symlinks()) {
        ret = util::selinux_set_context(request->path()->str(),
                                        request->label()->str());
    } else {
        ret = util::selinux_lset_context(request->path()->str(),
                                         request->label()->str());
    }

    fb::FlatBufferBuilder builder;
    fb::Offset<v3::PathSELinuxSetLabelError> error;

    if (!ret) {
        error = v3::CreatePathSELinuxSetLabelErrorDirect(
                builder, ret.error().value(), ret.error().message().c_str());
    }

    auto response = v3::CreatePathSELinuxSetLabelResponseDirect(
            builder, !!ret, ret ? nullptr : ret.error().message().c_str(),
            error);

    // Wrap response
    builder.Finish(v3::CreateResponse(
            builder, v3::ResponseType_PathSELinuxSetLabelResponse,
            response.Union()));

    return v3_send_response(fd, builder);
}

class DirectorySizeGetter : public util::FtsWrapper {
public:
    DirectorySizeGetter(std::string path, std::vector<std::string> exclusions)
        : FtsWrapper(path, util::FtsFlag::GroupSpecialFiles)
        , _exclusions(std::move(exclusions))
        , _total(0)
    {
    }

    Actions on_changed_path() override
    {
        // Exclude first-level directories
        if (_curr->fts_level == 1) {
            if (std::find(_exclusions.begin(), _exclusions.end(), _curr->fts_name)
                    != _exclusions.end()) {
                return Action::Skip;
            }
        }

        return Action::Ok;
    }

    Actions on_reached_file() override
    {
        dev_t dev = static_cast<dev_t>(_curr->fts_statp->st_dev);
        ino_t ino = static_cast<ino_t>(_curr->fts_statp->st_ino);

        // If this file has been visited before (hard link), then skip it
        if (_links.find(dev) != _links.end()
                && _links[dev].find(ino) != _links[dev].end()) {
            return Action::Ok;
        }

        _total += static_cast<uint64_t>(_curr->fts_statp->st_size);
        _links[dev].emplace(ino);

        return Action::Ok;
    }

    uint64_t total() const {
        return _total;
    }

private:
    std::vector<std::string> _exclusions;
    std::unordered_map<dev_t, std::unordered_set<ino_t>> _links;
    uint64_t _total;
};

static bool v3_path_get_directory_size(int fd, const v3::Request *msg)
{
    auto request = static_cast<const v3::PathGetDirectorySizeRequest *>(
            msg->request());
    if (!request->path()) {
        return v3_send_response_invalid(fd);
    }

    std::vector<std::string> exclusions;
    if (request->exclusions()) {
        for (auto const *exclusion : *request->exclusions()) {
            exclusions.push_back(exclusion->str());
        }
    }

    DirectorySizeGetter dsg(request->path()->str(), std::move(exclusions));
    bool ret = dsg.run();
    int saved_errno = errno;

    fb::FlatBufferBuilder builder;
    fb::Offset<v3::PathGetDirectorySizeError> error;

    if (!ret) {
        error = v3::CreatePathGetDirectorySizeErrorDirect(
                builder, saved_errno, strerror(saved_errno));
    }

    auto response = v3::CreatePathGetDirectorySizeResponseDirect(
            builder, ret, ret ? nullptr : strerror(saved_errno), dsg.total(),
            error);

    // Wrap response
    builder.Finish(v3::CreateResponse(
            builder, v3::ResponseType_PathGetDirectorySizeResponse,
            response.Union()));

    return v3_send_response(fd, builder);
}

static void signed_exec_output_cb(const char *line, bool error, void *userdata)
{
    (void) error;

    int *fd_ptr = static_cast<int *>(userdata);
    // TODO: Send line

    fb::FlatBufferBuilder builder;
    fb::Offset<fb::String> line_id = builder.CreateString(line);

    // Create response
    auto response = v3::CreateSignedExecOutputResponse(builder, line_id);

    // Wrap response
    builder.Finish(v3::CreateResponse(
            builder, v3::ResponseType_SignedExecOutputResponse,
            response.Union()));

    if (!v3_send_response(*fd_ptr, builder)) {
        // Can't kill the connection from this callback (yet...)
        LOGE("Failed to send output line: %s", strerror(errno));
    }
}

static bool v3_signed_exec(int fd, const v3::Request *msg)
{
    auto request = static_cast<const v3::SignedExecRequest *>(msg->request());
    if (!request->binary_path() || !request->signature_path()) {
        return v3_send_response_invalid(fd);
    }

    static const char *temp_dir = "/mbtool_exec_tmp";

    std::string target_binary;
    std::string target_sig;
    std::vector<std::string> argv;
    int status;
    SigVerifyResult sig_result;
    bool mounted_tmpfs = false;
    // Variables that are part of the response
    v3::SignedExecResult result = v3::SignedExecResult_OTHER_ERROR;
    std::string error_msg;
    int exit_status = -1;
    int term_sig = -1;

    target_binary = temp_dir;
    target_binary += "/binary";
    target_sig = temp_dir;
    target_sig += "/binary.sig";

    // Unmount tmpfs when we're done
    auto unmount_tmpfs = finally([&]{
        if (mounted_tmpfs) {
            umount(temp_dir);
        }
    });

    if (mount("", "/", "", MS_REMOUNT, "") < 0) {
        result = v3::SignedExecResult_OTHER_ERROR;
        error_msg = format("Failed to remount / as rw: %s", strerror(errno));
        LOGE("%s", error_msg.c_str());
        goto done;
    }

    if ((mkdir(temp_dir, 0000) < 0 && errno != EEXIST)
            || chmod(temp_dir, 0000) < 0) {
        result = v3::SignedExecResult_OTHER_ERROR;
        error_msg = format("Failed to create temp directory: %s",
                           strerror(errno));
        LOGE("%s", error_msg.c_str());
        goto done;
    }

    if (mount("", "/", "", MS_REMOUNT | MS_RDONLY, "") < 0) {
        LOGW("Failed to remount / as ro: %s", strerror(errno));
    }

    if (mount("tmpfs", temp_dir, "tmpfs", 0, "mode=000,uid=0,gid=0") < 0) {
        result = v3::SignedExecResult_OTHER_ERROR;
        error_msg = format("Failed to mount tmpfs at temp directory: %s",
                           strerror(errno));
        LOGE("%s", error_msg.c_str());
        goto done;
    }
    mounted_tmpfs = true;

    // Copy binary to tmpfs
    if (auto r = util::copy_file(
            request->binary_path()->str(), target_binary, 0); !r) {
        result = v3::SignedExecResult_OTHER_ERROR;
        error_msg = format("Failed to copy binary to tmpfs: %s",
                           r.error().message().c_str());
        LOGE("%s", error_msg.c_str());
        goto done;
    }

    // Copy signature to tmpfs
    if (auto r = util::copy_file(
            request->signature_path()->str(), target_sig, 0); !r) {
        result = v3::SignedExecResult_OTHER_ERROR;
        error_msg = format("Failed to copy signature to tmpfs: %s",
                           r.error().message().c_str());
        LOGE("%s", error_msg.c_str());
        goto done;
    }

    // Verify signature
    sig_result = verify_signature(target_binary.c_str(), target_sig.c_str());
    if (sig_result != SigVerifyResult::Valid) {
        if (sig_result == SigVerifyResult::Invalid) {
            result = v3::SignedExecResult_INVALID_SIGNATURE;
            error_msg = format("%s: Invalid signature",
                               request->binary_path()->c_str());
        } else {
            result = v3::SignedExecResult_OTHER_ERROR;
            error_msg = format("%s: Failed to verify signature",
                               request->binary_path()->c_str());
        }

        LOGE("%s", error_msg.c_str());
        goto done;
    }

    // Make binary executable
    if (chmod(target_binary.c_str(), 0700) < 0) {
        result = v3::SignedExecResult_OTHER_ERROR;
        error_msg = format("Failed to chmod binary in tmpfs: %s",
                           strerror(errno));
        LOGE("%s", error_msg.c_str());
        goto done;
    }

    // Build arguments
    {
        if (request->arg0()) {
            argv.push_back(request->arg0()->str());
        } else {
            argv.push_back(target_binary);
        }
        if (request->args()) {
            for (auto const *arg : *request->args()) {
                argv.push_back(arg->str());
            }
        }
    }

    // Run executable
    // TODO: Update libmbutil's command.cpp so the callback can return a bool
    //       Right now, if the connection is broken, the command will continue
    //       executing.
    status = util::run_command(target_binary, argv, {}, {},
                               &signed_exec_output_cb, &fd);
    if (status >= 0 && WIFEXITED(status)) {
        result = v3::SignedExecResult_PROCESS_EXITED;
        exit_status = WEXITSTATUS(status);
    } else if (status >= 0 && WIFSIGNALED(status)) {
        result = v3::SignedExecResult_PROCESS_KILLED_BY_SIGNAL;
        term_sig = WTERMSIG(status);
    } else {
        result = v3::SignedExecResult_OTHER_ERROR;
        error_msg = format("Failed to execute process: %s", strerror(errno));
        LOGE("%s", error_msg.c_str());
    }

done:
    fb::FlatBufferBuilder builder;
    fb::Offset<fb::String> error_msg_id = 0;
    fb::Offset<v3::SignedExecError> error;

    if (!error_msg.empty()) {
        error_msg_id = builder.CreateString(error_msg);

        error = v3::CreateSignedExecError(builder, error_msg_id);
    }

    // Create response
    auto response = v3::CreateSignedExecResponse(
            builder, result, error_msg_id, exit_status, term_sig, error);

    // Wrap response
    builder.Finish(v3::CreateResponse(
            builder, v3::ResponseType_SignedExecResponse, response.Union()));

    return v3_send_response(fd, builder);
}

static bool v3_mb_get_booted_rom_id(int fd, const v3::Request *msg)
{
    (void) msg;

    fb::FlatBufferBuilder builder;
    fb::Offset<fb::String> id;
    auto rom = Roms::get_current_rom();
    if (rom) {
        id = builder.CreateString(rom->id);
    }

    // Create response
    auto response = v3::CreateMbGetBootedRomIdResponse(builder, id);

    // Wrap response
    builder.Finish(v3::CreateResponse(
            builder, v3::ResponseType_MbGetBootedRomIdResponse,
            response.Union()));

    return v3_send_response(fd, builder);
}

static bool v3_mb_get_installed_roms(int fd, const v3::Request *msg)
{
    (void) msg;

    fb::FlatBufferBuilder builder;

    Roms roms;
    roms.add_installed();

    std::vector<fb::Offset<v3::MbRom>> fb_roms;

    for (auto r : roms.roms) {
        std::string system_path = r->full_system_path();
        std::string cache_path = r->full_cache_path();
        std::string data_path = r->full_data_path();

        auto fb_id = builder.CreateString(r->id);
        auto fb_system_path = builder.CreateString(system_path);
        auto fb_cache_path = builder.CreateString(cache_path);
        auto fb_data_path = builder.CreateString(data_path);
        fb::Offset<fb::String> fb_version;
        fb::Offset<fb::String> fb_build;

        std::string build_prop;
        if (r->system_is_image) {
            build_prop += "/raw/images/";
            build_prop += r->id;
        } else {
            build_prop += system_path;
        }
        build_prop += "/build.prop";

        std::unordered_map<std::string, std::string> props;

        RomConfig config;
        config.load_file(r->config_path());
        props.swap(config.cached_props);

        util::property_file_get_all(build_prop, props);

        if (auto it = props.find("ro.build.version.release"); it != props.end()) {
            fb_version = builder.CreateString(it->second);
        }
        if (auto it = props.find("ro.build.display.id"); it != props.end()) {
            fb_build = builder.CreateString(it->second);
        }

        v3::MbRomBuilder mrb(builder);
        mrb.add_id(fb_id);
        mrb.add_system_path(fb_system_path);
        mrb.add_cache_path(fb_cache_path);
        mrb.add_data_path(fb_data_path);
        mrb.add_version(fb_version);
        mrb.add_build(fb_build);
        auto fb_rom = mrb.Finish();

        fb_roms.push_back(fb_rom);
    }

    // Create response
    auto response = v3::CreateMbGetInstalledRomsResponseDirect(
            builder, &fb_roms);

    // Wrap response
    builder.Finish(v3::CreateResponse(
            builder, v3::ResponseType_MbGetInstalledRomsResponse,
            response.Union()));

    return v3_send_response(fd, builder);
}

static bool v3_mb_get_version(int fd, const v3::Request *msg)
{
    (void) msg;

    fb::FlatBufferBuilder builder;

    // Get version
    auto response = v3::CreateMbGetVersionResponseDirect(builder, version());

    // Wrap response
    builder.Finish(v3::CreateResponse(
            builder, v3::ResponseType_MbGetVersionResponse, response.Union()));

    return v3_send_response(fd, builder);
}

static bool v3_mb_set_kernel(int fd, const v3::Request *msg)
{
    auto request = static_cast<const v3::MbSetKernelRequest *>(msg->request());
    if (!request->rom_id() || !request->boot_blockdev()) {
        return v3_send_response_invalid(fd);
    }

    fb::FlatBufferBuilder builder;
    fb::Offset<v3::MbSetKernelError> error;

    bool ret = set_kernel(request->rom_id()->str(),
                          request->boot_blockdev()->str());

    if (!ret) {
        error = v3::CreateMbSetKernelError(builder);
    }

    // Create response
    auto response = v3::CreateMbSetKernelResponse(builder, ret, error);

    // Wrap response
    builder.Finish(v3::CreateResponse(
            builder, v3::ResponseType_MbSetKernelResponse, response.Union()));

    return v3_send_response(fd, builder);
}

static bool v3_mb_switch_rom(int fd, const v3::Request *msg)
{
    auto request = static_cast<const v3::MbSwitchRomRequest *>(msg->request());
    if (!request->rom_id() || !request->boot_blockdev()) {
        return v3_send_response_invalid(fd);
    }

    std::vector<std::string> block_dev_dirs;

    if (request->blockdev_base_dirs()) {
        for (auto const *base_dir : *request->blockdev_base_dirs()) {
            block_dev_dirs.push_back(base_dir->str());
        }
    }

    bool force_update_checksums = request->force_update_checksums();

    fb::FlatBufferBuilder builder;
    fb::Offset<v3::MbSwitchRomError> error;

    SwitchRomResult ret = switch_rom(request->rom_id()->str(),
                                     request->boot_blockdev()->str(),
                                     block_dev_dirs,
                                     force_update_checksums);

    bool success = ret == SwitchRomResult::Succeeded;
    v3::MbSwitchRomResult fb_ret = v3::MbSwitchRomResult_FAILED;
    switch (ret) {
    case SwitchRomResult::Succeeded:
        fb_ret = v3::MbSwitchRomResult_SUCCEEDED;
        break;
    case SwitchRomResult::Failed:
        fb_ret = v3::MbSwitchRomResult_FAILED;
        break;
    case SwitchRomResult::ChecksumNotFound:
        fb_ret = v3::MbSwitchRomResult_CHECKSUM_NOT_FOUND;
        break;
    case SwitchRomResult::ChecksumInvalid:
        fb_ret = v3::MbSwitchRomResult_CHECKSUM_INVALID;
        break;
    }

    if (!success) {
        error = v3::CreateMbSwitchRomError(builder);
    }

    // Create response
    auto response = v3::CreateMbSwitchRomResponse(
            builder, success, fb_ret, error);

    // Wrap response
    builder.Finish(v3::CreateResponse(
            builder, v3::ResponseType_MbSwitchRomResponse, response.Union()));

    return v3_send_response(fd, builder);
}

static bool v3_mb_wipe_rom(int fd, const v3::Request *msg)
{
    auto request = static_cast<const v3::MbWipeRomRequest *>(msg->request());
    if (!request->rom_id()) {
        return v3_send_response_invalid(fd);
    }

    // Find and verify ROM is installed
    Roms roms;
    roms.add_installed();

    auto rom = roms.find_by_id(request->rom_id()->str());
    if (!rom) {
        LOGE("Tried to wipe non-installed or invalid ROM ID: %s",
             request->rom_id()->c_str());
        return v3_send_response_invalid(fd);
    }

    // The GUI should check this, but we'll enforce it here
    auto current_rom = Roms::get_current_rom();
    if (current_rom && current_rom->id == rom->id) {
        LOGE("Cannot wipe currently booted ROM: %s", rom->id.c_str());
        return v3_send_response_invalid(fd);
    }

    // Wipe the selected targets
    std::vector<int16_t> succeeded;
    std::vector<int16_t> failed;

    if (request->targets()) {
        std::string raw_system = get_raw_path("/system");
        if (mount("", raw_system.c_str(), "", MS_REMOUNT, "") < 0) {
            LOGW("Failed to mount %s as writable: %s",
                 raw_system.c_str(), strerror(errno));
        }

        for (short target : *request->targets()) {
            bool success = false;

            if (target == v3::MbWipeTarget_SYSTEM) {
                success = wipe_system(rom);
            } else if (target == v3::MbWipeTarget_CACHE) {
                success = wipe_cache(rom);
            } else if (target == v3::MbWipeTarget_DATA) {
                success = wipe_data(rom);
            } else if (target == v3::MbWipeTarget_DALVIK_CACHE) {
                success = wipe_dalvik_cache(rom);
            } else if (target == v3::MbWipeTarget_MULTIBOOT) {
                success = wipe_multiboot(rom);
            } else {
                LOGE("Unknown wipe target %d", target);
            }

            if (success) {
                succeeded.push_back(target);
            } else {
                failed.push_back(target);
            }
        }
    }

    fb::FlatBufferBuilder builder;

    // Create response
    auto response = v3::CreateMbWipeRomResponseDirect(
            builder, &succeeded, &failed);

    // Wrap response
    builder.Finish(v3::CreateResponse(
            builder, v3::ResponseType_MbWipeRomResponse, response.Union()));

    return v3_send_response(fd, builder);
}

static bool v3_mb_get_packages_count(int fd, const v3::Request *msg)
{
    auto request = static_cast<const v3::MbGetPackagesCountRequest *>(
            msg->request());
    if (!request->rom_id()) {
        return v3_send_response_invalid(fd);
    }

    // Find and verify ROM is installed
    Roms roms;
    roms.add_installed();

    auto rom = roms.find_by_id(request->rom_id()->str());
    if (!rom) {
        return v3_send_response_invalid(fd);
    }

    std::string packages_xml(rom->full_data_path());
    packages_xml += "/system/packages.xml";

    fb::FlatBufferBuilder builder;
    fb::Offset<v3::MbGetPackagesCountError> error;
    unsigned int system_pkgs = 0;
    unsigned int update_pkgs = 0;
    unsigned int other_pkgs = 0;

    Packages pkgs;
    bool ret = pkgs.load_xml(packages_xml);

    if (ret) {
        for (std::shared_ptr<Package> pkg : pkgs.pkgs) {
            bool is_system = (pkg->pkg_flags & Package::Flag::SYSTEM)
                    || (pkg->pkg_public_flags & Package::PublicFlag::SYSTEM);
            bool is_update = (pkg->pkg_flags & Package::Flag::UPDATED_SYSTEM_APP)
                    || (pkg->pkg_public_flags & Package::PublicFlag::UPDATED_SYSTEM_APP);

            if (is_update) {
                ++update_pkgs;
            } else if (is_system) {
                ++system_pkgs;
            } else {
                ++other_pkgs;
            }
        }
    } else {
        error = v3::CreateMbGetPackagesCountError(builder);
    }

    auto response = v3::CreateMbGetPackagesCountResponse(
            builder, ret, system_pkgs, update_pkgs, other_pkgs, error);

    // Wrap response
    builder.Finish(v3::CreateResponse(
            builder, v3::ResponseType_MbGetPackagesCountResponse,
            response.Union()));

    return v3_send_response(fd, builder);
}

static bool v3_reboot(int fd, const v3::Request *msg)
{
    auto request = static_cast<const v3::RebootRequest *>(msg->request());

    fb::FlatBufferBuilder builder;
    fb::Offset<v3::RebootError> error;

    std::string reboot_arg;
    if (request->arg()) {
        reboot_arg = request->arg()->str();
    }

    // The client probably won't get the chance to see the success message, but
    // we'll still send it for the sake of symmetry
    bool ret = false;
    switch (request->type()) {
    case v3::RebootType_FRAMEWORK:
        ret = reboot_via_framework(request->confirm());
        break;
    case v3::RebootType_INIT:
        ret = reboot_via_init(reboot_arg);
        break;
    case v3::RebootType_DIRECT:
        ret = reboot_directly(reboot_arg);
        break;
    default:
        LOGE("Invalid reboot type: %d", request->type());
        return v3_send_response_invalid(fd);
    }

    if (!ret) {
        error = v3::CreateRebootError(builder);
    }

    // Create response
    auto response = v3::CreateRebootResponse(builder, ret, error);

    // Wrap response
    builder.Finish(v3::CreateResponse(
            builder, v3::ResponseType_RebootResponse, response.Union()));

    return v3_send_response(fd, builder);
}

static bool v3_shutdown(int fd, const v3::Request *msg)
{
    auto request = static_cast<const v3::ShutdownRequest *>(msg->request());

    fb::FlatBufferBuilder builder;
    fb::Offset<v3::ShutdownError> error;

    // The client probably won't get the chance to see the success message, but
    // we'll still send it for the sake of symmetry
    bool ret = false;
    switch (request->type()) {
    case v3::ShutdownType_INIT:
        ret = shutdown_via_init();
        break;
    case v3::ShutdownType_DIRECT:
        ret = shutdown_directly();
        break;
    default:
        LOGE("Invalid shutdown type: %d", request->type());
        return v3_send_response_invalid(fd);
    }

    if (!ret) {
        error = v3::CreateShutdownError(builder);
    }

    // Create response
    auto response = v3::CreateShutdownResponse(builder, ret, error);

    // Wrap response
    builder.Finish(v3::CreateResponse(
            builder, v3::ResponseType_ShutdownResponse, response.Union()));

    return v3_send_response(fd, builder);
}

typedef bool (*request_handler_fn)(int, const v3::Request *);

struct RequestMap
{
    v3::RequestType type;
    request_handler_fn fn;
};

static RequestMap request_map[] = {
    { v3::RequestType_FileChmodRequest, v3_file_chmod },
    { v3::RequestType_FileCloseRequest, v3_file_close },
    { v3::RequestType_FileOpenRequest, v3_file_open },
    { v3::RequestType_FileReadRequest, v3_file_read },
    { v3::RequestType_FileSeekRequest, v3_file_seek },
    { v3::RequestType_FileSELinuxGetLabelRequest, v3_file_selinux_get_label },
    { v3::RequestType_FileSELinuxSetLabelRequest, v3_file_selinux_set_label },
    { v3::RequestType_FileStatRequest, v3_file_stat },
    { v3::RequestType_FileWriteRequest, v3_file_write },
    { v3::RequestType_PathChmodRequest, v3_path_chmod },
    { v3::RequestType_PathCopyRequest, v3_path_copy },
    { v3::RequestType_PathDeleteRequest, v3_path_delete },
    { v3::RequestType_PathMkdirRequest, v3_path_mkdir },
    { v3::RequestType_PathReadlinkRequest, v3_path_readlink },
    { v3::RequestType_PathSELinuxGetLabelRequest, v3_path_selinux_get_label },
    { v3::RequestType_PathSELinuxSetLabelRequest, v3_path_selinux_set_label },
    { v3::RequestType_PathGetDirectorySizeRequest, v3_path_get_directory_size },
    { v3::RequestType_SignedExecRequest, v3_signed_exec },
    { v3::RequestType_MbGetBootedRomIdRequest, v3_mb_get_booted_rom_id },
    { v3::RequestType_MbGetInstalledRomsRequest, v3_mb_get_installed_roms },
    { v3::RequestType_MbGetVersionRequest, v3_mb_get_version },
    { v3::RequestType_MbSetKernelRequest, v3_mb_set_kernel },
    { v3::RequestType_MbSwitchRomRequest, v3_mb_switch_rom },
    { v3::RequestType_MbWipeRomRequest, v3_mb_wipe_rom },
    { v3::RequestType_MbGetPackagesCountRequest, v3_mb_get_packages_count },
    { v3::RequestType_RebootRequest, v3_reboot },
    { v3::RequestType_ShutdownRequest, v3_shutdown },
    { v3::RequestType_NONE, nullptr }
};

bool connection_version_3(int fd)
{
    std::string command;

    auto close_all_fds = finally([&]{
        // Ensure opened fd's are closed if the connection is lost
        for (auto &p : fd_map) {
            close(p.second);
        }
        fd_map.clear();
    });

    while (1) {
        auto data = util::socket_read_bytes(fd);
        if (!data) {
            LOGE("Failed to read request: %s",  data.error().message().c_str());
            return false;
        }

        auto verifier = fb::Verifier(data.value().data(), data.value().size());
        if (!v3::VerifyRequestBuffer(verifier)) {
            LOGE("Received invalid buffer");
            return false;
        }

        const v3::Request *request = v3::GetRequest(data.value().data());
        v3::RequestType type = request->request_type();
        request_handler_fn fn = nullptr;

        for (auto iter = request_map; iter->fn; ++iter) {
            if (type == iter->type) {
                fn = iter->fn;
                break;
            }
        }

        // NOTE: A false return value indicates a connection error, not a
        //       command failure!
        bool ret = true;

        if (fn) {
            ret = fn(fd, request);
        } else {
            // Invalid command; allow further commands
            ret = v3_send_response_unsupported(fd);
        }

        if (!ret) {
            return false;
        }
    }
}

}
