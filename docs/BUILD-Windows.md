Building for Windows
====================

DualBootPatcher is compatible with both Visual Studio (2013 and up) and MinGW (preferably mingw-w64 4.9 or newer).

Visual Studio 2013
------------------

Note: All commands below should be run within `VS2013 x86 Native Tools Command Prompt`

#### Build libxml2 2.9.2

1. Download and extract [libxml2 2.9.2](ftp://xmlsoft.org/libxml2/libxml2-2.9.2.tar.gz)

2. Rename `configure.ac` to `configure.in`

3. Compile and install to `C:\libxml2`

    ```dos
    cd path\to\libxml2-2.9.2\win32
    cscript configure.js compiler=msvc prefix=c:\libxml2 include=c:\libxml2\include lib=c:\libxml2\lib iconv=no
    nmake /f Makefile.msvc
    nmake /f Makefile.msvc install
    ```

#### Build minimal version of Qt 5.4.0

1. Download and extract `qtbase` and `qtwinextras`

   - http://download.qt.io/official_releases/qt/5.4/5.4.0/submodules/qtbase-opensource-src-5.4.0.zip
   - http://download.qt.io/official_releases/qt/5.4/5.4.0/submodules/qtwinextras-opensource-src-5.4.0.zip

2. Open `VS2013 x86 Native Tools Command Prompt` and navigate to `qtbase`'s directory

3. Compile `qtbase` and install to `C:\QtMinimal\5.4.0`

   ```dos
   cd path\to\qtbase-opensource-src-5.4.0
   configure -release -opensource -platform win32-msvc2013 -shared -nomake tests -nomake examples -no-icu -strip -mp -opengl desktop -prefix C:\QtMinimal\5.4.0
   nmake
   nmake install
   ```

 4. Compile `qtwinextras` and install it

   ```dos
   cd path\to\qtwinextras-opensource-src-5.4.0
   mkdir build
   cd build
   C:\QtMinimal\5.4.0\bin\qmake.exe ..
   nmake
   nmake install
   ```

#### Download and install Boost 1.57.0

http://downloads.sourceforge.net/project/boost/boost-binaries/1.57.0/boost_1_57_0-msvc-12.0-32.exe

#### Download and install CMake

http://www.cmake.org/

Note that DualBootPatcher requires version 3.1 or later.

#### Build DualBootPatcher

1. Set some environment variables

   ```dos
   set PATH="%PATH%;C:\QtMinimal\5.4.0"
   set BOOST_ROOT="C:\local\boost_1_57_0"
   set BOOST_LIBRARYDIR="C:\local\boost_1_57_0\lib32-msvc-12.0"
   set LIBXML2_ROOT="C:\libxml2"
   ```

2. Build

   ```dos
   mkdir build
   cd build
   "C:\Program Files (x86)\CMake\bin\cmake.exe" .. ^
       -DMBP_PORTABLE=ON ^
       -DBOOST_ROOT="C:\local\boost_1_57_0" ^
       -DBOOST_LIBRARYDIR="C:\local\boost_1_57_0\lib32-msvc-12.0" ^
       -DLIBXML2_ROOT="C:\libxml2" ^
       -DCMAKE_PREFIX_PATH="c:/QtMinimal/5.4.0/lib/cmake;C:/libxml2"
   ```


MinGW (Natively compiled w/msys2)
---------------------------------

TODO: Still to be written


MinGW (Cross compiled from Arch Linux)
--------------------------------------

1. Install the `mingw-w64` package group.

    ```sh
    pacman -S mingw-w64
    ```

2. Install `mingw-w64-xz`, `mingw-w64-libxml2`, and `mingw-w64-qt5-base` from the AUR.

   - https://aur.archlinux.org/packages/mingw-w64-xz
   - https://aur.archlinux.org/packages/mingw-w64-libxml2
   - https://aur.archlinux.org/packages/mingw-w64-qt5-base

   Note that `mingw-w64-qt5-base` requires more than 6GB to build, so avoid using AUR helpers that build in `/tmp/`.

3. Set up the CMake toolchain file.

    ```sh
    sed \
        -e 's,@MINGW_TRIPLET@,i686-w64-mingw32,g' \
        -e 's,@MINGW_ROOT_PATH@,/usr/i686-w64-mingw32,g' \
        < mingw/toolchain-mingw.cmake.in \
        > mingw/toolchain-mingw.cmake
    ```

4. Start the build!

    ```sh
    mkdir build && cd build
    cmake .. \
        -DMBP_PORTABLE=ON \
        -DCMAKE_TOOLCHAIN_FILE=../mingw/toolchain-mingw.cmake \
        -DMBP_USE_SYSTEM_LIBRARY_ZLIB=ON \
        -DMBP_USE_SYSTEM_LIBRARY_LIBLZMA=ON \
        -DMBP_MINGW_USE_STATIC_LIBS=ON
    ```

5. Create the distributable zip package.

    ```sh
    cpack -G ZIP
    ```

   The resulting file will be something like `DualBootPatcher-<version>-win32.zip`

6. Add needed DLLs to the zip.

    ```sh
    unzip DualBootPatcher-<version>-win32.zip
    rm DualBootPatcher-<version>-win32.zip
    pushd DualBootPatcher-<version>-win32
    dlls=(
        libgcc_s_sjlj-1.dll
        libGLESv2.dll
        libpcre16-0.dll
        libpng16-16.dll
        libstdc++-6.dll
        libwinpthread-1.dll
        Qt5Core.dll
        Qt5Gui.dll
        zlib1.dll
    )
    for dll in "${dlls[@]}"; do
      cp "${mingw_root_path}/bin/${dll}" bin/
    done
    # Optionally, compress dlls
    upx --lzma bin/*.dll
    popd
    zip -r DualBootPatcher-<version>-win32.zip DualBootPatcher-<version>-win32
    ```
