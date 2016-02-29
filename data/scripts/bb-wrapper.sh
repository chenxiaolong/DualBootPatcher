#!/sbin/busybox_orig sh

mount_parse_target() {
    local target=""

    while [ "${#}" -gt 0 ]; do
        case "${1}" in
        -o|-O|-t) shift 2 ;;
        -*) shift ;;
        *) target="${1}"; shift ;;
        esac
    done

    echo "${target}"
}

umount_parse_target() {
    local target=""

    while [ "${#}" -gt 0 ]; do
        case "${1}" in
        -*) shift ;;
        *) target="${1}"; shift ;;
        esac
    done

    echo "${target}"
}

do_mount() {
    local target
    target=$(mount_parse_target "${@}")

    if [ "${target:0:7}" = /system ] && [ "${target}" -ef /system ]; then
        exec /update-binary-tool mount /system
    elif [ "${target:0:6}" = /cache ] && [ "${target}" -ef /cache ]; then
        exec /update-binary-tool mount /cache
    elif [ "${target:0:5}" = /data ] && [ "${target}" -ef /data ]; then
        exec /update-binary-tool mount /data
    else
        exec /sbin/busybox_orig mount "${@}"
    fi
}

do_umount() {
    local target
    target=$(umount_parse_target "${@}")

    if [ "${target:0:7}" = /system ] && [ "${target}" -ef /system ]; then
        exec /update-binary-tool unmount /system
    elif [ "${target:0:6}" = /cache ] && [ "${target}" -ef /cache ]; then
        exec /update-binary-tool unmount /cache
    elif [ "${target:0:5}" = /data ] && [ "${target}" -ef /data ]; then
        exec /update-binary-tool unmount /data
    else
        exec /sbin/busybox_orig umount "${@}"
    fi
}

do_reboot() {
    echo "reboot command disabled in chroot environment" >&2
    exit 0
}

argv0="${0##*/}"
tool=""

if [ "x${argv0}" = "xbusybox" ]; then
    tool="${1}"
    shift
else
    tool="${argv0}"
fi

case "${tool}" in
    mount) do_mount "${@}" ;;
    umount) do_umount "${@}" ;;
    reboot) do_reboot "${@}" ;;
    *) exec /sbin/busybox_orig "${tool}" "${@}" ;;
esac
