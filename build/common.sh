download_md5() {
    local URL="${1}"
    local MD5="${2}"
    local DIR="${3}"
    if [[ ! -z "${4}" ]]; then
        local FILENAME="${4}"
    else
        local FILENAME="${URL##*/}"
    fi

    mkdir -p "${DIR}"

    if ! [ -f "${DIR}/${FILENAME}" ]; then
        if which axel >/dev/null && ! grep -q ftp <<< "${URL}"; then
            axel -an10 "${URL}" -o "${DIR}/${FILENAME}"
        else
            wget "${URL}" -O "${DIR}/${FILENAME}"
        fi
    fi

    if [[ "${MD5}" != "NONE" ]]; then
        if md5sum "${DIR}/${FILENAME}" | grep -q "${MD5}"; then
            return 0
        else
            return 1
        fi
    fi
}

download() {
    if [[ ! -z "${3}" ]]; then
        download_md5 "${1}" "NONE" "${2}" "${3}"
    else
        download_md5 "${1}" "NONE" "${2}"
    fi
}
