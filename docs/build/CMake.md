# CMake Options

## Contents

* [Main options](#main-options)
    * [`MBP_BUILD_TARGET`](#mbp_build_target)
    * [`MBP_ENABLE_TESTS`](#mbp_enable_tests)
    * [`MBP_ENABLE_QEMU`](#mbp_enable_qemu)
* [Signing](#signing)
    * [`MBP_SIGN_CONFIG_PATH`](#mbp_sign_config_path)
* [Prebuilts paths](#prebuilts-paths)
    * [`MBP_PREBUILTS_DIR`](#mbp_prebuilts_dir)
    * [`MBP_PREBUILTS_BINARY_DIR`](#mbp_prebuilts_binary_dir)


## Main options

#### `MBP_BUILD_TARGET`

##### Description:

Specifies the target system to build for.

##### Valid values:

- `device` - Build for target Android device
- `system` - Build for current system (only includes some command-line utilities)

##### Default value:

`device`

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

---

#### `MBP_ENABLE_QEMU`

##### Description:

Whether to enable the `qemu-tests-<abi>` and `qemu-shell-<abi>` targets. The `qemu-tests-<abi>` will run all tests for the specified Android ABI within a QEMU virtual machine.

Enabling this option requires the following programs to be installed:
- `qemu-system-arm` (for `armeabi-v7a`)
- `qemu-system-aarch64` (for `arm64-v8a`)
- `qemu-system-i386` (for `x86`)
- `qemu-system-x86_64` (for `x86_64`)

##### Valid values:

Boolean value.

##### Default value:

OFF

##### Required:

No


## Signing

The following options specify the parameters for signing all binaries. The signatures are verified at runtime when the DualBootPatcher UI or mbtool executes or loads one of the binaries.

**NOTE:** The signing config must exist for the project to build. The paths and passwords are *not* cached by CMake.

---

#### `MBP_SIGN_CONFIG_PATH`

##### Description:

Path to the signing config file containing the secret key path, secret key passphrase, and public key path to use for signing. See `cmake/SigningConfig.prop.in` for a sample config file.

##### Required:

Yes


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
