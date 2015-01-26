Building for Windows
====================

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
