#!/sbin/busybox_orig sh

do_mount() {
    while [ "${#}" -gt 0 ]; do
        case "${1}" in
        -o|-O|-t) shift; shift ;;
        -*) shift ;;
        /system) /update-binary-tool mount /system; exit "${?}" ;;
        /cache) /update-binary-tool mount /cache; exit "${?}" ;;
        /data) /update-binary-tool mount /data; exit "${?}" ;;
        *) shift ;;
        esac
    done
    echo "mount command disabled in chroot environment" >&2
    exit 0
}

do_umount() {
    while [ "${#}" -gt 0 ]; do
        case "${1}" in
        -*) shift ;;
        /system) /update-binary-tool unmount /system; exit "${?}" ;;
        /cache) /update-binary-tool unmount /cache; exit "${?}" ;;
        /data) /update-binary-tool unmount /data; exit "${?}" ;;
        *) shift ;;
        esac
    done
    echo "umount command disabled in chroot environment" >&2
    exit 0
}

do_unzip() {
    /sbin/unzip "${@}"
    exit "${?}"
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
    unzip) do_unzip "${@}" ;;
esac

if [ "x${argv0}" = "xbusybox" ]; then
    /sbin/busybox_orig "${tool}" "${@}"
else
    /sbin/busybox_orig "${@}"
fi
exit "${?}"
