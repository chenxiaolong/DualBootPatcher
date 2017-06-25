# Docker Setup

DualBootPatcher provides official docker images containing the necessary components to build the software. This allows for making consistent builds regardless of which computer they're built on. It also completely self contained and doesn't pollute the host with all the build dependencies.

This guide assumes that you already have docker installed.


## Building the docker images

Since the images are very large, they are not available in Docker Hub. They will need to be built on your local machine.

To build the docker images, simply run the following command to generate the `Dockerfile`s:

```sh
./docker/generate.sh
```

and then run the following command to build the actual images:

```sh
./docker/build.sh
```

Note that building the docker images will take a long time and consume a lot of bandwidth--multiple gigabytes at the very least. It will download all the dependencies for building DualBootPatcher for all supported targets. For example, the Android SDK will be downloaded for the `android` image and the mingw-w64 dependencies will be downloaded for the `mingw` image.

Once the images have been built, the resulting image tags are written to `docker/generated/images.properties`. It will look something like the following:

```dosini
android=chenxiaolong/dualbootpatcher:9.2.0-1-android
base=chenxiaolong/dualbootpatcher:9.2.0-1-base
linux=chenxiaolong/dualbootpatcher:9.2.0-1-linux
mingw=chenxiaolong/dualbootpatcher:9.2.0-1-mingw
```

The following table describes the images that are built:

| Image               | Description                                                           |
|---------------------|-----------------------------------------------------------------------|
| `<version>-base`    | Base image for the other images. Contains common dependencies         |
| `<version>-android` | Contains dependencies for building the Android app                    |
| `<version>-linux`   | Contains dependencies for building the desktop app for Linux          |
| `<version>-mingw`   | Contains dependencies for cross-compiling the desktop app for Windows |


## Using the docker images

To build DualBootPatcher inside of a docker container, simply mount the project directory in `/builder`. For example:

```sh
docker run --rm -it \
    -e USER_ID=$(id -u) \
    -e GROUP_ID=$(id -g) \
    -v "$(pwd):/builder/DualBootPatcher:rw,z" \
    -v "${HOME}/.android:/builder/.android:rw,z" \
    -v "${HOME}/.ccache:/builder/.ccache:rw,z" \
    -v "${HOME}/.gradle:/builder/.gradle:rw,z" \
    chenxiaolong/dualbootpatcher:9.2.0-1-android \
    bash
```

The `USER_ID` and `GROUP_ID` environment variables are set so that the user and group inside the container match what's outside of the container. Otherwise, the container might not be able to read and write to the bind-mounted volumes. These two environment variables might not be necessary for docker on Mac or Windows.

Mounting `~/.android`, `~/.ccache`, and `~/.gradle` will allow the various caches to be persisted even when the container is destroyed and recreated.

Once inside the docker container, simply `cd` to the project directory and run `cmake` as usual. For example:

```sh
cd DualBootPatcher
mkdir -p build
cd build
cmake .. -DMBP_BUILD_TARGET=android
make
```

See the other documentation files for more information on building with CMake.
