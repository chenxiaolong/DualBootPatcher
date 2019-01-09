#!/bin/sh

set -eu

build_dir=/mnt/workspace
uid_gid=$(stat -c '%u:%g' "${build_dir}")
current_uid_gid=$(id -u):$(id -g)

log() {
    echo "[${current_uid_gid}] ${*}"
}

log "${build_dir} is owned by ${uid_gid}"

if [[ "${uid_gid}" != "${current_uid_gid}" ]]; then
    if [[ "${#}" -gt 0 ]]; then
        log >&2 "Preventing infinite recursion"
        exit 1
    fi

    cleanup() {
        log "Cleaning up"
        rm -f /etc/passwd
    }
    trap cleanup EXIT

    log "Creating dummy user with UID:GID=${uid_gid}"
    echo "dummy:x:${uid_gid}::/dev/null:/bin/sh" > /etc/passwd

    log "Reexecuting script under ${uid_gid}"
    su dummy -c "${0} prevent-recursion"

    exit
fi

cd "${build_dir}"

host_build_dir=$(sed -n 's/^# Build directory: \(.*\)/\1/p' CTestTestfile.cmake)
log "Local build directory: ${build_dir}"
log "Host build directory: ${host_build_dir}"

log "Searching for CTestTestfile.cmake files"
ctest_files=$(find . -type f -name CTestTestfile.cmake)

log "Generating test runner script"
gen_script=$(mktemp)

cleanup() {
    log "Cleaning up"
    rm -f "${gen_script}"
}
trap cleanup EXIT

cat > "${gen_script}" << EOF
set -u
export TERM=xterm
exit_code=0
EOF

for file in ${ctest_files}; do
    # Unsafe, but it's the best we can do without the ctest binary
    test_cmd=$(sed -n 's/add_test([^ ]*[ ]*\(.*\))$/\1/p' "${file}")
    test_cmd=$(echo "${test_cmd}" | sed "s|${host_build_dir}|${build_dir}|g")

    if [[ -n "${test_cmd}" ]]; then
        echo "${test_cmd} || exit_code=1" >> "${gen_script}"
    fi
done

echo 'exit "${exit_code}"' >> "${gen_script}"

sh "${gen_script}"
