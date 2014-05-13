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

3. Create a configuration file `build/build.custom.conf` (or copy `build/build.conf`) for your system. Any options set here will override `build.conf`.

        [builder]
        android-ndk = /opt/android-ndk
        android-sdk = /opt/android-sdk

4. Compile!

    The build script is used like this:

        ./build/makedist.py [--debug] [--release] [--android] [--no-pc]

    Running the command with no arguments is equivalent to passing `--debug`. If you want to build the Android app, pass `--android`, and if you don't want to build the PC program, pass `--no-pc`. At the moment, the debug and release build types are used for the Android app only. Release mode requires that a signing key is set up.
