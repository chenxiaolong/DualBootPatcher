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
#include "adb_io.h"
#include "adb_log.h"

#include "file_sync_service.h"

// mbtool
#include "reboot.h"

struct stinfo {
    void (*func)(int fd, void *cookie);
    int fd;
    void *cookie;
};


void *service_bootstrap_func(void *x)
{
    stinfo* sti = reinterpret_cast<stinfo*>(x);
    sti->func(sti->fd, sti->cookie);
    free(sti);
    return 0;
}

static bool reboot_service_impl(int fd, const char* arg) {
    // Android init won't be running so we can't use mb::reboot_via_init()
    return mb::reboot_directly(arg);
}

void reboot_service(int fd, void* arg)
{
    if (reboot_service_impl(fd, static_cast<const char*>(arg))) {
        // Don't return early. Give the reboot command time to take effect
        // to avoid messing up scripts which do "adb reboot && adb wait-for-device"
        while (true) {
            pause();
        }
    }

    free(arg);
    close(fd);
}

static int create_service_thread(void (*func)(int, void *), void *cookie)
{
    int s[2];
    if (adb_socketpair(s)) {
        ADB_LOGE(ADB_SERV, "cannot create service socket pair");
        return -1;
    }
    ADB_LOGD(ADB_SERV, "socketpair: (%d,%d)", s[0], s[1]);

    stinfo* sti = reinterpret_cast<stinfo*>(malloc(sizeof(stinfo)));
    if (sti == nullptr) {
        fatal("cannot allocate stinfo");
    }
    sti->func = func;
    sti->cookie = cookie;
    sti->fd = s[1];

    pthread_t t;
    if (adb_thread_create(&t, service_bootstrap_func, sti)) {
        free(sti);
        close(s[0]);
        close(s[1]);
        ADB_LOGE(ADB_SERV, "cannot create service thread");
        return -1;
    }

    ADB_LOGD(ADB_SERV, "service thread started, %d:%d", s[0], s[1]);
    return s[0];
}

static void init_subproc_child()
{
    setsid();

    // Set OOM score adjustment to prevent killing
    int fd = adb_open("/proc/self/oom_score_adj", O_WRONLY | O_CLOEXEC);
    if (fd >= 0) {
        adb_write(fd, "0", 1);
        close(fd);
    } else {
       ADB_LOGW(ADB_SERV, "adb: unable to update oom_score_adj");
    }
}

static int create_subproc_pty(const char *cmd, const char *arg0, const char *arg1, pid_t *pid)
{
    ADB_LOGD(ADB_SERV, "create_subproc_pty(cmd=%s, arg0=%s, arg1=%s)",
             cmd, arg0, arg1);
    int ptm;

    ptm = unix_open("/dev/ptmx", O_RDWR | O_CLOEXEC); // | O_NOCTTY);
    if (ptm < 0) {
        ADB_LOGE(ADB_SERV, "[ cannot open /dev/ptmx - %s ]", strerror(errno));
        return -1;
    }

    char devname[64];
    if (grantpt(ptm) || unlockpt(ptm)
            || ptsname_r(ptm, devname, sizeof(devname)) != 0) {
        ADB_LOGE(ADB_SERV, "[ trouble with /dev/ptmx - %s ]", strerror(errno));
        close(ptm);
        return -1;
    }

    *pid = fork();
    if (*pid < 0) {
        ADB_LOGE(ADB_SERV, "- fork failed: %s -", strerror(errno));
        close(ptm);
        return -1;
    }

    if (*pid == 0) {
        init_subproc_child();

        int pts = unix_open(devname, O_RDWR | O_CLOEXEC);
        if (pts < 0) {
            ADB_LOGE(ADB_SERV, "child failed to open pseudo-term slave: %s",
                     devname);
            exit(-1);
        }

        dup2(pts, STDIN_FILENO);
        dup2(pts, STDOUT_FILENO);
        dup2(pts, STDERR_FILENO);

        close(pts);
        close(ptm);

        execl(cmd, cmd, arg0, arg1, NULL);
        ADB_LOGE(ADB_SERV, "- exec '%s' failed: %s (%d) -",
                 cmd, strerror(errno), errno);
        exit(-1);
    } else {
        return ptm;
    }
}

static int create_subproc_raw(const char *cmd, const char *arg0, const char *arg1, pid_t *pid)
{
    ADB_LOGD(ADB_SERV, "create_subproc_raw(cmd=%s, arg0=%s, arg1=%s)",
             cmd, arg0, arg1);

    // 0 is parent socket, 1 is child socket
    int sv[2];
    if (adb_socketpair(sv) < 0) {
        ADB_LOGE(ADB_SERV, "[ cannot create socket pair - %s ]",
                 strerror(errno));
        return -1;
    }
    ADB_LOGD(ADB_SERV, "socketpair: (%d,%d)", sv[0], sv[1]);

    *pid = fork();
    if (*pid < 0) {
        ADB_LOGE(ADB_SERV, "- fork failed: %s -", strerror(errno));
        close(sv[0]);
        close(sv[1]);
        return -1;
    }

    if (*pid == 0) {
        close(sv[0]);
        init_subproc_child();

        dup2(sv[1], STDIN_FILENO);
        dup2(sv[1], STDOUT_FILENO);
        dup2(sv[1], STDERR_FILENO);

        close(sv[1]);

        execl(cmd, cmd, arg0, arg1, NULL);
        ADB_LOGE(ADB_SERV, "- exec '%s' failed: %s (%d) -",
                 cmd, strerror(errno), errno);
        exit(-1);
    } else {
        close(sv[1]);
        return sv[0];
    }
}

#define SHELL_COMMAND "/system/bin/sh"
#define ALTERNATE_SHELL_COMMAND "/sbin/sh"

static void subproc_waiter_service(int fd, void *cookie)
{
    pid_t pid = (pid_t) (uintptr_t) cookie;

    ADB_LOGD(ADB_SERV, "entered. fd=%d of pid=%d", fd, pid);
    while (true) {
        int status;
        pid_t p = waitpid(pid, &status, 0);
        if (p == pid) {
            ADB_LOGD(ADB_SERV, "fd=%d, post waitpid(pid=%d) status=%04x",
                     fd, p, status);
            if (WIFSIGNALED(status)) {
                ADB_LOGD(ADB_SERV, "*** Killed by signal %d", WTERMSIG(status));
                break;
            } else if (!WIFEXITED(status)) {
                ADB_LOGD(ADB_SERV, "*** Didn't exit!!. status %d", status);
                break;
            } else if (WEXITSTATUS(status) >= 0) {
                ADB_LOGD(ADB_SERV, "*** Exit code %d", WEXITSTATUS(status));
                break;
            }
         }
    }
    ADB_LOGD(ADB_SERV, "shell exited fd=%d of pid=%d err=%d", fd, pid, errno);
    if (SHELL_EXIT_NOTIFY_FD >=0) {
      int res;
      res = WriteFdExactly(SHELL_EXIT_NOTIFY_FD, &fd, sizeof(fd)) ? 0 : -1;
      ADB_LOGD(ADB_SERV,
               "notified shell exit via fd=%d for pid=%d res=%d errno=%d",
               SHELL_EXIT_NOTIFY_FD, pid, res, errno);
    }
}

static int create_subproc_thread(const char *name, const subproc_mode mode)
{
    pthread_t t;
    int ret_fd;
    pid_t pid = -1;

    const char* shell_command;
    struct stat st;

    const char *arg0, *arg1;
    if (name == 0 || *name == 0) {
        arg0 = "-"; arg1 = 0;
    } else {
        arg0 = "-c"; arg1 = name;
    }

    if (stat(ALTERNATE_SHELL_COMMAND, &st) == 0) {
        shell_command = ALTERNATE_SHELL_COMMAND;
    }
    else {
        shell_command = SHELL_COMMAND;
    }

    switch (mode) {
    case SUBPROC_PTY:
        ret_fd = create_subproc_pty(shell_command, arg0, arg1, &pid);
        break;
    case SUBPROC_RAW:
        ret_fd = create_subproc_raw(shell_command, arg0, arg1, &pid);
        break;
    default:
        ADB_LOGE(ADB_SERV, "invalid subproc_mode %d", mode);
        return -1;
    }
    ADB_LOGD(ADB_SERV, "create_subproc ret_fd=%d pid=%d", ret_fd, pid);

    stinfo* sti = reinterpret_cast<stinfo*>(malloc(sizeof(stinfo)));
    if (sti == 0) fatal("cannot allocate stinfo");
    sti->func = subproc_waiter_service;
    sti->cookie = (void*) (uintptr_t) pid;
    sti->fd = ret_fd;

    if (adb_thread_create(&t, service_bootstrap_func, sti)) {
        free(sti);
        close(ret_fd);
        ADB_LOGE(ADB_SERV, "cannot create service thread");
        return -1;
    }

    ADB_LOGD(ADB_SERV, "service thread started, fd=%d pid=%d", ret_fd, pid);
    return ret_fd;
}

int service_to_fd(const char *name)
{
    int ret = -1;

    if (!strncmp(name, "shell:", 6)) {
        ret = create_subproc_thread(name + 6, SUBPROC_PTY);
    } else if (!strncmp(name, "exec:", 5)) {
        ret = create_subproc_thread(name + 5, SUBPROC_RAW);
    } else if (!strncmp(name, "sync:", 5)) {
        ret = create_service_thread(file_sync_service, NULL);
    } else if (!strncmp(name, "reboot:", 7)) {
        void* arg = strdup(name + 7);
        if (arg == NULL) return -1;
        ret = create_service_thread(reboot_service, arg);
    }
    if (ret >= 0) {
        close_on_exec(ret);
    }
    return ret;
}
