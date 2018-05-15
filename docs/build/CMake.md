# CMake Options

## Contents

* [Main options](#main-options)
    * [`MBP_BUILD_TARGET`](#mbp_build_target)
    * [`MBP_BUILD_TYPE`](#mbp_build_type)
    * [`MBP_ENABLE_TESTS`](#mbp_enable_tests)
* [Signing](#signing)
    * [`MBP_SIGN_CONFIG_PATH`](#mbp_sign_config_path)
* [Desktop options](#desktop-options)
    * [`MBP_PORTABLE`](#mbp_portable)
* [Prebuilts paths](#prebuilts-paths)
    * [`MBP_PREBUILTS_DIR`](#mbp_prebuilts_dir)
    * [`MBP_PREBUILTS_BINARY_DIR`](#mbp_prebuilts_binary_dir)


## Main options

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
- Must be signed with a valid key using the `MBP_SIGN_CONFIG_PATH` option below
- Android app is built with gradle's `assembleRelease` task

For debug builds:
- Meant for development and testing
- Automatically signed with Android SDK's debug key. If the debug keystore does does not exist (`$HOME/.android/debug.keystore` or `%USERPROFILE%\.android\debug.keystore`), it will be created automatically.
- Android app is built with gradle's `assembleDebug` task

For CI builds:
- Meant to be built by continuous integration systems, like Jenkins
- Version number can be overridden by specifying `MBP_CI_VERSION` so that relavent build information, such as the git commit can be included in the version number.
- Must be signed with a valid key using the `MBP_SIGN_CONFIG_PATH` option below
- Android app is built with gradle's `assembleCi` task

##### Valid values:

- `release` - Release build
- `debug` - Debug build
- `ci` - Continuous integration build

##### Default value:

`debug`

##### Required:

No

---

#### `MBP_ENABLE_TESTS`

##### Description:

Whether to build the DualBootPatcher tests. If built, the tests can be run with `ctest -VV`. gtest will need to be installed.

##### Valid values:

Boolean value.

##### Default value:

ON

##### Required:

No


## Signing

The following options specify the parameters for signing the Android APK. The options are also required in non-debug desktop builds because mbtool uses the certificate to verify the source of the socket connection for its daemon.

**NOTE:** The signing config must exist for the project to build. The paths and passwords are *not* cached by CMake.

---

#### `MBP_SIGN_CONFIG_PATH`

##### Description:

Path to the signing config file containing the keystore path, keystore passphrase, key alias, and key passphrase to use for signing. See `cmake/SigningConfig.prop.in` for a sample config file. If a signing config is not provided for a debug build, then one will be generated automatically for the default Android SDK debug signing keystore.

##### Required:

Only in non-debug builds.


## Desktop options

#### `MBP_PORTABLE`

##### Description:

If set to true, the application will be built to be self-contained. The path to the `data` directory will be a relative path. On Linux, the RPATH value will be set to a relative path so the linker can find the libraries. Also, on Linux, the icon and desktop file will not be included in the resulting archive.

##### Valid values:

Boolean value.

##### Default value:

OFF

##### Required:

No


## Prebuilts paths

#### `MBP_PREBUILTS_DIR`

##### Description:

Path to the directory to store downloaded prebuilt binaries. This can be set to a common directory outside of the project directory for better caching (eg. when doing multiple builds on the same machine). The CMake scripts will lock the directory as necessary to prevent two or more CMake processes from writing to it at the same time.

##### Valid values:

Path to directory that exists.

##### Default value:

`thirdparty/prebuilts`

##### Required:

No

---

#### `MBP_PREBUILTS_BINARY_DIR`

##### Description:

Path to the directory to store extracted prebuilt binaries. This can be set to a common directory outside of the project directory for better caching (eg. when doing multiple builds on the same machine). The CMake scripts will lock the directory as necessary to prevent two or more CMake processes from writing to it at the same time.

##### Valid values:

Path to directory that exists.

##### Default value:

`${CMAKE_BINARY_DIR}/thirdparty/prebuilts`

##### Required:

No
