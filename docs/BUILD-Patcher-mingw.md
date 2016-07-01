Building Patcher for Windows with mingw-w64
===========================================

DualBootPatcher can be natively compiled on Windows or cross-compiled from Linux using mingw-w64.


### MinGW (Natively compiled w/msys2)

TODO: Still to be written


### MinGW (Cross compiled from Linux)

To cross-compile DualBootPatcher, the following packages are needed:

- cmake
- git
- android-ndk
- mingw-w64 toolchain

Additionally, the following mingw-compiled packages are needed:

- qt5
- xz

On Arch Linux, the following command will install some of the needed packages:

    pacman -S cmake git mingw-w64

The remaining packages will have to be installed from the AUR. I'd highly recommend against using AUR helpers for this, since they build in `/tmp/`. Some of these packages need more than 6GB of disk space to build!

- android-ndk
- mingw-w64-qt5-base
- mingw-w64-xz


Once all the dependencies are installed, follow the steps below to build DualBootPatcher.

1. Clone the git repository and setup the submodules

    ```sh
    git clone https://github.com/chenxiaolong/DualBootPatcher.git
    cd DualBootPatcher
    git submodule --init
    git submodule update
    ```

2. Set up the environment and CMake toolchain file

    ```sh
    # Set these appropriately for your distro
    export MINGW_TRIPLET=i686-w64-mingw32
    export MINGW_ROOT_PATH=/usr/${MINGW_TRIPLET}

    export ANDROID_NDK_HOME=/opt/android-ndk

    sed \
        -e "s,@MINGW_TRIPLET@,${MINGW_TRIPLET},g" \
        -e "s,@MINGW_ROOT_PATH@,${MINGW_ROOT_PATH},g" \
        < cmake/toolchain-mingw.cmake.in \
        > cmake/toolchain-mingw.cmake
    ```

3. Start the build!

    ```sh
    mkdir build && cd build
    cmake .. \
        -DMBP_BUILD_TARGET=desktop \
        -DMBP_PORTABLE=ON \
        -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-mingw.cmake \
        -DMBP_USE_SYSTEM_LIBRARY_ZLIB=ON \
        -DMBP_USE_SYSTEM_LIBRARY_LIBLZMA=ON \
        -DMBP_MINGW_USE_STATIC_LIBS=ON
    make
    ```

    `MBP_PORTABLE` creates a portable build that can be run on any Windows system.
    `MBP_USE_SYSTEM_LIBRARY_*` avoids using the bundled libraries, which are statically compiled.
    `MBP_MINGW_USE_STATIC_LIBS` tells CMake to prefer static libraries over DLLs.

5. Create the distributable zip package.

    ```sh
    cpack -G ZIP
    ```

   The resulting file will be something like `DualBootPatcher-[version]-win32.zip`

6. Add needed DLLs to the zip.

    ```sh
    unzip DualBootPatcher-<version>-win32.zip
    rm DualBootPatcher-<version>-win32.zip
    pushd DualBootPatcher-<version>-win32

    dlls=(
        libfreetype-6.dll
        libgcc_s_sjlj-1.dll
        libGLESv2.dll
        libglib-2.0-0.dll
        libharfbuzz-0.dll
        libiconv-2.dll
        libintl-8.dll
        libpcre-1.dll
        libpcre16-0.dll
        libpng16-16.dll
        libstdc++-6.dll
        libwinpthread-1.dll
        Qt5Core.dll
        Qt5Gui.dll
        Qt5Widgets.dll
        zlib1.dll
    )
    for dll in "${dlls[@]}"; do
        cp "${MINGW_ROOT_PATH}/bin/${dll}" .
    done

    # Qt Windows plugin
    mkdir platforms/
    cp "${MINGW_ROOT_PATH}"/lib/qt/plugins/platforms/qwindows.dll platforms/

    # Optionally, compress dlls
    upx --lzma *.dll
    popd
    zip -r DualBootPatcher-<version>-win32.zip DualBootPatcher-<version>-win32
    ```
