DualBootPatcher now supports performing automated actions via intents. By default, the app will not allow intents from 3rd party apps. This must be explicitly enabled by the user by enabling the option in the settings.


# Switching ROMs

The app will ask for confirmation upon receiving the `SWITCH_ROM` intent. The user will have the option to suppress the confirmation for future intents. If the image checksums (`/sdcard/MultiBoot/[ROM ID]/*.img`) do not match the expected checksums (`/data/multiboot/checksums.prop`), DualBootPatcher will not allow the ROM switch. The user will need to manually switch ROMs once to confirm that the changes were intentional.

## Action

    com.github.chenxiaolong.dualbootpatcher.SWITCH_ROM

## Arguments/Extras

| Parameter  | Type    | Required | Default | Description                            |
|------------|---------|----------|---------|----------------------------------------|
| `rom_id`   | String  | Yes      | n/a     | The ROM ID to switch to                |
| `reboot`   | Boolean | No       | False   | Whether to reboot after switching ROMs |

**NOTE:** When `reboot` is set to `true`, the reboot will only occur if the switching was successful. If an error occurs, the device will not reboot.

## Result

The result of the ROM switching operation can be obtained by launching the intent with `startActivityForResult()` and implementing `onActivityResult()` for the calling `Activity`. DualBootPatcher will always return `Activity.RESULT_OK` for the `resultCode` parameter. The actual results are part of the returned intent's extras. The following values are returned.

| Parameter | Type   | Always returned | Description                       | Returns                                                                             |
|-----------|--------|-----------------|-----------------------------------|-------------------------------------------------------------------------------------|
| `code`    | String | Yes             | Result of the switching operation | One of `"SWITCHING_SUCCEEDED"`, `"SWITCHING_FAILED"`, or `"UNKNOWN_BOOT_PARTITION"` |
| `message` | String | No              | Untranslated error message        | A simple error message that should only be used for logging                         |


# Patching Files

There are no restrictions for patching files via an intent from a third party application.

## Action

    com.github.chenxiaolong.dualbootpatcher.PATCH_FILE

## Arguments/Extras

| Parameter | Type   | Required | Default                                     | Description           |
|-----------|--------|----------|---------------------------------------------|-----------------------|
| `path`    | String | Yes      | n/a                                         | Path to file to patch |
| `rom_id`  | String | Yes      | n/a                                         | Target ROM ID         |
| `device`  | String | No       | Autodetected from current device's codename | Target device ID      |

**NOTE:** The `device` parameter takes in a device **ID**, not a device **codename**. The list of device IDs can be found in `multiboot/info.prop` of any patched file or from [`data/devices`](../data/devices).

## Result

Link the `SWITCH_ROM` intent, `Activity.RESULT_OK` will always be returned for the `resultCode` parameter in `onActivityResult()`. The following values are returned as part of the `data` intent.

| Parameter  | Type   | Always returned | Description                      | Returns                                                                                 |
|------------|--------|-----------------|----------------------------------|-----------------------------------------------------------------------------------------|
| `code`     | String | Yes             | Result of the patching operation | One of `"PATCHING_SUCCEEDED"`, `"PATCHING_FAILED"`, or `"INVALID_OR_MISSING_ARGUMENTS"` |
| `message`  | String | No              | Untranslated error message       | A simple error message that should only be used for logging                             |
| `new_file` | String | No              | Path to newly patched file       | Returns path to new file only if patching succeeds                                      |
