Building Patcher for Windows
============================

DualBootPatcher can be natively compiled on Windows using Visual Studio 2013. I've only tested with the Ultimate edition, but I don't see why the community edition wouldn't work.

Note: All commands below should be run within `VS2013 x86 Native Tools Command Prompt`


### Download and install prerequisites

The following packages need to be downloaded and installed:

- [Android NDK (Windows 32-bit)](https://developer.android.com/tools/sdk/ndk/index.html)
- [CMake 3.1 or later](http://www.cmake.org/)
- [TortoiseGit](https://code.google.com/p/tortoisegit/)


### Clone the git repository and setup the submodules

1. Right click the directory where the source code should be kept and click "Git Clone..."

2. Paste the repository URL and click OK: https://github.com/chenxiaolong/DualBootPatcher.git

3. Enter the new DualBootPatcher folder, right click an empty area, and choose "Submodule update..."

4. Check "Init submodules", "Show Whole Project", "Select all", and click OK


### Build minimal version of Qt 5.4.0

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

### Build DualBootPatcher

1. Set some environment variables

   ```dos
   set PATH="%PATH%;C:\QtMinimal\5.4.0"
   set QTDIR="c:\QtMinimal\5.4.0"
   set PATH="%PATH%;C:\Program Files (x86)\CMake\bin"
   ```

2. Build

   ```dos
   mkdir build
   cd build
   cmake .. ^
       -DMBP_BUILD_TARGET=desktop ^
       -DMBP_PORTABLE=ON ^
       -DCMAKE_PREFIX_PATH="%QTDIR%\lib\cmake"
   cmake --build . --config Release
   cpack -G ZIP -D CPACK_BUILD_CONFIG=Release
   ```

   The resulting file will be something like `DualBootPatcher-[version]-win32.zip`

3. Extract `DualBootPatcher-[version]-win32.zip` and copy the following files to the `DualBootPatcher-[version]-win32` directory

    - C:\QtMinimal\5.4.0\bin\Qt5Core.dll
    - C:\QtMinimal\5.4.0\bin\Qt5Gui.dll
    - C:\QtMinimal\5.4.0\bin\Qt5Widgets.dll

4. Rezip the `DualBootPatcher-[version]-win32.zip` file
