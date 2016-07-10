/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#pragma once

#include <string>
#include <vector>

namespace mb
{

enum class VoldResponse : int
{
    ActionInitiated             = 100,

    VolumeListResult            = 110,
    AsecListResult              = 111,
    StorageUsersListResult      = 112,
    CryptfsGetfieldResult       = 113,

    // 200 series - Requested action has been successfully completed
    CommandOkay                 = 200,
    ShareStatusResult           = 210,
    AsecPathResult              = 211,
    ShareEnabledResult          = 212,
    PasswordTypeResult          = 213,

    // 400 series - The command was accepted but the requested action
    // did not take place.
    OperationFailed             = 400,
    OpFailedNoMedia             = 401,
    OpFailedMediaBlank          = 402,
    OpFailedMediaCorrupt        = 403,
    OpFailedVolNotMounted       = 404,
    OpFailedStorageBusy         = 405,
    OpFailedStorageNotFound     = 406,

    // 500 series - The command was not accepted and the requested
    // action did not take place.
    CommandSyntaxError          = 500,
    CommandParameterError       = 501,
    CommandNoPermission         = 502,

    // 600 series - Unsolicited broadcasts
    UnsolicitedInformational    = 600,
    VolumeStateChange           = 605,
    VolumeMountFailedBlank      = 610,
    VolumeMountFailedDamaged    = 611,
    VolumeMountFailedNoMedia    = 612,
    VolumeUuidChange            = 613,
    VolumeUserLabelChange       = 614,

    ShareAvailabilityChange     = 620,

    VolumeDiskInserted          = 630,
    VolumeDiskRemoved           = 631,
    VolumeBadRemoval            = 632,

    DiskCreated                 = 640,
    DiskSizeChanged             = 641,
    DiskLabelChanged            = 642,
    DiskScanned                 = 643,
    DiskSysPathChanged          = 644,
    DiskDestroyed               = 649,

    VolumeCreated               = 650,
    VolumeStateChanged          = 651,
    VolumeFsTypeChanged         = 652,
    VolumeFsUuidChanged         = 653,
    VolumeFsLabelChanged        = 654,
    VolumePathChanged           = 655,
    VolumeInternalPathChanged   = 656,
    VolumeDestroyed             = 659,

    MoveStatus                  = 660,
    BenchmarkResult             = 661,
    TrimResult                  = 662,
};

const char * vold_response_str(VoldResponse response);

struct VoldEvent
{
    int cmd_number;
    int code;
    std::string message;
    std::string raw_event;
    std::string log_message;
    std::vector<std::string> parsed;

    bool is_class_continue();
    bool is_class_ok();
    bool is_class_server_error();
    bool is_class_client_error();
    bool is_class_unsolicited();

    static bool is_class_continue(int code);
    static bool is_class_ok(int code);
    static bool is_class_server_error(int code);
    static bool is_class_client_error(int code);
    static bool is_class_unsolicited(int code);
};

class VoldConnection
{
public:
    VoldConnection();
    ~VoldConnection();
    bool connect();
    bool disconnect();

    bool execute_command(const std::vector<std::string> &args,
                         VoldEvent *result);

    static bool parse_raw_event(std::string raw_event, VoldEvent *event);
    static std::string escape_args(const std::vector<std::string> &args);
    static bool unescape_args(const std::string &encoded,
                              std::vector<std::string> *result);

    VoldConnection(const VoldConnection &) = delete;
    VoldConnection(VoldConnection &&) = default;
    VoldConnection & operator=(const VoldConnection &) & = delete;
    VoldConnection & operator=(VoldConnection &&) & = default;

private:
    int _fd_vold;
    int _fd_cryptd;
    int _seqnum_vold;
    int _seqnum_cryptd;
    char _buf_vold[4096];
    char _buf_cryptd[4096];
    size_t _buf_used_vold;
    size_t _buf_used_cryptd;

    bool send_msg(int fd, int *seqnum, const std::vector<std::string> &args);
    bool recv_msg(int fd, int seqnum, char *buf, size_t buf_size,
                  size_t *buf_used, VoldEvent *result);
};

int voldclient_main(int argc, char *argv[]);

}
