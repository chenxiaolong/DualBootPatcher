# Params:
#   $1 : Android API
#   $2 : Android ABI
# Output:
#   sysroot directory
get_ndk_sysroot() {
    local api="${1}"
    local abi="${2}"

    if [[ "${abi}" == "armeabi-v7a" ]]; then
        local abi_dir="arch-arm"
    elif [[ "${abi}" == "arm64-v8a" ]]; then
        local abi_dir="arch-arm64"
    else
        local abi_dir="arch-${abi}"
    fi

    echo "${ANDROID_NDK_HOME}/platforms/${api}/${abi_dir}"
}

# Params:
#   $1 : Toolchain name
#   $2 : Toolchain version
# Output:
#   Toolchain's bin directory
get_toolchain_dir() {
    local toolchain="${1}"
    local ver="${2}"

    if [[ "${toolchain}" == "i686-linux-android" ]]; then
        local dir="x86"
    elif [[ "${toolchain}" == "x86_64-linux-android" ]]; then
        local dir="x86_64"
    else
        local dir="${toolchain}"
    fi

    echo "${ANDROID_NDK_HOME}/toolchains/${dir}-${ver}/prebuilt/linux-x86_64/bin"
}

# Sets the following variables according to the toolchain and version:
#   PATH, CC, CXX, CPP, AR, AS, LD, OBJCOPY, OBJDUMP, RANLIB, STRIP
#
# Params:
#   $1 : Toolchain name
#   $2 : Toolchain version
push_android_env() {
    local toolchain="${1}"
    local ver="${2}"

    android_path="${PATH}"
    android_cc="${CC}"
    android_cxx="${CXX}"
    android_cpp="${CPP}"
    android_ar="${AR}"
    android_as="${AS}"
    android_ld="${LD}"
    android_objcopy="${OBJCOPY}"
    android_objdump="${OBJDUMP}"
    android_ranlib="${RANLIB}"
    android_strip="${STRIP}"

    export PATH="${PATH}:$(get_toolchain_dir ${toolchain} ${ver})"
    export CC="${toolchain}-gcc"
    export CXX="${toolchain}-g++"
    export CPP="${toolchain}-cpp"
    export AR="${toolchain}-ar"
    export AS="${toolchain}-as"
    export LD="${toolchain}-ld"
    export OBJCOPY="${toolchain}-objcopy"
    export OBJDUMP="${toolchain}-objdump"
    export RANLIB="${toolchain}-ranlib"
    export STRIP="${toolchain}-strip"
}

# Restores the following variables to their values before calling
# push_android_env():
#   PATH, CC, CXX, CPP, AR, AS, LD, OBJCOPY, OBJDUMP, RANLIB, STRIP
pop_android_env() {
    export PATH="${android_path}"
    export CC="${android_cc}"
    export CXX="${android_cxx}"
    export CPP="${android_cpp}"
    export AR="${android_ar}"
    export AS="${android_as}"
    export LD="${android_ld}"
    export OBJCOPY="${android_objcopy}"
    export OBJDUMP="${android_objdump}"
    export RANLIB="${android_ranlib}"
    export STRIP="${android_strip}"
}
