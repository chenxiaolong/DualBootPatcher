Building for Linux
==================

The following packages are needed for compiling:

- cmake
- gcc-multilib
- boost
- libarchive
- lz4
- xz
- zip
- qt5

System build (installation to /usr/local)
-----------------------------------------

```sh
cd /path/to/DualBootPatcher
mkdir build
cd build
cmake .. \
    -DMBP_USE_SYSTEM_ZLIB=ON \
    -DMBP_USE_SYSTEM_LIBLZMA=ON \
    -DMBP_USE_SYSTEM_LZ4=ON
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
    -DMBP_PORTABLE=ON
make
cpack -G ZIP # Or TBZ2, TGZ, TXZ, TZ, etc.
```

Note that the `.so` dependencies have to be manually added to the resulting archive in order to be used on other machines.
