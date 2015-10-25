/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "daemon.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <getopt.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <unistd.h>

#include <proc/readproc.h>

#include "multiboot.h"
#include "packages.h"
#include "reboot.h"
#include "roms.h"
#include "sepolpatch.h"
#include "switcher.h"
#include "validcerts.h"
#include "version.h"
#include "wipe.h"
#include "util/chown.h"
#include "util/copy.h"
#include "util/delete.h"
#include "util/directory.h"
#include "util/finally.h"
#include "util/logging.h"
#include "util/properties.h"
#include "util/selinux.h"
#include "util/socket.h"

// flatbuffers
#include "protocol/get_version_generated.h"
#include "protocol/get_roms_list_generated.h"
#include "protocol/get_builtin_rom_ids_generated.h"
#include "protocol/get_current_rom_generated.h"
#include "protocol/switch_rom_generated.h"
#include "protocol/set_kernel_generated.h"
#include "protocol/reboot_generated.h"
#include "protocol/open_generated.h"
#include "protocol/copy_generated.h"
#include "protocol/chmod_generated.h"
#include "protocol/wipe_rom_generated.h"
#include "protocol/selinux_get_label_generated.h"
#include "protocol/selinux_set_label_generated.h"
#include "protocol/request_generated.h"
#include "protocol/response_generated.h"

#define RESPONSE_ALLOW "ALLOW"                  // Credentials allowed
#define RESPONSE_DENY "DENY"                    // Credentials denied
#define RESPONSE_OK "OK"                        // Generic accepted response
#define RESPONSE_UNSUPPORTED "UNSUPPORTED"      // Generic unsupported response


namespace mb
{

namespace v2 = mbtool::daemon::v2;
namespace fb = flatbuffers;

static bool v2_send_response(int fd, const fb::FlatBufferBuilder &builder)
{
    return util::socket_write_bytes(
            fd, builder.GetBufferPointer(), builder.GetSize());
}

static bool v2_send_generic_response(int fd, v2::ResponseType type)
{
    fb::FlatBufferBuilder builder;
    v2::ResponseBuilder rb(builder);
    rb.add_type(type);
    builder.Finish(rb.Finish());
    return v2_send_response(fd, builder);
}

static bool v2_get_version(int fd, const v2::Request *msg)
{
    auto request = msg->get_version_request();
    if (!request) {
        return v2_send_generic_response(fd, v2::ResponseType_INVALID);
    }

    fb::FlatBufferBuilder builder;

    // Get version
    auto version = builder.CreateString(get_mbtool_version());
    auto response = v2::CreateGetVersionResponse(builder, version);

    // Wrap response
    v2::ResponseBuilder rb(builder);
    rb.add_type(v2::ResponseType_GET_VERSION);
    rb.add_get_version_response(response);
    builder.Finish(rb.Finish());

    return v2_send_response(fd, builder);
}

static bool v2_get_roms_list(int fd, const v2::Request *msg)
{
    auto request = msg->get_roms_list_request();
    if (!request) {
        return v2_send_generic_response(fd, v2::ResponseType_INVALID);
    }

    fb::FlatBufferBuilder builder;

    Roms roms;
    roms.add_installed();

    std::vector<fb::Offset<v2::Rom>> fb_roms;

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

        std::unordered_map<std::string, std::string> properties;
        util::file_get_all_properties(build_prop, &properties);

        if (properties.find("ro.build.version.release") != properties.end()) {
            const std::string &version = properties["ro.build.version.release"];
            fb_version = builder.CreateString(version);
        }
        if (properties.find("ro.build.display.id") != properties.end()) {
            const std::string &build = properties["ro.build.display.id"];
            fb_build = builder.CreateString(build);
        }

        auto fb_rom = v2::CreateRom(builder, fb_id,
                                    fb_system_path,
                                    fb_cache_path,
                                    fb_data_path,
                                    fb_version,
                                    fb_build);

        fb_roms.push_back(fb_rom);
    }

    // Create response
    auto fb_roms_vec = builder.CreateVector(fb_roms);
    auto response = v2::CreateGetRomsListResponse(builder, fb_roms_vec);

    // Wrap response
    v2::ResponseBuilder rb(builder);
    rb.add_type(v2::ResponseType_GET_ROMS_LIST);
    rb.add_get_roms_list_response(response);
    builder.Finish(rb.Finish());

    return v2_send_response(fd, builder);
}

static bool v2_get_builtin_rom_ids(int fd, const v2::Request *msg)
{
    auto request = msg->get_builtin_rom_ids_request();
    if (!request) {
        return v2_send_generic_response(fd, v2::ResponseType_INVALID);
    }

    fb::FlatBufferBuilder builder;

    std::vector<fb::Offset<fb::String>> ids;

    ids.push_back(builder.CreateString("primary"));
    ids.push_back(builder.CreateString("dual"));
    ids.push_back(builder.CreateString("multi-slot-1"));
    ids.push_back(builder.CreateString("multi-slot-2"));
    ids.push_back(builder.CreateString("multi-slot-3"));

    // Create response
    auto fb_ids = builder.CreateVector(ids);
    auto response = v2::CreateGetBuiltinRomIdsResponse(builder, fb_ids);

    // Wrap response
    v2::ResponseBuilder rb(builder);
    rb.add_type(v2::ResponseType_GET_BUILTIN_ROM_IDS);
    rb.add_get_builtin_rom_ids_response(response);
    builder.Finish(rb.Finish());

    return v2_send_response(fd, builder);
}

static bool v2_get_current_rom(int fd, const v2::Request *msg)
{
    auto request = msg->get_current_rom_request();
    if (!request) {
        return v2_send_generic_response(fd, v2::ResponseType_INVALID);
    }

    fb::FlatBufferBuilder builder;

    fb::Offset<fb::String> id;
    auto rom = Roms::get_current_rom();
    if (rom) {
        id = builder.CreateString(rom->id);
    }

    // Create response
    auto response = v2::CreateGetCurrentRomResponse(builder, id);

    // Wrap response
    v2::ResponseBuilder rb(builder);
    rb.add_type(v2::ResponseType_GET_CURRENT_ROM);
    rb.add_get_current_rom_response(response);
    builder.Finish(rb.Finish());

    return v2_send_response(fd, builder);
}

static bool v2_switch_rom(int fd, const v2::Request *msg)
{
    auto request = msg->switch_rom_request();
    if (!request || !request->rom_id() || !request->boot_blockdev()) {
        return v2_send_generic_response(fd, v2::ResponseType_INVALID);
    }

    std::vector<std::string> block_dev_dirs;

    if (request->blockdev_base_dirs()) {
        for (auto const &base_dir : *request->blockdev_base_dirs()) {
            block_dev_dirs.push_back(base_dir->c_str());
        }
    }

    bool force_update_checksums = request->force_update_checksums();

    fb::FlatBufferBuilder builder;

    SwitchRomResult ret = switch_rom(request->rom_id()->c_str(),
                                     request->boot_blockdev()->c_str(),
                                     block_dev_dirs,
                                     force_update_checksums);

    bool success = ret == SwitchRomResult::SUCCEEDED;
    v2::SwitchRomResult fb_ret = v2::SwitchRomResult_FAILED;
    switch (ret) {
    case SwitchRomResult::SUCCEEDED:
        fb_ret = v2::SwitchRomResult_SUCCEEDED;
        break;
    case SwitchRomResult::FAILED:
        fb_ret = v2::SwitchRomResult_FAILED;
        break;
    case SwitchRomResult::CHECKSUM_NOT_FOUND:
        fb_ret = v2::SwitchRomResult_CHECKSUM_NOT_FOUND;
        break;
    case SwitchRomResult::CHECKSUM_INVALID:
        fb_ret = v2::SwitchRomResult_CHECKSUM_INVALID;
        break;
    }

    // Create response
    auto response = v2::CreateSwitchRomResponse(builder, success, fb_ret);

    // Wrap response
    v2::ResponseBuilder rb(builder);
    rb.add_type(v2::ResponseType_SWITCH_ROM);
    rb.add_switch_rom_response(response);
    builder.Finish(rb.Finish());

    return v2_send_response(fd, builder);
}

static bool v2_set_kernel(int fd, const v2::Request *msg)
{
    auto request = msg->set_kernel_request();
    if (!request || !request->rom_id() || !request->boot_blockdev()) {
        return v2_send_generic_response(fd, v2::ResponseType_INVALID);
    }

    fb::FlatBufferBuilder builder;

    bool success = set_kernel(request->rom_id()->c_str(),
                              request->boot_blockdev()->c_str());

    // Create response
    auto response = v2::CreateSetKernelResponse(builder, success);

    // Wrap response
    v2::ResponseBuilder rb(builder);
    rb.add_type(v2::ResponseType_SET_KERNEL);
    rb.add_set_kernel_response(response);
    builder.Finish(rb.Finish());

    return v2_send_response(fd, builder);
}

static bool v2_reboot(int fd, const v2::Request *msg)
{
    auto request = msg->reboot_request();
    if (!request) {
        return v2_send_generic_response(fd, v2::ResponseType_INVALID);
    }

    fb::FlatBufferBuilder builder;

    std::string reboot_arg;
    if (request->arg()) {
        reboot_arg = request->arg()->c_str();
    }

    // The client probably won't get the chance to see the success message, but
    // we'll still send it for the sake of symmetry
    bool success = reboot_via_init(reboot_arg);

    // Create response
    auto response = v2::CreateRebootResponse(builder, success);

    // Wrap response
    v2::ResponseBuilder rb(builder);
    rb.add_type(v2::ResponseType_REBOOT);
    rb.add_reboot_response(response);
    builder.Finish(rb.Finish());

    return v2_send_response(fd, builder);
}

static bool v2_open(int fd, const v2::Request *msg)
{
    auto request = msg->open_request();
    if (!request || !request->path()) {
        return v2_send_generic_response(fd, v2::ResponseType_INVALID);
    }

    int flags = 0;

    if (request->flags()) {
        for (short openflag : *request->flags()) {
            if (openflag == v2::OpenFlag_APPEND) {
                flags |= O_APPEND;
            } else if (openflag == v2::OpenFlag_CREAT) {
                flags |= O_CREAT;
            } else if (openflag == v2::OpenFlag_EXCL) {
                flags |= O_EXCL;
            } else if (openflag == v2::OpenFlag_RDWR) {
                flags |= O_RDWR;
            } else if (openflag == v2::OpenFlag_TRUNC) {
                flags |= O_TRUNC;
            } else if (openflag == v2::OpenFlag_WRONLY) {
                flags |= O_WRONLY;
            }
        }
    }

    fb::FlatBufferBuilder builder;

    int ffd = open(request->path()->c_str(), flags, 0666);
    if (ffd < 0) {
        // Create response
        auto error = builder.CreateString(strerror(errno));
        auto response = v2::CreateOpenResponse(builder, false, error);

        // Wrap response
        v2::ResponseBuilder rb(builder);
        rb.add_type(v2::ResponseType_OPEN);
        rb.add_open_response(response);
        builder.Finish(rb.Finish());

        return v2_send_response(fd, builder);
    }

    auto close_ffd = util::finally([&]{
        close(ffd);
    });

    // Send SUCCESS, so the client knows to receive an fd
    auto response = v2::CreateOpenResponse(builder, true);

    // Wrap response
    v2::ResponseBuilder rb(builder);
    rb.add_type(v2::ResponseType_OPEN);
    rb.add_open_response(response);
    builder.Finish(rb.Finish());

    if (!v2_send_response(fd, builder)) {
        return false;
    }

    return util::socket_send_fds(fd, { ffd });
}

static bool v2_copy(int fd, const v2::Request *msg)
{
    auto request = msg->copy_request();
    if (!request || !request->source() || !request->target()) {
        return v2_send_generic_response(fd, v2::ResponseType_INVALID);
    }

    fb::FlatBufferBuilder builder;

    fb::Offset<v2::CopyResponse> response;

    if (util::copy_contents(request->source()->c_str(),
                            request->target()->c_str())) {
        response = v2::CreateCopyResponse(builder, true);
    } else {
        auto error = builder.CreateString(strerror(errno));
        response = v2::CreateCopyResponse(builder, false, error);
    }

    // Wrap response
    v2::ResponseBuilder rb(builder);
    rb.add_type(v2::ResponseType_COPY);
    rb.add_copy_response(response);
    builder.Finish(rb.Finish());

    return v2_send_response(fd, builder);
}

static bool v2_chmod(int fd, const v2::Request *msg)
{
    auto request = msg->chmod_request();
    if (!request || !request->path()) {
        return v2_send_generic_response(fd, v2::ResponseType_INVALID);
    }

    // Don't allow setting setuid or setgid permissions
    uint32_t mode = request->mode();
    uint32_t masked = mode & (S_IRWXU | S_IRWXG | S_IRWXO);
    if (masked != mode) {
        return v2_send_generic_response(fd, v2::ResponseType_INVALID);
    }

    fb::FlatBufferBuilder builder;

    fb::Offset<v2::ChmodResponse> response;

    if (chmod(request->path()->c_str(), mode) < 0) {
        auto error = builder.CreateString(strerror(errno));
        response = v2::CreateChmodResponse(builder, false, error);
    } else {
        response = v2::CreateChmodResponse(builder, true);
    }

    // Wrap response
    v2::ResponseBuilder rb(builder);
    rb.add_type(v2::ResponseType_CHMOD);
    rb.add_chmod_response(response);
    builder.Finish(rb.Finish());

    return v2_send_response(fd, builder);
}

static bool v2_wipe_rom(int fd, const v2::Request *msg)
{
    auto request = msg->wipe_rom_request();
    if (!request || !request->rom_id()) {
        return v2_send_generic_response(fd, v2::ResponseType_INVALID);
    }

    // Find and verify ROM is installed
    Roms roms;
    roms.add_installed();

    auto rom = roms.find_by_id(request->rom_id()->c_str());
    if (!rom) {
        LOGE("Tried to wipe non-installed or invalid ROM ID: %s",
             request->rom_id()->c_str());
        return v2_send_generic_response(fd, v2::ResponseType_INVALID);
    }

    // The GUI should check this, but we'll enforce it here
    auto current_rom = Roms::get_current_rom();
    if (current_rom && current_rom->id == rom->id) {
        LOGE("Cannot wipe currently booted ROM: %s", rom->id.c_str());
        return v2_send_generic_response(fd, v2::ResponseType_INVALID);
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

            if (target == v2::WipeTarget_SYSTEM) {
                success = wipe_system(rom);
            } else if (target == v2::WipeTarget_CACHE) {
                success = wipe_cache(rom);
            } else if (target == v2::WipeTarget_DATA) {
                success = wipe_data(rom);
            } else if (target == v2::WipeTarget_DALVIK_CACHE) {
                success = wipe_dalvik_cache(rom);
            } else if (target == v2::WipeTarget_MULTIBOOT) {
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
    auto fb_succeeded = builder.CreateVector(succeeded);
    auto fb_failed = builder.CreateVector(failed);
    auto response = v2::CreateWipeRomResponse(builder, fb_succeeded, fb_failed);

    // Wrap response
    v2::ResponseBuilder rb(builder);
    rb.add_type(v2::ResponseType_WIPE_ROM);
    rb.add_wipe_rom_response(response);
    builder.Finish(rb.Finish());

    return v2_send_response(fd, builder);
}

static bool v2_selinux_get_label(int fd, const v2::Request *msg)
{
    auto request = msg->selinux_get_label_request();
    if (!request || !request->path()) {
        return v2_send_generic_response(fd, v2::ResponseType_INVALID);
    }

    std::string label;
    bool ret;
    if (request->follow_symlinks()) {
        ret = util::selinux_get_context(request->path()->c_str(), &label);
    } else {
        ret = util::selinux_lget_context(request->path()->c_str(), &label);
    }

    fb::FlatBufferBuilder builder;

    fb::Offset<v2::SELinuxGetLabelResponse> response;

    if (!ret) {
        auto error = builder.CreateString(strerror(errno));
        response = v2::CreateSELinuxGetLabelResponse(
                builder, false, 0, error);
    } else {
        auto fb_label = builder.CreateString(label);
        response = v2::CreateSELinuxGetLabelResponse(
                builder, true, fb_label, 0);
    }

    // Wrap response
    v2::ResponseBuilder rb(builder);
    rb.add_type(v2::ResponseType_SELINUX_GET_LABEL);
    rb.add_selinux_get_label_response(response);
    builder.Finish(rb.Finish());

    return v2_send_response(fd, builder);
}

static bool v2_selinux_set_label(int fd, const v2::Request *msg)
{
    auto request = msg->selinux_set_label_request();
    if (!request || !request->path()) {
        return v2_send_generic_response(fd, v2::ResponseType_INVALID);
    }

    bool ret;
    if (request->follow_symlinks()) {
        ret = util::selinux_set_context(request->path()->c_str(),
                                        request->label()->c_str());
    } else {
        ret = util::selinux_lset_context(request->path()->c_str(),
                                         request->label()->c_str());
    }

    fb::FlatBufferBuilder builder;

    fb::Offset<v2::SELinuxSetLabelResponse> response;

    if (!ret) {
        auto error = builder.CreateString(strerror(errno));
        response = v2::CreateSELinuxSetLabelResponse(builder, false, error);
    } else {
        response = v2::CreateSELinuxSetLabelResponse(builder, true);
    }

    // Wrap response
    v2::ResponseBuilder rb(builder);
    rb.add_type(v2::ResponseType_SELINUX_SET_LABEL);
    rb.add_selinux_set_label_response(response);
    builder.Finish(rb.Finish());

    return v2_send_response(fd, builder);
}

static bool connection_version_2(int fd)
{
    std::string command;

    while (1) {
        std::vector<uint8_t> data;
        if (!util::socket_read_bytes(fd, &data)) {
            return false;
        }

        auto verifier = fb::Verifier(data.data(), data.size());
        if (!v2::VerifyRequestBuffer(verifier)) {
            LOGE("Received invalid buffer");
            return false;
        }

        const v2::Request *request = v2::GetRequest(data.data());

        // NOTE: A false return value indicates a connection error, not a
        //       command failure!
        bool ret = true;

        if (request->type() == v2::RequestType_GET_VERSION) {
            ret = v2_get_version(fd, request);
        } else if (request->type() == v2::RequestType_GET_ROMS_LIST) {
            ret = v2_get_roms_list(fd, request);
        } else if (request->type() == v2::RequestType_GET_BUILTIN_ROM_IDS) {
            ret = v2_get_builtin_rom_ids(fd, request);
        } else if (request->type() == v2::RequestType_GET_CURRENT_ROM) {
            ret = v2_get_current_rom(fd, request);
        } else if (request->type() == v2::RequestType_SWITCH_ROM) {
            ret = v2_switch_rom(fd, request);
        } else if (request->type() == v2::RequestType_SET_KERNEL) {
            ret = v2_set_kernel(fd, request);
        } else if (request->type() == v2::RequestType_REBOOT) {
            ret = v2_reboot(fd, request);
        } else if (request->type() == v2::RequestType_OPEN) {
            ret = v2_open(fd, request);
        } else if (request->type() == v2::RequestType_COPY) {
            ret = v2_copy(fd, request);
        } else if (request->type() == v2::RequestType_CHMOD) {
            ret = v2_chmod(fd, request);
        } else if (request->type() == v2::RequestType_WIPE_ROM) {
            ret = v2_wipe_rom(fd, request);
        } else if (request->type() == v2::RequestType_SELINUX_GET_LABEL) {
            ret = v2_selinux_get_label(fd, request);
        } else if (request->type() == v2::RequestType_SELINUX_SET_LABEL) {
            ret = v2_selinux_set_label(fd, request);
        } else {
            // Invalid command; allow further commands
            ret = v2_send_generic_response(fd, v2::ResponseType_UNSUPPORTED);
        }

        if (!ret) {
            return false;
        }
    }

    return true;
}

static bool verify_credentials(uid_t uid)
{
    // Rely on the OS for signature checking and simply compare strings in
    // packages.xml. The only way that file changes is if the package is
    // removed and reinstalled, in which case, Android will kill the client and
    // the connection will terminate. Or, the client already has root access, in
    // which case, there's not much we can do to prevent damage.

    Packages pkgs;
    if (!pkgs.load_xml("/data/system/packages.xml")) {
        LOGE("Failed to load /data/system/packages.xml");
        return false;
    }

    std::shared_ptr<Package> pkg = pkgs.find_by_uid(uid);
    if (!pkg) {
        LOGE("Failed to find package for UID %u", uid);
        return false;
    }

    pkg->dump();
    LOGD("%s has %zu signatures", pkg->name.c_str(), pkg->sig_indexes.size());

    for (const std::string &index : pkg->sig_indexes) {
        if (pkgs.sigs.find(index) == pkgs.sigs.end()) {
            LOGW("Signature index %s has no key", index.c_str());
            continue;
        }

        const std::string &key = pkgs.sigs[index];
        if (std::find(valid_certs.begin(), valid_certs.end(), key)
                != valid_certs.end()) {
            LOGV("%s matches whitelisted signatures", pkg->name.c_str());
            return true;
        }
    }

    LOGE("%s does not match whitelisted signatures", pkg->name.c_str());
    return false;
}

static bool client_connection(int fd)
{
    bool ret = true;
    auto fail = util::finally([&] {
        if (!ret) {
            LOGE("Killing connection");
        }
    });

    LOGD("Accepted connection from %d", fd);

    struct ucred cred;
    socklen_t cred_len = sizeof(struct ucred);

    if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cred, &cred_len) < 0) {
        LOGE("Failed to get socket credentials: %s", strerror(errno));
        return ret = false;
    }

    LOGD("Client PID: %u", cred.pid);
    LOGD("Client UID: %u", cred.uid);
    LOGD("Client GID: %u", cred.gid);

    if (verify_credentials(cred.uid)) {
        if (!util::socket_write_string(fd, RESPONSE_ALLOW)) {
            LOGE("Failed to send credentials allowed message");
            return ret = false;
        }
    } else {
        if (!util::socket_write_string(fd, RESPONSE_DENY)) {
            LOGE("Failed to send credentials denied message");
        }
        return ret = false;
    }

    int32_t version;
    if (!util::socket_read_int32(fd, &version)) {
        LOGE("Failed to get interface version");
        return ret = false;
    }

    if (version == 2) {
        if (!util::socket_write_string(fd, RESPONSE_OK)) {
            return false;
        }

        if (!connection_version_2(fd)) {
            LOGE("[Version 2] Communication error");
        }
        return true;
    } else {
        LOGE("Unsupported interface version: %d", version);
        util::socket_write_string(fd, RESPONSE_UNSUPPORTED);
        return ret = false;
    }

    return true;
}

static bool run_daemon(void)
{
    int fd;
    struct sockaddr_un addr;

    fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (fd < 0) {
        LOGE("Failed to create socket: %s", strerror(errno));
        return false;
    }

    auto close_fd = util::finally([&] {
        close(fd);
    });

    char abs_name[] = "\0mbtool.daemon";
    size_t abs_name_len = sizeof(abs_name) - 1;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_LOCAL;
    memcpy(addr.sun_path, abs_name, abs_name_len);

    // Calculate correct length so the trailing junk is not included in the
    // abstract socket name
    socklen_t addr_len = offsetof(struct sockaddr_un, sun_path) + abs_name_len;

    if (bind(fd, (struct sockaddr *) &addr, addr_len) < 0) {
        LOGE("Failed to bind socket: %s", strerror(errno));
        LOGE("Is another instance running?");
        return false;
    }

    if (listen(fd, 3) < 0) {
        LOGE("Failed to listen on socket: %s", strerror(errno));
        return false;
    }

    // Eat zombies!
    // SIG_IGN reaps zombie processes (it's not just a dummy function)
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGCHLD, &sa, 0) < 0) {
        LOGE("Failed to set SIGCHLD handler: %s", strerror(errno));
        return false;
    }

    LOGD("Socket ready, waiting for connections");

    int client_fd;
    while ((client_fd = accept(fd, nullptr, nullptr)) >= 0) {
        pid_t child_pid = fork();
        if (child_pid < 0) {
            LOGE("Failed to fork: %s", strerror(errno));
        } else if (child_pid == 0) {
            bool ret = client_connection(client_fd);
            close(client_fd);
            _exit(ret ? EXIT_SUCCESS : EXIT_FAILURE);
        }
        close(client_fd);
    }

    if (client_fd < 0) {
        LOGE("Failed to accept connection on socket: %s", strerror(errno));
        return false;
    }

    return true;
}

__attribute__((noreturn))
static void run_daemon_fork(void)
{
    pid_t pid = fork();
    if (pid < 0) {
        LOGE("Failed to fork: %s", strerror(errno));
        _exit(EXIT_FAILURE);
    } else if (pid > 0) {
        _exit(EXIT_SUCCESS);
    }

    if (setsid() < 0) {
        LOGE("Failed to become session leader: %s", strerror(errno));
        _exit(EXIT_FAILURE);
    }

    signal(SIGHUP, SIG_IGN);

    pid = fork();
    if (pid < 0) {
        LOGE("Failed to fork: %s", strerror(errno));
        _exit(EXIT_FAILURE);
    } else if (pid > 0) {
        _exit(EXIT_SUCCESS);
    }

    if (chdir("/") < 0) {
        LOGE("Failed to change cwd to /: %s", strerror(errno));
        _exit(EXIT_FAILURE);
    }

    umask(0);

    LOGD("Started daemon in background");

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    if (open("/dev/null", O_RDONLY) < 0) {
        LOGE("Failed to reopen stdin: %s", strerror(errno));
        _exit(EXIT_FAILURE);
    }
    if (open("/dev/null", O_WRONLY) < 0) {
        LOGE("Failed to reopen stdout: %s", strerror(errno));
        _exit(EXIT_FAILURE);
    }
    if (open("/dev/null", O_RDWR) < 0) {
        LOGE("Failed to reopen stderr: %s", strerror(errno));
        _exit(EXIT_FAILURE);
    }

    run_daemon();
    _exit(EXIT_SUCCESS);
}

static bool patch_sepolicy_daemon()
{
    policydb_t pdb;

    if (policydb_init(&pdb) < 0) {
        LOGE("Failed to initialize policydb");
        return false;
    }

    if (!util::selinux_read_policy(SELINUX_POLICY_FILE, &pdb)) {
        LOGE("Failed to read SELinux policy file: %s", SELINUX_POLICY_FILE);
        policydb_destroy(&pdb);
        return false;
    }

    LOGD("Policy version: %u", pdb.policyvers);

    util::selinux_add_rule(&pdb, "untrusted_app", "init",
                           "unix_stream_socket", "connectto");

    if (!util::selinux_write_policy(SELINUX_LOAD_FILE, &pdb)) {
        LOGE("Failed to write SELinux policy file: %s", SELINUX_LOAD_FILE);
        policydb_destroy(&pdb);
        return false;
    }

    policydb_destroy(&pdb);

    return true;
}

static void daemon_usage(int error)
{
    FILE *stream = error ? stderr : stdout;

    fprintf(stream,
            "Usage: daemon [OPTION]...\n\n"
            "Options:\n"
            "  -d, --daemonize  Fork to background\n"
            "  -r, --replace    Kill existing daemon (if any) before starting\n"
            "  -h, --help       Display this help message\n");
}

int daemon_main(int argc, char *argv[])
{
    int opt;
    bool fork_flag = false;
    bool replace_flag = false;

    static struct option long_options[] = {
        {"daemonize", no_argument, 0, 'd'},
        {"replace",   no_argument, 0, 'r'},
        {"help",      no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, "drh", long_options, &long_index)) != -1) {
        switch (opt) {
        case 'd':
            fork_flag = true;
            break;

        case 'r':
            replace_flag = true;
            break;

        case 'h':
            daemon_usage(0);
            return EXIT_SUCCESS;

        default:
            daemon_usage(1);
            return EXIT_FAILURE;
        }
    }

    // There should be no other arguments
    if (argc - optind != 0) {
        daemon_usage(1);
        return EXIT_FAILURE;
    }

    // Patch SELinux policy to make init permissive
    patch_loaded_sepolicy();

    // Allow untrusted_app to connect to our daemon
    patch_sepolicy_daemon();

    // Set version property if we're the system mbtool (i.e. launched by init)
    // Possible to override this with another program by double forking, letting
    // 2nd child reparent to init, and then calling execve("/mbtool", ...), but
    // meh ...
    if (getppid() == 1) {
        if (!util::set_property("ro.multiboot.version", get_mbtool_version())) {
            std::printf("Failed to set 'ro.multiboot.version' to '%s'\n",
                        get_mbtool_version());
        }
    }

    if (replace_flag) {
        PROCTAB *proc = openproc(PROC_FILLCOM | PROC_FILLSTAT);
        if (proc) {
            pid_t curpid = getpid();

            while (proc_t *info = readproc(proc, nullptr)) {
                if (strcmp(info->cmd, "mbtool") == 0          // This is mbtool
                        && info->cmdline                      // And we can see the command line
                        && info->cmdline[1]                   // And argc > 1
                        && strstr(info->cmdline[1], "daemon") // And it's a daemon process
                        && info->tid != curpid) {             // And we're not killing ourself
                    // Kill the daemon process
                    std::printf("Killing PID %d\n", info->tid);
                    kill(info->tid, SIGTERM);
                }

                freeproc(info);
            }

            closeproc(proc);
        }

        // Give processes a chance to exit
        usleep(500000);
    }

    // Set up logging
    typedef std::unique_ptr<std::FILE, int (*)(std::FILE *)> file_ptr;

    if (!util::mkdir_parent(MULTIBOOT_LOG_DAEMON, 0775) && errno != EEXIST) {
        fprintf(stderr, "Failed to create parent directory of %s: %s\n",
                MULTIBOOT_LOG_DAEMON, strerror(errno));
        return EXIT_FAILURE;
    }

    file_ptr fp(fopen(MULTIBOOT_LOG_DAEMON, "w"), fclose);
    if (!fp) {
        fprintf(stderr, "Failed to open log file %s: %s\n",
                MULTIBOOT_LOG_DAEMON, strerror(errno));
        return EXIT_FAILURE;
    }

    fix_multiboot_permissions();

    // mbtool logging
    util::log_set_logger(std::make_shared<util::StdioLogger>(fp.get(), true));

    if (fork_flag) {
        run_daemon_fork();
    } else {
        return run_daemon() ? EXIT_SUCCESS : EXIT_FAILURE;
    }
}

}
