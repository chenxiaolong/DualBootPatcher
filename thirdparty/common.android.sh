#!/bin/bash

# NOTE: This script should not be executed! The shebang line is there only to
#       tell shellcheck that this is a bash script.

# Description:
#   Get path relative to the NDK root
# Params:
#   $1 : Relative path
# Output:
#   Full path including the NDK root
# Returns:
#   1 if the Android NDK could not be found
android_ndk_path() {
    if [[ -z "${ANDROID_NDK_HOME}" ]]; then
        error "Please set ANDROID_NDK_HOME to the path of the NDK"
        return 1
    fi

    if [[ "${#}" -eq 0 ]]; then
        echo "${ANDROID_NDK_HOME}"
    else
        local relpath="${1}"
        echo "${ANDROID_NDK_HOME}/${relpath}"
    fi
}

# Description:
#   Get ABI name from pacman's CARCH variable
# Output:
#   ABI name
# Returns:
#   1 if the CARCH value is unrecognized
android_get_abi_name() {
    local abi

    case "${CARCH}" in
    armv7)
        abi="armeabi-v7a"
        ;;
    aarch64)
        abi="arm64-v8a"
        ;;
    x86|x86_64)
        abi="${CARCH}"
        ;;
    *)
        error "Unrecognized CARCH: ${CARCH}"
        return 1
        ;;
    esac

    echo "${abi}"
}

# Description:
#   Get arch name from pacman's CARCH variable
# Output:
#   Arch name
# Returns:
#   1 if the CARCH value is unrecognized
android_get_arch_name() {
    local arch

    case "${CARCH}" in
    armv7)
        arch=arm
        ;;
    aarch64)
        arch=arm64
        ;;
    x86|x86_64)
        arch=${CARCH}
        ;;
    *)
        error "Unrecognized CARCH: ${CARCH}"
        return 1
        ;;
    esac

    echo "${arch}"
}

# Description:
#   Build a standalone toolchain
# Params:
#   $1 : Output directory
#   $2 : Android API
android_build_standalone_toolchain() {
    local output_dir=${1}
    local api=${2}
    local script
    local arch

    script=$(android_ndk_path build/tools/make_standalone_toolchain.py)
    arch=$(android_get_arch_name)

    msg "Building standalone toolchain for API ${api}..."

    "${script}" \
        --arch "${arch}" \
        --api "${api}" \
        --stl libc++ \
        --install-dir "${output_dir}" \
        -v
}
