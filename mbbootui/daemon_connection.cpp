/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "daemon_connection.h"

#include <cerrno>
#include <cstring>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "mbcommon/finally.h"
#include "mblog/logging.h"
#include "mbutil/socket.h"

// Hackish, but gets the job done
#include "../mbtool/include/protocol/mb_get_booted_rom_id_generated.h"
#include "../mbtool/include/protocol/mb_get_installed_roms_generated.h"
#include "../mbtool/include/protocol/mb_get_version_generated.h"
#include "../mbtool/include/protocol/mb_switch_rom_generated.h"
#include "../mbtool/include/protocol/reboot_generated.h"
#include "../mbtool/include/protocol/shutdown_generated.h"
#include "../mbtool/include/protocol/request_generated.h"
#include "../mbtool/include/protocol/response_generated.h"

#define LOG_TAG "mbbootui/daemon_connection"

#define SOCKET_ADDRESS                  "mbtool.daemon"
#define PROTOCOL_VERSION                3

#define HANDSHAKE_RESPONSE_ALLOW        "ALLOW"
#define HANDSHAKE_RESPONSE_DENY         "DENY"
#define HANDSHAKE_RESPONSE_OK           "OK"
#define HANDSHAKE_RESPONSE_UNSUPPORTED  "UNSUPPORTED"

// Singleton
MbtoolConnection mbtool_connection;
MbtoolInterface *mbtool_interface = nullptr;

namespace v3 = mbtool::daemon::v3;
namespace fb = flatbuffers;

class MbtoolInterfaceV3 : public MbtoolInterface
{
public:
    MbtoolInterfaceV3(int fd) : _fd(fd)
    {
    }

    virtual ~MbtoolInterfaceV3()
    {
    }

    virtual bool get_installed_roms(std::vector<Rom> &result)
    {
        fb::FlatBufferBuilder builder;

        std::vector<fb::Offset<v3::MbRom>> fb_roms;

        // Create request
        auto request = v3::CreateMbGetInstalledRomsRequest(builder);

        // Send request
        std::vector<unsigned char> buf;
        const v3::MbGetInstalledRomsResponse *response;
        if (!send_request(buf, builder, request.Union(),
                          v3::RequestType_MbGetInstalledRomsRequest,
                          v3::ResponseType_MbGetInstalledRomsResponse,
                          response)) {
            return false;
        }

        std::vector<Rom> roms;

        if (response->roms()) {
            for (auto const *mb_rom : *response->roms()) {
                roms.emplace_back();
                if (mb_rom->id()) {
                    roms.back().id = mb_rom->id()->str();
                }
                if (mb_rom->system_path()) {
                    roms.back().system_path = mb_rom->system_path()->str();
                }
                if (mb_rom->cache_path()) {
                    roms.back().cache_path = mb_rom->cache_path()->str();
                }
                if (mb_rom->data_path()) {
                    roms.back().data_path = mb_rom->data_path()->str();
                }
                if (mb_rom->version()) {
                    roms.back().version = mb_rom->version()->str();
                }
                if (mb_rom->build()) {
                    roms.back().build = mb_rom->build()->str();
                }
            }
        }

        result.swap(roms);
        return true;
    }

    virtual bool get_booted_rom_id(std::string &result)
    {
        fb::FlatBufferBuilder builder;

        // Create request
        auto request = v3::CreateMbGetBootedRomIdRequest(builder);

        // Send request
        std::vector<unsigned char> buf;
        const v3::MbGetBootedRomIdResponse *response;
        if (!send_request(buf, builder, request.Union(),
                          v3::RequestType_MbGetBootedRomIdRequest,
                          v3::ResponseType_MbGetBootedRomIdResponse,
                          response)) {
            return false;
        }

        if (response->rom_id()) {
            result = response->rom_id()->str();
        } else {
            result.clear();
        }

        return true;
    }

    virtual bool switch_rom(const std::string &id,
                            const std::string &boot_block_dev,
                            const std::vector<std::string> &block_dev_base_dirs,
                            bool force_checksums_update,
                            SwitchRomResult &result)
    {
        fb::FlatBufferBuilder builder;

        std::vector<fb::Offset<fb::String>> base_dir_offsets;

        // Create request
        for (auto const &dir : block_dev_base_dirs) {
            base_dir_offsets.push_back(builder.CreateString(dir));
        }
        auto fb_id = builder.CreateString(id);
        auto fb_boot_block_dev = builder.CreateString(boot_block_dev);
        auto fb_block_dev_base_dirs = builder.CreateVector(base_dir_offsets);
        auto request = v3::CreateMbSwitchRomRequest(builder, fb_id,
                                                    fb_boot_block_dev,
                                                    fb_block_dev_base_dirs,
                                                    force_checksums_update);

        // Send request
        std::vector<unsigned char> buf;
        const v3::MbSwitchRomResponse *response;
        if (!send_request(buf, builder, request.Union(),
                          v3::RequestType_MbSwitchRomRequest,
                          v3::ResponseType_MbSwitchRomResponse,
                          response)) {
            return false;
        }

        SwitchRomResult srr;
        switch (response->result()) {
        case v3::MbSwitchRomResult_SUCCEEDED:
            srr = SwitchRomResult::Succeeded;
            break;
        case v3::MbSwitchRomResult_FAILED:
            srr = SwitchRomResult::Failed;
            break;
        case v3::MbSwitchRomResult_CHECKSUM_INVALID:
            srr = SwitchRomResult::ChecksumInvalid;
            break;
        case v3::MbSwitchRomResult_CHECKSUM_NOT_FOUND:
            srr = SwitchRomResult::ChecksumNotFound;
            break;
        default:
            return false;
        }

        result = srr;
        return true;
    }

    virtual bool reboot(const std::string &arg, bool &result)
    {
        fb::FlatBufferBuilder builder;

        // Create request
        auto fb_arg = builder.CreateString(arg);
        auto request = v3::CreateRebootRequest(builder, fb_arg,
                                               v3::RebootType_DIRECT, false);

        // Send request
        std::vector<unsigned char> buf;
        const v3::RebootResponse *response;
        if (!send_request(buf, builder, request.Union(),
                          v3::RequestType_RebootRequest,
                          v3::ResponseType_RebootResponse,
                          response)) {
            return false;
        }

        result = response->success();
        return true;
    }

    virtual bool shutdown(bool &result)
    {
        fb::FlatBufferBuilder builder;

        // Create request
        auto request = v3::CreateShutdownRequest(builder,
                                                 v3::ShutdownType_DIRECT);

        // Send request
        std::vector<unsigned char> buf;
        const v3::ShutdownResponse *response;
        if (!send_request(buf, builder, request.Union(),
                          v3::RequestType_ShutdownRequest,
                          v3::ResponseType_ShutdownResponse,
                          response)) {
            return false;
        }

        result = response->success();
        return true;
    }

    virtual bool version(std::string &result)
    {
        fb::FlatBufferBuilder builder;

        // Create request
        auto request = v3::CreateMbGetVersionRequest(builder);

        // Send request
        std::vector<unsigned char> buf;
        const v3::MbGetVersionResponse *response;
        if (!send_request(buf, builder, request.Union(),
                          v3::RequestType_MbGetVersionRequest,
                          v3::ResponseType_MbGetVersionResponse,
                          response)) {
            return false;
        }

        if (response->version()) {
            result = response->version()->str();
        } else {
            result.clear();
        }

        return true;
    }

private:
    template<typename Result>
    bool send_request(std::vector<unsigned char> &buf,
                      fb::FlatBufferBuilder &builder,
                      const fb::Offset<void> &fb_request,
                      v3::RequestType request_type,
                      v3::ResponseType expected_type,
                      Result &result)
    {
        // Build request table
        v3::RequestBuilder rb(builder);
        rb.add_request_type(request_type);
        rb.add_request(fb_request);
        builder.Finish(rb.Finish());

        // Send request
        if (auto ret = mb::util::socket_write_bytes(
                _fd, builder.GetBufferPointer(), builder.GetSize()); !ret) {
            LOGE("Failed to send request: %s", ret.error().message().c_str());
            return false;
        }

        // Read response
        auto response_buf = mb::util::socket_read_bytes(_fd);
        if (!response_buf) {
            LOGE("Failed to receive response: %s",
                 response_buf.error().message().c_str());
            return false;
        }
        response_buf.value().swap(buf);

        // Verify response
        auto verifier = fb::Verifier(buf.data(), buf.size());
        if (!v3::VerifyResponseBuffer(verifier)) {
            LOGE("Received invalid buffer");
            return false;
        }

        // Verify response type
        const v3::Response *response = v3::GetResponse(buf.data());
        v3::ResponseType type = response->response_type();

        if (type == v3::ResponseType_Unsupported) {
            LOGE("Daemon does not support request type: %d", request_type);
            return false;
        } else if (type == v3::ResponseType_Invalid) {
            LOGE("Daemon says request is invalid: %d", request_type);
            return false;
        } else if (type != expected_type) {
            LOGE("Unexpected response type (actual=%d, expected=%d)",
                 type, expected_type);
            return false;
        }

        result = static_cast<std::remove_reference_t<Result>>(
                response->response());
        return true;
    }

    int _fd;
};

MbtoolConnection::MbtoolConnection() : _fd(-1), _iface(nullptr)
{
}

MbtoolConnection::MbtoolConnection::~MbtoolConnection()
{
    disconnect();
    delete _iface;
}

bool MbtoolConnection::connect()
{
    if (_fd >= 0) {
        // Already connected
        return true;
    }

    int fd;
    sockaddr_un addr = {};

    fd = socket(AF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        LOGE("Failed to create socket: %s", strerror(errno));
        return false;
    }

    auto close_on_return = mb::finally([&]{
        close(fd);
    });

    char abs_name[] = "\0" SOCKET_ADDRESS;
    size_t abs_name_len = sizeof(abs_name) - 1;

    addr.sun_family = AF_LOCAL;
    memcpy(addr.sun_path, abs_name, abs_name_len);

    // Calculate correct length so the trailing junk is not included in the
    // abstract socket name
    socklen_t addr_len = offsetof(sockaddr_un, sun_path) + abs_name_len;

    if (::connect(fd, reinterpret_cast<sockaddr *>(&addr), addr_len) < 0) {
        LOGE("Failed to connect to socket: %s", strerror(errno));
        return false;
    }

    LOGD("Connected to daemon. Negotiating authorization...");

    // Check authorization result
    auto result = mb::util::socket_read_string(fd);
    if (!result) {
        LOGE("Failed to receive authorization result: %s",
             result.error().message().c_str());
        return false;
    } else if (result.value() == HANDSHAKE_RESPONSE_DENY) {
        LOGE("Daemon denied authorization");
        return false;
    } else if (result.value() != HANDSHAKE_RESPONSE_ALLOW) {
        LOGE("Invalid authorization result from daemon: %s",
             result.value().c_str());
        return false;
    }

    // Send requested interface version
    if (auto ret = mb::util::socket_write_int32(fd, PROTOCOL_VERSION); !ret) {
        LOGE("Failed to send interface version: %s",
             ret.error().message().c_str());
        return false;
    }

    // Check interface request's response
    result = mb::util::socket_read_string(fd);
    if (!result) {
        LOGE("Failed to receive interface request result: %s",
             result.error().message().c_str());
        return false;
    } else if (result.value() == HANDSHAKE_RESPONSE_UNSUPPORTED) {
        LOGE("Daemon does not support interface version %d", PROTOCOL_VERSION);
        return false;
    } else if (result.value() != HANDSHAKE_RESPONSE_OK) {
        LOGE("Invalid interface request result from daemon: %s",
             result.value().c_str());
        return false;
    }

    _fd = fd;
    _iface = new MbtoolInterfaceV3(_fd);

    close_on_return.dismiss();

    return true;
}

bool MbtoolConnection::disconnect()
{
    int ret = 0;

    if (_fd >= 0) {
        ret = close(_fd);
        _fd = -1;
    }

    return ret == 0;
}

MbtoolInterface * MbtoolConnection::interface()
{
    return _iface;
}
