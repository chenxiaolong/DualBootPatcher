Building for Android
====================

The following packages are needed for compiling for Android:

- Android SDK
- Android NDK
- cmake

At this time, the host system must be running Linux (though I have not tried compiling on Windows or OS X).

1. Set the environment variables for the Android SDK and NDK path

   ```sh
   export ANDROID_HOME=/path/to/android-sdk
   export ANDROID_NDK_HOME=/path/to/android-ndk
   ```

2. Build!

   For a release build:

   ```sh
   mkdir build && cd build
   cmake .. -DMBP_BUILD_ANDROID=ON
   make
   rm -rf assets && cpack -G TXZ
   cd ../Android_GUI
   ./gradlew assembleRelease
   ```

   For a debug build:

   ```sh
   mkdir build && cd build
   cmake .. -DMBP_BUILD_ANDROID=ON -DANDROID_DEBUG=ON
   make
   rm -rf assets && cpack -G TXZ
   cd ../Android_GUI
   ./gradlew assembleDebug
   ```
