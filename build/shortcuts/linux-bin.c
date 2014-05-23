/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
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

#include <linux/limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef PYTHON3
#define PYTHON3 0
#endif

#ifndef PYTHON2
#define PYTHON2 0
#endif

#ifndef SCRIPT
#error SCRIPT is not defined!
#endif

pid_t pid;

void build_path(char *path, char *executable, char *buf) {
    buf[0] = 0;
    strncat(buf, path, strlen(path));
    strncat(buf, "/", 1);
    strncat(buf, executable, strlen(executable));
}

int find_python_in_path(char *path, char *buf) {
    if (PYTHON3 && PYTHON2) {
        build_path(path, "python3", buf);
        if (access(buf, X_OK) == 0) {
            return 0;
        }
        build_path(path, "python2", buf);
        if (access(buf, X_OK) == 0) {
            return 0;
        }
        return -1;
    } else if (PYTHON3) {
        build_path(path, "python3", buf);
        if (access(buf, X_OK) == 0) {
            return 0;
        }
        return -1;
    } else if (PYTHON2) {
        build_path(path, "python2", buf);
        if (access(buf, X_OK) == 0) {
            return 0;
        }
        return -1;
    }
}

void signal_handler(int signo) {
    // Kill child process
    printf("Got signal %i. Sending to child process.\n", signo);
    kill(pid, signo);
}

int main(int argc, char *argv[]) {
    if (!PYTHON2 && !PYTHON3) {
        printf("This program was not compiled correctly!\n");
        exit(1);
    }

    char *path = getenv("PATH");
    char *dir;

    char buf[PATH_MAX];
    int found = 0;
    for (dir = strtok(path, ":"); dir != NULL; dir = strtok(NULL, ":")) {
        if (find_python_in_path(dir, buf) == 0) {
            found = 1;
            break;
        }
    }

    if (found == 0) {
        if (PYTHON2 && PYTHON3) {
            printf("Neither Python 3 nor Python 2 was found.\n");
        } else if (PYTHON3) {
            printf("Python 3 was not found.\n");
        } else if (PYTHON2) {
            printf("Python 2 was not found.\n");
        }
        exit(1);
    }

    pid = fork();
    if (pid == 0) {
        // Python + script + argv[1+] + NULL
        char *args[argc + 2];
        args[0] = buf;
        args[1] = SCRIPT;
        int i;
        for (i = 1; i < argc; i++) {
            args[i + 1] = argv[i];
        }
        args[argc + 1] = NULL;

        execvp(buf, args);

        printf("Failed to run command:");
        for (i = 0; i < 2 + argc + 1; i++) {
            printf(" %s", args[i]);
        }
        printf("\n");

        exit(1);
    } else {
        signal(SIGINT, signal_handler);

        int status;
        wait(&status);
        return status;
    }
}
