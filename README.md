[![Build Status](http://jenkins.cxl.epac.to/job/DualBoot_Patcher/badge/icon)](https://jenkins.cxl.epac.to/job/DualBoot_Patcher/)

Multi Boot Patcher
==================

Please see forum thread at XDA for more information: [link](http://forum.xda-developers.com/showthread.php?t=2447534)

Downloads
=========
[Binary releases for Linux, Windows, OS X, and Android](http://d-h.st/users/chenxiaolong/?fld_id=24930#files)

[Android app for switching between ROMs](http://d-h.st/users/chenxiaolong/?fld_id=24392#files)

[Recovery utilities](http://d-h.st/users/chenxiaolong/?fld_id=24393&s=file_name&d=ASC)

---

Continuous integration build statuses can be found at the build status link above.

[git snapshot releases (from Jenkins CI)](http://dl.dropbox.com/u/486665/Snapshots/DualBootPatcher/index.html)

Compiling from Source
=====================
1. Clone git repository from GitHub:

        git clone https://github.com/chenxiaolong/DualBootPatcher.git

2. Install needed dependencies

    - wget (or axel)
    - p7zip
    - patch
    - gcc-multilib
    - zip
    - upx (`upx-ucl` package in Ubuntu)

3. Create a configuration file `build/config-override.sh` for your system:

        ANDROID_NDK=/path/to/android-ndk
        ANDROID_HOME=/path/to/android-sdk

        # If you want the C++ Windows launcher instead of the C# one, uncomment:
        #WIN32_LAUNCHER="C++"

        # Uncomment if you want a custom version number
        #VERSION="4.0.0.custom"

4. Compile!

    The build script is used like this:

        ./build/makedist.sh [device] [debug|release|none]

    The second argument refers to how the Android app should be built. If you don't want to build it, just use `none` or don't specify anything.

    Example: building the PC version of the patcher for the Samsung Galaxy S4

        ./build/makedist.sh jflte

    Building for jflte just means that the default in the `defaults.conf` file will be set to jflte. If you build for, say hammerhead, the build will work for jflte as long as the defaults.conf file is edited.
