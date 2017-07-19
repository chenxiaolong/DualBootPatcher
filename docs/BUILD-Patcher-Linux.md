# Building the Patcher for Linux

## Environment Setup

If you have docker installed, follow the directions at [`DOCKER.md`](DOCKER.md) to set up a docker build environment. It includes all the dependencies needed for building DualBootPatcher. With docker, DualBootPatcher can also be built from Windows and Mac hosts. For building the Qt patcher for Linux, you'll need to use the `<version>-linux` image.

If you don't have docker installed, the following packages are needed:

- Android NDK
- cmake
- gtest
- jansson
- libarchive
- OpenSSL
- qt5
- yaml-cpp

## Build process

1. If you haven't cloned the repo and its submodules yet, follow the directions at [`BUILD-Git-Clone.md`](BUILD-Git-Clone.md).

2. Set the environment variables for the Android NDK path. If you're using the docker image, this step is not needed as the environment variables are already set in the image.

    ```sh
    export ANDROID_NDK_HOME=/path/to/android-ndk
    export ANDROID_NDK=/path/to/android-ndk
    ```

3. If making a release build, make a copy of `cmake/SigningConfig.prop.in` and edit it to specify the keystore path, keystore passphrase, key alias, and key passphrase.

4. Configure the build with CMake.

    ```sh
    mkdir build && cd build
    cmake .. -DMBP_BUILD_TARGET=desktop
    ```

    By default, CMake configures the build such that it will be installed to `/usr/local`. This can be changed by passing `-DCMAKE_BUILD_PREFIX=/usr` (or some other path) to CMake. If you want to create a portable build that does not need to be installed instead, pass `-DMBP_PORTABLE=ON`.

    If you're making a release build, you'll need to pass the `-DMBP_BUILD_TYPE=release` and `-DMBP_SIGN_CONFIG_PATH=<signing config path>` arguments. See [`CMAKE.md`](CMAKE.md) for a complete listing of CMake options.

5. Build the system components. This includes things like mbtool and odinupdater. The build can be sped up by running things in parallel. Just pass `-j<number of cores>` (eg. `-j4`) to `make`.

    ```sh
    make
    ```

6. Install or package the patcher.

    If you're not making a portable build, simply run the following command to install DualBootPatcher.

    ```sh
    sudo make install
    ```

    If you're making a portable build, use `cpack` to generate an archive.

    ```sh
    cpack -G ZIP # Or TBZ2, TGZ, TXZ, TZ, etc.
    ```

    Note that the `.so`/`.dylib` dependencies have to be manually added to the resulting archive in order to be used on other machines.
