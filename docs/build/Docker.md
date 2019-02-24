# Docker Setup

DualBootPatcher provides an official docker image containing the necessary components to build the software. This allows for making consistent builds regardless of which computer they're built on. It also completely self contained and doesn't pollute the host with all the build dependencies.

This guide assumes that you already have docker installed.


## Building the docker image

Since the image is very large, it is not available on Docker Hub. It will need to be built on your local machine.

To build the docker image, simply run the following command:

```sh
./docker/build.sh
```

Note that building the docker image will take a long time and consume a lot of bandwidth--multiple gigabytes at the very least. It will download all the dependencies for building DualBootPatcher for all supported targets.

Once the image has been built, the resulting image tag is written to `docker/image.txt`. It will look something like the following:

```
chenxiaolong/dualbootpatcher:9.3.0-15
```


## Using the docker images

To build DualBootPatcher inside of a docker container, simply mount the project directory in `/builder`. For example:

```sh
docker run --rm -it \
    -e USER_ID=$(id -u) \
    -e GROUP_ID=$(id -g) \
    -v "$(pwd):/builder/DualBootPatcher:rw,z" \
    -v "${HOME}/.android:/builder/.android:rw,z" \
    -v "${HOME}/.ccache:/builder/.ccache:rw,z" \
    "$(<docker/image.txt)" \
    bash
```

The `USER_ID` and `GROUP_ID` environment variables are set so that the user and group inside the container match what's outside of the container. Otherwise, the container might not be able to read and write to the bind-mounted volumes. These two environment variables might not be necessary for docker on Mac or Windows.

Mounting `~/.android` and `~/.ccache` will allow the various caches to be persisted even when the container is destroyed and recreated.

Once inside the docker container, simply `cd` to the project directory and run `cmake` as usual. For example:

```sh
cd DualBootPatcher
mkdir -p build
cd build
cmake .. -DMBP_BUILD_TARGET=android
make
```

See the other documentation files for more information on building with CMake.
