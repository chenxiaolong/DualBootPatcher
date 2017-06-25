# Building the Android App

## Environment Setup

If you have docker installed, follow the directions at [`DOCKER.md`](DOCKER.md) to set up a docker build environment. It includes all the dependencies needed for building DualBootPatcher. With docker, DualBootPatcher can also be built from Windows and Mac hosts. For building the Android app, you'll need to use the `<version>-android` image.

If you don't have docker installed, the following packages are needed:

- Android SDK
- Android NDK
- cmake
- gtest
- jansson
- JDK 1.8
- OpenSSL
- yaml-cpp

## Build process

1. If you haven't cloned the repo and its submodules yet, follow the directions at [`BUILD-Git-Clone.md`](BUILD-Git-Clone.md).

2. Set the environment variables for the Android SDK and NDK path. If you're using the docker image, this step is not needed as the environment variables are already set in the image.

    ```sh
    export ANDROID_HOME=/path/to/android-sdk
    export ANDROID_NDK_HOME=/path/to/android-ndk
    export ANDROID_NDK=/path/to/android-ndk
    ```

3. If making a release build, make a copy of `cmake/SigningConfig.prop.in` and edit it to specify the keystore path, keystore passphrase, key alias, and key passphrase.

4. Configure the build with CMake.

    ```sh
    mkdir build && cd build
    cmake .. -DMBP_BUILD_TARGET=android
    ```

    If you're making a release build, you'll need to pass the `-DMBP_BUILD_TYPE=release` and `-DMBP_SIGN_CONFIG_PATH=<signing config path>` arguments. See [`CMAKE.md`](CMAKE.md) for a complete listing of CMake options.

5. Build the system components. This includes things like mbtool, odinupdater, and various native libraries used by the app. The build can be sped up by running things in parallel. Just pass `-j<number of cores>` (eg. `-j4`) to `make`.

    ```sh
    make
    ```

6. Package the system components into a tarball.

    ```sh
    rm -rf assets && cpack -G TXZ
    ```

7. Build the APK.

    ```sh
    make apk
    ```

    The resulting APK file will be in the `Android_GUI/build/outputs/apk` directory.
