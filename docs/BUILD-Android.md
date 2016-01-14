Building for Android
====================

The following packages are needed for compiling for Android:

- Android SDK
- Android NDK
- cmake

At this time, the host system must be running Linux (though I have not tried compiling on Windows or OS X).

0. initialize and update dependencies

While cloning the repository initially with ```git clone```, prefer using the ```--recursive``` flag to automatically fetch all dependencies.

   ```sh
   # if using https
   git clone --recursive https://github.com/chenxiaolong/DualBootPatcher.git
   # or if using ssh
   git clone --recursive git@github.com:chenxiaolong/DualBootPatcher.git
   ```

In case you have not cloned this repository using the ```git clone --recursive``` command, you will have to initialize and update the dependencies required for building the apk.

   ```sh
   git submodule update --init --recursive
   ```

1. Set the environment variables for the Android SDK and NDK path

   ```sh
   export ANDROID_HOME=/path/to/android-sdk
   export ANDROID_NDK_HOME=/path/to/android-ndk
   ```

2. Build!

   See [`CMAKE.md`](CMAKE.md) for a complete listing of CMake options. The following commands provide common commands for building release and debug versions of the app.

   For a release build:

   ```sh
   mkdir build && cd build
   cmake .. \
       -DMBP_BUILD_TARGET=android \
       -DMBP_BUILD_TYPE=release \
       -DMBP_SIGN_JAVA_KEYSTORE_PATH=<keystore path> \
       -DMBP_SIGN_JAVA_KEYSTORE_PASSWORD=<keystore password> \
       -DMBP_SIGN_JAVA_KEY_ALIAS=<key alias> \
       -DMBP_SIGN_JAVA_KEY_PASSWORD=<key password>
   make
   rm -rf assets && cpack -G TXZ
   make apk
   ```

   For a debug build:

   ```sh
   mkdir build && cd build
   cmake .. \
       -DMBP_BUILD_TARGET=android \
       -DMBP_BUILD_TYPE=debug
   make
   rm -rf assets && cpack -G TXZ
   make apk
   ```
