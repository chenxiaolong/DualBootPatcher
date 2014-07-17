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

    The following packages are needed for compiling:

    - wget (or axel)
    - p7zip
    - patch
    - gcc-multilib
    - zip
    - upx
    - mono
    - ImageMagick

    On *Fedora*, run:

    `sudo yum install axel p7zip-plugins patch gcc-c++ zip upx mono-core ImageMagick`

    On *Arch Linux*, run:

    `sudo pacman -Sy axel p7zip patch gcc-multilib zip upx mono imagemagick`

    On *Ubuntu*, run:

    `sudo apt-get install axel p7zip-full patch g++ zip upx-ucl mono-mcs imagemagick`

3. Create a configuration file `build/build.custom.conf` (or copy `build/build.conf`) for your system. Any options set here will override `build.conf`.

        [builder]
        android-ndk = /opt/android-ndk
        android-sdk = /opt/android-sdk

4. Compile!

    The build script is used like this:

        ./build/makedist.py [--debug] [--release] [--android] [--no-pc]

    Running the command with no arguments is equivalent to passing `--debug`. If you want to build the Android app, pass `--android`, and if you don't want to build the PC program, pass `--no-pc`. At the moment, the debug and release build types are used for the Android app only. Release mode requires that a signing key is set up.

License
-------
The patcher is licensed under GPLv3+ (see the LICENSE file). Third party libraries and programs are used under their respective licenses. Copies of these licenses are in the licenses/ directory of this repository. Patches and other source code modifications to third party software are under the same license as the original software.
