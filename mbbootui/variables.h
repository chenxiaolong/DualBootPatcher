/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _VARIABLES_HEADER_
#define _VARIABLES_HEADER_

// Boot UI version
#define TW_VERSION                  "tw_version"
// mbtool daemon version
#define TW_MBTOOL_VERSION           "tw_mbtool_version"

// Sort order for lists
#define TW_GUI_SORT_ORDER           "tw_gui_sort_order"

// Whether the GUI has finished loading
#define TW_LOADED                   "tw_loaded"

// Current language
#define TW_LANGUAGE                 "tw_language"
// Current language's display name
#define TW_LANGUAGE_DISPLAY         "tw_language_display"

// Current time (dynamic variable)
#define TW_TIME                     "tw_time"
// Whether time should be displayed in military time
#define TW_MILITARY_TIME            "tw_military_time"
// Current time zone
#define TW_TIME_ZONE                "tw_time_zone"
// Current time zone selection
#define TW_TIME_ZONE_GUISEL         "tw_time_zone_guisel"
// Current time zone offset option
#define TW_TIME_ZONE_GUIOFFSET      "tw_time_zone_guioffset"
// Current time zone daylight savings option
#define TW_TIME_ZONE_GUIDST         "tw_time_zone_guidst"

// Whether an operation is in progress
#define TW_ACTION_BUSY              "tw_busy"

// Whether the screen timeout is disabled
#define TW_NO_SCREEN_TIMEOUT        "tw_no_screen_timeout"
// Number of seconds until screen timeout
#define TW_SCREEN_TIMEOUT_SECS      "tw_screen_timeout_secs"

// Whether reboot to system is enabled
#define TW_REBOOT_SYSTEM            "tw_reboot_system"
// Whether reboot to recovery is enabled
#define TW_REBOOT_RECOVERY          "tw_reboot_recovery"
// Whether reboot to bootloader is enabled
#define TW_REBOOT_BOOTLOADER        "tw_reboot_bootloader"
// Whether reboot to download mode is enabled
#define TW_REBOOT_DOWNLOAD          "tw_reboot_download"
// Whether shut down is enabled
#define TW_REBOOT_POWEROFF          "tw_reboot_poweroff"

// Current battery percentage (dynamic variable)
#define TW_BATTERY                  "tw_battery"
// Whether the battery percentage should be hidden
#define TW_NO_BATTERY_PERCENT       "tw_no_battery_percent"

// Encryption
#define TW_IS_ENCRYPTED             "tw_is_encrypted"
#define TW_CRYPTO_PWTYPE            "tw_crypto_pwtype"
#define TW_CRYPTO_PASSWORD          "tw_crypto_password"

// Button press vibration intensity
#define TW_BUTTON_VIBRATE           "tw_button_vibrate"
// Keyboard press vibration intensity
#define TW_KEYBOARD_VIBRATE         "tw_keyboard_vibrate"
// Action completion vibration intensity
#define TW_ACTION_VIBRATE           "tw_action_vibrate"

// Whether the display brightness control file was found
#define TW_HAS_BRIGHTNESS_FILE      "tw_has_brightnesss_file"
// Path to display brightness control file
#define TW_BRIGHTNESS_FILE          "tw_brightness_file"
// Current brightness
#define TW_BRIGHTNESS               "tw_brightness"
// Current brightness percentage
#define TW_BRIGHTNESS_PCT           "tw_brightness_pct"
// Maximum brightness
#define TW_BRIGHTNESS_MAX           "tw_brightness_max"

// Current CPU temperature (dynamic variable)
#define TW_CPU_TEMP                 "tw_cpu_temp"
// Whether the CPU temperature should be hidden
#define TW_NO_CPU_TEMP              "tw_no_cpu_temp"

// Progress bar percentage (range: 0-100)
#define TW_UI_PROGRESS              "ui_progress"
#define TW_UI_PROGRESS_FRAMES       "ui_progress_frames"
#define TW_UI_PROGRESS_PORTION      "ui_progress_portion"

// Name of current operation
#define TW_OPERATION                "tw_operation"
// Whether an operation is in progress
#define TW_OPERATION_STATE          "tw_operation_state"
// Operation result
#define TW_OPERATION_STATUS         "tw_operation_status"

// Whether the page should exit (useful for stop_on_page_done option)
#define TW_PAGE_DONE                "tw_page_done"
// Whether the GUI should exit
#define TW_GUI_DONE                 "tw_gui_done"
// Action to perform when exiting
#define TW_EXIT_ACTION              "tw_exit_action"

// Selected ROM ID
#define TW_ROM_ID                   "tw_rom_id"

// Autoboot timeout
#define TW_AUTOBOOT_TIMEOUT         "tw_autoboot_timeout"

#endif  // _VARIABLES_HEADER_
