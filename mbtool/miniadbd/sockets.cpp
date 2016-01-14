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

#include <cstdlib>
#include <cstring>

#include "adb.h"
#include "adb_log.h"
#include "transport.h"

pthread_mutex_t socket_list_lock = PTHREAD_MUTEX_INITIALIZER;

static void local_socket_close_locked(asocket *s);

static unsigned local_socket_next_id = 1;

static asocket local_socket_list = {
    .next = &local_socket_list,
    .prev = &local_socket_list,
};

/* the the list of currently closing local sockets.
** these have no peer anymore, but still packets to
** write to their fd.
*/
static asocket local_socket_closing_list = {
    .next = &local_socket_closing_list,
    .prev = &local_socket_closing_list,
};

// Parse the global list of sockets to find one with id |local_id|.
// If |peer_id| is not 0, also check that it is connected to a peer
// with id |peer_id|. Returns an asocket handle on success, NULL on failure.
asocket *find_local_socket(unsigned local_id, unsigned peer_id)
{
    asocket *s;
    asocket *result = NULL;

    pthread_mutex_lock(&socket_list_lock);
    for (s = local_socket_list.next; s != &local_socket_list; s = s->next) {
        if (s->id != local_id)
            continue;
        if (peer_id == 0 || (s->peer && s->peer->id == peer_id)) {
            result = s;
        }
        break;
    }
    pthread_mutex_unlock(&socket_list_lock);

    return result;
}

static void
insert_local_socket(asocket*  s, asocket*  list)
{
    s->next       = list;
    s->prev       = s->next->prev;
    s->prev->next = s;
    s->next->prev = s;
}


void install_local_socket(asocket *s)
{
    pthread_mutex_lock(&socket_list_lock);

    s->id = local_socket_next_id++;

    // Socket ids should never be 0.
    if (local_socket_next_id == 0)
        local_socket_next_id = 1;

    insert_local_socket(s, &local_socket_list);

    pthread_mutex_unlock(&socket_list_lock);
}

void remove_socket(asocket *s)
{
    // socket_list_lock should already be held
    if (s->prev && s->next) {
        s->prev->next = s->next;
        s->next->prev = s->prev;
        s->next = 0;
        s->prev = 0;
        s->id = 0;
    }
}

void close_all_sockets(atransport *t)
{
    asocket *s;

        /* this is a little gross, but since s->close() *will* modify
        ** the list out from under you, your options are limited.
        */
    pthread_mutex_lock(&socket_list_lock);
restart:
    for (s = local_socket_list.next; s != &local_socket_list; s = s->next) {
        if (s->transport == t || (s->peer && s->peer->transport == t)) {
            local_socket_close_locked(s);
            goto restart;
        }
    }
    pthread_mutex_unlock(&socket_list_lock);
}

static int local_socket_enqueue(asocket *s, apacket *p)
{
    ADB_LOGD(ADB_SOCK, "LS(%d): enqueue %d", s->id, p->len);

    p->ptr = p->data;

    /* if there is already data queue'd, we will receive
    ** events when it's time to write.  just add this to
    ** the tail
    */
    if (s->pkt_first) {
        goto enqueue;
    }

    /* write as much as we can, until we
    ** would block or there is an error/eof
    */
    while (p->len > 0) {
        int r = adb_write(s->fd, p->ptr, p->len);
        if (r > 0) {
            p->len -= r;
            p->ptr += r;
            continue;
        }
        if ((r == 0) || (errno != EAGAIN)) {
            ADB_LOGE(ADB_SOCK, "LS(%d): not ready, errno=%d: %s",
                     s->id, errno, strerror(errno));
            s->close(s);
            return 1; /* not ready (error) */
        } else {
            break;
        }
    }

    if (p->len == 0) {
        put_apacket(p);
        return 0; /* ready for more data */
    }

enqueue:
    p->next = 0;
    if (s->pkt_first) {
        s->pkt_last->next = p;
    } else {
        s->pkt_first = p;
    }
    s->pkt_last = p;

    /* make sure we are notified when we can drain the queue */
    fdevent_add(&s->fde, FDE_WRITE);

    return 1; /* not ready (backlog) */
}

static void local_socket_ready(asocket *s)
{
    /* far side is ready for data, pay attention to
       readable events */
    fdevent_add(&s->fde, FDE_READ);
}

static void local_socket_close(asocket *s)
{
    pthread_mutex_lock(&socket_list_lock);
    local_socket_close_locked(s);
    pthread_mutex_unlock(&socket_list_lock);
}

// be sure to hold the socket list lock when calling this
static void local_socket_destroy(asocket  *s)
{
    apacket *p, *n;
    int exit_on_close = s->exit_on_close;

    ADB_LOGD(ADB_SOCK, "LS(%d): destroying fde.fd=%d", s->id, s->fde.fd);

    /* IMPORTANT: the remove closes the fd
     * that belongs to this socket
     */
    fdevent_remove(&s->fde);

    /* dispose of any unwritten data */
    for (p = s->pkt_first; p; p = n) {
        ADB_LOGD(ADB_SOCK, "LS(%d): discarding %d bytes", s->id, p->len);
        n = p->next;
        put_apacket(p);
    }
    remove_socket(s);
    free(s);

    if (exit_on_close) {
        ADB_LOGD(ADB_SOCK, "local_socket_destroy: exiting");
        exit(1);
    }
}


static void local_socket_close_locked(asocket *s)
{
    ADB_LOGD(ADB_SOCK, "entered local_socket_close_locked. LS(%d) fd=%d",
             s->id, s->fd);
    if (s->peer) {
        ADB_LOGD(ADB_SOCK, "LS(%d): closing peer. peer->id=%d peer->fd=%d",
                 s->id, s->peer->id, s->peer->fd);
        /* Note: it's important to call shutdown before disconnecting from
         * the peer, this ensures that remote sockets can still get the id
         * of the local socket they're connected to, to send a CLOSE()
         * protocol event. */
        if (s->peer->shutdown)
            s->peer->shutdown(s->peer);
        s->peer->peer = 0;
        // tweak to avoid deadlock
        if (s->peer->close == local_socket_close) {
            local_socket_close_locked(s->peer);
        } else {
            s->peer->close(s->peer);
        }
        s->peer = 0;
    }

        /* If we are already closing, or if there are no
        ** pending packets, destroy immediately
        */
    if (s->closing || s->pkt_first == NULL) {
        int   id = s->id;
        local_socket_destroy(s);
        ADB_LOGD(ADB_SOCK, "LS(%d): closed", id);
        return;
    }

        /* otherwise, put on the closing list
        */
    ADB_LOGD(ADB_SOCK, "LS(%d): closing", s->id);
    s->closing = 1;
    fdevent_del(&s->fde, FDE_READ);
    remove_socket(s);
    ADB_LOGD(ADB_SOCK, "LS(%d): put on socket_closing_list fd=%d",
             s->id, s->fd);
    insert_local_socket(s, &local_socket_closing_list);
}

static void local_socket_event_func(int fd, unsigned ev, void* _s)
{
    asocket* s = reinterpret_cast<asocket*>(_s);
    ADB_LOGD(ADB_SOCK, "LS(%d): event_func(fd=%d(==%d), ev=%04x)",
             s->id, s->fd, fd, ev);

    /* put the FDE_WRITE processing before the FDE_READ
    ** in order to simplify the code.
    */
    if (ev & FDE_WRITE) {
        apacket* p;
        while ((p = s->pkt_first) != nullptr) {
            while (p->len > 0) {
                int r = adb_write(fd, p->ptr, p->len);
                if (r == -1) {
                    /* returning here is ok because FDE_READ will
                    ** be processed in the next iteration loop
                    */
                    if (errno == EAGAIN) {
                        return;
                    }
                } else if (r > 0) {
                    p->ptr += r;
                    p->len -= r;
                    continue;
                }

                ADB_LOGE(ADB_SOCK,
                         " closing after write because r=%d and errno is %d",
                         r, errno);
                s->close(s);
                return;
            }

            if (p->len == 0) {
                s->pkt_first = p->next;
                if (s->pkt_first == 0) {
                    s->pkt_last = 0;
                }
                put_apacket(p);
            }
        }

        /* if we sent the last packet of a closing socket,
        ** we can now destroy it.
        */
        if (s->closing) {
            ADB_LOGD(ADB_SOCK, " closing because 'closing' is set after write");
            s->close(s);
            return;
        }

        /* no more packets queued, so we can ignore
        ** writable events again and tell our peer
        ** to resume writing
        */
        fdevent_del(&s->fde, FDE_WRITE);
        s->peer->ready(s->peer);
    }


    if (ev & FDE_READ) {
        apacket *p = get_apacket();
        unsigned char *x = p->data;
        size_t avail = MAX_PAYLOAD;
        int r;
        int is_eof = 0;

        while (avail > 0) {
            r = adb_read(fd, x, avail);
            ADB_LOGE(ADB_SOCK,
                     "LS(%d): post adb_read(fd=%d,...) r=%d (errno=%d) avail=%zu",
                     s->id, s->fd, r, r < 0 ? errno : 0, avail);
            if (r == -1) {
                if (errno == EAGAIN) {
                    break;
                }
            } else if (r > 0) {
                avail -= r;
                x += r;
                continue;
            }

            /* r = 0 or unhandled error */
            is_eof = 1;
            break;
        }
        ADB_LOGD(ADB_SOCK,
                 "LS(%d): fd=%d post avail loop. r=%d is_eof=%d forced_eof=%d",
                 s->id, s->fd, r, is_eof, s->fde.force_eof);
        if ((avail == MAX_PAYLOAD) || (s->peer == 0)) {
            put_apacket(p);
        } else {
            p->len = MAX_PAYLOAD - avail;

            r = s->peer->enqueue(s->peer, p);
            ADB_LOGD(ADB_SOCK, "LS(%d): fd=%d post peer->enqueue(). r=%d",
                     s->id, s->fd, r);

            if (r < 0) {
                    /* error return means they closed us as a side-effect
                    ** and we must return immediately.
                    **
                    ** note that if we still have buffered packets, the
                    ** socket will be placed on the closing socket list.
                    ** this handler function will be called again
                    ** to process FDE_WRITE events.
                    */
                return;
            }

            if (r > 0) {
                    /* if the remote cannot accept further events,
                    ** we disable notification of READs.  They'll
                    ** be enabled again when we get a call to ready()
                    */
                fdevent_del(&s->fde, FDE_READ);
            }
        }
        /* Don't allow a forced eof if data is still there */
        if ((s->fde.force_eof && !r) || is_eof) {
            ADB_LOGD(ADB_SOCK,
                     " closing because is_eof=%d r=%d s->fde.force_eof=%d",
                     is_eof, r, s->fde.force_eof);
            s->close(s);
        }
    }

    if (ev & FDE_ERROR) {
            /* this should be caught be the next read or write
            ** catching it here means we may skip the last few
            ** bytes of readable data.
            */
        ADB_LOGE(ADB_SOCK, "LS(%d): FDE_ERROR (fd=%d)", s->id, s->fd);

        return;
    }
}

asocket *create_local_socket(int fd)
{
    asocket *s = reinterpret_cast<asocket*>(calloc(1, sizeof(asocket)));
    if (s == NULL) fatal("cannot allocate socket");
    s->fd = fd;
    s->enqueue = local_socket_enqueue;
    s->ready = local_socket_ready;
    s->shutdown = NULL;
    s->close = local_socket_close;
    install_local_socket(s);

    fdevent_install(&s->fde, fd, local_socket_event_func, s);
    ADB_LOGD(ADB_SOCK, "LS(%d): created (fd=%d)", s->id, s->fd);
    return s;
}

asocket *create_local_service_socket(const char *name)
{
    int fd = service_to_fd(name);
    if (fd < 0) return 0;

    asocket* s = create_local_socket(fd);
    ADB_LOGD(ADB_SOCK, "LS(%d): bound to '%s' via %d", s->id, name, fd);

    if (!strncmp(name, "usb:", 4)) {
        ADB_LOGD(ADB_SOCK, "LS(%d): enabling exit_on_close", s->id);
        s->exit_on_close = 1;
    }

    return s;
}

/* a Remote socket is used to send/receive data to/from a given transport object
** it needs to be closed when the transport is forcibly destroyed by the user
*/
struct aremotesocket {
    asocket      socket;
    adisconnect  disconnect;
};

static int remote_socket_enqueue(asocket *s, apacket *p)
{
    ADB_LOGD(ADB_SOCK,
             "entered remote_socket_enqueue RS(%d) WRITE fd=%d peer.fd=%d",
             s->id, s->fd, s->peer->fd);
    p->msg.command = A_WRTE;
    p->msg.arg0 = s->peer->id;
    p->msg.arg1 = s->id;
    p->msg.data_length = p->len;
    send_packet(p, s->transport);
    return 1;
}

static void remote_socket_ready(asocket *s)
{
    ADB_LOGD(ADB_SOCK,
             "entered remote_socket_ready RS(%d) OKAY fd=%d peer.fd=%d",
             s->id, s->fd, s->peer->fd);
    apacket *p = get_apacket();
    p->msg.command = A_OKAY;
    p->msg.arg0 = s->peer->id;
    p->msg.arg1 = s->id;
    send_packet(p, s->transport);
}

static void remote_socket_shutdown(asocket *s)
{
    ADB_LOGD(ADB_SOCK,
             "entered remote_socket_shutdown RS(%d) CLOSE fd=%d peer->fd=%d",
             s->id, s->fd, s->peer?s->peer->fd:-1);
    apacket *p = get_apacket();
    p->msg.command = A_CLSE;
    if (s->peer) {
        p->msg.arg0 = s->peer->id;
    }
    p->msg.arg1 = s->id;
    send_packet(p, s->transport);
}

static void remote_socket_close(asocket *s)
{
    if (s->peer) {
        s->peer->peer = 0;
        ADB_LOGD(ADB_SOCK,
                 "RS(%d) peer->close()ing peer->id=%d peer->fd=%d",
                 s->id, s->peer->id, s->peer->fd);
        s->peer->close(s->peer);
    }
    ADB_LOGD(ADB_SOCK,
             "entered remote_socket_close RS(%d) CLOSE fd=%d peer->fd=%d",
             s->id, s->fd, s->peer?s->peer->fd:-1);
    ADB_LOGD(ADB_SOCK, "RS(%d): closed", s->id);
    remove_transport_disconnect( s->transport, &((aremotesocket*)s)->disconnect );
    free(s);
}

static void remote_socket_disconnect(void*  _s, atransport*  t)
{
    asocket* s = reinterpret_cast<asocket*>(_s);
    asocket* peer = s->peer;

    ADB_LOGD(ADB_SOCK, "remote_socket_disconnect RS(%d)", s->id);
    if (peer) {
        peer->peer = NULL;
        peer->close(peer);
    }
    remove_transport_disconnect( s->transport, &((aremotesocket*)s)->disconnect );
    free(s);
}

/* Create an asocket to exchange packets with a remote service through transport
  |t|. Where |id| is the socket id of the corresponding service on the other
   side of the transport (it is allocated by the remote side and _cannot_ be 0).
   Returns a new non-NULL asocket handle. */
asocket *create_remote_socket(unsigned id, atransport *t)
{
    if (id == 0) fatal("invalid remote socket id (0)");
    asocket* s = reinterpret_cast<asocket*>(calloc(1, sizeof(aremotesocket)));
    adisconnect* dis = &reinterpret_cast<aremotesocket*>(s)->disconnect;

    if (s == NULL) fatal("cannot allocate socket");
    s->id = id;
    s->enqueue = remote_socket_enqueue;
    s->ready = remote_socket_ready;
    s->shutdown = remote_socket_shutdown;
    s->close = remote_socket_close;
    s->transport = t;

    dis->func   = remote_socket_disconnect;
    dis->opaque = s;
    add_transport_disconnect( t, dis );
    ADB_LOGD(ADB_SOCK, "RS(%d): created", s->id);
    return s;
}
