CMake options
=============

Contents
--------

* [Main options](#main-options)
    * [`MBP_BUILD_TARGET`](#mbp_build_target)
    * [`MBP_BUILD_TYPE`](#mbp_build_type)
* [Signing](#signing)
    * [`MBP_SIGN_CONFIG_PATH`](#mbp_sign_config_path)
* [Android options](#miscellaneous-options)
    * [`MBP_ANDROID_DEBUG`](#mbp_android_debug)
    * [`MBP_ANDROID_NDK_PARALLEL_PROCESSES`](#mbp_android_ndk_parallel_processes)
* [Desktop options](#desktop-options)
    * [`MBP_PORTABLE`](#mbp_portable)
    * [`MBP_USE_SYSTEM_<library>`](#mbp_use_system_library)
    * [`MBP_USE_SYSTEM_LIBRARIES`](#mbp_use_system_libraries)
    * [`MBP_MINGW_USE_STATIC_LIBS`](#mbp_mingw_use_static_libs)


Main options
------------

#### `MBP_BUILD_TARGET`

##### Description:

Specifies the target system to build for.

##### Valid values:

- `desktop` Build for conventional desktop operating systems
- `android` Build for Android

##### Default value:

`desktop`

##### Required:

No

---

#### `MBP_BUILD_TYPE`

##### Description:

Specifies the build type.

For release builds:
- Meant for distribution to users
- Must be signed with a valid key using the `MBP_SIGN_*` options below
- Android app is built with gradle's `assembleRelease` task

For debug builds:
- Meant for development and testing
- Automatically signed with Android SDK's debug key. If the debug keystore does does not exist ($HOME/.android/debug.keystore or %USERPROFILE%\.android\debug.keystore), it will be created automatically.
- Android app is built with gradle's `assembleDebug` task

For CI builds:
- Meant to be built by continuous integration systems, like Jenkins
- Version number can be overridden by specifying `MBP_CI_VERSION` so that relavent build information, such as the git branch can be included in the version number.
- Must be signed with a valid key using the `MBP_SIGN_*` options below
- Android app is built with gradle's `assembleCi` task

##### Valid values:

- `release` - Release build
- `debug` - Debug build
- `ci` - Continuous integration build

##### Default value:

`debug`

##### Required:

No


Signing
-------

The following options specify the parameters for signing the Android APK. The options are also required in non-debug desktop builds because mbtool uses the certificate to verify the source of the socket connection for its daemon.

**NOTE:** The signing config must exist for the project to build. The paths and passwords are *not* cached by CMake.

---

#### `MBP_SIGN_CONFIG_PATH`

##### Description:

Path to the signing config file containing the keystore path, keystore passphrase, key alias, and key passphrase to use for signing. See `cmake/SigningConfig.prop.in` for a sample config file. If a signing config is not provided for a debug build, then one will be generated automatically for the default Android SDK debug signing keystore.

##### Required:

Only in non-debug builds.


Android options
---------------------

#### `MBP_ANDROID_DEBUG`

##### Description:

If set to true, `NDK_DEBUG=1` is passed to all `ndk-build` commands, causing native libraries and binaries to include debugging symbols. There is not need to specify this option unless you are debugging the native components.

Note that when this option is enabled, the resulting binaries can be very large. For example, mbtool can reach 20MB, which won't fit in the boot partition on most devices, leaving the device unbootable.

##### Valid values:

Any boolean value CMake accepts (0/1, ON/OFF, TRUE/FALSE, etc.)

##### Default value:

OFF

##### Required:

No

---

#### `MBP_ANDROID_NDK_PARALLEL_PROCESSES`

##### Description:

Specifies the number of parallel processes to use when building native components. This is passed via the `-j<count>` parameter to all `ndk-build` commands.

##### Valid values:

Any positive integer.

##### Default:

The number of physical cores (not hyper-threading threads) on the host machine.

##### Required:

No


Desktop options
---------------

#### `MBP_PORTABLE`

##### Description:

If set to true, the application will be built to be self-contained. The path to the `data` directory will be a relative path. On Linux, the RPATH value will be set to a relative path so the linker can find `libmbp.so`. Also, on Linux, the icon and desktop file will not be included in the resulting archive.

##### Valid values:

Boolean value.

##### Default:

OFF

##### Required:

No

---

#### `MBP_USE_SYSTEM_<library>`

##### Description:

If set to true, CMake will look for the system version of the specified library instead of using the bundled version. It is strongly recommended to enable this option for libarchive as the latest stable release is far too old (the bundled version is based on the libarchive master branch).

**NOTE:** If this option is enabled and CMake cannot find the system version of the specified library, then CMake will fail. It will not fall back to the bundled version.

##### Valid values:

Boolean value. `<library>` can be one of: `LIBARCHIVE`, `ZLIB`, `LIBLZMA`, `LZO`, or `LZ4`

##### Required:

No

---

#### `MBP_USE_SYSTEM_LIBRARIES`

##### Description:

Enables the above option ([`MBP_USE_SYSTEM_<library>`](#mbp_use_system_library)) for all bundled libraries.

##### Valid values:

No value. Specified by `-DMBP_USE_SYSTEM_LIBRARIES`

##### Required:

No

---

#### `MBP_MINGW_USE_STATIC_LIBS`

##### Description:

**NOTE:** Only valid when cross-compiling with mingw.

If set to true, CMake will only search for libraries with a `.a` extension.

##### Valid values:

Boolean value.

##### Default:

OFF

##### Required:

No
