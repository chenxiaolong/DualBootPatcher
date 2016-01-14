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

#include "adb_log.h"

static int remote_read(apacket *p, atransport *t)
{
    if (usb_read(t->usb, &p->msg, sizeof(amessage))) {
        ADB_LOGE(ADB_TSPT, "remote usb: read terminated (message)");
        return -1;
    }

    if (check_header(p)) {
        ADB_LOGE(ADB_TSPT, "remote usb: check_header failed");
        return -1;
    }

    if (p->msg.data_length) {
        if (usb_read(t->usb, p->data, p->msg.data_length)) {
            ADB_LOGE(ADB_TSPT, "remote usb: terminated (data)");
            return -1;
        }
    }

    if (check_data(p)) {
        ADB_LOGE(ADB_TSPT, "remote usb: check_data failed");
        return -1;
    }

    return 0;
}

static int remote_write(apacket *p, atransport *t)
{
    unsigned size = p->msg.data_length;

    if (usb_write(t->usb, &p->msg, sizeof(amessage))) {
        ADB_LOGE(ADB_TSPT, "remote usb: 1 - write terminated");
        return -1;
    }
    if (p->msg.data_length == 0) return 0;
    if (usb_write(t->usb, &p->data, size)) {
        ADB_LOGE(ADB_TSPT, "remote usb: 2 - write terminated");
        return -1;
    }

    return 0;
}

static void remote_close(atransport *t)
{
    usb_close(t->usb);
    t->usb = 0;
}

static void remote_kick(atransport *t)
{
    usb_kick(t->usb);
}

void init_usb_transport(atransport *t, usb_handle *h, int state)
{
    ADB_LOGD(ADB_TSPT, "transport: usb");
    t->close = remote_close;
    t->kick = remote_kick;
    t->read_from_remote = remote_read;
    t->write_to_remote = remote_write;
    t->sync_token = 1;
    t->connection_state = state;
    t->type = kTransportUsb;
    t->usb = h;
}
