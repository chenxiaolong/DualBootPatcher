/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "sysdeps.h"
#include "adb.h"

#include <cstdlib>
#include <cstring>

#include <fcntl.h>
#include <unistd.h>

#include "adb_log.h"
#include "transport.h"

#include "mbcommon/string.h"
#include "mbutil/properties.h"
#include "mbutil/string.h"

pthread_mutex_t D_lock = PTHREAD_MUTEX_INITIALIZER;

const char *adb_device_banner = "device";

void fatal(const char *fmt, ...)
{
    std::string new_fmt = mb::format("error: %s", fmt);

    va_list ap;
    va_start(ap, fmt);
    ADB_VLOGE(ADB_CONN, new_fmt.c_str(), ap);
    va_end(ap);

    exit(-1);
}

void fatal_errno(const char *fmt, ...)
{
    std::string new_fmt = mb::format("error: %s: %s", strerror(errno), fmt);

    va_list ap;
    va_start(ap, fmt);
    ADB_VLOGE(ADB_CONN, new_fmt.c_str(), ap);
    va_end(ap);

    exit(-1);
}

apacket* get_apacket(void)
{
    apacket* p = reinterpret_cast<apacket*>(malloc(sizeof(apacket)));
    if (p == nullptr) {
      fatal("failed to allocate an apacket");
    }

    memset(p, 0, sizeof(apacket) - MAX_PAYLOAD);
    return p;
}

void put_apacket(apacket *p)
{
    free(p);
}

void handle_online(atransport *t)
{
    ADB_LOGD(ADB_CONN, "adb: online");
    t->online = 1;
}

void handle_offline(atransport *t)
{
    ADB_LOGD(ADB_CONN, "adb: offline");
    //Close the associated usb
    t->online = 0;
    run_transport_disconnects(t);
}

#if DEBUG_PACKETS
#define DUMPMAX 32
void print_packet(const char *label, apacket *p)
{
    const char *tag;
    char *x;
    unsigned count;

    switch (p->msg.command) {
    case A_SYNC: tag = "SYNC"; break;
    case A_CNXN: tag = "CNXN" ; break;
    case A_OPEN: tag = "OPEN"; break;
    case A_OKAY: tag = "OKAY"; break;
    case A_CLSE: tag = "CLSE"; break;
    case A_WRTE: tag = "WRTE"; break;
    default: tag = "????"; break;
    }

    ADB_LOGD(ADB_CONN, "%s: %s %08x %08x %04x",
             label, tag, p->msg.arg0, p->msg.arg1, p->msg.data_length);
    count = p->msg.data_length;
    x = (char*) p->data;

    if (count > DUMPMAX) {
        count = DUMPMAX;
    }

    static const int line_length = 80;

    std::string line;
    line.reserve(line_length);

    while (count > 0) {
        if (line.empty()) {
            line += "- ";
        } else if (line.size() == line_length) {
            ADB_LOGD(ADB_CONN, "%s", line.c_str());
            line.clear();
            continue;
        }
        if (isprint(*x)) {
            line += *x;
        } else {
            line += '.';
        }

        --count;
        ++x;
    }
    if (!line.empty()) {
        ADB_LOGD(ADB_CONN, "%s", line.c_str());
    }

    if (p->msg.data_length > DUMPMAX) {
        ADB_LOGD(ADB_CONN, "- ...");
    }
}
#endif

static void send_ready(unsigned local, unsigned remote, atransport *t)
{
    ADB_LOGD(ADB_CONN, "Calling send_ready");
    apacket *p = get_apacket();
    p->msg.command = A_OKAY;
    p->msg.arg0 = local;
    p->msg.arg1 = remote;
    send_packet(p, t);
}

static void send_close(unsigned local, unsigned remote, atransport *t)
{
    ADB_LOGD(ADB_CONN, "Calling send_close");
    apacket *p = get_apacket();
    p->msg.command = A_CLSE;
    p->msg.arg0 = local;
    p->msg.arg1 = remote;
    send_packet(p, t);
}

static size_t fill_connect_data(char *buf, size_t bufsize)
{
    size_t len;
    len = snprintf(buf, bufsize, "%s::", adb_device_banner);
    return len + 1;
}

void send_connect(atransport *t)
{
    ADB_LOGD(ADB_CONN, "Calling send_connect");
    apacket *cp = get_apacket();
    cp->msg.command = A_CNXN;
    cp->msg.arg0 = A_VERSION;
    cp->msg.arg1 = MAX_PAYLOAD;
    cp->msg.data_length = fill_connect_data((char *)cp->data,
                                            sizeof(cp->data));
    send_packet(cp, t);
}

// qual_overwrite is used to overwrite a qualifier string.  dst is a
// pointer to a char pointer.  It is assumed that if *dst is non-NULL, it
// was malloc'ed and needs to freed.  *dst will be set to a dup of src.
// TODO: switch to std::string for these atransport fields instead.
static void qual_overwrite(char** dst, const std::string& src) {
    free(*dst);
    *dst = strdup(src.c_str());
}

void parse_banner(const char* banner, atransport* t) {
    ADB_LOGD(ADB_CONN, "parse_banner: %s", banner);

    // The format is something like:
    // "device::ro.product.name=x;ro.product.model=y;ro.product.device=z;".
    std::vector<std::string> pieces = mb::split(banner, ':');

    if (pieces.size() > 2) {
        const std::string& props = pieces[2];
        for (auto& prop : mb::split(props, ';')) {
            // The list of properties was traditionally ;-terminated rather than ;-separated.
            if (prop.empty()) continue;

            std::vector<std::string> key_value = mb::split(prop, '=');
            if (key_value.size() != 2) continue;

            const std::string& key = key_value[0];
            const std::string& value = key_value[1];
            if (key == "ro.product.name") {
                qual_overwrite(&t->product, value);
            } else if (key == "ro.product.model") {
                qual_overwrite(&t->model, value);
            } else if (key == "ro.product.device") {
                qual_overwrite(&t->device, value);
            }
        }
    }

    const std::string& type = pieces[0];
    if (type == "device") {
        ADB_LOGD(ADB_CONN, "setting connection_state to CS_DEVICE");
        t->connection_state = CS_DEVICE;
        update_transports();
    } else {
        ADB_LOGD(ADB_CONN, "setting connection_state to CS_HOST");
        t->connection_state = CS_HOST;
    }
}

void handle_packet(apacket *p, atransport *t)
{
    asocket *s;

    ADB_LOGD(ADB_CONN, "handle_packet() %c%c%c%c",
             ((char*) (&(p->msg.command)))[0],
             ((char*) (&(p->msg.command)))[1],
             ((char*) (&(p->msg.command)))[2],
             ((char*) (&(p->msg.command)))[3]);
    print_packet("recv", p);

    switch (p->msg.command) {
    case A_SYNC:
        if (p->msg.arg0) {
            send_packet(p, t);
        } else {
            t->connection_state = CS_OFFLINE;
            handle_offline(t);
            send_packet(p, t);
        }
        return;

    case A_CNXN: /* CONNECT(version, maxdata, "system-id-string") */
            /* XXX verify version, etc */
        if (t->connection_state != CS_OFFLINE) {
            t->connection_state = CS_OFFLINE;
            handle_offline(t);
        }

        parse_banner(reinterpret_cast<const char*>(p->data), t);

        handle_online(t);
        send_connect(t);
        break;

    case A_OPEN: /* OPEN(local-id, 0, "destination") */
        if (t->online && p->msg.arg0 != 0 && p->msg.arg1 == 0) {
            char *name = (char*) p->data;
            name[p->msg.data_length > 0 ? p->msg.data_length - 1 : 0] = 0;
            s = create_local_service_socket(name);
            if (s == 0) {
                send_close(0, p->msg.arg0, t);
            } else {
                s->peer = create_remote_socket(p->msg.arg0, t);
                s->peer->peer = s;
                send_ready(s->id, s->peer->id, t);
                s->ready(s);
            }
        }
        break;

    case A_OKAY: /* READY(local-id, remote-id, "") */
        if (t->online && p->msg.arg0 != 0 && p->msg.arg1 != 0) {
            if ((s = find_local_socket(p->msg.arg1, 0))) {
                if (s->peer == 0) {
                    /* On first READY message, create the connection. */
                    s->peer = create_remote_socket(p->msg.arg0, t);
                    s->peer->peer = s;
                    s->ready(s);
                } else if (s->peer->id == p->msg.arg0) {
                    /* Other READY messages must use the same local-id */
                    s->ready(s);
                } else {
                    ADB_LOGW(ADB_CONN,
                             "Invalid A_OKAY(%d,%d), expected A_OKAY(%d,%d) on transport %s",
                             p->msg.arg0, p->msg.arg1, s->peer->id, p->msg.arg1, t->serial);
                }
            }
        }
        break;

    case A_CLSE: /* CLOSE(local-id, remote-id, "") or CLOSE(0, remote-id, "") */
        if (t->online && p->msg.arg1 != 0) {
            if ((s = find_local_socket(p->msg.arg1, p->msg.arg0))) {
                /* According to protocol.txt, p->msg.arg0 might be 0 to indicate
                 * a failed OPEN only. However, due to a bug in previous ADB
                 * versions, CLOSE(0, remote-id, "") was also used for normal
                 * CLOSE() operations.
                 *
                 * This is bad because it means a compromised adbd could
                 * send packets to close connections between the host and
                 * other devices. To avoid this, only allow this if the local
                 * socket has a peer on the same transport.
                 */
                if (p->msg.arg0 == 0 && s->peer && s->peer->transport != t) {
                    ADB_LOGW(ADB_CONN,
                             "Invalid A_CLSE(0, %u) from transport %s, expected transport %s",
                             p->msg.arg1, t->serial, s->peer->transport->serial);
                } else {
                    s->close(s);
                }
            }
        }
        break;

    case A_WRTE: /* WRITE(local-id, remote-id, <data>) */
        if (t->online && p->msg.arg0 != 0 && p->msg.arg1 != 0) {
            if ((s = find_local_socket(p->msg.arg1, p->msg.arg0))) {
                unsigned rid = p->msg.arg0;
                p->len = p->msg.data_length;

                if (s->enqueue(s, p) == 0) {
                    ADB_LOGD(ADB_CONN, "Enqueue the socket");
                    send_ready(s->id, rid, t);
                }
                return;
            }
        }
        break;

    default:
        ADB_LOGV(ADB_CONN, "handle_packet: what is %08x?!", p->msg.command);
    }

    put_apacket(p);
}
