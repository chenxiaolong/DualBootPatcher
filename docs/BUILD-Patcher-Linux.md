Building Patcher for Linux
==========================

The following packages are needed for compiling:

- cmake
- jansson
- yaml-cpp
- OpenSSL
- qt5
- libarchive (optional)
- lz4 (optional)
- lzo2 (optional)
- xz (optional)
- zlib (optional)

System build (installation to /usr/local)
-----------------------------------------

If an optional dependency is not installed, a bundled version can be used by omitting the corresponding `-DMBP_USE_SYSTEM_<package>=ON` option below. It is strongly recommended to use the bundled version of libarchive as most distros use ship libarchive-3.1.2, which is missing features that are used by DualBootPatcher.

See [`CMAKE.md`](CMAKE.md) for a complete list of all the CMake options.

```sh
cd /path/to/DualBootPatcher
mkdir build
cd build
cmake .. \
    -DMBP_BUILD_TARGET=desktop \
    -DMBP_USE_SYSTEM_ZLIB=ON \
    -DMBP_USE_SYSTEM_LIBLZMA=ON \
    -DMBP_USE_SYSTEM_LZ4=ON \
    -DMBP_USE_SYSTEM_LZO2=ON
make
make install
```

Portable build
--------------

```sh
cd /path/to/DualBootPatcher
mkdir build
cd build
cmake .. \
    -DMBP_BUILD_TARGET=desktop \
    -DMBP_PORTABLE=ON
make
cpack -G ZIP # Or TBZ2, TGZ, TXZ, TZ, etc.
```

Note that the `.so` dependencies have to be manually added to the resulting archive in order to be used on other machines.
