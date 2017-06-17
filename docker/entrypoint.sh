#!/usr/local/bin/tini /bin/bash

set -eu

uid=$(id -u)
gid=$(id -g)

if [[ "${uid}" -eq 0 ]] && [[ "${gid}" -eq 0 ]]; then
    uid=${USER_ID:-1000}
    gid=${GROUP_ID:-1000}

    if ! getent group "${gid}" >/dev/null; then
        groupadd -g "${gid}" builder
    else
        echo >&2 "WARNING WARNING WARNING"
        echo >&2 "GID ${gid} already exists"
        echo >&2 "WARNING WARNING WARNING"
    fi

    if ! getent passwd "${uid}" >/dev/null; then
        useradd -s /bin/bash -u "${uid}" -g "${gid}" -d "${BUILDER_HOME}" -M builder

        # BUILDER_HOME already exists so useradd won't copy skel
        cp -r /etc/skel/. "${BUILDER_HOME}"

        # Fix permissions for anything that's not a mountpoint in the home directory
        find "${BUILDER_HOME}" \
            -exec mountpoint -q {} \; -prune \
            -o -exec chown "${uid}:${gid}" {} \+
    else
        echo >&2 "WARNING WARNING WARNING"
        echo >&2 "UID ${uid} already exists"
        echo >&2 "WARNING WARNING WARNING"
    fi

    export HOME=${BUILDER_HOME}

    exec gosu "${uid}:${gid}" "${@}"
else
    echo >&2 "WARNING WARNING WARNING"
    echo >&2 "Skipping user creation because container is not running as root"
    echo >&2 "Expected (uid=0, gid=0), but have (uid=${uid}, gid=${gid})"
    echo >&2 "WARNING WARNING WARNING"

    exec "${@}"
fi
