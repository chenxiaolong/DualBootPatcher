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
#   Determine if an ABI is supported
# Returns:
#   0 if supported, 1 if not
android_check_abi_supported() {
    local abi="${1}"

    case "${abi}" in
    armeabi-v7a|arm64-v8a|x86|x86_64)
        return 0
        ;;
    *)
        error "Unsupported ABI: ${abi}"
        return 1
    esac
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
#   Get toolchain name for an ABI
# Params:
#   $1 : Android ABI
# Output:
#   Toolchain name
# Returns:
#   1 if the ABI name is invalid
android_get_toolchain_name() {
    local abi="${1}"
    local toolchain
    android_check_abi_supported "${abi}"

    case "${abi}" in
    armeabi-v7a)
        toolchain=arm-linux-androideabi
        ;;
    arm64-v8a)
        toolchain=aarch64-linux-android
        ;;
    x86)
        toolchain=x86
        ;;
    x86_64)
        toolchain=x86_64
        ;;
    esac

    echo "${toolchain}"
}

# Description:
#   Get toolchain triplet for a toolchain
# Params:
#   $1 : Toolchain name
# Output:
#   Toolchain triplet
# Returns:
#   1 if the ABI name is invalid
android_get_toolchain_triplet() {
    local toolchain="${1}"
    local triplet

    case "${toolchain}" in
    arm-linux-androideabi|aarch64-linux-android)
        triplet="${toolchain}"
        ;;
    x86)
        triplet=i686-linux-android
        ;;
    x86_64)
        triplet=x86_64-linux-android
        ;;
    *)
        error "Invalid toolchain: ${toolchain}"
        return 1
        ;;
    esac

    echo "${triplet}"
}

# Description:
#   Get the toolchain's bin directory for the given toolchain
# Params:
#   $1 : Toolchain name
#   $2 : Toolchain version
# Output:
#   Toolchain's bin directory
# Returns:
#   1 if the toolchain is not recognized or if there are no prebuilt binaries
#   for the current system
android_get_toolchain_bin_dir() {
    local toolchain="${1}"
    local toolchain_ver="${2}"
    local prebuilt

    if [[ "$(uname -s)" == Linux ]] && [[ "$(uname -m)" == x86_64 ]]; then
        prebuilt=linux-x86_64
    else
        error "Unsupported system: $(uname -s) ($(uname -m))"
        return 1
    fi

    local base_dir bin_dir
    base_dir=$(android_ndk_path "toolchains/${toolchain}-${toolchain_ver}")
    bin_dir="${base_dir}/prebuilt/${prebuilt}/bin"

    if [[ ! -d "${base_dir}" ]]; then
        error "Invalid toolchain: ${toolchain}-${ver}"
        return 1
    fi
    if [[ ! -d "${bin_dir}" ]]; then
        error "No prebuilts found for: ${prebuilt}"
        return 1
    fi

    echo "${bin_dir}"
}

# Description:
#   Convert Android ABI name (eg. armeabi-v7a) to platform directory name
#   (eg. arch-arm)
# Params:
#   $1 : Android ABI
# Output:
#   Platform directory name
# Returns:
#   1 if the ABI name is invalid
android_abi_name_to_platform_dir() {
    local abi="${1}"
    local platform_dir
    android_check_abi_supported "${abi}"

    case "${abi}" in
    armeabi-v7a)
        platform_dir="arch-arm"
        ;;
    arm64-v8a)
        platform_dir="arch-arm64"
        ;;
    x86|x86_64)
        platform_dir="arch-${abi}"
        ;;
    esac

    echo "${platform_dir}"
}

# Description:
#   Get NDK sysroot directory for a particular API and ABI
# Params:
#   $1 : Android API
#   $2 : Android ABI
# Output:
#   sysroot directory
# Returns:
#   1 if the ABI name is invalid
android_get_ndk_sysroot() {
    local api="${1}"
    local abi="${2}"
    local abi_dir
    android_check_abi_supported "${abi}"

    abi_dir=$(android_abi_name_to_platform_dir "${abi}")

    echo "$(android_ndk_path "platforms/${api}/${abi_dir}")"
}

# Description:
#   Sets the following variables according to the toolchain and version:
#     PATH, CC, CXX, CPP, AR, AS, LD, OBJCOPY, OBJDUMP, RANLIB, STRIP
# Params:
#   $1 : Toolchain name
#   $2 : Toolchain version
# Returns:
#   1 if a toolchain environment was already entered
android_enter_toolchain_env() {
    local toolchain="${1}"
    local toolchain_ver="${2}"
    local triplet
    local bin_dir

    if [[ ! -z "${_android_in_toolchain_env}" ]]; then
        error "Already in Android toolchain environment"
        return 1
    fi

    triplet=$(android_get_toolchain_triplet "${toolchain}")
    bin_dir=$(android_get_toolchain_bin_dir "${toolchain}" "${toolchain_ver}")

    _android_path="${PATH}"
    _android_cc="${CC}"
    _android_cxx="${CXX}"
    _android_cpp="${CPP}"
    _android_ar="${AR}"
    _android_as="${AS}"
    _android_ld="${LD}"
    _android_objcopy="${OBJCOPY}"
    _android_objdump="${OBJDUMP}"
    _android_ranlib="${RANLIB}"
    _android_strip="${STRIP}"

    export PATH="${PATH}:${bin_dir}"
    export CC="${triplet}-gcc"
    export CXX="${triplet}-g++"
    export CPP="${triplet}-cpp"
    export AR="${triplet}-ar"
    export AS="${triplet}-as"
    export LD="${triplet}-ld"
    export OBJCOPY="${triplet}-objcopy"
    export OBJDUMP="${triplet}-objdump"
    export RANLIB="${triplet}-ranlib"
    export STRIP="${triplet}-strip"

    _android_in_toolchain_env=1
}

# Description:
#   Restores the following variables to their values before calling
#   push_android_env():
#     PATH, CC, CXX, CPP, AR, AS, LD, OBJCOPY, OBJDUMP, RANLIB, STRIP
# Returns:
#   1 if a toolchain environment was not entered
android_leave_toolchain_env() {
    if [[ -z "${_android_in_toolchain_env}" ]]; then
        error "Not in Android toolchain environment"
        return 1
    fi

    export PATH="${_android_path}"
    export CC="${_android_cc}"
    export CXX="${_android_cxx}"
    export CPP="${_android_cpp}"
    export AR="${_android_ar}"
    export AS="${_android_as}"
    export LD="${_android_ld}"
    export OBJCOPY="${_android_objcopy}"
    export OBJDUMP="${_android_objdump}"
    export RANLIB="${_android_ranlib}"
    export STRIP="${_android_strip}"

    unset _android_path
    unset _android_cc
    unset _android_cxx
    unset _android_cpp
    unset _android_ar
    unset _android_as
    unset _android_ld
    unset _android_objcopy
    unset _android_objdump
    unset _android_ranlib
    unset _android_strip

    unset _android_in_toolchain_env
}
