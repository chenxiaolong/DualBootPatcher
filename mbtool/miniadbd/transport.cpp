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
#include "transport.h"

#include <cstdlib>
#include <cstring>

#include "adb_log.h"
#include "adb_utils.h"

static void transport_unref(atransport *t);

static atransport transport_list = {
    .next = &transport_list,
    .prev = &transport_list,
};

static atransport pending_list = {
    .next = &pending_list,
    .prev = &pending_list,
};

pthread_mutex_t transport_lock = PTHREAD_MUTEX_INITIALIZER;

void kick_transport(atransport* t)
{
    if (t && !t->kicked) {
        int  kicked;

        pthread_mutex_lock(&transport_lock);
        kicked = t->kicked;
        if (!kicked)
            t->kicked = 1;
        pthread_mutex_unlock(&transport_lock);

        if (!kicked)
            t->kick(t);
    }
}

// Each atransport contains a list of adisconnects (t->disconnects).
// An adisconnect contains a link to the next/prev adisconnect, a function
// pointer to a disconnect callback which takes a void* piece of user data and
// the atransport, and some user data for the callback (helpfully named
// "opaque").
//
// The list is circular. New items are added to the entry member of the list
// (t->disconnects) by add_transport_disconnect.
//
// run_transport_disconnects invokes each function in the list.
//
// Gotchas:
//   * run_transport_disconnects assumes that t->disconnects is non-null, so
//     this can't be run on a zeroed atransport.
//   * The callbacks in this list are not removed when called, and this function
//     is not guarded against running more than once. As such, ensure that this
//     function is not called multiple times on the same atransport.
//     TODO(danalbert): Just fix this so that it is guarded once you have tests.
void run_transport_disconnects(atransport* t)
{
    adisconnect* dis = t->disconnects.next;

    ADB_LOGD(ADB_TSPT, "%s: run_transport_disconnects", t->serial);
    while (dis != &t->disconnects) {
        adisconnect* next = dis->next;
        dis->func(dis->opaque, t);
        dis = next;
    }
}

#if 0
static void dump_packet(const char* name, const char* func, apacket* p) {
    unsigned  command = p->msg.command;
    int       len     = p->msg.data_length;
    char      cmd[9];
    char      arg0[12], arg1[12];
    int       n;

    for (n = 0; n < 4; n++) {
        int  b = (command >> (n*8)) & 255;
        if (b < 32 || b >= 127)
            break;
        cmd[n] = (char)b;
    }
    if (n == 4) {
        cmd[4] = 0;
    } else {
        /* There is some non-ASCII name in the command, so dump
            * the hexadecimal value instead */
        snprintf(cmd, sizeof cmd, "%08x", command);
    }

    if (p->msg.arg0 < 256U)
        snprintf(arg0, sizeof arg0, "%d", p->msg.arg0);
    else
        snprintf(arg0, sizeof arg0, "0x%x", p->msg.arg0);

    if (p->msg.arg1 < 256U)
        snprintf(arg1, sizeof arg1, "%d", p->msg.arg1);
    else
        snprintf(arg1, sizeof arg1, "0x%x", p->msg.arg1);

    ADB_LOGD(ADB_TSPT, "%s: %s: [%s] arg0=%s arg1=%s (len=%d) ",
             name, func, cmd, arg0, arg1, len);
    dump_hex(p->data, len);
}
#endif

static int
read_packet(int  fd, const char* name, apacket** ppacket)
{
    char *p = (char*) ppacket;  /* really read a packet address */
    int   r;
    int   len = sizeof(*ppacket);
    char  buff[8];
    if (!name) {
        snprintf(buff, sizeof buff, "fd=%d", fd);
        name = buff;
    }
    while (len > 0) {
        r = adb_read(fd, p, len);
        if (r > 0) {
            len -= r;
            p   += r;
        } else {
            ADB_LOGE(ADB_TSPT,
                     "%s: read_packet (fd=%d), error ret=%d errno=%d: %s",
                     name, fd, r, errno, strerror(errno));
            if ((r < 0) && (errno == EINTR)) continue;
            return -1;
        }
    }

#if 0
    dump_packet(name, "from remote", *ppacket);
#endif
    return 0;
}

static int
write_packet(int fd, const char* name, apacket** ppacket)
{
    char *p = (char*) ppacket;  /* we really write the packet address */
    int r, len = sizeof(ppacket);
    char buff[8];
    if (!name) {
        snprintf(buff, sizeof buff, "fd=%d", fd);
        name = buff;
    }

#if 0
    dump_packet(name, "to remote", *ppacket);
#endif
    len = sizeof(ppacket);
    while (len > 0) {
        r = adb_write(fd, p, len);
        if (r > 0) {
            len -= r;
            p += r;
        } else {
            ADB_LOGE(ADB_TSPT,
                     "%s: write_packet (fd=%d) error ret=%d errno=%d: %s",
                     name, fd, r, errno, strerror(errno));
            if ((r < 0) && (errno == EINTR)) continue;
            return -1;
        }
    }
    return 0;
}

static void transport_socket_events(int fd, unsigned events, void *_t)
{
    atransport *t = reinterpret_cast<atransport*>(_t);
    ADB_LOGD(ADB_TSPT, "transport_socket_events(fd=%d, events=%04x,...)",
             fd, events);
    if (events & FDE_READ) {
        apacket *p = 0;
        if (read_packet(fd, t->serial, &p)) {
            ADB_LOGE(ADB_TSPT,
                     "%s: failed to read packet from transport socket on fd %d",
                     t->serial, fd);
        } else {
            handle_packet(p, (atransport *) _t);
        }
    }
}

void send_packet(apacket *p, atransport *t)
{
    unsigned char *x;
    unsigned sum;
    unsigned count;

    p->msg.magic = p->msg.command ^ 0xffffffff;

    count = p->msg.data_length;
    x = (unsigned char *) p->data;
    sum = 0;
    while (count-- > 0) {
        sum += *x++;
    }
    p->msg.data_check = sum;

    print_packet("send", p);

    if (t == NULL) {
        ADB_LOGW(ADB_TSPT, "Transport is null");
        // Zap errno because print_packet() and other stuff have errno effect.
        errno = 0;
        fatal_errno("Transport is null");
    }

    if (write_packet(t->transport_socket, t->serial, &p)) {
        fatal_errno("cannot enqueue packet on transport socket");
    }
}

/* The transport is opened by transport_register_func before
** the input and output threads are started.
**
** The output thread issues a SYNC(1, token) message to let
** the input thread know to start things up.  In the event
** of transport IO failure, the output thread will post a
** SYNC(0,0) message to ensure shutdown.
**
** The transport will not actually be closed until both
** threads exit, but the input thread will kick the transport
** on its way out to disconnect the underlying device.
*/

static void *output_thread(void *_t)
{
    atransport *t = reinterpret_cast<atransport*>(_t);
    apacket *p;

    ADB_LOGD(ADB_TSPT,
             "%s: starting transport output thread on fd %d, SYNC online (%d)",
             t->serial, t->fd, t->sync_token + 1);
    p = get_apacket();
    p->msg.command = A_SYNC;
    p->msg.arg0 = 1;
    p->msg.arg1 = ++(t->sync_token);
    p->msg.magic = A_SYNC ^ 0xffffffff;
    if (write_packet(t->fd, t->serial, &p)) {
        put_apacket(p);
        ADB_LOGE(ADB_TSPT, "%s: failed to write SYNC packet", t->serial);
        goto oops;
    }

    ADB_LOGD(ADB_TSPT, "%s: data pump started", t->serial);
    for (;;) {
        p = get_apacket();

        if (t->read_from_remote(p, t) == 0) {
            ADB_LOGD(ADB_TSPT,
                     "%s: received remote packet, sending to transport",
                     t->serial);
            if (write_packet(t->fd, t->serial, &p)) {
                put_apacket(p);
                ADB_LOGE(ADB_TSPT,
                         "%s: failed to write apacket to transport", t->serial);
                goto oops;
            }
        } else {
            ADB_LOGE(ADB_TSPT,
                     "%s: remote read failed for transport", t->serial);
            put_apacket(p);
            break;
        }
    }

    ADB_LOGD(ADB_TSPT, "%s: SYNC offline for transport", t->serial);
    p = get_apacket();
    p->msg.command = A_SYNC;
    p->msg.arg0 = 0;
    p->msg.arg1 = 0;
    p->msg.magic = A_SYNC ^ 0xffffffff;
    if (write_packet(t->fd, t->serial, &p)) {
        put_apacket(p);
        ADB_LOGW(ADB_TSPT,
                 "%s: failed to write SYNC apacket to transport", t->serial);
    }

oops:
    ADB_LOGD(ADB_TSPT, "%s: transport output thread is exiting", t->serial);
    kick_transport(t);
    transport_unref(t);
    return 0;
}

static void *input_thread(void *_t)
{
    atransport *t = reinterpret_cast<atransport*>(_t);
    apacket *p;
    int active = 0;

    ADB_LOGD(ADB_TSPT,
             "%s: starting transport input thread, reading from fd %d",
             t->serial, t->fd);

    for (;;) {
        if (read_packet(t->fd, t->serial, &p)) {
            ADB_LOGE(ADB_TSPT,
                     "%s: failed to read apacket from transport on fd %d",
                     t->serial, t->fd);
            break;
        }
        if (p->msg.command == A_SYNC) {
            if (p->msg.arg0 == 0) {
                ADB_LOGE(ADB_TSPT, "%s: transport SYNC offline", t->serial);
                put_apacket(p);
                break;
            } else {
                if (p->msg.arg1 == t->sync_token) {
                    ADB_LOGD(ADB_TSPT, "%s: transport SYNC online", t->serial);
                    active = 1;
                } else {
                    ADB_LOGD(ADB_TSPT, "%s: transport ignoring SYNC %d != %d",
                             t->serial, p->msg.arg1, t->sync_token);
                }
            }
        } else {
            if (active) {
                ADB_LOGD(ADB_TSPT,
                         "%s: transport got packet, sending to remote",
                         t->serial);
                t->write_to_remote(p, t);
            } else {
                ADB_LOGD(ADB_TSPT,
                         "%s: transport ignoring packet while offline",
                         t->serial);
            }
        }

        put_apacket(p);
    }

    // this is necessary to avoid a race condition that occured when a transport closes
    // while a client socket is still active.
    close_all_sockets(t);

    ADB_LOGD(ADB_TSPT, "%s: transport input thread is exiting, fd %d",
             t->serial, t->fd);
    kick_transport(t);
    transport_unref(t);
    return 0;
}


static int transport_registration_send = -1;
static int transport_registration_recv = -1;
static fdevent transport_registration_fde;


void update_transports() {
    // Nothing to do on the device side.
}

struct tmsg
{
    atransport *transport;
    int         action;
};

static int
transport_read_action(int  fd, struct tmsg*  m)
{
    char *p   = (char*) m;
    int   len = sizeof(*m);
    int   r;

    while (len > 0) {
        r = adb_read(fd, p, len);
        if (r > 0) {
            len -= r;
            p   += r;
        } else {
            if ((r < 0) && (errno == EINTR)) continue;
            ADB_LOGE(ADB_TSPT, "transport_read_action: on fd %d, error %d: %s",
                     fd, errno, strerror(errno));
            return -1;
        }
    }
    return 0;
}

static int
transport_write_action(int  fd, struct tmsg*  m)
{
    char *p   = (char*) m;
    int   len = sizeof(*m);
    int   r;

    while (len > 0) {
        r = adb_write(fd, p, len);
        if (r > 0) {
            len -= r;
            p   += r;
        } else {
            if ((r < 0) && (errno == EINTR)) continue;
            ADB_LOGE(ADB_TSPT, "transport_write_action: on fd %d, error %d: %s",
                     fd, errno, strerror(errno));
            return -1;
        }
    }
    return 0;
}

static void transport_registration_func(int _fd, unsigned ev, void *data)
{
    tmsg m;
    pthread_t output_thread_ptr;
    pthread_t input_thread_ptr;
    int s[2];
    atransport *t;

    if (!(ev & FDE_READ)) {
        return;
    }

    if (transport_read_action(_fd, &m)) {
        fatal_errno("cannot read transport registration socket");
    }

    t = m.transport;

    if (m.action == 0) {
        ADB_LOGD(ADB_TSPT, "transport: %s removing and free'ing %d",
                 t->serial, t->transport_socket);

            /* IMPORTANT: the remove closes one half of the
            ** socket pair.  The close closes the other half.
            */
        fdevent_remove(&(t->transport_fde));
        close(t->fd);

        pthread_mutex_lock(&transport_lock);
        t->next->prev = t->prev;
        t->prev->next = t->next;
        pthread_mutex_unlock(&transport_lock);

        run_transport_disconnects(t);

        if (t->product)
            free(t->product);
        if (t->serial)
            free(t->serial);
        if (t->model)
            free(t->model);
        if (t->device)
            free(t->device);
        if (t->devpath)
            free(t->devpath);

        memset(t,0xee,sizeof(atransport));
        free(t);

        update_transports();
        return;
    }

    /* don't create transport threads for inaccessible devices */
    if (t->connection_state != CS_NOPERM) {
        /* initial references are the two threads */
        t->ref_count = 2;

        if (adb_socketpair(s)) {
            fatal_errno("cannot open transport socketpair");
        }

        ADB_LOGD(ADB_TSPT, "transport: %s socketpair: (%d,%d) starting",
                 t->serial, s[0], s[1]);

        t->transport_socket = s[0];
        t->fd = s[1];

        fdevent_install(&(t->transport_fde),
                        t->transport_socket,
                        transport_socket_events,
                        t);

        fdevent_set(&(t->transport_fde), FDE_READ);

        if (adb_thread_create(&input_thread_ptr, input_thread, t)) {
            fatal_errno("cannot create input thread");
        }

        if (adb_thread_create(&output_thread_ptr, output_thread, t)) {
            fatal_errno("cannot create output thread");
        }
    }

    pthread_mutex_lock(&transport_lock);
    /* remove from pending list */
    t->next->prev = t->prev;
    t->prev->next = t->next;
    /* put us on the master device list */
    t->next = &transport_list;
    t->prev = transport_list.prev;
    t->next->prev = t;
    t->prev->next = t;
    pthread_mutex_unlock(&transport_lock);

    t->disconnects.next = t->disconnects.prev = &t->disconnects;

    update_transports();
}

void init_transport_registration(void)
{
    int s[2];

    if (adb_socketpair(s)) {
        fatal_errno("cannot open transport registration socketpair");
    }
    ADB_LOGD(ADB_TSPT, "socketpair: (%d,%d)", s[0], s[1]);

    transport_registration_send = s[0];
    transport_registration_recv = s[1];

    fdevent_install(&transport_registration_fde,
                    transport_registration_recv,
                    transport_registration_func,
                    0);

    fdevent_set(&transport_registration_fde, FDE_READ);
}

/* the fdevent select pump is single threaded */
static void register_transport(atransport *transport)
{
    tmsg m;
    m.transport = transport;
    m.action = 1;
    ADB_LOGD(ADB_TSPT, "transport: %s registered", transport->serial);
    if (transport_write_action(transport_registration_send, &m)) {
        fatal_errno("cannot write transport registration socket");
    }
}

static void remove_transport(atransport *transport)
{
    tmsg m;
    m.transport = transport;
    m.action = 0;
    ADB_LOGD(ADB_TSPT, "transport: %s removed", transport->serial);
    if (transport_write_action(transport_registration_send, &m)) {
        fatal_errno("cannot write transport registration socket");
    }
}


static void transport_unref_locked(atransport *t)
{
    t->ref_count--;
    if (t->ref_count == 0) {
        ADB_LOGD(ADB_TSPT, "transport: %s unref (kicking and closing)",
                 t->serial);
        if (!t->kicked) {
            t->kicked = 1;
            t->kick(t);
        }
        t->close(t);
        remove_transport(t);
    } else {
        ADB_LOGD(ADB_TSPT, "transport: %s unref (count=%d)",
                 t->serial, t->ref_count);
    }
}

static void transport_unref(atransport *t)
{
    if (t) {
        pthread_mutex_lock(&transport_lock);
        transport_unref_locked(t);
        pthread_mutex_unlock(&transport_lock);
    }
}

void add_transport_disconnect(atransport*  t, adisconnect*  dis)
{
    pthread_mutex_lock(&transport_lock);
    dis->next       = &t->disconnects;
    dis->prev       = dis->next->prev;
    dis->prev->next = dis;
    dis->next->prev = dis;
    pthread_mutex_unlock(&transport_lock);
}

void remove_transport_disconnect(atransport*  t, adisconnect*  dis)
{
    dis->prev->next = dis->next;
    dis->next->prev = dis->prev;
    dis->next = dis->prev = dis;
}

const char* atransport::connection_state_name() const {
    switch (connection_state) {
    case CS_OFFLINE: return "offline";
    case CS_DEVICE: return "device";
    case CS_HOST: return "host";
    case CS_NOPERM: return "no permissions";
    default: return "unknown";
    }
}

void register_usb_transport(usb_handle *usb, const char *serial, const char *devpath, unsigned writeable)
{
    atransport *t = reinterpret_cast<atransport*>(calloc(1, sizeof(atransport)));
    if (t == nullptr) fatal("cannot allocate USB atransport");
    ADB_LOGD(ADB_TSPT, "transport: %p init'ing for usb_handle %p (sn='%s')",
             t, usb, serial ? serial : "");
    init_usb_transport(t, usb, (writeable ? CS_OFFLINE : CS_NOPERM));
    if (serial) {
        t->serial = strdup(serial);
    }
    if (devpath) {
        t->devpath = strdup(devpath);
    }

    pthread_mutex_lock(&transport_lock);
    t->next = &pending_list;
    t->prev = pending_list.prev;
    t->next->prev = t;
    t->prev->next = t;
    pthread_mutex_unlock(&transport_lock);

    register_transport(t);
}

/* this should only be used for transports with connection_state == CS_NOPERM */
void unregister_usb_transport(usb_handle *usb)
{
    atransport *t;
    pthread_mutex_lock(&transport_lock);
    for (t = transport_list.next; t != &transport_list; t = t->next) {
        if (t->usb == usb && t->connection_state == CS_NOPERM) {
            t->next->prev = t->prev;
            t->prev->next = t->next;
            break;
        }
    }
    pthread_mutex_unlock(&transport_lock);
}

int check_header(apacket *p)
{
    if (p->msg.magic != (p->msg.command ^ 0xffffffff)) {
        ADB_LOGE(ADB_TSPT, "check_header(): invalid magic");
        return -1;
    }

    if (p->msg.data_length > MAX_PAYLOAD) {
        ADB_LOGE(ADB_TSPT, "check_header(): %d > MAX_PAYLOAD",
                 p->msg.data_length);
        return -1;
    }

    return 0;
}

int check_data(apacket *p)
{
    unsigned count, sum;
    unsigned char *x;

    count = p->msg.data_length;
    x = p->data;
    sum = 0;
    while (count-- > 0) {
        sum += *x++;
    }

    if (sum != p->msg.data_check) {
        return -1;
    } else {
        return 0;
    }
}
