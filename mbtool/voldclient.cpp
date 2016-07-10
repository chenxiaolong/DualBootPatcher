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

#include "voldclient.h"

#include <cstring>

#include <getopt.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include "mblog/logging.h"
#include "mbutil/finally.h"
#include "mbutil/integer.h"
#include "mbutil/socket.h"
#include "mbutil/string.h"


#define VOLD_SOCKET_PATH                "/dev/socket/vold"
#define CRYPTD_SOCKET_PATH              "/dev/socket/cryptd"

#define SENSITIVE_MARKER                "{{sensitive}}"

#define UNESCAPE_DEBUG_PRINT            0
#define VOLD_EVENT_DEBUG_PRINT          0

#if UNESCAPE_DEBUG_PRINT
#define UNESCAPE_DEBUG(...)             fprintf(stderr, __VA_ARGS__)
#else
#define UNESCAPE_DEBUG(...)
#endif


namespace mb
{

const char * vold_response_str(VoldResponse response)
{
#define RESPONSE_ITEM(ITEM) { VoldResponse::ITEM, #ITEM }

    static struct {
        VoldResponse response;
        const char *name;
    } mapping[] = {
        RESPONSE_ITEM(ActionInitiated),             /* 100 */

        RESPONSE_ITEM(VolumeListResult),            /* 110 */
        RESPONSE_ITEM(AsecListResult),              /* 111 */
        RESPONSE_ITEM(StorageUsersListResult),      /* 112 */
        RESPONSE_ITEM(CryptfsGetfieldResult),       /* 113 */

        RESPONSE_ITEM(CommandOkay),                 /* 200 */
        RESPONSE_ITEM(ShareStatusResult),           /* 210 */
        RESPONSE_ITEM(AsecPathResult),              /* 211 */
        RESPONSE_ITEM(ShareEnabledResult),          /* 212 */
        RESPONSE_ITEM(PasswordTypeResult),          /* 213 */

        RESPONSE_ITEM(OperationFailed),             /* 400 */
        RESPONSE_ITEM(OpFailedNoMedia),             /* 401 */
        RESPONSE_ITEM(OpFailedMediaBlank),          /* 402 */
        RESPONSE_ITEM(OpFailedMediaCorrupt),        /* 403 */
        RESPONSE_ITEM(OpFailedVolNotMounted),       /* 404 */
        RESPONSE_ITEM(OpFailedStorageBusy),         /* 405 */
        RESPONSE_ITEM(OpFailedStorageNotFound),     /* 406 */

        RESPONSE_ITEM(CommandSyntaxError),          /* 500 */
        RESPONSE_ITEM(CommandParameterError),       /* 501 */
        RESPONSE_ITEM(CommandNoPermission),         /* 502 */

        RESPONSE_ITEM(UnsolicitedInformational),    /* 600 */
        RESPONSE_ITEM(VolumeStateChange),           /* 605 */
        RESPONSE_ITEM(VolumeMountFailedBlank),      /* 610 */
        RESPONSE_ITEM(VolumeMountFailedDamaged),    /* 611 */
        RESPONSE_ITEM(VolumeMountFailedNoMedia),    /* 612 */
        RESPONSE_ITEM(VolumeUuidChange),            /* 613 */
        RESPONSE_ITEM(VolumeUserLabelChange),       /* 614 */

        RESPONSE_ITEM(ShareAvailabilityChange),     /* 620 */

        RESPONSE_ITEM(VolumeDiskInserted),          /* 630 */
        RESPONSE_ITEM(VolumeDiskRemoved),           /* 631 */
        RESPONSE_ITEM(VolumeBadRemoval),            /* 632 */

        RESPONSE_ITEM(DiskCreated),                 /* 640 */
        RESPONSE_ITEM(DiskSizeChanged),             /* 641 */
        RESPONSE_ITEM(DiskLabelChanged),            /* 642 */
        RESPONSE_ITEM(DiskScanned),                 /* 643 */
        RESPONSE_ITEM(DiskSysPathChanged),          /* 644 */
        RESPONSE_ITEM(DiskDestroyed),               /* 649 */

        RESPONSE_ITEM(VolumeCreated),               /* 650 */
        RESPONSE_ITEM(VolumeStateChanged),          /* 651 */
        RESPONSE_ITEM(VolumeFsTypeChanged),         /* 652 */
        RESPONSE_ITEM(VolumeFsUuidChanged),         /* 653 */
        RESPONSE_ITEM(VolumeFsLabelChanged),        /* 654 */
        RESPONSE_ITEM(VolumePathChanged),           /* 655 */
        RESPONSE_ITEM(VolumeInternalPathChanged),   /* 656 */
        RESPONSE_ITEM(VolumeDestroyed),             /* 659 */

        RESPONSE_ITEM(MoveStatus),                  /* 660 */
        RESPONSE_ITEM(BenchmarkResult),             /* 661 */
        RESPONSE_ITEM(TrimResult),                  /* 662 */

        { static_cast<VoldResponse>(0), nullptr },
    };

    for (auto it = mapping; it->name; ++it) {
        if (it->response == response) {
            return it->name;
        }
    }

    return nullptr;
}

VoldConnection::VoldConnection()
    : _fd_vold(-1)
    , _fd_cryptd(-1)
    , _seqnum_vold(0)
    , _seqnum_cryptd(0)
    , _buf_used_vold(0)
    , _buf_used_cryptd(0)
{
}

VoldConnection::~VoldConnection()
{
    disconnect();
}

static int connect_to_socket(const char *path)
{
    int fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (fd < 0) {
        LOGD("%s: Failed to create socket: %s", path, strerror(errno));
        return false;
    }

    bool ret = true;

    auto on_return = mb::util::finally([&]{
        if (!ret) {
            close(fd);
        }
    });

    struct sockaddr_un addr;
    socklen_t addr_len;

    if (strlen(path) > sizeof(addr.sun_path) - 1) {
        LOGE("%s: Socket path is too long", path);
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_LOCAL;
    strcpy(addr.sun_path, path);

    addr_len = offsetof(struct sockaddr_un, sun_path) + strlen(path) + 1;

    if (connect(fd, (struct sockaddr *) &addr, addr_len) < 0) {
        LOGE("%s: Failed to connect to socket: %s", path, strerror(errno));
        return -1;
    }

    return fd;
}

bool VoldConnection::connect()
{
    if (_fd_vold >= 0) {
        // Already connected
        return true;
    }

    int fd_vold = -1;
    int fd_cryptd = -1;

    fd_vold = connect_to_socket(VOLD_SOCKET_PATH);
    if (fd_vold < 0) {
        return false;
    }

    LOGD("Connected to vold socket");

    struct stat sb;
    if (stat(CRYPTD_SOCKET_PATH, &sb) == 0) {
        fd_cryptd = connect_to_socket(CRYPTD_SOCKET_PATH);
        if (fd_cryptd < 0) {
            close(_fd_vold);
            return false;
        }

        LOGD("Connected to cryptd socket");
    }

    _fd_vold = fd_vold;
    _fd_cryptd = fd_cryptd;
    _seqnum_vold = 0;
    _seqnum_cryptd = 0;

    return true;
}

bool VoldConnection::disconnect()
{
    int ret1 = 0;
    int ret2 = 0;

    if (_fd_vold >= 0) {
        ret1 = close(_fd_vold);
        _fd_vold = -1;
    }
    if (_fd_cryptd >= 0) {
        ret2 = close(_fd_cryptd);
        _fd_cryptd = -1;
    }

    return ret1 == 0 && ret2 == 0;
}

bool VoldConnection::execute_command(const std::vector<std::string> &args,
                                     VoldEvent *result)
{
    int fd = _fd_vold;
    int *seqnum = &_seqnum_vold;
    char *buf = _buf_vold;
    size_t buf_size = sizeof(_buf_vold);
    size_t *buf_used = &_buf_used_vold;

    if (_fd_cryptd >= 0 && !args.empty() && args[0] == "cryptfs") {
        fd = _fd_cryptd;
        seqnum = &_seqnum_cryptd;
        buf = _buf_cryptd;
        buf_size = sizeof(_buf_cryptd);
        buf_used = &_buf_used_cryptd;
    }

    return send_msg(fd, seqnum, args)
            && recv_msg(fd, *seqnum, buf, buf_size, buf_used, result);
}

bool VoldConnection::send_msg(int fd, int *seqnum,
                              const std::vector<std::string> &args)
{
    std::string cmd = escape_args(args);
    char cmd_buf[255];

    int ret = snprintf(cmd_buf, sizeof(cmd_buf), "%d %s", *seqnum, cmd.c_str());
    if (ret >= (int) sizeof(cmd_buf)) {
        // Command is too large
        LOGE("Command is too long");
        errno = EINVAL;
        return false;
    }

    if (util::socket_write(fd, cmd_buf, strlen(cmd_buf) + 1) < 0) {
        LOGE("Failed to send command to vold: %s", strerror(errno));
        return false;
    }

    ++(*seqnum);
    return true;
}

bool VoldConnection::recv_msg(int fd, int seqnum, char *buf, size_t buf_size,
                              size_t *buf_used, VoldEvent *result)
{
    while (true) {
        VoldEvent event;
        bool failed = false;
        bool found = false;

        // Try getting the message we want from the current buffer
        size_t offset = 0;
        for (size_t i = 0; i < *buf_used; ++i) {
            if (buf[i] == '\0') {
                // Store raw event
                std::string raw_event = std::string(buf + offset, buf + i);

                // Skip over consumed data
                offset = i + 1;

                if (!parse_raw_event(raw_event, &event)) {
                    LOGE("Failed to parse vold message");
                    return false;
                }

#if VOLD_EVENT_DEBUG_PRINT
                LOGD("Received vold event:");
                LOGD("Code:           %d", event.code);
                LOGD("Command number: %d", event.cmd_number);
                LOGD("Message:        %s", event.message.c_str());
#endif

                if (event.is_class_unsolicited()) {
                    LOGD("Skipping unsolicited message");
                    continue;
                } else if (event.is_class_continue()) {
                    // We can't handle multi-message replies
                    LOGD("Skipping message of the 'continue' class");
                    continue;
                }

                // Sequence number has already advanced
                if (event.cmd_number < seqnum - 1) {
                    // Sequence number is smaller, so just discard the
                    // unconsumed message
                    LOGD("Discarding unconsumed message (seq: %d)",
                         event.cmd_number);
                    continue;
                } else if (event.cmd_number != seqnum - 1) {
                    LOGE("Invalid sequence number (got: %d, expected: %d)",
                         event.cmd_number, seqnum - 1);
                    failed = true;
                } else {
                    // Found the message we want
                    found = true;
                }

                break;
            }
        }

        // If there is an incomplete message, move it to the beginning of the
        // buffer
        if (offset < *buf_used) {
            size_t remaining = *buf_used - offset;
            memmove(buf, buf + offset, remaining);
            *buf_used = remaining;
        } else {
            // Everything was consumed
            *buf_used = 0;
        }

        // Return if we can
        if (failed) {
            return false;
        } else if (found) {
            *result = std::move(event);
            return true;
        }

        // If we got to this point, then we don't have the needed message yet

        if (*buf_used == buf_size) {
            LOGE("Message exceeds buffer size");
            return false;
        }

        ssize_t n = read(fd, buf + *buf_used, buf_size - *buf_used);
        if (n < 0) {
            LOGE("Failed to read data from socket: %s", strerror(errno));
            return false;
        } else if (n == 0) {
            LOGE("Got EOF when reading data from socket");
            return false;
        }

        // Update the bytes we now have
        *buf_used += n;

        // Loop again to try to parse the buffer
    }
}

bool VoldConnection::parse_raw_event(std::string raw_event, VoldEvent *event)
{
    int code;
    int cmd_number = -1;

    // Find space
    auto space = raw_event.find(' ');
    if (space == std::string::npos) {
        LOGE("Missing result code in vold event");
        return false;
    }

    // Temporarily terminate string
    raw_event[space] = '\0';

    // Parse result code
    if (!util::str_to_snum(raw_event.c_str(), 10, &code)) {
        LOGE("Failed to parse result code: %s", strerror(errno));
        return false;
    }

    // Restore space character
    raw_event[space] = ' ';

    // Find next argument
    auto arg_pos = raw_event.find_first_not_of(' ', space + 1);

    if (!VoldEvent::is_class_unsolicited(code)) {
        if (arg_pos == std::string::npos) {
            LOGE("Missing command number in vold event");
            return false;
        }

        // Find space and temporarily terminate the string
        space = raw_event.find(' ', arg_pos);
        if (space == std::string::npos) {
            LOGE("Missing command number in vold event");
            return false;
        }

        // Temporarily terminate string
        raw_event[space] = '\0';

        // Parse command number
        if (!util::str_to_snum(raw_event.c_str() + arg_pos, 10, &cmd_number)) {
            LOGE("Failed to parse command number: %s", strerror(errno));
            return false;
        }

        // Restore space character
        raw_event[space] = ' ';

        // Find remaining args
        arg_pos = raw_event.find_first_not_of(' ', space + 1);
    }

    if (arg_pos == std::string::npos) {
        LOGE("Insufficient arguments for vold event");
        return false;
    }

    std::string log_message(raw_event);
    if (!VoldEvent::is_class_unsolicited(code)
            && arg_pos != std::string::npos
            && raw_event.compare(arg_pos, strlen(SENSITIVE_MARKER),
                                 SENSITIVE_MARKER) == 0) {
        arg_pos += strlen(SENSITIVE_MARKER) + 1;
        log_message = util::format("%d %d {}", code, cmd_number);
    }

    event->cmd_number = cmd_number;
    event->code = code;
    event->message = raw_event.substr(arg_pos);
    event->raw_event = std::move(raw_event);
    event->log_message = std::move(log_message);
    return true;
}

std::string VoldConnection::escape_args(const std::vector<std::string> &args)
{
    std::string result;
    bool first = true;
    for (auto const &arg : args) {
        if (first) {
            first = false;
        } else {
            result += ' ';
        }

//         if (arg.find(' ') == std::string::npos) {
//             // Can't just quote everything since vold is too dumb to parse that
//             result += arg;
//         } else {
            result += '"';
            for (char c : arg) {
                if (c == ' ' || c == '\\') {
                    result += '\\';
                }
                result += c;
            }
            result += '"';
//         }
    }

    return result;
}

bool VoldConnection::unescape_args(const std::string& encoded,
                                   std::vector<std::string>* result)
{
    std::vector<std::string> parsed;
    auto const length = encoded.size();

    std::string::size_type current = 0;
    bool quoted = false;

    // Skip leading whitespace
    for (; current < length && isspace(encoded[current]); ++current);

    if (current < length && encoded[current] == '\"') {
        quoted = true;
        ++current;
    }
    while (current < length) {
        // Find the end of the word
        char terminator = quoted ? '\"' : ' ';

        std::string word;
        for (; current < length && encoded[current] != terminator;) {
            if (!quoted && encoded[current] == '"') {
                UNESCAPE_DEBUG("Quote character found in unquoted word (pos: %zu)\n",
                               current);
                return false;
            } else if (encoded[current] == '\\') {
                if (!quoted) {
                    UNESCAPE_DEBUG("Found escape character in unquoted word (pos: %zu)\n",
                                   current);
                    return false;
                } else if (current == length - 1) {
                    UNESCAPE_DEBUG("Ended with escape character");
                    return false;
                } else if (encoded[current + 1] != '\\'
                        && encoded[current + 1] != '"') {
                    // Unknown escape character
                    UNESCAPE_DEBUG("Unknown escape character 0x%02x (pos: %zu)\n",
                                   encoded[current + 1], current + 1);
                    return false;
                }
                ++current;
            }
            word += encoded[current++];
        }

        if (!quoted) {
            // Trim whitespace if not quoted
            util::trim(word);
        } else if (current == length) {
            // No ending quote
            UNESCAPE_DEBUG("Missing ending quote for word\n");
            return false;
        } else {
            // Skip quote character
            ++current;
        }

        parsed.push_back(word);

        if (current < length && encoded[current] != ' ') {
            UNESCAPE_DEBUG("Missing separator before next word (pos: %zu)\n",
                           current);
            return false;
        } else {
            // Skip whitespace until next word
            for (; current < length && encoded[current] == ' '; ++current);
        }

        if (current < length && encoded[current] == '"') {
            quoted = true;
            ++current;
        } else {
            quoted = false;
        }
    }

    // Check if final word is quoted
    if (quoted) {
        UNESCAPE_DEBUG("Missing ending quote for last word\n");
        return false;
    }

    result->swap(parsed);
    return true;
}

bool VoldEvent::is_class_continue()
{
    return is_class_continue(code);
}

bool VoldEvent::is_class_ok()
{
    return is_class_ok(code);
}

bool VoldEvent::is_class_server_error()
{
    return is_class_server_error(code);
}

bool VoldEvent::is_class_client_error()
{
    return is_class_client_error(code);
}

bool VoldEvent::is_class_unsolicited()
{
    return is_class_unsolicited(code);
}

bool VoldEvent::is_class_continue(int code)
{
    return code >= 100 && code < 200;
}

bool VoldEvent::is_class_ok(int code)
{
    return code >= 200 && code < 300;
}

bool VoldEvent::is_class_server_error(int code)
{
    return code >= 400 && code < 500;
}

bool VoldEvent::is_class_client_error(int code)
{
    return code >= 500 && code < 600;
}

bool VoldEvent::is_class_unsolicited(int code)
{
    return code >= 600 && code < 700;
}

static void print_vold_event(const VoldEvent &event)
{
    printf("Response: %s\n", event.log_message.c_str());
}

static void voldclient_usage(FILE *stream)
{
    fprintf(stream,
            "Usage: voldclient [VOLD ARG]...\n\n"
            "Options:\n"
            "  -h, --help       Display this help message\n");
}

int voldclient_main(int argc, char *argv[])
{
    int opt;

    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, "h", long_options, &long_index)) != -1) {
        switch (opt) {
        case 'h':
            voldclient_usage(stdout);
            return EXIT_SUCCESS;

        default:
            voldclient_usage(stderr);
            return EXIT_FAILURE;
        }
    }

    VoldEvent event;
    VoldConnection conn;

    if (!conn.connect()) {
        fprintf(stderr, "Failed to connect to vold\n");
        return EXIT_FAILURE;
    }

    if (argc - optind == 0) {
        // Enter interactive mode if there are no arguments
        printf("VoldClient interactive mode\n");
        char buf[256];

        while (true) {
            printf("$ ");
            if (!fgets(buf, sizeof(buf), stdin)) {
                return ferror(stdin) ? EXIT_FAILURE : EXIT_SUCCESS;
            }

            // Strip newline
            size_t size = strlen(buf);
            if (size > 0 && buf[size - 1] == '\n') {
                buf[size - 1] = '\0';
            }

            if (strcmp(buf, "exit") == 0) {
                break;
            }

            std::vector<std::string> args;
            if (!VoldConnection::unescape_args(buf, &args)) {
                fprintf(stderr, "Syntax error\n");
                continue;
            }

            if (!conn.execute_command(args, &event)) {
                fprintf(stderr, "Failed to execute vold command\n");
                return EXIT_FAILURE;
            }

            print_vold_event(event);
        }
    } else {
        // Non-interactive mode
        std::vector<std::string> args;
        for (int i = optind; i < argc; ++i) {
            args.push_back(argv[i]);
        }

        if (!conn.execute_command(args, &event)) {
            fprintf(stderr, "Failed to execute vold command\n");
            return EXIT_FAILURE;
        }

        print_vold_event(event);
    }

    return EXIT_SUCCESS;
}

}
