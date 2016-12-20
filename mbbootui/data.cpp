/*
 * Copyright 2012 to 2016 bigbiff/Dees_Troy TeamWin
 * This file is part of TWRP/TeamWin Recovery Project.
 *
 * TWRP is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * TWRP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with TWRP.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "data.hpp"

#include <fstream>
#include <sstream>

#include <cstdlib>
#include <cstring>

#include <cutils/properties.h>

#include "mbcommon/version.h"
#include "mblog/logging.h"
#include "mbutil/directory.h"
#include "mbutil/file.h"
#include "mbutil/path.h"
#include "mbutil/string.h"

#include "config/config.hpp"
#include "gui/blanktimer.hpp"
#include "gui/pages.h"
#include "minuitwrp/minui.h"

#include "find_file.hpp"
#include "infomanager.hpp"
#include "twrp-functions.hpp"
#include "variables.h"

#define DEVID_MAX 64
#define HWID_MAX 32

#define FILE_VERSION 0x00010010 // Do not set to 0

std::string DataManager::mBackingFile;
int         DataManager::mInitialized = 0;
InfoManager DataManager::mPersist;  // Data that that is not constant and will be saved to the settings file
InfoManager DataManager::mData;     // Data that is not constant and will not be saved to settings file
InfoManager DataManager::mConst;    // Data that is constant and will not be saved to settings file

extern bool datamedia;

#ifndef PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
pthread_mutex_t DataManager::m_valuesLock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
#else
pthread_mutex_t DataManager::m_valuesLock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif

int DataManager::ResetDefaults()
{
    pthread_mutex_lock(&m_valuesLock);
    mPersist.Clear();
    mData.Clear();
    mConst.Clear();
    pthread_mutex_unlock(&m_valuesLock);

    SetDefaultValues();
    return 0;
}

int DataManager::LoadValues(const std::string& filename)
{
    std::string str;

    if (!mInitialized) {
        SetDefaultValues();
    }

    // Save off the backing file for set operations
    mBackingFile = filename;
    mPersist.SetFile(filename);
    mPersist.SetFileVersion(FILE_VERSION);

    // Read in the file, if possible
    pthread_mutex_lock(&m_valuesLock);
    mPersist.LoadValues();

    if (!(tw_flags & TW_FLAG_NO_SCREEN_TIMEOUT)) {
        blankTimer.setTime(mPersist.GetIntValue(TW_SCREEN_TIMEOUT_SECS));
    }

    pthread_mutex_unlock(&m_valuesLock);
    return 0;
}

int DataManager::Flush()
{
    return SaveValues();
}

int DataManager::SaveValues()
{
#ifndef TW_OEM_BUILD
    if (mBackingFile.empty()) {
        return -1;
    }

    mPersist.SetFile(mBackingFile);
    mPersist.SetFileVersion(FILE_VERSION);
    pthread_mutex_lock(&m_valuesLock);
    mPersist.SaveValues();
    pthread_mutex_unlock(&m_valuesLock);

    LOGI("Saved settings file values\n");
#endif // ifdef TW_OEM_BUILD
    return 0;
}

int DataManager::GetValue(const std::string& varName, std::string& value)
{
    std::string localStr = varName;
    int ret = 0;

    if (!mInitialized) {
        SetDefaultValues();
    }

    // Strip off leading and trailing '%' if provided
    if (localStr.length() > 2 && localStr[0] == '%' && localStr[localStr.length()-1] == '%') {
        localStr.erase(0, 1);
        localStr.erase(localStr.length() - 1, 1);
    }

    // Handle magic values
    if (GetMagicValue(localStr, value) == 0) {
        return 0;
    }

    // Handle property
    if (localStr.length() > 9 && localStr.substr(0, 9) == "property.") {
        char property_value[PROPERTY_VALUE_MAX];
        property_get(localStr.substr(9).c_str(), property_value, "");
        value = property_value;
        return 0;
    }

    pthread_mutex_lock(&m_valuesLock);
    ret = mConst.GetValue(localStr, value);
    if (ret == 0) {
        goto exit;
    }

    ret = mPersist.GetValue(localStr, value);
    if (ret == 0) {
        goto exit;
    }

    ret = mData.GetValue(localStr, value);
exit:
    pthread_mutex_unlock(&m_valuesLock);
    return ret;
}

int DataManager::GetValue(const std::string& varName, int& value)
{
    std::string data;

    if (GetValue(varName,data) != 0) {
        return -1;
    }

    value = atoi(data.c_str());
    return 0;
}

int DataManager::GetValue(const std::string& varName, float& value)
{
    std::string data;

    if (GetValue(varName,data) != 0) {
        return -1;
    }

    value = atof(data.c_str());
    return 0;
}

unsigned long long DataManager::GetValue(const std::string& varName, unsigned long long& value)
{
    std::string data;

    if (GetValue(varName,data) != 0) {
        return -1;
    }

    value = strtoull(data.c_str(), nullptr, 10);
    return 0;
}

// This function will return an empty string if the value doesn't exist
std::string DataManager::GetStrValue(const std::string& varName)
{
    std::string retVal;

    GetValue(varName, retVal);
    return retVal;
}

// This function will return 0 if the value doesn't exist
int DataManager::GetIntValue(const std::string& varName)
{
    std::string retVal;

    GetValue(varName, retVal);
    return atoi(retVal.c_str());
}

int DataManager::SetValue(const std::string& varName, const std::string& value, const int persist /* = 0 */)
{
    if (!mInitialized) {
        SetDefaultValues();
    }

    // Handle property
    if (varName.length() > 9 && varName.substr(0, 9) == "property.") {
        int ret = property_set(varName.substr(9).c_str(), value.c_str());
        if (ret) {
            LOGE("Error setting property '%s' to '%s'", varName.substr(9).c_str(), value.c_str());
        }
        return ret;
    }

    // Don't allow empty values or numerical starting values
    if (varName.empty() || (varName[0] >= '0' && varName[0] <= '9')) {
        return -1;
    }

    std::string test;
    pthread_mutex_lock(&m_valuesLock);
    int constChk = mConst.GetValue(varName, test);
    if (constChk == 0) {
        pthread_mutex_unlock(&m_valuesLock);
        return -1;
    }

    if (persist) {
        mPersist.SetValue(varName, value);
    } else {
        int persistChk = mPersist.GetValue(varName, test);
        if (persistChk == 0) {
            mPersist.SetValue(varName, value);
        } else {
            mData.SetValue(varName, value);
        }
    }

    pthread_mutex_unlock(&m_valuesLock);

    if (!(tw_flags & TW_FLAG_NO_SCREEN_TIMEOUT) && varName == TW_SCREEN_TIMEOUT_SECS) {
        blankTimer.setTime(atoi(value.c_str()));
    }
    gui_notifyVarChange(varName.c_str(), value.c_str());
    return 0;
}

int DataManager::SetValue(const std::string& varName, const int value, const int persist /* = 0 */)
{
    std::ostringstream valStr;
    valStr << value;
    return SetValue(varName, valStr.str(), persist);
}

int DataManager::SetValue(const std::string& varName, const float value, const int persist /* = 0 */)
{
    std::ostringstream valStr;
    valStr << value;
    return SetValue(varName, valStr.str(), persist);;
}

int DataManager::SetValue(const std::string& varName, const unsigned long long& value, const int persist /* = 0 */)
{
    std::ostringstream valStr;
    valStr << value;
    return SetValue(varName, valStr.str(), persist);
}

int DataManager::SetProgress(const float Fraction)
{
    return SetValue(TW_UI_PROGRESS, (float) (Fraction * 100.0));
}

int DataManager::ShowProgress(const float Portion, const float Seconds)
{
    float Starting_Portion;
    GetValue(TW_UI_PROGRESS_PORTION, Starting_Portion);
    if (SetValue(TW_UI_PROGRESS_PORTION, (float) (Portion * 100.0) + Starting_Portion) != 0) {
        return -1;
    }
    if (SetValue(TW_UI_PROGRESS_FRAMES, Seconds * 30) != 0) {
        return -1;
    }
    return 0;
}

void DataManager::update_tz_environment_variables()
{
    setenv("TZ", GetStrValue(TW_TIME_ZONE).c_str(), 1);
    tzset();
}

void DataManager::SetDefaultValues()
{
    std::string str, path;

    mConst.SetConst();

    pthread_mutex_lock(&m_valuesLock);

    mInitialized = 1;

    mConst.SetValue("true", "1");
    mConst.SetValue("false", "0");

    mConst.SetValue(TW_VERSION, mb::version());
    mPersist.SetValue(TW_BUTTON_VIBRATE, "80");
    mPersist.SetValue(TW_KEYBOARD_VIBRATE, "40");
    mPersist.SetValue(TW_ACTION_VIBRATE, "160");

    mConst.SetValue(TW_REBOOT_SYSTEM, "1");
    mConst.SetValue(TW_REBOOT_RECOVERY, "1");
    if (tw_flags & TW_FLAG_HAS_DOWNLOAD_MODE) {
        mConst.SetValue(TW_REBOOT_BOOTLOADER, "0");
        mConst.SetValue(TW_REBOOT_DOWNLOAD, "1");
    } else {
        mConst.SetValue(TW_REBOOT_BOOTLOADER, "1");
        mConst.SetValue(TW_REBOOT_DOWNLOAD, "0");
    }
    mConst.SetValue(TW_REBOOT_POWEROFF, "1");

    mConst.SetValue(TW_NO_BATTERY_PERCENT, "0");

    if (tw_flags & TW_FLAG_NO_CPU_TEMP) {
        LOGD("TW_NO_CPU_TEMP := true");
        mConst.SetValue(TW_NO_CPU_TEMP, "1");
    } else {
        std::string cpu_temp_file;
        if (tw_custom_cpu_temp_path) {
            cpu_temp_file = tw_custom_cpu_temp_path;
        } else {
            cpu_temp_file = "/sys/class/thermal/thermal_zone0/temp";
        }
        if (mb::util::path_exists(cpu_temp_file.c_str(), true)) {
            mConst.SetValue(TW_NO_CPU_TEMP, "0");
        } else {
            LOGI("CPU temperature file '%s' not found, disabling CPU temp.", cpu_temp_file.c_str());
            mConst.SetValue(TW_NO_CPU_TEMP, "1");
        }
    }

    mPersist.SetValue(TW_GUI_SORT_ORDER, "1");
    mPersist.SetValue(TW_TIME_ZONE, "CST6CDT,M3.2.0,M11.1.0");
    mPersist.SetValue(TW_TIME_ZONE_GUISEL, "CST6;CDT,M3.2.0,M11.1.0");
    mPersist.SetValue(TW_TIME_ZONE_GUIOFFSET, "0");
    mPersist.SetValue(TW_TIME_ZONE_GUIDST, "1");
    mData.SetValue(TW_ACTION_BUSY, "0");
    mData.SetValue(TW_IS_ENCRYPTED, "0");
    mData.SetValue(TW_CRYPTO_PASSWORD, "0");
    mData.SetValue("tw_terminal_state", "0");
    mData.SetValue("tw_background_thread_running", "0");
    mPersist.SetValue(TW_MILITARY_TIME, "0");
    if (tw_flags & TW_FLAG_NO_SCREEN_TIMEOUT) {
        mConst.SetValue(TW_SCREEN_TIMEOUT_SECS, "0");
        mConst.SetValue(TW_NO_SCREEN_TIMEOUT, "1");
    } else {
        mPersist.SetValue(TW_SCREEN_TIMEOUT_SECS, "60");
        mPersist.SetValue(TW_NO_SCREEN_TIMEOUT, "0");
    }
    mData.SetValue(TW_GUI_DONE, "0");

    // Brightness handling
    std::string findbright;
    if (tw_brightness_path) {
        findbright = tw_brightness_path;
        LOGI("TW_BRIGHTNESS_PATH := %s", findbright.c_str());
        if (!mb::util::path_exists(findbright.c_str(), true)) {
            LOGI("Specified brightness file '%s' not found.", findbright.c_str());
            findbright = "";
        }
    }
    if (findbright.empty()) {
        static const char *backlight = "/sys/class/backlight";
        static const char *lcd_backlight = "/sys/class/leds/lcd-backlight";

        // Attempt to locate the brightness file
        findbright = Find_File::Find("brightness",
                                     (tw_flags & TW_FLAG_PREFER_LCD_BACKLIGHT)
                                     ? lcd_backlight : backlight);
        if (findbright.empty()) {
            findbright = Find_File::Find("brightness",
                                         (tw_flags & TW_FLAG_PREFER_LCD_BACKLIGHT)
                                         ? backlight : lcd_backlight);
        }
    }
    if (findbright.empty()) {
        LOGI("Unable to locate brightness file");
        mConst.SetValue(TW_HAS_BRIGHTNESS_FILE, "0");
    } else {
        LOGI("Found brightness file at '%s'", findbright.c_str());
        mConst.SetValue(TW_HAS_BRIGHTNESS_FILE, "1");
        mConst.SetValue(TW_BRIGHTNESS_FILE, findbright);
        std::string maxBrightness;
        if (tw_max_brightness >= 0) {
            std::ostringstream maxVal;
            maxVal << tw_max_brightness;
            maxBrightness = maxVal.str();
        } else {
            // Attempt to locate the max_brightness file
            std::string maxbrightpath = findbright.insert(findbright.rfind('/') + 1, "max_");
            if (mb::util::path_exists(maxbrightpath.c_str(), true)) {
                std::ifstream maxVal(maxbrightpath.c_str());
                if (maxVal >> maxBrightness) {
                    LOGI("Got max brightness %s from '%s'", maxBrightness.c_str(), maxbrightpath.c_str());
                } else {
                    // Something went wrong, set that to indicate error
                    maxBrightness = "-1";
                }
            }
            if (atoi(maxBrightness.c_str()) <= 0) {
                // Fallback into default
                std::ostringstream maxVal;
                maxVal << 255;
                maxBrightness = maxVal.str();
            }
        }
        mConst.SetValue(TW_BRIGHTNESS_MAX, maxBrightness);
        mPersist.SetValue(TW_BRIGHTNESS, maxBrightness);
        mPersist.SetValue(TW_BRIGHTNESS_PCT, "100");
        if (tw_secondary_brightness_path) {
            std::string secondfindbright = tw_secondary_brightness_path;
            if (secondfindbright != "" && mb::util::path_exists(secondfindbright.c_str(), true)) {
                LOGI("Will use a second brightness file at '%s'", secondfindbright.c_str());
                mConst.SetValue("tw_secondary_brightness_file", secondfindbright);
            } else {
                LOGI("Specified secondary brightness file '%s' not found.", secondfindbright.c_str());
            }
        }
        if (tw_default_brightness >= 0) {
            int defValInt = tw_default_brightness;
            int maxValInt = atoi(maxBrightness.c_str());
            // Deliberately int so the % is always a whole number
            int defPctInt = (((double) defValInt / maxValInt) * 100);
            std::ostringstream defPct;
            defPct << defPctInt;
            mPersist.SetValue(TW_BRIGHTNESS_PCT, defPct.str());

            std::ostringstream defVal;
            defVal << tw_default_brightness;
            mPersist.SetValue(TW_BRIGHTNESS, defVal.str());
            TWFunc::Set_Brightness(defVal.str());
        } else {
            TWFunc::Set_Brightness(maxBrightness);
        }
    }

    mPersist.SetValue(TW_LANGUAGE, "en");

    // Set default autoboot timeout to 5 seconds
    mPersist.SetValue(TW_AUTOBOOT_TIMEOUT, 5);

    pthread_mutex_unlock(&m_valuesLock);
}

// Magic Values
int DataManager::GetMagicValue(const std::string& varName, std::string& value)
{
    // Handle special dynamic cases
    if (varName == TW_TIME) {
        char tmp[32];

        struct tm *current;
        time_t now;
        int tw_military_time;
        now = time(0);
        current = localtime(&now);
        GetValue(TW_MILITARY_TIME, tw_military_time);
        if (current->tm_hour >= 12) {
            if (tw_military_time == 1) {
                sprintf(tmp, "%d:%02d", current->tm_hour, current->tm_min);
            } else {
                sprintf(tmp, "%d:%02d PM", current->tm_hour == 12 ? 12 : current->tm_hour - 12, current->tm_min);
            }
        } else {
            if (tw_military_time == 1) {
                sprintf(tmp, "%d:%02d", current->tm_hour, current->tm_min);
            } else {
                sprintf(tmp, "%d:%02d AM", current->tm_hour == 0 ? 12 : current->tm_hour, current->tm_min);
            }
        }
        value = tmp;
        return 0;
    } else if (varName == TW_CPU_TEMP) {
        int tw_no_cpu_temp;
        GetValue(TW_NO_CPU_TEMP, tw_no_cpu_temp);
        if (tw_no_cpu_temp == 1) {
            return -1;
        }

        std::string cpu_temp_file;
        static unsigned long convert_temp = 0;
        static time_t cpuSecCheck = 0;
        struct timespec curTime;
        std::string results;

        clock_gettime(CLOCK_MONOTONIC, &curTime);
        if (curTime.tv_sec > cpuSecCheck) {
            if (tw_custom_cpu_temp_path) {
                cpu_temp_file = tw_custom_cpu_temp_path;
                if (!mb::util::file_first_line(cpu_temp_file, &results)) {
                    return -1;
                }
            } else {
                cpu_temp_file = "/sys/class/thermal/thermal_zone0/temp";
                if (!mb::util::file_first_line(cpu_temp_file, &results)) {
                    return -1;
                }
            }
            convert_temp = strtoul(results.c_str(), nullptr, 0) / 1000;
            if (convert_temp <= 0) {
                convert_temp = strtoul(results.c_str(), nullptr, 0);
            }
            if (convert_temp >= 150) {
                convert_temp = strtoul(results.c_str(), nullptr, 0) / 10;
            }
            cpuSecCheck = curTime.tv_sec + 5;
        }
        value = mb::util::format("%lu", convert_temp);
        return 0;
    } else if (varName == TW_BATTERY) {
        char tmp[16];
        static char charging = ' ';
        static int lastVal = -1;
        static time_t nextSecCheck = 0;
        struct timespec curTime;
        clock_gettime(CLOCK_MONOTONIC, &curTime);
        if (curTime.tv_sec > nextSecCheck) {
            char cap_s[4];
            FILE *cap;
            if (tw_custom_battery_path) {
                std::string capacity_file = tw_custom_battery_path;
                capacity_file += "/capacity";
                cap = fopen(capacity_file.c_str(), "rt");
            } else {
                cap = fopen("/sys/class/power_supply/battery/capacity", "rt");
            }
            if (cap) {
                fgets(cap_s, 4, cap);
                fclose(cap);
                lastVal = atoi(cap_s);
                if (lastVal > 100) {
                    lastVal = 101;
                }
                if (lastVal < 0) {
                    lastVal = 0;
                }
            }
            if (tw_custom_battery_path) {
                std::string status_file = tw_custom_battery_path;
                status_file += "/status";
                cap = fopen(status_file.c_str(), "rt");
            } else {
                cap = fopen("/sys/class/power_supply/battery/status", "rt");
            }
            if (cap) {
                fgets(cap_s, 2, cap);
                fclose(cap);
                if (cap_s[0] == 'C') {
                    charging = '+';
                } else {
                    charging = ' ';
                }
            }
            nextSecCheck = curTime.tv_sec + 60;
        }

        sprintf(tmp, "%i%%%c", lastVal, charging);
        value = tmp;
        return 0;
    }
    return -1;
}

void DataManager::ReadSettingsFile()
{
#ifndef TW_OEM_BUILD
    mb::util::mkdir_parent(tw_settings_path, 0700);

    LOGI("Attempt to load settings from settings file...");
    LoadValues(tw_settings_path);
#endif // ifdef TW_OEM_BUILD
    update_tz_environment_variables();
    TWFunc::Set_Brightness(GetStrValue(TW_BRIGHTNESS));
}

void DataManager::Vibrate(const std::string& varName)
{
    int vib_value = 0;
    GetValue(varName, vib_value);
    if (vib_value) {
        vibrate(vib_value);
    }
}
