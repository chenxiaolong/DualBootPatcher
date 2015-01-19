[![Build Status](http://jenkins.cxl.epac.to/job/DualBoot_Patcher/badge/icon)](https://jenkins.cxl.epac.to/job/DualBoot_Patcher/)

Multi Boot Patcher
==================

Please see forum thread at XDA for more information: [link](http://forum.xda-developers.com/showthread.php?t=2447534)

Downloads
---------
[Binary releases for Linux, Windows, OS X, and Android](http://d-h.st/users/chenxiaolong/?fld_id=24930#files)

[Android app for switching between ROMs](http://d-h.st/users/chenxiaolong/?fld_id=24392#files)

[Recovery utilities](http://d-h.st/users/chenxiaolong/?fld_id=24393&s=file_name&d=ASC)

---

Continuous integration build statuses can be found at the build status link above.

[git snapshot releases (from Jenkins CI)](http://dl.dropbox.com/u/486665/Snapshots/DualBootPatcher/index.html)

Compiling from Source
---------------------
1. Clone git repository from GitHub:

        git clone https://github.com/chenxiaolong/DualBootPatcher.git

2. Install needed dependencies

    The following packages are needed for compiling for PC:

    - cmake
    - gcc-multilib
    - libarchive
    - boost
    - zip
    - qt5

    The following packages are needed for compiling for Android:

    - Android SDK
    - Android NDK
    - cmake

    If building for Android, make sure that the environment variables for the SDK and NDK are set to the appropriate directories.

        export ANDROID_HOME=/path/to/android-sdk
        export ANDROID_NDK_HOME=/path/to/android-ndk

3. Configure the build

    (TODO: Instructions for compiling with VS2013)

    Linux:

        mkdir build && cd build
        cmake ..
        make
        make install

    Linux (portable):

        mkdir build && cd build
        cmake ..  -DPORTABLE=ON
        make
        cpack -G ZIP # Or TBZ2, TGZ, TZ, etc.

    Android (release):

        mkdir build && cd build
        cmake .. -DBUILD_ANDROID=ON
        make
        rm -rf assets && cpack -G TXZ
        cd ../Android_GUI
        ./gradlew assembleRelease

    Android (debug):

        mkdir build && cd build
        cmake .. -DBUILD_ANDROID=ON -DANDROID_DEBUG=ON
        make
        rm -rf assets && cpack -G TXZ
        cd ../Android_GUI
        ./gradlew assembleDebug
        
    Note that by passing `-DBUILD_ANDROID=ON` to CMake, it will build only the Android version of the patcher. If you want to build both the PC and Android versions of the patcher, create one build directory for each.

License
-------
The patcher is licensed under GPLv3+ (see the LICENSE file). Third party libraries and programs are used under their respective licenses. Copies of these licenses are in the licenses/ directory of this repository. Patches and other source code modifications to third party software are under the same license as the original software.
