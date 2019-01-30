#!/usr/local/bin/tini /bin/bash

set -eu

uid=$(id -u)
gid=$(id -g)

if [[ "${uid}" -eq 0 ]] && [[ "${gid}" -eq 0 ]]; then
    uid=${USER_ID:-1000}
    gid=${GROUP_ID:-1000}

    echo >&2 "Deleting all local users and creating builder user with UID=${uid} and GID=${gid}"
    echo "builder:x:${gid}:" > /etc/group
    echo "builder:!::" > /etc/gshadow
    echo "builder:x:${uid}:${gid}::/builder:/bin/bash" > /etc/passwd
    echo "builder:!!:::::::" > /etc/shadow

    # BUILDER_HOME already exists so useradd won't copy skel
    cp -r /etc/skel/. "${BUILDER_HOME}"

    # Fix permissions for anything that's not a mountpoint in the home directory
    find "${BUILDER_HOME}" \
        -exec mountpoint -q {} \; -prune \
        -o -exec chown "${uid}:${gid}" {} \+

    exec gosu "${uid}:${gid}" "${@}"
else
    echo >&2 "WARNING WARNING WARNING"
    echo >&2 "Skipping user creation because container is not running as root"
    echo >&2 "Expected (uid=0, gid=0), but have (uid=${uid}, gid=${gid})"
    echo >&2 "WARNING WARNING WARNING"

    exec "${@}"
fi
