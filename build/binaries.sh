create_binaries_windows() {
    local TD="${TARGETDIR}/binaries/windows/"
    mkdir -p "${TD}"

    mkdir -p "${BUILDDIR}/windowsbinaries"
    pushd "${BUILDDIR}/windowsbinaries"

    # Our mini-Cygwin :)
    local URLBASE="http://mirrors.kernel.org/sourceware/cygwin/x86/release"

    # cygwin library
    download "${URLBASE}/cygwin/cygwin-1.7.28-2.tar.xz" .

    # libintl
    download "${URLBASE}/gettext/libintl8/libintl8-0.18.1.1-2.tar.bz2" .

    # libiconv
    download "${URLBASE}/libiconv/libiconv2/libiconv2-1.14-2.tar.bz2" .

    # patch
    download "${URLBASE}/patch/patch-2.7.1-1.tar.bz2" .

    # diff
    download "${URLBASE}/diffutils/diffutils-3.2-1.tar.bz2" .

    tar Jxvf cygwin-1.7.28-2.tar.xz usr/bin/cygwin1.dll \
        --to-stdout > "${TD}/cygwin1.dll"
    tar jxvf libintl8-0.18.1.1-2.tar.bz2 usr/bin/cygintl-8.dll \
        --to-stdout > "${TD}/cygintl-8.dll"
    tar jxvf libiconv2-1.14-2.tar.bz2 usr/bin/cygiconv-2.dll \
        --to-stdout > "${TD}/cygiconv-2.dll"
    tar jxvf patch-2.7.1-1.tar.bz2 usr/bin/patch.exe \
        --to-stdout > "${TD}/hctap.exe"
    tar jxvf diffutils-3.2-1.tar.bz2 usr/bin/diff.exe \
        --to-stdout > "${TD}/diff.exe"

    chmod +x "${TD}"/*.{exe,dll}

    if [ "x${BUILDTYPE}" != "xci" ]; then
        # Compress binaries and libraries
        upx -v -9 "${TD}"/*.{exe,dll} || :
    fi

    popd
}

create_binaries_android() {
    local TD="${TARGETDIR}/binaries/android/"
    mkdir -p "${TD}"

    local TEMPDIR="$(mktemp -d --tmpdir="$(pwd)")"
    ${ANDROID_NDK}/build/tools/make-standalone-toolchain.sh \
        --verbose \
        --platform=android-18 \
        --install-dir=${TEMPDIR} \
        --ndk-dir=${ANDROID_NDK} \
        --system=linux-x86_64

    mkdir -p "${BUILDDIR}/androidbinaries"
    pushd "${BUILDDIR}/androidbinaries"

    download 'ftp://ftp.gnu.org/gnu/patch/patch-2.7.tar.xz' .

    tar Jxvf patch-2.7.tar.xz
    cd patch-2.7
    PATH="${TEMPDIR}/bin:${PATH}" ./configure --host=arm-linux-androideabi
    PATH="${TEMPDIR}/bin:${PATH}" make
    cp src/patch "${TD}"
    cd ..
    rm -r patch-2.7

    popd

    rm -r ${TEMPDIR}

    cp "${CURDIR}/ramdisks/busybox-static" "${TD}/"
}
